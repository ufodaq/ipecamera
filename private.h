#ifndef _IPECAMERA_PRIVATE_H
#define _IPECAMERA_PRIVATE_H

#include <pthread.h>
#include <pcilib/model.h>
#include <pcilib/debug.h>
#include "ipecamera.h"

#define IPECAMERA_DEBUG
#ifdef IPECAMERA_DEBUG
//# define IPECAMERA_DEBUG_RAW_FRAMES		//**< Store all raw frames */
# define IPECAMERA_DEBUG_BROKEN_FRAMES		//**< Store broken frames in the specified directory */
# define IPECAMERA_DEBUG_RAW_PACKETS		//**< Store all raw packets read from DMA grouped in frames */
# define IPECAMERA_DEBUG_HARDWARE		//**< Produce various debugging information about ipecamera operation */
#endif /* IPECAMERA_DEBUG */

//#define IPECAMERA_BUG_MISSING_PAYLOAD		//**< CMOSIS fails to provide a first payload for each frame, therefore the frame is 32 bit shorter */
#define IPECAMERA_BUG_MULTIFRAME_PACKETS	//**< This is by design, start of packet comes directly after the end of last one in streaming mode */
//#define IPECAMERA_BUG_INCOMPLETE_PACKETS	//**< Support incomplete packets, i.e. check for frame magic even if full frame size is not reached yet (slow) */
//#define IPECAMERA_ANNOUNCE_READY		//**< Announce new event only after the reconstruction is done */
//#define IPECAMERA_CLEAN_ON_START		//**< Read all the data from DMA before starting of recording */
#define IPECAMERA_ADJUST_BUFFER_SIZE		//**< Adjust default buffer size based on the hardware capabilities */

#define IPECAMERA_DEFAULT_BUFFER_SIZE 64  	//**< should be power of 2 */
#define IPECAMERA_RESERVE_BUFFERS 2		//**< Return Frame is Lost error, if requested frame will be overwritten after specified number of frames

#define IPECAMERA_DMA_TIMEOUT 50000		//**< Default DMA timeout */
#define IPECAMERA_TRIGGER_TIMEOUT 200000	//**< In trigger call allow specified timeout for camera to get out of busy state. Set 0 to fail immideatly */
#define IPECAMERA_CMOSIS_RESET_DELAY 250000 	//**< Michele thinks 250 should be enough, but reset failing in this case */
#define IPECAMERA_CMOSIS_REGISTER_DELAY 250000 	//**< Michele thinks 250 should be enough, but reset failing in this case */
#define IPECAMERA_SPI_REGISTER_DELAY 10000	//**< Delay between consequitive access to the registers */
#define IPECAMERA_NEXT_FRAME_DELAY 1000 	//**< Michele requires 30000 to sync between End Of Readout and next Frame Req */
#define IPECAMERA_TRIGGER_DELAY 0 		//**< Defines how long the trigger bits should be set */
#define IPECAMERA_READ_STATUS_DELAY 1000	//**< According to Uros, 1ms delay needed before consequitive reads from status registers */
#define IPECAMERA_NOFRAME_SLEEP 100		//**< Sleep while polling for a new frame in reader */
#define IPECAMERA_NOFRAME_PREPROC_SLEEP 100	//**< Sleep while polling for a new frame in pre-processor */

//#define IPECAMERA_MAX_LINES 1088
#define IPECAMERA_MAX_LINES 2048
#define IPECAMERA_EXPECTED_STATUS_4 0x08409FFFF
#define IPECAMERA_EXPECTED_STATUS 0x08449FFFF

#define IPECAMERA_END_OF_SEQUENCE 0x1F001001

#define IPECAMERA_MAX_CHANNELS 16
#define IPECAMERA_PIXELS_PER_CHANNEL 128
#define IPECAMERA_WIDTH (IPECAMERA_MAX_CHANNELS * IPECAMERA_PIXELS_PER_CHANNEL)

#define IPECAMERA_FRAME_REQUEST 		0x80000209 // 0x1E9
#define IPECAMERA_IDLE 				0x80000201 // 0x1E1
#define IPECAMERA_START_INTERNAL_STIMULI 	0x1F1

#define IPECAMERA_MODE_16_CHAN_IO		0
#define IPECAMERA_MODE_4_CHAN_IO		2

#define IPECAMERA_MODE_12_BIT_ADC		2
#define IPECAMERA_MODE_11_BIT_ADC		1
#define IPECAMERA_MODE_10_BIT_ADC		0

#ifdef IPECAMERA_DEBUG_RAW_FRAMES
# define IPECAMERA_DEBUG_RAW_FRAMES_MESSAGE(function, ...) pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__) 
# define IPECAMERA_DEBUG_RAW_FRAMES_BUFFER(function, ...) pcilib_debug_data_buffer (#function, __VA_ARGS__) 
#else /* IPECAMERA_DEBUG_RAW_FRAMES */
# define IPECAMERA_DEBUG_RAW_FRAMES_MESSAGE(function, ...)
# define IPECAMERA_DEBUG_RAW_FRAMES_BUFFER(function, ...)
#endif /* IPECAMERA_DEBUG_RAW_FRAMES */

#ifdef IPECAMERA_DEBUG_BROKEN_FRAMES
# define IPECAMERA_DEBUG_BROKEN_FRAMES_MESSAGE(function, ...) pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__) 
# define IPECAMERA_DEBUG_BROKEN_FRAMES_BUFFER(function, ...) pcilib_debug_data_buffer (#function, __VA_ARGS__) 
#else /* IPECAMERA_DEBUG_BROKEN_FRAMES */
# define IPECAMERA_DEBUG_BROKEN_FRAMES_MESSAGE(function, ...)
# define IPECAMERA_DEBUG_BROKEN_FRAMES_BUFFER(function, ...)
#endif /* IPECAMERA_DEBUG_BROKEN_FRAMES */

#ifdef IPECAMERA_DEBUG_RAW_PACKETS
# define IPECAMERA_DEBUG_RAW_PACKETS_MESSAGE(function, ...) pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__) 
# define IPECAMERA_DEBUG_RAW_PACKETS_BUFFER(function, ...) pcilib_debug_data_buffer (#function, __VA_ARGS__) 
#else /* IPECAMERA_DEBUG_RAW_PACKETS */
# define IPECAMERA_DEBUG_RAW_PACKETS_MESSAGE(function, ...)
# define IPECAMERA_DEBUG_RAW_PACKETS_BUFFER(function, ...)
#endif /* IPECAMERA_DEBUG_RAW_PACKETS */

#ifdef IPECAMERA_DEBUG_HARDWARE
# define IPECAMERA_DEBUG_HARDWARE_MESSAGE(function, ...) pcilib_debug_message (#function, __FILE__, __LINE__, __VA_ARGS__) 
# define IPECAMERA_DEBUG_HARDWARE_BUFFER(function, ...) pcilib_debug_data_buffer (#function, __VA_ARGS__) 
#else /* IPECAMERA_DEBUG_HARDWARE */
# define IPECAMERA_DEBUG_HARDWARE_MESSAGE(function, ...)
# define IPECAMERA_DEBUG_HARDWARE_BUFFER(function, ...)
#endif /* IPECAMERA_DEBUG_HARDWARE */


#define ipecamera_debug(function, ...) \
    IPECAMERA_DEBUG_##function##_MESSAGE(IPECAMERA_DEBUG_##function, PCILIB_LOG_DEFAULT, __VA_ARGS__)

#define ipecamera_debug_buffer(function, ...) \
    IPECAMERA_DEBUG_##function##_BUFFER(IPECAMERA_DEBUG_##function, __VA_ARGS__)


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
    UfoDecoder *ipedec;

    char *data;
    ipecamera_pixel_t *image;
    size_t size;

    volatile pcilib_event_id_t event_id;
    volatile pcilib_event_id_t preproc_id;
    pcilib_event_id_t reported_id;

    pcilib_dma_engine_t rdma;

    pcilib_register_t control_reg, status_reg;
    pcilib_register_t status2_reg, status3_reg;
    pcilib_register_t n_lines_reg;
    uint16_t line_reg;
    pcilib_register_t exposure_reg;
    pcilib_register_t flip_reg;

    pcilib_register_t firmware_version_reg;
    pcilib_register_t adc_resolution_reg;
    pcilib_register_t output_mode_reg;
    
    pcilib_register_t max_frames_reg;
    pcilib_register_t num_frames_reg;

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
    size_t raw_size;		/**< Expected maximum size of raw data in bytes */
    size_t padded_size;		/**< Expected maximum size of buffer for raw data, including additional padding */
    size_t roi_raw_size;	/**< Expected size (for currently configured ROI) of raw data in bytes */
    size_t roi_padded_size;	/**< Expected size (for currently configured ROI) of buffer for raw data, including additional padding */
    
    size_t image_size;		/**< Size of a single image in bytes */
    
    size_t max_frames;		/**< Maximal number of frames what may be buffered in camera DDR memory */
    int firmware;		/**< Firmware version */
    int cmosis_outputs;		/**< Number of active cmosis outputs: 4 or 16 */
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
