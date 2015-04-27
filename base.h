#ifndef _IPECAMERA_BASE_H
#define _IPECAMERA_BASE_H

#include <stdio.h>

#include <pcilib.h>
#include "ipecamera.h"

pcilib_context_t *ipecamera_init(pcilib_t *pcilib);
void ipecamera_free(pcilib_context_t *ctx);

pcilib_dma_context_t *ipecamera_init_dma(pcilib_context_t *ctx);

int ipecamera_reset(pcilib_context_t *ctx);
int ipecamera_start(pcilib_context_t *ctx, pcilib_event_t event_mask, pcilib_event_flags_t flags);
int ipecamera_stop(pcilib_context_t *ctx, pcilib_event_flags_t flags);
int ipecamera_trigger(pcilib_context_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);
int ipecamera_stream(pcilib_context_t *vctx, pcilib_event_callback_t callback, void *user);
int ipecamera_next_event(pcilib_context_t *vctx, pcilib_timeout_t timeout, pcilib_event_id_t *evid, size_t info_size, pcilib_event_info_t *info);

int ipecamera_get(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size, void **buf);
int ipecamera_return(pcilib_context_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, void *data);

#endif /* _IPECAMERA_BASE_H */
