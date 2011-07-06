#ifndef _PCILIB_KMEM_H
#define _PCILIB_KMEM_H

#include "pcilib.h"

typedef int pcilib_kmem_flags_t;

typedef struct {
    int handle_id;
    uintptr_t pa;
//    uintptr_t va;
    void *ua;
    size_t size;
} pcilib_kmem_addr_t;

/**
 * single allocation - we set only addr, n_blocks = 0
 * multiple allocation - addr is not set, blocks are set, n_blocks > 0
 * sgmap allocation - addr contains ua, but pa's are set in blocks, n_blocks > 0
 */
typedef struct {
    pcilib_kmem_addr_t addr;

    size_t n_blocks;
    pcilib_kmem_addr_t blocks[];
} pcilib_kmem_buffer_t;

typedef void pcilib_kmem_handle_t;


typedef struct pcilib_kmem_list_s pcilib_kmem_list_t;
struct pcilib_kmem_list_s {
    pcilib_kmem_list_t *next, *prev;

    pcilib_kmem_buffer_t buf;	// variable size, should be last item in struct
};

pcilib_kmem_handle_t *pcilib_alloc_kernel_memory(pcilib_t *ctx, pcilib_kmem_type_t type, size_t nmemb, size_t size, size_t alignment, pcilib_kmem_use_t use, pcilib_kmem_flags_t flags);
void pcilib_free_kernel_memory(pcilib_t *ctx, pcilib_kmem_handle_t *k);
int pcilib_sync_kernel_memory(pcilib_t *ctx, pcilib_kmem_handle_t *k, pcilib_kmem_sync_direction_t dir);
void *pcilib_kmem_get_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k);
uintptr_t pcilib_kmem_get_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k);
void *pcilib_kmem_get_block_ua(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);
uintptr_t pcilib_kmem_get_block_pa(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);
size_t pcilib_kmem_get_block_size(pcilib_t *ctx, pcilib_kmem_handle_t *k, size_t block);

#endif /* _PCILIB_KMEM_H */