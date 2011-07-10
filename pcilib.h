#ifndef _PCITOOL_PCILIB_H
#define _PCITOOL_PCILIB_H

#define PCILIB_MAX_BANKS 6
#define PCILIB_MAX_DMA_ENGINES 32

#include <time.h>
#include <stdint.h>

#ifndef __timespec_defined
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#endif /* __timespec_defined */

#define pcilib_memcpy pcilib_memcpy32
#define pcilib_datacpy pcilib_datacpy32

typedef struct pcilib_s pcilib_t;
typedef void pcilib_context_t;
typedef void pcilib_dma_context_t;

typedef struct pcilib_dma_api_description_s pcilib_dma_api_description_t;
typedef struct pcilib_event_api_description_s pcilib_event_api_description_t;
typedef struct  pcilib_protocol_description_s pcilib_protocol_description_t;
typedef unsigned int pcilib_irq_source_t;

typedef uint8_t pcilib_bar_t;			/**< Type holding the PCI Bar number */
typedef uint8_t pcilib_register_t;		/**< Type holding the register ID within the Bank */
typedef uint32_t pcilib_register_addr_t;	/**< Type holding the register ID within the Bank */
typedef uint8_t pcilib_register_bank_t;		/**< Type holding the register bank number */
typedef uint8_t pcilib_register_bank_addr_t;	/**< Type holding the register bank number */
typedef uint8_t pcilib_register_size_t;		/**< Type holding the size in bits of the register */
typedef uint32_t pcilib_register_value_t;	/**< Type holding the register value */
typedef uint8_t pcilib_dma_engine_addr_t;
typedef uint8_t pcilib_dma_engine_t;
typedef uint64_t pcilib_event_id_t;
typedef uint32_t pcilib_event_t;

typedef enum {
    PCILIB_HOST_ENDIAN = 0,
    PCILIB_LITTLE_ENDIAN,
    PCILIB_BIG_ENDIAN
} pcilib_endianess_t;

typedef enum {
    PCILIB_MODEL_DETECT,
    PCILIB_MODEL_PCI,
    PCILIB_MODEL_IPECAMERA
} pcilib_model_t;

typedef enum {
    PCILIB_REGISTER_R = 1,
    PCILIB_REGISTER_W = 2,
    PCILIB_REGISTER_RW = 3,
    PCILIB_REGISTER_W1C = 4,		/**< writting 1 resets the flag */
    PCILIB_REGISTER_RW1C = 5
} pcilib_register_mode_t;

typedef enum {
    PCILIB_DEFAULT_PROTOCOL,
    IPECAMERA_REGISTER_PROTOCOL
} pcilib_register_protocol_t;

typedef enum {
    PCILIB_EVENT_DATA
} pcilib_event_data_type_t;

typedef enum {
    PCILIB_DMA_FLAGS_DEFAULT = 0,
    PCILIB_DMA_FLAG_EOP = 1
} pcilib_dma_flags_t;

typedef enum {
    PCILIB_REGISTER_STANDARD = 0,
    PCILIB_REGISTER_FIFO,
    PCILIB_REGISTER_BITS
} pcilib_register_type_t;

#define PCILIB_BAR_DETECT 		((pcilib_bar_t)-1)
#define PCILIB_BAR_INVALID		((pcilib_bar_t)-1)
#define PCILIB_BAR0			0
#define PCILIB_BAR1			1
#define PCILIB_DMA_INVALID		((pcilib_dma_engine_t)-1)
#define PCILIB_REGISTER_INVALID		((pcilib_register_t)-1)
#define PCILIB_ADDRESS_INVALID		((uintptr_t)-1)
#define PCILIB_REGISTER_BANK_INVALID	((pcilib_register_bank_t)-1)
#define PCILIB_REGISTER_BANK0 		0
#define PCILIB_REGISTER_BANK1 		1
#define PCILIB_REGISTER_BANK2 		2
#define PCILIB_REGISTER_BANK3 		3
#define PCILIB_REGISTER_BANK_DMA	128
#define PCILIB_EVENT0			1
#define PCILIB_EVENT1			2
#define PCILIB_EVENT2			4
#define PCILIB_EVENT3			8
#define PCILIB_EVENTS_ALL		((pcilib_event_t)-1)
#define PCILIB_EVENT_INVALID		((pcilib_event_t)-1)
#define PCILIB_EVENT_ID_INVALID		0
#define PCILIB_TIMEOUT_INFINITE		((size_t)-1)
#define PCILIB_TIMEOUT_IMMEDIATE	0

typedef int (*pcilib_dma_callback_t)(void *ctx, pcilib_dma_flags_t flags, size_t bufsize, void *buf);
typedef int (*pcilib_event_callback_t)(pcilib_event_t event, pcilib_event_id_t event_id, void *user);

typedef struct {
    pcilib_register_bank_addr_t addr;

    pcilib_bar_t bar;			// optional
    size_t size;
    
    pcilib_register_protocol_t protocol;

    uintptr_t read_addr;		// or offset if bar specified
    uintptr_t write_addr;		// or offset if bar specified
    uint8_t raw_endianess;

    uint8_t access;
    uint8_t endianess;
    
    const char *format;
    const char *name;
    const char *description;
} pcilib_register_bank_description_t;

typedef struct {
    pcilib_register_addr_t addr;
    pcilib_register_size_t offset;
    pcilib_register_size_t bits;
    pcilib_register_value_t defvalue;
    pcilib_register_value_t rwmask;	/**< 1 - read before write bits, 0 - zero should be written to preserve value */
    pcilib_register_mode_t mode;
    pcilib_register_type_t type;
    
    pcilib_register_bank_t bank;
    
    const char *name;
    const char *description;
} pcilib_register_description_t;

/**
  * Default mappings
  */
typedef struct {
    uintptr_t start;
    uintptr_t end;
    pcilib_register_bank_t bank;
} pcilib_register_range_t;

typedef struct {
    const char *name;
    const char *description;
} pcilib_event_description_t;

typedef enum {
    PCILIB_DMA_FROM_DEVICE = 1,
    PCILIB_DMA_TO_DEVICE = 2,
    PCILIB_DMA_BIDIRECTIONAL = 3
} pcilib_dma_direction_t;

typedef enum {
    PCILIB_DMA_TYPE_BLOCK,
    PCILIB_DMA_TYPE_PACKET,
    PCILIB_DMA_TYPE_UNKNOWN
} pcilib_dma_engine_type_t;

typedef struct {
    pcilib_dma_engine_addr_t addr;
    pcilib_dma_engine_type_t type;
    pcilib_dma_direction_t direction;
    size_t addr_bits;
} pcilib_dma_engine_description_t;

typedef struct {
    pcilib_dma_engine_description_t *engines[PCILIB_MAX_DMA_ENGINES +  1];
} pcilib_dma_info_t;

typedef struct {
    uint8_t access;
    uint8_t endianess;
    
    pcilib_register_description_t *registers;
    pcilib_register_bank_description_t *banks;
    pcilib_register_range_t *ranges;
    pcilib_event_description_t *events;

    pcilib_dma_api_description_t *dma_api;    
    pcilib_event_api_description_t *event_api;
} pcilib_model_description_t;

int pcilib_set_error_handler(void (*err)(const char *msg, ...), void (*warn)(const char *msg, ...));

pcilib_model_t pcilib_get_model(pcilib_t *ctx);
pcilib_model_description_t *pcilib_get_model_description(pcilib_t *ctx);
pcilib_context_t *pcilib_get_implementation_context(pcilib_t *ctx);

pcilib_t *pcilib_open(const char *device, pcilib_model_t model);
void pcilib_close(pcilib_t *ctx);

int pcilib_clear_irq(pcilib_t *ctx, pcilib_irq_source_t source);
int pcilib_wait_irq(pcilib_t *ctx, pcilib_irq_source_t source, size_t timeout, size_t *count);

void *pcilib_map_bar(pcilib_t *ctx, pcilib_bar_t bar);
void pcilib_unmap_bar(pcilib_t *ctx, pcilib_bar_t bar, void *data);
char *pcilib_resolve_register_address(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr);	// addr is offset if bar is specified
char *pcilib_resolve_data_space(pcilib_t *ctx, uintptr_t addr, size_t *size);

pcilib_register_bank_t pcilib_find_bank_by_addr(pcilib_t *ctx, pcilib_register_bank_addr_t bank);
pcilib_register_bank_t pcilib_find_bank_by_name(pcilib_t *ctx, const char *bankname);
pcilib_register_bank_t pcilib_find_bank(pcilib_t *ctx, const char *bank);
pcilib_register_t pcilib_find_register(pcilib_t *ctx, const char *bank, const char *reg);
pcilib_event_t pcilib_find_event(pcilib_t *ctx, const char *event);
pcilib_dma_engine_t pcilib_find_dma_by_addr(pcilib_t *ctx, pcilib_dma_direction_t direction, pcilib_dma_engine_addr_t dma);

int pcilib_read(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);
int pcilib_write(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, size_t size, void *buf);
int pcilib_write_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t fifo_size, size_t n, void *buf);
int pcilib_read_fifo(pcilib_t *ctx, pcilib_bar_t bar, uintptr_t addr, uint8_t fifo_size, size_t n, void *buf);

int pcilib_skip_dma(pcilib_t *ctx, pcilib_dma_engine_t dma);
size_t pcilib_stream_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, pcilib_dma_callback_t cb, void *cbattr);
size_t pcilib_push_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, pcilib_dma_flags_t flags, size_t timeout, void *buf);
size_t pcilib_read_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf);
size_t pcilib_write_dma(pcilib_t *ctx, pcilib_dma_engine_t dma, uintptr_t addr, size_t size, void *buf);
double pcilib_benchmark_dma(pcilib_t *ctx, pcilib_dma_engine_addr_t dma, uintptr_t addr, size_t size, size_t iterations, pcilib_dma_direction_t direction);

int pcilib_read_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf);
int pcilib_write_register_space(pcilib_t *ctx, const char *bank, pcilib_register_addr_t addr, size_t n, pcilib_register_value_t *buf);
int pcilib_read_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t *value);
int pcilib_write_register_by_id(pcilib_t *ctx, pcilib_register_t reg, pcilib_register_value_t value);
int pcilib_read_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t *value);
int pcilib_write_register(pcilib_t *ctx, const char *bank, const char *regname, pcilib_register_value_t value);

int pcilib_reset(pcilib_t *ctx);
int pcilib_start(pcilib_t *ctx, pcilib_event_t event_mask, void *callback, void *user);
int pcilib_stop(pcilib_t *ctx);

int pcilib_trigger(pcilib_t *ctx, pcilib_event_t event, size_t trigger_size, void *trigger_data);

pcilib_event_id_t pcilib_get_next_event(pcilib_t *ctx, pcilib_event_t event_mask, const struct timespec *timeout);
void *pcilib_get_data(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t *size);
void *pcilib_get_data_with_argument(pcilib_t *ctx, pcilib_event_id_t event_id, pcilib_event_data_type_t data_type, size_t arg_size, void *arg, size_t *size);
/*
 * This function is provided to find potentially corrupted data. If the data is overwritten by 
 * the time return_data is called it will return error.
 */
int pcilib_return_data(pcilib_t *ctx, pcilib_event_id_t event_id);

/*
 * @param data - will be allocated and shuld be freed if NULL, otherwise used and size should contain correct size.
 *   In case of failure the content of data is undefined.
 * @param timeout - will be autotriggered if NULL
 */
int pcilib_grab(pcilib_t *ctx, pcilib_event_t event_mask, size_t *size, void **data, const struct timespec *timeout);

#endif /* _PCITOOL_PCILIB_H */
