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


#define GET_REG(reg, var) \
    if (!err) { \
	err = pcilib_read_register_by_id(pcilib, ctx->reg, &var); \
	if (err) { \
	    pcilib_error("Error reading %s register", model_info->registers[ctx->reg].name); \
	} \
    }

#define SET_REG(reg, val) \
    if (!err) { \
	err = pcilib_write_register_by_id(pcilib, ctx->reg, val); \
	if (err) { \
	    pcilib_error("Error writting %s register", model_info->registers[ctx->reg].name); \
	} \
    }

/*
#define CHECK_FRAME_MAGIC(buf) \
	memcmp(buf, ((void*)frame_magic) + 1, sizeof(frame_magic) - 1)
*/

#define CHECK_FRAME_MAGIC(buf) \
	memcmp(((ipecamera_payload_t*)(buf)) + 1, &frame_magic[1], sizeof(frame_magic) - sizeof(ipecamera_payload_t))

static ipecamera_payload_t frame_magic[3] = { 0x51111111, 0x52222222, 0x53333333 };



int ipecamera_compute_buffer_size(ipecamera_t *ctx, ipecamera_format_t format, size_t header_size, size_t lines) {
//    const size_t header_size = 8 * sizeof(ipecamera_payload_t);
    const size_t footer_size = CMOSIS_FRAME_TAIL_SIZE;

    size_t max_channels;
    size_t line_size, raw_size, padded_blocks;

    switch (format) {
     case IPECAMERA_FORMAT_CMOSIS:
	max_channels = CMOSIS_MAX_CHANNELS;
	line_size = (1 + CMOSIS_PIXELS_PER_CHANNEL) * 32; 
	break;
     case IPECAMERA_FORMAT_CMOSIS20:
	max_channels = CMOSIS20_MAX_CHANNELS;
	line_size = (1 + CMOSIS20_PIXELS_PER_CHANNEL) * 32 / 2;
	break;
     default:
	pcilib_warning("Unsupported version (%u) of frame format...", format);
	return PCILIB_ERROR_NOTSUPPORTED;
    }

    raw_size = lines * line_size;
    raw_size *= max_channels / ctx->cmosis_outputs;
    raw_size += header_size + footer_size;

#ifdef IPECAMERA_BUG_MISSING_PAYLOAD
        // As I understand, the first 32-byte packet is missing, so we need to substract 32 (both CMOSIS and CMOSIS20)
    raw_size -= 32;
#endif /* IPECAMERA_BUG_MISSING_PAYLOAD */

    padded_blocks = raw_size / IPECAMERA_DMA_PACKET_LENGTH + ((raw_size % IPECAMERA_DMA_PACKET_LENGTH)?1:0);

    ctx->roi_raw_size = raw_size;
    ctx->roi_padded_size = padded_blocks * IPECAMERA_DMA_PACKET_LENGTH;

    return 0;
}


static int ipecamera_parse_header(ipecamera_t *ctx, ipecamera_payload_t *buf, size_t buf_size) {
    int last = buf[0] & 1;
    int version = (buf[0] >> 1) & 7;
    size_t size = 0, n_lines;
    ipecamera_format_t format = IPECAMERA_FORMAT_CMOSIS;

    switch (version) {
     case 0:
	n_lines = ((uint32_t*)buf)[5] & 0x7FF;
	ctx->frame[ctx->buffer_pos].event.info.seqnum = buf[6] & 0xFFFFFF;
	ctx->frame[ctx->buffer_pos].event.info.offset = (buf[7] & 0xFFFFFF) * 80;
	break;
     case 1:
	n_lines = ((uint32_t*)buf)[5] & 0xFFFF;
	if (!n_lines) {
	    pcilib_error("The frame header claims 0 lines in the data");
	    return 0;
	}

	ctx->frame[ctx->buffer_pos].event.info.seqnum = buf[6] & 0xFFFFFF;
	ctx->frame[ctx->buffer_pos].event.info.offset = (buf[7] & 0xFFFFFF) * 80;
	format = (buf[6] >> 24)&0x0F;
        break;
     default:
	ipecamera_debug(HARDWARE, "Incorrect version of the frame header, ignoring broken data...");
	return 0;
    }
    gettimeofday(&ctx->frame[ctx->buffer_pos].event.info.timestamp, NULL);

    ipecamera_debug(FRAME_HEADERS, "frame %lu: %x %x %x %x", ctx->frame[ctx->buffer_pos].event.info.seqnum, buf[0], buf[1], buf[2], buf[3]);
    ipecamera_debug(FRAME_HEADERS, "frame %lu: %x %x %x %x", ctx->frame[ctx->buffer_pos].event.info.seqnum, buf[4], buf[5], buf[6], buf[7]);

    while ((!last)&&((size + CMOSIS_FRAME_HEADER_SIZE) <= buf_size)) {
	size += CMOSIS_FRAME_HEADER_SIZE;
	last = buf[size] & 1;
    }

    size += CMOSIS_FRAME_HEADER_SIZE;
    ipecamera_compute_buffer_size(ctx, format, size, n_lines);

	// Returns total size of found headers or 0 on the error
    return size;
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
#ifdef IPECAMERA_BUG_MULTIFRAME_HEADERS
	if (ctx->saved_header_size) {
	    void *buf2 = alloca(ctx->saved_header_size + bufsize);
	    if (!buf2) {
		pcilib_error("Error allocating %zu bytes of memory in stack", ctx->saved_header_size + bufsize);
		return -PCILIB_ERROR_MEMORY;
	    }
	    memcpy(buf2, ctx->saved_header, ctx->saved_header_size);
	    memcpy(buf2 + ctx->saved_header_size, buf, bufsize);

	    buf = buf2;
	    bufsize += ctx->saved_header_size;

	    ctx->saved_header_size = 0;
	}
#endif /* IPECAMERA_BUG_MULTIFRAME_HEADERS */

#if defined(IPECAMERA_BUG_INCOMPLETE_PACKETS)||defined(IPECAMERA_BUG_MULTIFRAME_PACKETS)
	size_t startpos;
	for (startpos = 0; (startpos + CMOSIS_ENTITY_SIZE) <= bufsize; startpos += sizeof(ipecamera_payload_t)) {
	    if (!CHECK_FRAME_MAGIC(buf + startpos)) break;
	}
	
	if ((startpos +  CMOSIS_ENTITY_SIZE) > bufsize) {
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

	if ((bufsize >= CMOSIS_FRAME_HEADER_SIZE)&&(!CHECK_FRAME_MAGIC(buf))) {
		// We should handle the case when multi-header is split between multiple DMA packets
	    if (!ipecamera_parse_header(ctx, buf, bufsize))
		return PCILIB_STREAMING_CONTINUE;

#ifdef IPECAMERA_BUG_MULTIFRAME_HEADERS
	} else if ((bufsize >= CMOSIS_ENTITY_SIZE)&&(!CHECK_FRAME_MAGIC(buf))) {
	    memcpy(ctx->saved_header, buf, bufsize);
	    ctx->saved_header_size = bufsize;
	    return PCILIB_STREAMING_REQ_FRAGMENT;
#endif /* IPECAMERA_BUG_MULTIFRAME_HEADERS */
	} else {
	    ipecamera_debug(HARDWARE, "Frame magic is not found in the remaining DMA packet consisting of %u bytes, ignoring broken data...", bufsize);
	    return PCILIB_STREAMING_CONTINUE;
	}
    }

#ifdef IPECAMERA_BUG_MULTIFRAME_PACKETS
	// for rawdata_callback with complete padding
    real_size = bufsize;
    
    if (ctx->cur_size + bufsize > ctx->roi_raw_size) {
        size_t need;
	
	for (need = ctx->roi_raw_size - ctx->cur_size; (need + CMOSIS_ENTITY_SIZE) <= bufsize; need += sizeof(uint32_t)) {
	    if (!CHECK_FRAME_MAGIC(buf + need)) break;
	}
	
	if ((need + CMOSIS_ENTITY_SIZE) <= bufsize) {
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

	if (bufsize) {
#ifdef IPECAMERA_BUG_REPEATING_DATA
	    if ((bufsize > 16)&&(ctx->cur_size > 16)) {
		if (!memcmp(ctx->buffer + ctx->buffer_pos * ctx->padded_size +  ctx->cur_size - 16, buf, 16)) {
		    pcilib_warning("Skipping repeating bytes at offset %zu of frame %zu", ctx->cur_size, ctx->event_id);
		    buf += 16;
		    bufsize -=16;
		}
	    }
#endif /* IPECAMERA_BUG_REPEATING_DATA */
	    memcpy(ctx->buffer + ctx->buffer_pos * ctx->padded_size +  ctx->cur_size, buf, bufsize);
	}
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
#ifdef IPECAMERA_BUG_STUCKED_BUSY
    pcilib_register_value_t saved, value;
    pcilib_t *pcilib = ctx->event.pcilib;
    const pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);
#endif /* IPECAMERA_BUG_STUCKED_BUSY */

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
#ifdef IPECAMERA_BUG_STUCKED_BUSY
		GET_REG(status2_reg, value);
		if ((!err)&&(value&0x2FFFFFFF)) {
		    pcilib_warning("Camera stuck in busy, trying to recover...");
		    GET_REG(control_reg, saved);
		    SET_REG(control_reg, IPECAMERA_IDLE);
		    while ((value&0x2FFFFFFF)&&(ctx->run_reader)) {
			usleep(IPECAMERA_NOFRAME_SLEEP);
		    }
		}
#endif /* IPECAMERA_BUG_STUCKED_BUSY */
		usleep(IPECAMERA_NOFRAME_SLEEP);
	    } else pcilib_error("DMA error while reading IPECamera frames, error: %i", err);
	} 
    }
    
    ctx->run_streamer = 0;
    
    if (ctx->cur_size)
	pcilib_info("partialy read frame after stop signal, %zu bytes in the buffer", ctx->cur_size);

    return NULL;
}
