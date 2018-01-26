#ifndef _IPECAMERA_H
#define _IPECAMERA_H

#include <ufodecode.h>

typedef struct ipecamera_s ipecamera_t;

typedef  struct {
    unsigned int bpp;			/*<< Bits per pixel (8, 16, or 32) as returned by IPECAMERA_IMAGE_DATA */
    unsigned int real_bpp;		/*<< Bits per pixel as returned by camera and IPECAMERA_PACKED_IMAGE */
    unsigned int width, height;
} ipecamera_image_dimensions_t;

typedef enum {
    IPECAMERA_IMAGE_DATA = 0,
    IPECAMERA_RAW_DATA = 1,
    IPECAMERA_DIMENSIONS = 0x8000,
    IPECAMERA_IMAGE_REGION = 0x8010,
    IPECAMERA_PACKED_IMAGE = 0x8020,
    IPECAMERA_PACKED_LINE = 0x8021,
    IPECAMERA_PACKED_PAYLOAD = 0x8022,
    IPECAMERA_CHANGE_MASK = 0x8030
} ipecamera_data_type_t;

typedef uint16_t ipecamera_change_mask_t;
typedef uint16_t ipecamera_pixel_t;

typedef struct {
    pcilib_event_info_t info;
    UfoDecoderMeta meta;	/**< Frame metadata declared in ufodecode.h */
    int image_ready;		/**< Indicates if image data is parsed */
    int image_broken;		/**< Unlike the info.flags this is bound to the reconstructed image (i.e. is not updated on rawdata overwrite) */
    size_t raw_size;		/**< Indicates the actual size of raw data */
} ipecamera_event_info_t;

#ifdef __cplusplus
extern "C" {
#endif

int ipecamera_set_buffer_size(ipecamera_t *ctx, int size);
pcilib_event_id_t ipecamera_get_last_event_id(ipecamera_t *ctx);

#ifdef __cplusplus
}
#endif


#endif /* _IPECAMERA_H */
