#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>

#include <ufodecode.h>

#include <pcilib.h>
#include <pcilib/tools.h>
#include <pcilib/error.h>

#include "model.h"
#include "private.h"
#include "reader.h"


int ipecamera_compute_buffer_size(ipecamera_t *ctx, size_t lines) {
    const size_t header_size = 8 * sizeof(ipecamera_payload_t);
    const size_t footer_size = 8 * sizeof(ipecamera_payload_t);

    size_t line_size, raw_size, padded_blocks;

    switch (ctx->firmware) {
     default:
	line_size = (1 + IPECAMERA_PIXELS_PER_CHANNEL) * 32; 
	raw_size = lines * line_size;
	raw_size *= 16 / ctx->cmosis_outputs;
	raw_size += header_size  + footer_size;

#ifdef IPECAMERA_BUG_MISSING_PAYLOAD
	    // As I understand, the first 32-byte packet is missing, so we need to substract 32
	raw_size -= 32;
#endif /* IPECAMERA_BUG_MISSING_PAYLOAD */
    }

    padded_blocks = raw_size / IPECAMERA_DMA_PACKET_LENGTH + ((raw_size % IPECAMERA_DMA_PACKET_LENGTH)?1:0);
    
    ctx->roi_raw_size = raw_size;
    ctx->roi_padded_size = padded_blocks * IPECAMERA_DMA_PACKET_LENGTH;
//    printf("%lu %lu\n", ctx->roi_raw_size, ctx->roi_padded_size);

    return 0;
}

static inline int ipecamera_new_frame(ipecamera_t *ctx) {
    ctx->frame[ctx->buffer_pos].event.raw_size = ctx->cur_size;

    if (ctx->cur_size < ctx->roi_raw_size) {
	ctx->frame[ctx->buffer_pos].event.info.flags |= PCILIB_EVENT_INFO_FLAG_BROKEN;
    }
    
    ctx->buffer_pos = (++ctx->event_id) % ctx->buffer_size;
    ctx->cur_size = 0;

    ctx->frame[ctx->buffer_pos].event.info.type = PCILIB_EVENT0;
    ctx->frame[ctx->buffer_pos].event.info.flags = 0;
    ctx->frame[ctx->buffer_pos].event.image_ready = 0;

    if ((ctx->event_id == ctx->autostop.evid)&&(ctx->event_id)) {
	ctx->run_reader = 0;
	return 1;
    }
	
    if (pcilib_check_deadline(&ctx->autostop.timestamp, 0)) {
	ctx->run_reader = 0;
	return 1;
    }
    
    return 0;
}

static uint32_t frame_magic[5] = { 0x51111111, 0x52222222, 0x53333333, 0x54444444, 0x55555555 };

static int ipecamera_data_callback(void *user, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    int res;
    int eof = 0;
    
    static unsigned long packet_id = 0;

#ifdef IPECAMERA_BUG_MULTIFRAME_PACKETS
    size_t real_size;
    size_t extra_data = 0;
#endif /* IPECAMERA_BUG_MULTIFRAME_PACKETS */
    
    ipecamera_t *ctx = (ipecamera_t*)user;

#if defined(IPECAMERA_BUG_INCOMPLETE_PACKETS)||defined(IPECAMERA_BUG_MULTIFRAME_PACKETS)
    static  pcilib_event_id_t invalid_frame_id = (pcilib_event_id_t)-1;
#endif

    packet_id++;
    ipecamera_debug_buffer(RAW_PACKETS, bufsize, buf, PCILIB_DEBUG_BUFFER_MKDIR, "frame%4lu/frame%9lu", ctx->event_id, packet_id);

    if (!ctx->cur_size) {
#if defined(IPECAMERA_BUG_INCOMPLETE_PACKETS)||defined(IPECAMERA_BUG_MULTIFRAME_PACKETS)
	size_t startpos;
	for (startpos = 0; (startpos + sizeof(frame_magic)) <= bufsize; startpos += sizeof(uint32_t)) {
	    if (!memcmp(buf + startpos, frame_magic, sizeof(frame_magic))) break;
	}
	
	if ((startpos + sizeof(frame_magic)) > bufsize) {
	    ipecamera_debug_buffer(RAW_PACKETS, bufsize, NULL, 0, "frame%4lu/frame%9lu.invalid", ctx->event_id, packet_id);
	    
	    if (invalid_frame_id != ctx->event_id) {
		ipecamera_debug(HARDWARE, "No frame magic in DMA packet of %u bytes, current event %lu", bufsize, ctx->event_id);
		invalid_frame_id = ctx->event_id;
	    }

	    return PCILIB_STREAMING_CONTINUE;
	}
	
	if (startpos) {
		// pass padding to rawdata callback
	    if (ctx->event.params.rawdata.callback) {
		res = ctx->event.params.rawdata.callback(0, NULL, PCILIB_EVENT_FLAG_RAW_DATA_ONLY, startpos, buf, ctx->event.params.rawdata.user);
		if (res <= 0) {
		    if (res < 0) return res;
		    ctx->run_reader = 0;
		}
	    }


	    buf += startpos;
	    bufsize -= startpos;
	}
#endif /* IPECAMERA_BUG_INCOMPLETE_PACKETS */

	if ((bufsize >= 8)&&(!memcmp(buf, frame_magic, sizeof(frame_magic)))) {
	    size_t n_lines = ((uint32_t*)buf)[5] & 0x7FF;
	    ipecamera_compute_buffer_size(ctx, n_lines);

	    ctx->frame[ctx->buffer_pos].event.info.seqnum = ((uint32_t*)buf)[6] & 0x1FFFFFF;
	    ctx->frame[ctx->buffer_pos].event.info.offset = (((uint32_t*)buf)[7] & 0xFFFFFF) * 80;
	    gettimeofday(&ctx->frame[ctx->buffer_pos].event.info.timestamp, NULL);
	} else {
	    ipecamera_debug(HARDWARE, "Frame magic is not found, ignoring broken data...");
	    return PCILIB_STREAMING_CONTINUE;
	}
    }

#ifdef IPECAMERA_BUG_MULTIFRAME_PACKETS
	// for rawdata_callback with complete padding
    real_size = bufsize;
    
    if (ctx->cur_size + bufsize > ctx->roi_raw_size) {
        size_t need;
	
	for (need = ctx->roi_raw_size - ctx->cur_size; (need + sizeof(frame_magic)) < bufsize; need += sizeof(uint32_t)) {
	    if (!memcmp(buf + need, frame_magic, sizeof(frame_magic))) break;
	}
	
	if ((need + sizeof(frame_magic)) < bufsize) {
	    extra_data = bufsize - need;
	    eof = 1;
	}

	    // just rip of padding
	bufsize = ctx->roi_raw_size - ctx->cur_size;
	ipecamera_debug_buffer(RAW_PACKETS, bufsize, buf, 0, "frame%4lu/frame%9lu.partial", ctx->event_id, packet_id);
    }
#endif /* IPECAMERA_BUG_MULTIFRAME_PACKETS */

    if (ctx->parse_data) {
	if (ctx->cur_size + bufsize > ctx->padded_size) {
    	    pcilib_error("Unexpected event data, we are expecting at maximum (%zu) bytes, but (%zu) already read", ctx->padded_size, ctx->cur_size + bufsize);
	    return -PCILIB_ERROR_TOOBIG;
	}

	if (bufsize)
	    memcpy(ctx->buffer + ctx->buffer_pos * ctx->padded_size +  ctx->cur_size, buf, bufsize);
    }

    ctx->cur_size += bufsize;

    if (ctx->cur_size >= ctx->roi_raw_size) {
	eof = 1;
    }

    if (ctx->event.params.rawdata.callback) {
	res = ctx->event.params.rawdata.callback(ctx->event_id, (pcilib_event_info_t*)(ctx->frame + ctx->buffer_pos), (eof?PCILIB_EVENT_FLAG_EOF:PCILIB_EVENT_FLAGS_DEFAULT), bufsize, buf, ctx->event.params.rawdata.user);
	if (res <= 0) {
	    if (res < 0) return res;
	    ctx->run_reader = 0;
	}
    }
    
    if (eof) {
	if ((ipecamera_new_frame(ctx))||(!ctx->run_reader)) {
	    return PCILIB_STREAMING_STOP;
	}
	
#ifdef IPECAMERA_BUG_MULTIFRAME_PACKETS
	if (extra_data) {
	    return ipecamera_data_callback(user, flags, extra_data, buf + (real_size - extra_data));
	}
#endif /* IPECAMERA_BUG_MULTIFRAME_PACKETS */
    }

    return PCILIB_STREAMING_REQ_FRAGMENT;
}

void *ipecamera_reader_thread(void *user) {
    int err;
    ipecamera_t *ctx = (ipecamera_t*)user;
    
    while (ctx->run_reader) {
	err = pcilib_stream_dma(ctx->event.pcilib, ctx->rdma, 0, 0, PCILIB_DMA_FLAG_MULTIPACKET, IPECAMERA_DMA_TIMEOUT, &ipecamera_data_callback, user);
	if (err) {
	    if (err == PCILIB_ERROR_TIMEOUT) {
		if (ctx->cur_size >= ctx->roi_raw_size) ipecamera_new_frame(ctx);
#ifdef IPECAMERA_BUG_INCOMPLETE_PACKETS
		else if (ctx->cur_size > 0) ipecamera_new_frame(ctx);
#endif /* IPECAMERA_BUG_INCOMPLETE_PACKETS */
		if (pcilib_check_deadline(&ctx->autostop.timestamp, 0)) {
		    ctx->run_reader = 0;
		    break;
		}
		usleep(IPECAMERA_NOFRAME_SLEEP);
	    } else pcilib_error("DMA error while reading IPECamera frames, error: %i", err);
	} 
    }
    
    ctx->run_streamer = 0;
    
    if (ctx->cur_size)
	pcilib_info("partialy read frame after stop signal, %zu bytes in the buffer", ctx->cur_size);

    return NULL;
}
