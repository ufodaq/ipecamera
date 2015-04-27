#ifndef _IPECAMERA_MODEL_H
#define _IPECAMERA_MODEL_H

#include <stdio.h>
#include <pcilib/model.h>

//#define IPECAMERA_DEBUG

#define IPECAMERA_DMA_ADDRESS 			0					/**< Address of DMA engine to use for communication */
#define IPECAMERA_DMA_PACKET_LENGTH 		4096					/**< IPECamera always use buffers of fixed size adding padding in the end. 
											This is used to compute expected amount of data for each frame */
#define IPECAMERA_REGISTER_SPACE 0x9000
#define IPECAMERA_CMOSIS_REGISTER_WRITE 	(IPECAMERA_REGISTER_SPACE + 0)
#define IPECAMERA_CMOSIS_REGISTER_READ 		(IPECAMERA_REGISTER_SPACE + 16)

const pcilib_model_description_t *pcilib_get_event_model(pcilib_t *pcilib, unsigned short vendor_id, unsigned short device_id, const char *model);

#endif /* _IPECAMERA_MODEL_H */
