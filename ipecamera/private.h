#ifndef _IPECAMERA_PRIVATE_H
#define _IPECAMERA_PRIVATE_H

#include "ipecamera.h"

#define IPECAMERA_BUG_EXTRA_DATA
#define IPECAMERA_BUG_MULTIFRAME_PACKETS
#define IPECAMERA_BUG_INCOMPLETE_PACKETS

//#define IPECAMERA_ANNOUNCE_READY		//**< announce new event only after the reconstruction is done */

#define IPECAMERA_DEFAULT_BUFFER_SIZE 16//64  	//**< should be power of 2 */
#define IPECAMERA_RESERVE_BUFFERS 2		//**< Return Frame is Lost error, if requested frame will be overwritten after specified number of frames
#define IPECAMERA_SLEEP_TIME 250000 		//**< Michele thinks 250 should be enough, but reset failing in this case */
#define IPECAMERA_NEXT_FRAME_DELAY 1000 	//**< Michele requires 30000 to sync between End Of Readout and next Frame Req */
#define IPECAMERA_WAIT_FRAME_RCVD_TIME 0 	//**< by Uros ,wait 6 ms */
#define IPECAMERA_NOFRAME_SLEEP 100
#define IPECAMERA_NOFRAME_PREPROC_SLEEP 100

#define IPECAMERA_MAX_LINES 1088
#define IPECAMERA_EXPECTED_STATUS 0x08409FFFF
#define IPECAMERA_END_OF_SEQUENCE 0x1F001001

#define IPECAMERA_MAX_CHANNELS 16
#define IPECAMERA_PIXELS_PER_CHANNEL 128
#define IPECAMERA_WIDTH (IPECAMERA_MAX_CHANNELS * IPECAMERA_PIXELS_PER_CHANNEL)

#define IPECAMERA_FRAME_REQUEST 		0x1E9
#define IPECAMERA_READOUT_FLAG			0x200
#define IPECAMERA_READOUT			0x3E1
#define IPECAMERA_IDLE 				0x1E1
#define IPECAMERA_START_INTERNAL_STIMULI 	0x1F1


typedef uint32_t ipecamera_payload_t;

typedef struct {
    pcilib_event_id_t evid;
    struct timeval timestamp;
} ipecamera_autostop_t;

typedef struct {
    size_t i;
    pthread_t thread;
    ipecamera_t *ipecamera;
    
    int started;			/**< flag indicating that join & cleanup is required */
} ipecamera_preprocessor_t;


typedef struct {
    ipecamera_event_info_t event;	/**< this structure is overwritten by the reader thread, we need a copy */
    pthread_rwlock_t mutex;		/**< this mutex protects reconstructed buffers only, the raw data, event_info, etc. will be overwritten by reader thread anyway */
} ipecamera_frame_t;

struct ipecamera_s {
    pcilib_context_t event;
    ufo_decoder ipedec;

    char *data;
    ipecamera_pixel_t *image;
    size_t size;

    pcilib_event_callback_t cb;
    void *cb_user;

    volatile pcilib_event_id_t event_id;
    volatile pcilib_event_id_t preproc_id;
    pcilib_event_id_t reported_id;
    
    pcilib_dma_engine_t rdma, wdma;

    pcilib_register_t packet_len_reg;
    pcilib_register_t control_reg, status_reg;
    pcilib_register_t status3_reg;
    pcilib_register_t n_lines_reg;
    uint16_t line_reg;
    pcilib_register_t exposure_reg;
    pcilib_register_t flip_reg;

    int started;		/**< Camera is in grabbing mode (start function is called) */
    int streaming;		/**< Camera is in streaming mode (we are within stream call) */
    int parse_data;		/**< Indicates if some processing of the data is required, otherwise only rawdata_callback will be called */

    volatile int run_reader;		/**< Instructs the reader thread to stop processing */
    volatile int run_streamer;		/**< Indicates request to stop streaming events and can be set by reader_thread upon exit or by user request */
    volatile int run_preprocessors;	/**< Instructs preprocessors to exit */
    
    ipecamera_autostop_t autostop;

    struct timeval autostop_time;
    struct timeval next_trigger;	/**< The minimal delay between trigger signals is mandatory, this indicates time when next trigger is possible */

    size_t buffer_size;		/**< How many images to store */
    size_t buffer_pos;		/**< Current image offset in the buffer, due to synchronization reasons should not be used outside of reader_thread */
    size_t cur_size;		/**< Already written part of data in bytes */
    size_t raw_size;		/**< Size of raw data in bytes */
    size_t full_size;		/**< Size of raw data including the padding */
    size_t padded_size;		/**< Size of buffer for raw data, including the padding for performance */
    
    size_t image_size;		/**< Size of a single image in bytes */
    
    int width, height;

    
//    void *raw_buffer;
    void *buffer;
    ipecamera_change_mask_t *cmask;
    ipecamera_frame_t *frame;
    

    ipecamera_image_dimensions_t dim;

    pthread_t rthread;
    
    size_t n_preproc;
    ipecamera_preprocessor_t *preproc;
    pthread_mutex_t preproc_mutex;
    
    int preproc_mutex_destroy;
    int frame_mutex_destroy;
};

#endif /* _IPECAMERA_PRIVATE_H */
