#define _BSD_SOURCE
#define _IPECAMERA_MODEL_C
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>

#include <pcilib.h>
#include <pcilib/tools.h>
#include <pcilib/error.h>
#include <pcilib/locking.h>
#include <pcilib/model.h>
#include <pcilib/datacpy.h>

#include "cmosis.h"
#include "private.h"

#define ADDR_MASK 0x7F00
#define WRITE_BIT 0x8000
#define RETRIES 10

//ToDo: check bot 1 and 2 bits for READY
#define READ_READY_BIT 0x20000
#define READ_ERROR_BIT 0x40000

#define ipecamera_datacpy(dst, src, bank)   pcilib_datacpy(dst, src, 4, 1, bank->raw_endianess)

//#define IPECAMERA_SIMPLIFIED_READOUT
//#define IPECAMERA_RETRY_ERRORS
#define IPECAMERA_MULTIREAD


typedef struct ipecamera_cmosis_context_s ipecamera_cmosis_context_t;

struct ipecamera_cmosis_context_s {
    pcilib_register_bank_context_t bank_ctx;	/**< the bank context associated with the software registers */
    pcilib_lock_t *lock;			/**< the lock to serialize access through GPIO */
};

void ipecamera_cmosis_close(pcilib_t *ctx, pcilib_register_bank_context_t *reg_bank_ctx) {
	ipecamera_cmosis_context_t *bank_ctx = (ipecamera_cmosis_context_t*)reg_bank_ctx;

	if (bank_ctx->lock)
	    pcilib_return_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, bank_ctx->lock);
	free(bank_ctx);
}

pcilib_register_bank_context_t* ipecamera_cmosis_open(pcilib_t *ctx, pcilib_register_bank_t bank, const char* model, const void *args) {
	ipecamera_cmosis_context_t *bank_ctx;

	bank_ctx = calloc(1, sizeof(ipecamera_cmosis_context_t));
	if (!bank_ctx) {
	    pcilib_error("Memory allocation for bank context has failed");
	    return NULL;
	}

	bank_ctx->lock = pcilib_get_lock(ctx, PCILIB_LOCK_FLAGS_DEFAULT, "cmosis");
	if (!bank_ctx->lock) {
	    ipecamera_cmosis_close(ctx, (pcilib_register_bank_context_t*)bank_ctx);
	    pcilib_error("Failed to initialize a lock to protect CMOSIS register bank");
	    return NULL;
	}

	return (pcilib_register_bank_context_t*)bank_ctx;
}

int ipecamera_cmosis_read(pcilib_t *ctx, pcilib_register_bank_context_t *reg_bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t *value) {
    int err;
    uint32_t val, tmp[4];
    char *wr, *rd;
    struct timeval start;
    int retries = RETRIES;
    ipecamera_cmosis_context_t *bank_ctx = (ipecamera_cmosis_context_t*)reg_bank_ctx;
    const pcilib_register_bank_description_t *bank = reg_bank_ctx->bank;
    
    assert(addr < 128);
    
    wr =  pcilib_resolve_bar_address(ctx, bank->bar, bank->write_addr);
    rd =  pcilib_resolve_bar_address(ctx, bank->bar, bank->read_addr);
    if ((!rd)||(!wr)) {
	pcilib_error("Error resolving addresses of read & write registers");
	return PCILIB_ERROR_INVALID_ADDRESS;
    }

retry:
    err = pcilib_lock(bank_ctx->lock);
    if (err) {
	pcilib_error("Error (%i) obtaining a lock to serialize access to CMOSIS registers ", err);
	return err;
    }

    val = (addr << 8);

    ipecamera_datacpy(wr, &val, bank);

#ifdef IPECAMERA_SIMPLIFIED_READOUT
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
    ipecamera_datacpy(wr, &val, bank);
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
    ipecamera_datacpy(wr, &val, bank);
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
#endif /* IPECAMERA_SIMPLIFIED_READOUT */
    
    gettimeofday(&start, NULL);

#ifdef IPECAMERA_MULTIREAD
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
    pcilib_datacpy(tmp, rd, 4, 4, bank->raw_endianess);
    val = tmp[0];
#else /* IPECAMERA_MULTIREAD */
    ipecamera_datacpy(&val, rd, bank);

    while ((val & READ_READY_BIT) == 0) {
        gettimeofday(&cur, NULL);
	if (((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) > IPECAMERA_SPI_REGISTER_DELAY) break;
	
	ipecamera_datacpy(&val, rd, bank);
    }
#endif /* IPECAMERA_MULTIREAD */

    pcilib_unlock(bank_ctx->lock);

    if ((val & READ_READY_BIT) == 0) {
	if (--retries > 0) {
	    pcilib_warning("Timeout reading register value (CMOSIS %lu, status: %lx), retrying (try %i of %i)...", addr, val, RETRIES - retries, RETRIES);
	    goto retry;
	}

	pcilib_error("Timeout reading register value (CMOSIS %lu, status: %lx)", addr, val);
	return PCILIB_ERROR_TIMEOUT;
    }

    if (val & READ_ERROR_BIT) {
#ifdef IPECAMERA_RETRY_ERRORS
	if (--retries > 0) {
	    pcilib_warning("Error reading register (CMOSIS %lu, status: %lx), retrying (try %i of %i)...", addr, val, RETRIES - retries, RETRIES);
	    goto retry;
	}
#endif /* IPECAMERA_RETRY_ERRORS */

	pcilib_error("Error reading register value (CMOSIS %lu, status: %lx)", addr, val);
	return PCILIB_ERROR_FAILED;
    }

    if (((val&ADDR_MASK) >> 8) != addr) {
#ifdef IPECAMERA_RETRY_ERRORS
	if (--retries > 0) {
	    pcilib_warning("Address verification failed during register read (CMOSIS %lu, status: %lx), retrying (try %i of %i)...", addr, val, RETRIES - retries, RETRIES);
	    goto retry;
	}
#endif /* IPECAMERA_RETRY_ERRORS */

	pcilib_error("Address verification failed during register read (CMOSIS %lu, status: %lx)", addr, val);
	return PCILIB_ERROR_VERIFY;
    }

    *value = val&0xFF;

    return 0;
}

int ipecamera_cmosis_write(pcilib_t *ctx, pcilib_register_bank_context_t *reg_bank_ctx, pcilib_register_addr_t addr, pcilib_register_value_t value) {
    int err;
    uint32_t val, tmp[4];
    char *wr, *rd;
    struct timeval start;
    int retries = RETRIES;
    ipecamera_cmosis_context_t *bank_ctx = (ipecamera_cmosis_context_t*)reg_bank_ctx;
    const pcilib_register_bank_description_t *bank = reg_bank_ctx->bank;

    assert(addr < 128);
    assert(value < 256);
    
    wr =  pcilib_resolve_bar_address(ctx, bank->bar, bank->write_addr);
    rd =  pcilib_resolve_bar_address(ctx, bank->bar, bank->read_addr);
    if ((!rd)||(!wr)) {
	pcilib_error("Error resolving addresses of read & write registers");
	return PCILIB_ERROR_INVALID_ADDRESS;
    }

retry:
    err = pcilib_lock(bank_ctx->lock);
    if (err) {
	pcilib_error("Error (%i) obtaining a lock to serialize access to CMOSIS registers ", err);
	return err;
    }

    val = WRITE_BIT|(addr << 8)|(value&0xFF);
    ipecamera_datacpy(wr, &val, bank);

#ifdef IPECAMERA_SIMPLIFIED_READOUT
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
    ipecamera_datacpy(wr, &val, bank);
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
    ipecamera_datacpy(wr, &val, bank);
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
#endif /* IPECAMERA_SIMPLIFIED_READOUT */

    gettimeofday(&start, NULL);

#ifdef IPECAMERA_MULTIREAD
    usleep(IPECAMERA_SPI_REGISTER_DELAY);
    pcilib_datacpy(tmp, rd, 4, 4, bank->raw_endianess);
    val = tmp[0];
#else /* IPECAMERA_MULTIREAD */
    ipecamera_datacpy(&val, rd, bank);
    while ((val & READ_READY_BIT) == 0) {
        gettimeofday(&cur, NULL);
	if (((cur.tv_sec - start.tv_sec)*1000000 + (cur.tv_usec - start.tv_usec)) > IPECAMERA_SPI_REGISTER_DELAY) break;
	
	ipecamera_datacpy(&val, rd, bank);
    }
#endif /* IPECAMERA_MULTIREAD */

    pcilib_unlock(bank_ctx->lock);

    if ((val & READ_READY_BIT) == 0) {
#ifdef IPECAMERA_RETRY_ERRORS
	if (--retries > 0) {
	    pcilib_warning("Timeout occured during register write (CMOSIS %lu, value: %lu, status: %lx), retrying (try %i of %i)...", addr, value, val, RETRIES - retries, RETRIES);
	    goto retry;
	}
#endif /* IPECAMERA_RETRY_ERRORS */

	pcilib_error("Timeout writting register value (CMOSIS %lu, value: %lu, status: %lx)", addr, value, val);
	return PCILIB_ERROR_TIMEOUT;
    }
    
    if (val & READ_ERROR_BIT) {
#ifdef IPECAMERA_RETRY_ERRORS
	if (--retries > 0) {
	    pcilib_warning("Register write has failed (CMOSIS %lu, value: %lu, status: %lx), retrying (try %i of %i)...", addr, value, val, RETRIES - retries, RETRIES);
	    goto retry;
	}
#endif /* IPECAMERA_RETRY_ERRORS */

	pcilib_error("Error writting register value (CMOSIS %lu, value: %lu, status: %lx)", addr, value, val);
	return PCILIB_ERROR_FAILED;
    }

    if (((val&ADDR_MASK) >> 8) != addr) {
	if (--retries > 0) {
	    pcilib_warning("Address verification failed during register write (CMOSIS %lu, value: %lu, status: %lx), retrying (try %i of %i)...", addr, value, val, RETRIES - retries, RETRIES);
	    goto retry;
	}
	pcilib_error("Address verification failed during register write (CMOSIS %lu, value: %lu, status: %lx)", addr, value, val);
	return PCILIB_ERROR_VERIFY;
    }

    if ((val&0xFF) != value) {
	pcilib_error("Value verification failed during register read (CMOSIS %lu, value: %lu != %lu)", addr, val/*&ipecamera_bit_mask[bits]*/, value);
	return PCILIB_ERROR_VERIFY;
    }

    return 0;
}



