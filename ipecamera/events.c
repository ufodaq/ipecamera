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

#include "../tools.h"
#include "../error.h"

#include "pcilib.h"
#include "public.h"
#include "private.h"
#include "events.h"

int ipecamera_stream(pcilib_context_t *vctx, pcilib_event_callback_t callback, void *user) {
    int run_flag = 1;
    int res, err = 0;
    int do_stop = 0;
    
    ipecamera_event_info_t info;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    ctx->streaming = 1;
    ctx->run_streamer = 1;

    if (!ctx->started) {
	err = ipecamera_start(vctx, PCILIB_EVENTS_ALL, PCILIB_EVENT_FLAGS_DEFAULT);
	if (err) {
	    ctx->streaming = 0;
	    return err;
	}
	
	do_stop = 1;
    }
    
    if (ctx->parse_data) {
	    // This loop iterates while the generation
	while ((run_flag)&&((ctx->run_streamer)||(ctx->reported_id != ctx->event_id))) {
#ifdef IPECAMERA_ANNOUNCE_READY
	    while (((!ctx->preproc)&&(ctx->reported_id != ctx->event_id))||((ctx->preproc)&&(ctx->reported_id != ctx->preproc_id))) {
#else /* IPECAMERA_ANNOUNCE_READY */
	    while (ctx->reported_id != ctx->event_id) {
#endif /* IPECAMERA_ANNOUNCE_READY */
		if ((ctx->event_id - ctx->reported_id) > (ctx->buffer_size - IPECAMERA_RESERVE_BUFFERS)) ctx->reported_id = ctx->event_id - (ctx->buffer_size - 1) - IPECAMERA_RESERVE_BUFFERS;
		else ++ctx->reported_id;

		memcpy(&info, ctx->frame + ((ctx->reported_id-1)%ctx->buffer_size), sizeof(ipecamera_event_info_t));

		if ((ctx->event_id - ctx->reported_id) < ctx->buffer_size) {
		    res = callback(ctx->reported_id, (pcilib_event_info_t*)&info, user);
		    if (res <= 0) {
			if (res < 0) err = -res;
			run_flag = 0;
			break;
		    }
		}
	    }
	    usleep(IPECAMERA_NOFRAME_SLEEP);
	}
    } else {
	while ((run_flag)&&(ctx->run_streamer)) {
	    usleep(IPECAMERA_NOFRAME_SLEEP);
	}
    }

    ctx->streaming = 0;

    if (do_stop) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
    }
    

    return err;
}

int ipecamera_next_event(pcilib_context_t *vctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, size_t info_size, pcilib_event_info_t *info) {
    struct timeval tv;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (!ctx->started) {
	pcilib_error("IPECamera is not in grabbing mode");
	return PCILIB_ERROR_INVALID_REQUEST;
    }
    
    if (!ctx->parse_data) {
	pcilib_error("RAWData only mode is requested");
	return PCILIB_ERROR_INVALID_REQUEST;
    }

#ifdef IPECAMERA_ANNOUNCE_READY
    if (((!ctx->preproc)&&(ctx->reported_id == ctx->event_id))||((ctx->preproc)&&(ctx->reported_id == ctx->preproc_id))) {
#else /* IPECAMERA_ANNOUNCE_READY */
    if (ctx->reported_id == ctx->event_id) {
#endif /* IPECAMERA_ANNOUNCE_READY */

	if (timeout) {
	    if (timeout == PCILIB_TIMEOUT_INFINITE) {
#ifdef IPECAMERA_ANNOUNCE_READY
		while ((((!ctx->preproc)&&(ctx->reported_id == ctx->event_id))||((ctx->preproc)&&(ctx->reported_id == ctx->preproc_id)))) {
#else /* IPECAMERA_ANNOUNCE_READY */
		while ((ctx->reported_id == ctx->event_id)) {
#endif /* IPECAMERA_ANNOUNCE_READY */
		usleep(IPECAMERA_NOFRAME_SLEEP);
		}
	    } else {	    
		pcilib_calc_deadline(&tv, timeout);

#ifdef IPECAMERA_ANNOUNCE_READY
		while ((pcilib_calc_time_to_deadline(&tv) > 0)&&(((!ctx->preproc)&&(ctx->reported_id == ctx->event_id))||((ctx->preproc)&&(ctx->reported_id == ctx->preproc_id)))) {
#else /* IPECAMERA_ANNOUNCE_READY */
		while ((pcilib_calc_time_to_deadline(&tv) > 0)&&(ctx->reported_id == ctx->event_id)) {
#endif /* IPECAMERA_ANNOUNCE_READY */
		usleep(IPECAMERA_NOFRAME_SLEEP);
		}
	    }
	    
	}
	
	if (ctx->reported_id == ctx->event_id) return PCILIB_ERROR_TIMEOUT;
	
    }

retry:
    if ((ctx->event_id - ctx->reported_id) > (ctx->buffer_size - IPECAMERA_RESERVE_BUFFERS)) ctx->reported_id = ctx->event_id - (ctx->buffer_size - 1) - IPECAMERA_RESERVE_BUFFERS;
    else ++ctx->reported_id;

    if (evid) *evid = ctx->reported_id;

    if (info) {
	if (info_size >= sizeof(ipecamera_event_info_t))
	    memcpy(info, ctx->frame + ((ctx->reported_id-1)%ctx->buffer_size), sizeof(ipecamera_event_info_t));
	else if (info_size >= sizeof(pcilib_event_info_t))
	    memcpy(info, ctx->frame + ((ctx->reported_id-1)%ctx->buffer_size), sizeof(pcilib_event_info_t));
	else
	    return PCILIB_ERROR_INVALID_ARGUMENT;
    }
    
    if ((ctx->event_id - ctx->reported_id) >= ctx->buffer_size) goto retry;
    
    return 0;
}

