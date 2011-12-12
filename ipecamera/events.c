#define _IPECAMERA_IMAGE_C
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
    int err = 0;
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
    
	// This loop iterates while the generation
    while ((ctx->run_streamer)||(ctx->reported_id != ctx->event_id)) {
	while (ctx->reported_id != ctx->event_id) {
	    if ((ctx->event_id - ctx->reported_id) > (ctx->buffer_size - IPECAMERA_RESERVE_BUFFERS)) ctx->reported_id = ctx->event_id - (ctx->buffer_size - 1) - IPECAMERA_RESERVE_BUFFERS;
	    else ++ctx->reported_id;

	    memcpy(&info, ctx->frame + ((ctx->reported_id-1)%ctx->buffer_size), sizeof(ipecamera_event_info_t));

	    if ((ctx->event_id - ctx->reported_id) < ctx->buffer_size) {
		err = callback(ctx->reported_id, (pcilib_event_info_t*)&info, user);
		if (err <= 0) {
		    if (err < 0) err = -err;
		    break;
		}
	    }
	}
	usleep(IPECAMERA_NOFRAME_SLEEP);
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

    if (ctx->reported_id == ctx->event_id) {
	if (timeout) {
	    pcilib_calc_deadline(&tv, timeout);
	    
	    while ((pcilib_calc_time_to_deadline(&tv) > 0)&&(ctx->reported_id == ctx->event_id))
		usleep(IPECAMERA_NOFRAME_SLEEP);
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
