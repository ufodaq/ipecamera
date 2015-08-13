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

#include <pcilib.h>
#include <pcilib/tools.h>
#include <pcilib/error.h>
#include <pcilib/event.h>

#include "private.h"
#include "model.h"
#include "reader.h"
#include "events.h"
#include "data.h"


#define FIND_REG(var, bank, name)  \
        ctx->var = pcilib_find_register(pcilib, bank, name); \
	if (ctx->var ==  PCILIB_REGISTER_INVALID) { \
	    err = PCILIB_ERROR_NOTFOUND; \
	    pcilib_error("Unable to find a %s register", name); \
	}
    

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

#define CHECK_REG(reg, check) \
    if (!err) { \
	err = pcilib_read_register_by_id(pcilib, ctx->reg, &value); \
	if (err) { \
	    pcilib_error("Error reading %s register", model_info->registers[ctx->reg].name); \
	} \
	if (value != check) { \
	    pcilib_error("Unexpected value (0x%lx) of register %s", value, model_info->registers[ctx->reg].name); \
	    err = PCILIB_ERROR_INVALID_DATA; \
	} \
    }

#define CHECK_STATUS()
	    //CHECK_REG(status_reg, IPECAMERA_GET_EXPECTED_STATUS(ctx))

#define CHECK_VALUE(value, val) \
    if ((!err)&&(value != val)) { \
	pcilib_error("Unexpected value (0x%x) in data stream (0x%x is expected)", value, val); \
	err = PCILIB_ERROR_INVALID_DATA; \
    }

#define CHECK_FLAG(flag, check, ...) \
    if ((!err)&&(!(check))) { \
	pcilib_error("Unexpected value (0x%x) of " flag,  __VA_ARGS__); \
	err = PCILIB_ERROR_INVALID_DATA; \
    }

#define LOCK(lock_name) \
    err = pcilib_try_lock(ctx->lock_name##_lock); \
    if (err) { \
	pcilib_error("IPECamera is busy"); \
	return PCILIB_ERROR_BUSY; \
    } \
    ctx->lock_name##_locked = 1;

#define UNLOCK(lock_name) \
    if (ctx->lock_name##_locked) { \
	pcilib_unlock(ctx->lock_name##_lock); \
	ctx->lock_name##_locked = 0; \
    }

pcilib_context_t *ipecamera_init(pcilib_t *pcilib) {
    int err = 0; 
    
    const pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);

    ipecamera_t *ctx = malloc(sizeof(ipecamera_t));

    if (ctx) {
	pcilib_register_value_t value;
	
	memset(ctx, 0, sizeof(ipecamera_t));

	ctx->run_lock = pcilib_get_lock(pcilib, PCILIB_LOCK_FLAGS_DEFAULT, "ipecamera");
	ctx->stream_lock = pcilib_get_lock(pcilib, PCILIB_LOCK_FLAGS_DEFAULT, "ipecamera/stream");
	ctx->trigger_lock = pcilib_get_lock(pcilib, PCILIB_LOCK_FLAGS_DEFAULT, "ipecamera/trigger");

	if (!ctx->run_lock||!ctx->stream_lock||!ctx->trigger_lock) {
	    free(ctx);
	    pcilib_error("Failed to initialize locks to protect ipecamera operation");
	    return NULL;
	}

	ctx->dim.bpp = sizeof(ipecamera_pixel_t) * 8;
	ctx->buffer_size = IPECAMERA_DEFAULT_BUFFER_SIZE;

	FIND_REG(status_reg, "fpga", "status");
	FIND_REG(control_reg, "fpga", "control");

	FIND_REG(status2_reg, "fpga", "status2");
	FIND_REG(status3_reg, "fpga", "status3");

	FIND_REG(firmware_version_reg, "fpga", "firmware_version");
	FIND_REG(adc_resolution_reg, "fpga", "adc_resolution");
	FIND_REG(output_mode_reg, "fpga", "output_mode");
	
	FIND_REG(max_frames_reg, "fpga", "ddr_max_frames");
	FIND_REG(num_frames_reg, "fpga", "ddr_num_frames");

	GET_REG(firmware_version_reg, value);
	switch (value) {
	 case IPECAMERA_FIRMWARE_UFO5:
	    ctx->firmware = value;
	    err = pcilib_add_registers(pcilib, 0, cmosis_registers);
	    break;
	 case IPECAMERA_FIRMWARE_CMOSIS20:
	    ctx->firmware = value;
	    ctx->buffer_size = IPECAMERA_DEFAULT_CMOSIS20_BUFFER_SIZE;
	    err = pcilib_add_registers(pcilib, 0, cmosis20000_registers);
	    break;
	 default:
	    ctx->firmware = IPECAMERA_FIRMWARE_UNKNOWN;
    	    pcilib_warning("Unsupported version of firmware (%lu)", value);
	}

//	FIND_REG(n_lines_reg, "cmosis", "cmosis_number_lines"); // cmosis_number_lines_single v.6 ?
//	FIND_REG(line_reg, "cmosis", "cmosis_start1"); // cmosis_start_single v.6 ?
//	FIND_REG(exposure_reg, "cmosis", "cmosis_exp_time");
//	FIND_REG(flip_reg, "cmosis", "cmosis_image_flipping");


#ifdef IPECAMERA_ADJUST_BUFFER_SIZE 
	GET_REG(max_frames_reg, value);
	if ((value + IPECAMERA_RESERVE_BUFFERS + 3) > ctx->buffer_size) {
	    int val, bits = 0;
	    ctx->buffer_size = (value + 1) + IPECAMERA_RESERVE_BUFFERS + 2;
	    for (val = ctx->buffer_size; val; val = val >> 1) bits++;
	    ctx->buffer_size = 1 << bits; 
	}
#endif /* IPECAMERA_ADJUST_BUFFER_SIZE */


	ctx->rdma = PCILIB_DMA_ENGINE_INVALID;

	if (err) {
	    free(ctx);
	    return NULL;
	}
    }
    
    return (pcilib_context_t*)ctx;
}

void ipecamera_free(pcilib_context_t *vctx) {
    if (vctx) {
	ipecamera_t *ctx = (ipecamera_t*)vctx;
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);

	if (ctx->trigger_lock)
	    pcilib_return_lock(vctx->pcilib, PCILIB_LOCK_FLAGS_DEFAULT, ctx->trigger_lock);

	if (ctx->stream_lock)
	    pcilib_return_lock(vctx->pcilib, PCILIB_LOCK_FLAGS_DEFAULT, ctx->stream_lock);
	
	if (ctx->run_lock)
	    pcilib_return_lock(vctx->pcilib, PCILIB_LOCK_FLAGS_DEFAULT, ctx->run_lock);

	free(ctx);
    }
}

pcilib_dma_context_t *ipecamera_init_dma(pcilib_context_t *vctx) {
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    
    const pcilib_model_description_t *model_info = pcilib_get_model_description(vctx->pcilib);
    if ((!model_info->dma)||(!model_info->dma->api)||(!model_info->dma->api->init)) {
	pcilib_error("The DMA engine is not configured in model");
	return NULL;
    }

    if (ctx->firmware) {
	return model_info->dma->api->init(vctx->pcilib, NULL, NULL);
    } else {
	return model_info->dma->api->init(vctx->pcilib, "pci", NULL);
    }
}


int ipecamera_set_buffer_size(ipecamera_t *ctx, int size) {
    if (ctx->started) {
	pcilib_error("Can't change buffer size while grabbing");
	return PCILIB_ERROR_INVALID_REQUEST;
    }
    
    if (size < 2) {
	pcilib_error("The buffer size is too small");
	return PCILIB_ERROR_INVALID_REQUEST;
    }
    
    if ((size^(size-1)) < size) {
	pcilib_error("The buffer size is not power of 2");
    }
    
    ctx->buffer_size = size;
    
    return 0;
}

int ipecamera_reset(pcilib_context_t *vctx) {
    int err = 0;
    ipecamera_t *ctx = (ipecamera_t*)vctx;
    pcilib_t *pcilib = vctx->pcilib;

    pcilib_register_t control, status;
    pcilib_register_value_t value;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }
    
    pcilib = vctx->pcilib;
    control = ctx->control_reg;
    status = ctx->status_reg;

    LOCK(run);

    ipecamera_debug(API, "ipecamera: starting");

    if (ctx->firmware == IPECAMERA_FIRMWARE_UFO5) {
	    // Set Reset bit to CMOSIS
	err = pcilib_write_register_by_id(pcilib, control, 0x1e4);
	if (err) {
	    UNLOCK(run);
	    pcilib_error("Error setting FPGA reset bit");
	    return err;
	}
	usleep(IPECAMERA_CMOSIS_RESET_DELAY);

	// Remove Reset bit to CMOSIS
	err = pcilib_write_register_by_id(pcilib, control, 0x1e1);
	if (err) {
	    UNLOCK(run);
	    pcilib_error("Error reseting FPGA reset bit");
	    return err;
	}
	usleep(IPECAMERA_CMOSIS_REGISTER_DELAY);

	// Special settings for CMOSIS v.2
	value = 01; err = pcilib_write_register_space(pcilib, "cmosis", 115, 1, &value);
	if (err) {
	    UNLOCK(run);
	    pcilib_error("Error setting CMOSIS configuration");
	    return err;
	}
	usleep(IPECAMERA_CMOSIS_REGISTER_DELAY);

	value = 07; err = pcilib_write_register_space(pcilib, "cmosis", 82, 1, &value);
	if (err) {
	    UNLOCK(run);
	    pcilib_error("Error setting CMOSIS configuration");
	    return err;
	}
	usleep(IPECAMERA_CMOSIS_REGISTER_DELAY);
	pcilib_warning("Reset procedure is not complete");
    } else {
	pcilib_warning("Reset procedure is not implemented");
    }

	// Set default parameters
    err = pcilib_write_register_by_id(pcilib, control, IPECAMERA_IDLE);
    if (err) {
	UNLOCK(run);
	pcilib_error("Error bringing FPGA in default mode");
	return err;
    }

    usleep(10000);


    CHECK_STATUS();
    if (err) {
	err = pcilib_read_register_by_id(pcilib, status, &value);

	UNLOCK(run);
	if (err) pcilib_error("Error reading status register");
	else pcilib_error("Camera returns unexpected status (status: %lx)", value);

	return PCILIB_ERROR_VERIFY;
    }

    err = pcilib_skip_dma(vctx->pcilib, ctx->rdma);
    UNLOCK(run);

    ipecamera_debug(API, "ipecamera: reset done");
    return err;
}


int ipecamera_start(pcilib_context_t *vctx, pcilib_event_t event_mask, pcilib_event_flags_t flags) {
    int i;
    int err = 0;

    ipecamera_t *ctx = (ipecamera_t*)vctx;
    pcilib_t *pcilib = vctx->pcilib;
    pcilib_register_value_t value;
    
    const pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);

    pthread_attr_t attr;
    struct sched_param sched;
    
    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (ctx->started) {
	pcilib_error("IPECamera grabbing is already started");
	return PCILIB_ERROR_INVALID_REQUEST;
    }

    LOCK(run);

    ipecamera_debug(API, "ipecamera: starting");

    ctx->event_id = 0;
    ctx->preproc_id = 0;
    ctx->reported_id = 0;
    ctx->buffer_pos = 0;
    ctx->parse_data = (flags&PCILIB_EVENT_FLAG_RAW_DATA_ONLY)?0:1;
    ctx->cur_size = 0;

#ifdef IPECAMERA_BUG_MULTIFRAME_HEADERS
    ctx->saved_header_size = 0;
#endif /* IPECAMERA_BUG_MULTIFRAME_HEADERS */

    switch (ctx->firmware) {
     case IPECAMERA_FIRMWARE_UFO5:
	ctx->dim.width = CMOSIS_WIDTH;
	ctx->dim.height = CMOSIS_MAX_LINES;
	break;
     case IPECAMERA_FIRMWARE_CMOSIS20:
	ctx->dim.width = CMOSIS20_WIDTH;
	ctx->dim.height = CMOSIS20_MAX_LINES;
	ctx->cmosis_outputs = CMOSIS20_MAX_CHANNELS;
	break;
     default:
	UNLOCK(run);
	pcilib_error("Can't start undefined version (%lu) of IPECamera", ctx->firmware);
	return PCILIB_ERROR_INVALID_REQUEST;
    }

    if (ctx->firmware == IPECAMERA_FIRMWARE_UFO5) {
	GET_REG(output_mode_reg, value);
	switch (value) {
	 case IPECAMERA_MODE_16_CHAN_IO:
	    ctx->cmosis_outputs = 16;
	    break;
         case IPECAMERA_MODE_4_CHAN_IO:
	    ctx->cmosis_outputs = 4;
	    break;
	 default:
	    UNLOCK(run);
	    pcilib_error("IPECamera reporting invalid output_mode 0x%lx", value);
	    return PCILIB_ERROR_INVALID_STATE;
	}
    }

	// We should be careful here (currently firmware matches format, but this may not be the case in future)
    ipecamera_compute_buffer_size(ctx, ctx->firmware, CMOSIS_FRAME_HEADER_SIZE, ctx->dim.height);

    ctx->raw_size = ctx->roi_raw_size;
    ctx->padded_size = ctx->roi_padded_size;

    ctx->image_size = ctx->dim.width * ctx->dim.height;

    
    GET_REG(max_frames_reg, value);
    ctx->max_frames = value;

    ctx->buffer = malloc(ctx->padded_size * ctx->buffer_size);
    if (!ctx->buffer) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate ring buffer (%lu bytes)", ctx->padded_size * ctx->buffer_size);
	return PCILIB_ERROR_MEMORY;
    }

    ctx->image = (ipecamera_pixel_t*)malloc(ctx->image_size * ctx->buffer_size * sizeof(ipecamera_pixel_t));
    if (!ctx->image) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate image buffer (%lu bytes)", ctx->image_size * ctx->buffer_size * sizeof(ipecamera_pixel_t));
	return PCILIB_ERROR_MEMORY;
    }

    ctx->cmask = malloc(ctx->dim.height * ctx->buffer_size * sizeof(ipecamera_change_mask_t));
    if (!ctx->cmask) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate change-mask buffer");
	return PCILIB_ERROR_MEMORY;
    }

    ctx->frame = (ipecamera_frame_t*)malloc(ctx->buffer_size * sizeof(ipecamera_frame_t));
    if (!ctx->frame) {
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to allocate frame-info buffer");
	return PCILIB_ERROR_MEMORY;
    }
    
    memset(ctx->frame, 0, ctx->buffer_size * sizeof(ipecamera_frame_t));
    
    for (i = 0; i < ctx->buffer_size; i++) {
	err = pthread_rwlock_init(&ctx->frame[i].mutex, NULL);
	if (err) break;
    }

    ctx->frame_mutex_destroy = i;

    if (err) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Initialization of rwlock mutexes for frame synchronization has failed");
	return PCILIB_ERROR_FAILED;
    }
    
    ctx->ipedec = ufo_decoder_new(ctx->dim.height, ctx->dim.width, NULL, 0);
    if (!ctx->ipedec) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Unable to initialize IPECamera decoder library");
	return PCILIB_ERROR_FAILED;
    }

    if (!err) {
	ctx->rdma = pcilib_find_dma_by_addr(vctx->pcilib, PCILIB_DMA_FROM_DEVICE, IPECAMERA_DMA_ADDRESS);
	if (ctx->rdma == PCILIB_DMA_ENGINE_INVALID) {
	    err = PCILIB_ERROR_NOTFOUND;
	    pcilib_error("The C2S channel of IPECamera DMA Engine (%u) is not found", IPECAMERA_DMA_ADDRESS);
	} else {
	    err = pcilib_start_dma(vctx->pcilib, ctx->rdma, PCILIB_DMA_FLAGS_DEFAULT);
	    if (err) {
		ctx->rdma = PCILIB_DMA_ENGINE_INVALID;
		pcilib_error("Failed to initialize C2S channel of IPECamera DMA Engine (%u)", IPECAMERA_DMA_ADDRESS);
	    }
	}
    }
    
    if (err) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	return err;
    }

#ifdef IPECAMERA_CLEAN_ON_START
    err = pcilib_skip_dma(vctx->pcilib, ctx->rdma);
    if (err) {
        ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	pcilib_error("Can't start grabbing, device continuously writes unexpected data using DMA engine");
	return err;
    }
#endif /* IPECAMERA_CLEAN_ON_START */

    if (vctx->params.autostop.duration) {
	gettimeofday(&ctx->autostop.timestamp, NULL);
	ctx->autostop.timestamp.tv_usec += vctx->params.autostop.duration % 1000000;
	if (ctx->autostop.timestamp.tv_usec > 999999) {
	    ctx->autostop.timestamp.tv_sec += 1 + vctx->params.autostop.duration / 1000000;
	    ctx->autostop.timestamp.tv_usec -= 1000000;
	} else {
	    ctx->autostop.timestamp.tv_sec += vctx->params.autostop.duration / 1000000;
	}
    }
    
    if (vctx->params.autostop.max_events) {
	ctx->autostop.evid = vctx->params.autostop.max_events;
    }
    
    if ((ctx->parse_data)&&(flags&PCILIB_EVENT_FLAG_PREPROCESS)) {
	ctx->n_preproc = pcilib_get_cpu_count();
	
	    // it would be greate to detect hyperthreading cores and ban them
	switch (ctx->n_preproc) {
	    case 1: break;
	    case 2 ... 3: ctx->n_preproc -= 1; break;
	    default: ctx->n_preproc -= 2; break;
	}

	if ((vctx->params.parallel.max_threads)&&(vctx->params.parallel.max_threads < ctx->n_preproc))
	    ctx->n_preproc = vctx->params.parallel.max_threads;

	ctx->preproc = (ipecamera_preprocessor_t*)malloc(ctx->n_preproc * sizeof(ipecamera_preprocessor_t));
	if (!ctx->preproc) {
	    ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	    pcilib_error("Unable to allocate memory for preprocessor contexts");
	    return PCILIB_ERROR_MEMORY;
	}

	memset(ctx->preproc, 0, ctx->n_preproc * sizeof(ipecamera_preprocessor_t));

	err = pthread_mutex_init(&ctx->preproc_mutex, NULL);
	if (err) {
	    ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	    pcilib_error("Failed to initialize event mutex");
	    return PCILIB_ERROR_FAILED;
	}
	ctx->preproc_mutex_destroy = 1;
	

	ctx->run_preprocessors = 1;
	for (i = 0; i < ctx->n_preproc; i++) {
	    ctx->preproc[i].i = i;
	    ctx->preproc[i].ipecamera = ctx;
	    err = pthread_create(&ctx->preproc[i].thread, NULL, ipecamera_preproc_thread, ctx->preproc + i);
	    if (err) {
		err = PCILIB_ERROR_FAILED;
		break;
	    } else {
		ctx->preproc[i].started = 1;
	    }
	}
	
	if (err) {
	    ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	    pcilib_error("Failed to schedule some of the preprocessor threads");
	    return err;
	}
    } else {
	ctx->n_preproc = 0;
    }

    ctx->started = 1;
    ctx->run_reader = 1;

    pthread_attr_init(&attr);

    if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
	pcilib_warning("Can't schedule a real-time thread, you may consider running as root");
    } else {
	sched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;	// Let 1 priority for something really critcial
	pthread_attr_setschedparam(&attr, &sched);
    }

    if (pthread_create(&ctx->rthread, &attr, &ipecamera_reader_thread, (void*)ctx)) {
	ctx->started = 0;
	ipecamera_stop(vctx, PCILIB_EVENT_FLAGS_DEFAULT);
	err = PCILIB_ERROR_FAILED;
    }
    
    pthread_attr_destroy(&attr);    

    ipecamera_debug(API, "ipecamera: started");

    return err;
}


int ipecamera_stop(pcilib_context_t *vctx, pcilib_event_flags_t flags) {
    int i;
    int err;
    void *retcode;
    ipecamera_t *ctx = (ipecamera_t*)vctx;

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    if (flags&PCILIB_EVENT_FLAG_STOP_ONLY) {
	ctx->run_reader = 0;
	return 0;
    }

    ipecamera_debug(API, "ipecamera: stopping");

    if (ctx->started) {
	ctx->run_reader = 0;
	err = pthread_join(ctx->rthread, &retcode);
	if (err) pcilib_error("Error joining the reader thread");
    }
    
    if (ctx->preproc) {
	ctx->run_preprocessors = 0;
	
	for (i = 0; i < ctx->n_preproc; i++) {
	    if (ctx->preproc[i].started) {
		pthread_join(ctx->preproc[i].thread, &retcode);
		ctx->preproc[i].started = 0;
	    }
	}

	if (ctx->preproc_mutex_destroy) {
	    pthread_mutex_destroy(&ctx->preproc_mutex);
	    ctx->preproc_mutex_destroy = 0;
	}
	
	free(ctx->preproc);
	ctx->preproc = NULL;
    }
    
    if (ctx->frame_mutex_destroy) {
	for (i = 0; i < ctx->frame_mutex_destroy; i++) {
	    pthread_rwlock_destroy(&ctx->frame[i].mutex);
	}
	ctx->frame_mutex_destroy = 0;
    }

    if (ctx->rdma != PCILIB_DMA_ENGINE_INVALID) {
	pcilib_stop_dma(vctx->pcilib, ctx->rdma, PCILIB_DMA_FLAGS_DEFAULT);
	ctx->rdma = PCILIB_DMA_ENGINE_INVALID;
    }

    while (ctx->streaming) {
        usleep(IPECAMERA_NOFRAME_SLEEP);
    }

    if (ctx->ipedec) {
	ufo_decoder_free(ctx->ipedec);
	ctx->ipedec = NULL;
    }

    if (ctx->frame) {
	free(ctx->frame);
	ctx->frame = NULL;
    }

    if (ctx->cmask) {
	free(ctx->cmask);
	ctx->cmask = NULL;
    }

    if (ctx->image) {
	free(ctx->image);
	ctx->image = NULL;
    }

    if (ctx->buffer) {
	free(ctx->buffer);
	ctx->buffer = NULL;
    }
    

    memset(&ctx->autostop, 0, sizeof(ipecamera_autostop_t));

    ctx->event_id = 0;
    ctx->reported_id = 0;
    ctx->buffer_pos = 0; 
    ctx->started = 0;

    ipecamera_debug(API, "ipecamera: stopped");
    UNLOCK(run);

    return 0;
}


int ipecamera_trigger(pcilib_context_t *vctx, pcilib_event_t event, size_t trigger_size, void *trigger_data) {
    int err = 0;
    pcilib_register_value_t value;

    ipecamera_t *ctx = (ipecamera_t*)vctx;
    pcilib_t *pcilib = vctx->pcilib;

    const pcilib_model_description_t *model_info = pcilib_get_model_description(pcilib);

    if (!ctx) {
	pcilib_error("IPECamera imaging is not initialized");
	return PCILIB_ERROR_NOTINITIALIZED;
    }

    ipecamera_debug(API, "ipecamera: trigger");
    LOCK(trigger);

    pcilib_sleep_until_deadline(&ctx->next_trigger);
/*
    GET_REG(num_frames_reg, value);
    if (value == ctx->max_frames) {
	return PCILIB_ERROR_BUSY;
    }
*/

    GET_REG(status2_reg, value);
    if (value&0x40000000) {
	if (value == 0xffffffff)
	    pcilib_info("Failed to read status2_reg while triggering");

#ifdef IPECAMERA_TRIGGER_TIMEOUT
	if (IPECAMERA_TRIGGER_TIMEOUT) {
	    struct timeval deadline;
	    pcilib_calc_deadline(&deadline, IPECAMERA_TRIGGER_TIMEOUT);
	    do {
		usleep(IPECAMERA_READ_STATUS_DELAY);
		GET_REG(status2_reg, value);
	    } while ((value&0x40000000)&&(pcilib_calc_time_to_deadline(&deadline) > 0));
	}
	if (value&0x40000000) {
#endif /* IPECAMERA_TRIGGER_WAIT_IDLE */
	    UNLOCK(trigger);
	    return PCILIB_ERROR_BUSY;
#ifdef IPECAMERA_TRIGGER_TIMEOUT
	}
#endif /* IPECAMERA_TRIGGER_WAIT_IDLE */
    }

    GET_REG(control_reg, value);
    SET_REG(control_reg, value|IPECAMERA_FRAME_REQUEST);
    usleep(IPECAMERA_TRIGGER_DELAY);
    SET_REG(control_reg, value);

	// DS: We need to compute it differently, on top of that add exposure time and the time FPGA takes to read frame from CMOSIS
    pcilib_calc_deadline(&ctx->next_trigger, IPECAMERA_NEXT_FRAME_DELAY);
    
    UNLOCK(trigger);

    return 0;
}
