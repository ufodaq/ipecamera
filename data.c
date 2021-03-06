#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <assert.h>

#include <ufodecode.h>

#include <pcilib.h>
#include <pcilib/tools.h>
#include <pcilib/error.h>

#include "private.h"
#include "data.h"

// DS: Currently, on event_id overflow we are assuming the buffer is lost
static int ipecamera_resolve_event_id(ipecamera_t *ctx, pcilib_event_id_t evid) {
    pcilib_event_id_t diff;

    if (evid > ctx->event_id) {
	diff = (((pcilib_event_id_t)-1) - ctx->event_id) + evid;
	if (diff >= ctx->buffer_size) return -1;
    } else {
	diff = ctx->event_id - evid;
        if (diff >= ctx->buffer_size) return -1;
    }
    
	// DS: Request buffer_size to be power of 2 and replace to shifts (just recompute in set_buffer_size)
    return (evid - 1) % ctx->buffer_size;
}

inline static int ipecamera_decode_frame(ipecamera_t *ctx, pcilib_event_id_t event_id) {
    int err = 0;
    size_t res;
    uint16_t *pixels;
    
    int buf_ptr = ipecamera_resolve_event_id(ctx, event_id);
    if (buf_ptr < 0) return PCILIB_ERROR_OVERWRITTEN;
    
    if (ctx->frame[buf_ptr].event.image_ready) return 0;
    
    if (ctx->frame[buf_ptr].event.info.flags&PCILIB_EVENT_INFO_FLAG_BROKEN) {
	err = PCILIB_ERROR_INVALID_DATA;
	ctx->frame[buf_ptr].event.image_broken = err;
	goto ready;
    }
	
		
    pixels = ctx->image + buf_ptr * ctx->image_size;
    memset(ctx->cmask + ctx->buffer_pos * ctx->dim.height, 0, ctx->dim.height * sizeof(ipecamera_change_mask_t));

    ipecamera_debug_buffer(RAW_FRAMES, ctx->frame[buf_ptr].event.raw_size, ctx->buffer + buf_ptr * ctx->padded_size, PCILIB_DEBUG_BUFFER_MKDIR, "raw_frame.%4lu", ctx->event_id);

    res = ufo_decoder_decode_frame(ctx->ipedec, ctx->buffer + buf_ptr * ctx->padded_size, ctx->frame[buf_ptr].event.raw_size, pixels, &ctx->frame[buf_ptr].event.meta);
    if (!res) {
	ipecamera_debug_buffer(BROKEN_FRAMES, ctx->frame[buf_ptr].event.raw_size, ctx->buffer + buf_ptr * ctx->padded_size, PCILIB_DEBUG_BUFFER_MKDIR, "broken_frame.%4lu", ctx->event_id);
        err = PCILIB_ERROR_INVALID_DATA;
        ctx->frame[buf_ptr].event.image_broken = err;
	goto ready;
    } 

    ctx->frame[buf_ptr].event.image_broken = 0;

ready:
    ctx->frame[buf_ptr].event.image_ready = 1;

    if (ipecamera_resolve_event_id(ctx, event_id) < 0) {
	ctx->frame[buf_ptr].event.image_ready = 0;
	return PCILIB_ERROR_OVERWRITTEN;
    }

    return err;
}

static int ipecamera_get_next_buffer_to_process(ipecamera_t *ctx, pcilib_event_id_t *evid) {
    int err, res;

    if (ctx->preproc_id == ctx->event_id) return -1;
    
    if (ctx->preproc) 
	pthread_mutex_lock(&ctx->preproc_mutex);
	
    if (ctx->preproc_id == ctx->event_id) {
	if (ctx->preproc)
	    pthread_mutex_unlock(&ctx->preproc_mutex);
	return -1;
    }

    if ((ctx->event_id - ctx->preproc_id) > (ctx->buffer_size - IPECAMERA_RESERVE_BUFFERS)) {
	size_t preproc_id = ctx->preproc_id;
	ctx->preproc_id = ctx->event_id - (ctx->buffer_size - 1 - IPECAMERA_RESERVE_BUFFERS - 1);
	ipecamera_debug(HARDWARE, "Skipping preprocessing of events %zu to %zu as decoding is not fast enough. We are currently %zu buffers beyond, but only %zu buffers are available and safety limit is %zu",
	    preproc_id, ctx->preproc_id - 1, ctx->event_id - ctx->preproc_id, ctx->buffer_size, IPECAMERA_RESERVE_BUFFERS);
    }

    res = ctx->preproc_id%ctx->buffer_size;

    if ((err = pthread_rwlock_trywrlock(&ctx->frame[res].mutex)) != 0) {
	if (ctx->preproc)
	    pthread_mutex_unlock(&ctx->preproc_mutex);
	ipecamera_debug(HARDWARE, "Can't lock buffer %i, errno %i", res, err);
	return -1;
    }
    
    *evid = ++ctx->preproc_id;

    if (ctx->preproc)
	pthread_mutex_unlock(&ctx->preproc_mutex);

    return res;
}


void *ipecamera_preproc_thread(void *user) {
    int err;
    int buf_ptr;
    pcilib_event_id_t evid;
    
    ipecamera_preprocessor_t *preproc = (ipecamera_preprocessor_t*)user;
    ipecamera_t *ctx = preproc->ipecamera;
    
    while (ctx->run_preprocessors) {
	buf_ptr = ipecamera_get_next_buffer_to_process(ctx, &evid);
	if (buf_ptr < 0) {
	    usleep(IPECAMERA_NOFRAME_PREPROC_SLEEP);
	    continue;
	}
	
	err = ipecamera_decode_frame(ctx, evid);
	
	pthread_rwlock_unlock(&ctx->frame[buf_ptr].mutex);

#ifdef IPECAMERA_DEBUG_HARDWARE
	if (err) {
	    switch (err) {
	     case PCILIB_ERROR_OVERWRITTEN:
		ipecamera_debug(HARDWARE, "The frame (%zu) was overwritten while preprocessing", evid);
	     break;
	     case PCILIB_ERROR_INVALID_DATA:
		ipecamera_debug(HARDWARE, "The frame (%zu) is corrupted, decoding have failed", evid);
	     break;
	     default:
		ipecamera_debug(HARDWARE, "The frame (%zu) is corrupted, decoding have failed", evid);
	    }
	}
    }
#endif /* IPECAMERA_DEBUG_HARDWARE */

    return NULL;
}

static int ipecamera_get_frame(ipecamera_t *ctx, pcilib_event_id_t event_id) {
    int err;
    int buf_ptr = (event_id - 1) % ctx->buffer_size;

    if (!ctx->preproc) {
	pthread_rwlock_rdlock(&ctx->frame[buf_ptr].mutex);

	err = ipecamera_decode_frame(ctx, event_id);

	if (err) {
	    pthread_rwlock_unlock(&ctx->frame[buf_ptr].mutex);
	    return err;
	}
	
	return 0;
    }


    while (!((volatile ipecamera_t*)ctx)->frame[buf_ptr].event.image_ready) {
	usleep(IPECAMERA_NOFRAME_PREPROC_SLEEP);

	buf_ptr = ipecamera_resolve_event_id(ctx, event_id);
	if (buf_ptr < 0) return PCILIB_ERROR_OVERWRITTEN;
    }

    if (((volatile ipecamera_t*)ctx)->frame[buf_ptr].event.image_broken)
	return ctx->frame[buf_ptr].event.image_broken;

    pthread_rwlock_rdlock(&ctx->frame[buf_ptr].mutex);

    buf_ptr = ipecamera_resolve_event_id(ctx, event_id);
    if ((buf_ptr < 0)||(!ctx->frame[buf_ptr].event.image_ready)) {
	pthread_rwlock_unlock(&ctx->frame[buf_ptr].mutex);
	return PCILIB_ERROR_OVERWRITTEN;
    }

    return 0;
}


/*
 We will lock the data for non-raw data to prevent ocasional overwritting. The 
 raw data will be overwritten by the reader thread anyway and we can't do 
 anything to prevent it for performance reasons.
*/
int ipecamera_get(pcilib_context_t *vctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size, void **ret) {
    int err;
    int buf_ptr;
    size_t raw_size;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    void *data = *ret;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    ipecamera_debug(API, "ipecamera: get (data)");

    buf_ptr = ipecamera_resolve_event_id(ctx, event_id);
    if (buf_ptr < 0) {
	ipecamera_debug(HARDWARE, "The data of the requested frame %zu has been meanwhile overwritten", event_id);
	return PCILIB_ERROR_OVERWRITTEN;
    }

    switch ((ipecamera_data_type_t)data_type) {
	case IPECAMERA_RAW_DATA:
	    raw_size = ctx->frame[buf_ptr].event.raw_size;
	    if (data) {
		if ((!size)||(*size < raw_size)) {
		    pcilib_warning("The raw data associated with frame %zu is too big (%zu bytes) for user supplied buffer (%zu bytes)", event_id, raw_size, (size?*size:0));
		    return PCILIB_ERROR_TOOBIG;
		}
		memcpy(data, ctx->buffer + buf_ptr * ctx->padded_size, raw_size);
		if (ipecamera_resolve_event_id(ctx, event_id) < 0) {
		    ipecamera_debug(HARDWARE, "The data of requested frame %zu was overwritten while copying", event_id);
		    return PCILIB_ERROR_OVERWRITTEN;
		}
		*size = raw_size;
		return 0;
	    }
	    if (size) *size = raw_size;
	    *ret = ctx->buffer + buf_ptr * ctx->padded_size;
	    return 0;
	case IPECAMERA_IMAGE_DATA:
	    err = ipecamera_get_frame(ctx, event_id);
	    if (err) {
#ifdef IPECAMERA_DEBUG_HARDWARE
		switch (err) {
	         case PCILIB_ERROR_OVERWRITTEN:
		    ipecamera_debug(HARDWARE, "The requested frame (%zu) was overwritten", event_id);
		    break;
	         case PCILIB_ERROR_INVALID_DATA:
		    ipecamera_debug(HARDWARE, "The requested frame (%zu) is corrupted", event_id);
		    break;
		 default:
		    ipecamera_debug(HARDWARE, "Error getting the data associated with the requested frame (%zu), error %i", event_id, err);
		}
#endif /* IPECAMERA_DEBUG_HARDWARE */
		return err;
	    }

	    if (data) {
		if ((!size)||(*size < ctx->image_size * sizeof(ipecamera_pixel_t))) {
		    pcilib_warning("The image associated with frame %zu is too big (%zu bytes) for user supplied buffer (%zu bytes)", event_id, ctx->image_size * sizeof(ipecamera_pixel_t), (size?*size:0));
		    return PCILIB_ERROR_TOOBIG;
		}
		memcpy(data, ctx->image + buf_ptr * ctx->image_size, ctx->image_size * sizeof(ipecamera_pixel_t));
		pthread_rwlock_unlock(&ctx->frame[buf_ptr].mutex);
		*size =  ctx->image_size * sizeof(ipecamera_pixel_t);
		return 0;
	    }
	
	    if (size) *size = ctx->image_size * sizeof(ipecamera_pixel_t);
	    *ret = ctx->image + buf_ptr * ctx->image_size;
	    return 0;
	case IPECAMERA_CHANGE_MASK:
	    err = ipecamera_get_frame(ctx, event_id);
	    if (err) return err;

	    if (data) {
		if ((!size)||(*size < ctx->dim.height * sizeof(ipecamera_change_mask_t))) return PCILIB_ERROR_TOOBIG;
		memcpy(data, ctx->image + buf_ptr * ctx->dim.height, ctx->dim.height * sizeof(ipecamera_change_mask_t));
		pthread_rwlock_unlock(&ctx->frame[buf_ptr].mutex);
		*size =  ctx->dim.height * sizeof(ipecamera_change_mask_t);
		return 0;
	    }

	    if (size) *size = ctx->dim.height * sizeof(ipecamera_change_mask_t);
	    *ret = ctx->cmask + buf_ptr * ctx->dim.height;
	    return 0;
	case IPECAMERA_DIMENSIONS:
	    if (size) *size = sizeof(ipecamera_image_dimensions_t);
	    ret = (void*)&ctx->dim;
	    return 0;
	case IPECAMERA_IMAGE_REGION:
	case IPECAMERA_PACKED_IMAGE:
	    // Shall we return complete image or only changed parts?
	case IPECAMERA_PACKED_LINE:
	case IPECAMERA_PACKED_PAYLOAD:
	    pcilib_error("Support for data type (%li) is not implemented yet", data_type);
	    return PCILIB_ERROR_NOTSUPPORTED;
	default:
	    pcilib_error("Unknown data type (%li) is requested", data_type);
	    return PCILIB_ERROR_INVALID_REQUEST;
    }
}


/*
 We will unlock non-raw data and check if the raw data is not overwritten yet
*/
int ipecamera_return(pcilib_context_t *vctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, void *data) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;

    }

    if ((ipecamera_data_type_t)data_type == IPECAMERA_RAW_DATA) {
	if (ipecamera_resolve_event_id(ctx, event_id) < 0) return PCILIB_ERROR_OVERWRITTEN;
    } else {
	int buf_ptr = (event_id - 1) % ctx->buffer_size;
	pthread_rwlock_unlock(&ctx->frame[buf_ptr].mutex);
    }

    ipecamera_debug(API, "ipecamera: return (data)");

    return 0;
}
