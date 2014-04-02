/**
 *
 * @file kmem.c
 * @brief This file contains all functions dealing with kernel memory.
 * @author Guillermo Marcus
 * @date 2009-04-05
 *
 */
#include <linux/version.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/pagemap.h>

#include "config.h"			/* compile-time configuration */
#include "compat.h"			/* compatibility definitions for older linux */
#include "pciDriver.h"			/* external interface for the driver */
#include "common.h"			/* internal definitions for all parts */
#include "kmem.h"			/* prototypes for kernel memory */
#include "sysfs.h"			/* prototypes for sysfs */

/* VM_RESERVED is removed in 3.7-rc1 */
#ifndef VM_RESERVED
# define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

/**
 *
 * Allocates new kernel memory including the corresponding management structure, makes
 * it available via sysfs if possible.
 *
 */
int pcidriver_kmem_alloc(pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle)
{
	int flags;
	pcidriver_kmem_entry_t *kmem_entry;
	void *retptr;

	if (kmem_handle->flags&KMEM_FLAG_REUSE) {
	    kmem_entry = pcidriver_kmem_find_entry_use(privdata, kmem_handle->use, kmem_handle->item);
	    if (kmem_entry) {
		unsigned long flags = kmem_handle->flags;
		
		if (flags&KMEM_FLAG_TRY) {
		    kmem_handle->type = kmem_entry->type;
		    kmem_handle->size = kmem_entry->size;
		    kmem_handle->align = kmem_entry->align;
		} else {
		    if (kmem_handle->type != kmem_entry->type) {
		    	mod_info("Invalid type of reusable kmem_entry, currently: %lu, but requested: %lu\n", kmem_entry->type, kmem_handle->type);
			return -EINVAL;
		    }

		    if ((kmem_handle->type&PCILIB_KMEM_TYPE_MASK) == PCILIB_KMEM_TYPE_PAGE) {
			    kmem_handle->size = kmem_entry->size;
		    } else if (kmem_handle->size != kmem_entry->size) {
			mod_info("Invalid size of reusable kmem_entry, currently: %lu, but requested: %lu\n", kmem_entry->size, kmem_handle->size);
			return -EINVAL;
		    }
		    
		    if (kmem_handle->align != kmem_entry->align) {
			mod_info("Invalid alignment of reusable kmem_entry, currently: %lu, but requested: %lu\n", kmem_entry->align, kmem_handle->align);
			return -EINVAL;
		    }

		    if (((kmem_entry->mode&KMEM_MODE_EXCLUSIVE)?1:0) != ((flags&KMEM_FLAG_EXCLUSIVE)?1:0)) {
			mod_info("Invalid mode of reusable kmem_entry\n");
			return -EINVAL;
		    }
		}
		

		if ((kmem_entry->mode&KMEM_MODE_COUNT)==KMEM_MODE_COUNT) {
			mod_info("Reuse counter of kmem_entry is overflown");
			return -EBUSY;
		}
		
		
		kmem_handle->handle_id = kmem_entry->id;
		kmem_handle->pa = (unsigned long)(kmem_entry->dma_handle);

		kmem_handle->flags = KMEM_FLAG_REUSED;
		if (kmem_entry->refs&KMEM_REF_HW) kmem_handle->flags |= KMEM_FLAG_REUSED_HW;
		if (kmem_entry->mode&KMEM_MODE_PERSISTENT) kmem_handle->flags |= KMEM_FLAG_REUSED_PERSISTENT;

		kmem_entry->mode += 1;
		if (flags&KMEM_FLAG_HW) {
		    if ((kmem_entry->refs&KMEM_REF_HW)==0)
			pcidriver_module_get(privdata);
			
		    kmem_entry->refs |= KMEM_REF_HW;
		}
		if (flags&KMEM_FLAG_PERSISTENT) kmem_entry->mode |= KMEM_MODE_PERSISTENT;

		privdata->kmem_cur_id = kmem_entry->id;
		
		return 0;
	    }
	    
	    if (kmem_handle->flags&KMEM_FLAG_TRY) return -ENOENT;
	}

	/* First, allocate zeroed memory for the kmem_entry */
	if ((kmem_entry = kcalloc(1, sizeof(pcidriver_kmem_entry_t), GFP_KERNEL)) == NULL)
		goto kmem_alloc_entry_fail;

	/* Initialize the kmem_entry */
	kmem_entry->id = atomic_inc_return(&privdata->kmem_count) - 1;
	privdata->kmem_cur_id = kmem_entry->id;
	kmem_handle->handle_id = kmem_entry->id;

	kmem_entry->use = kmem_handle->use;
	kmem_entry->item = kmem_handle->item;
	kmem_entry->type = kmem_handle->type;
	kmem_entry->align = kmem_handle->align;
	kmem_entry->direction = PCI_DMA_NONE;

	/* Initialize sysfs if possible */
	if (pcidriver_sysfs_initialize_kmem(privdata, kmem_entry->id, &(kmem_entry->sysfs_attr)) != 0)
		goto kmem_alloc_mem_fail;

	/* ...and allocate the DMA memory */
	/* note this is a memory pair, referencing the same area: the cpu address (cpua)
	 * and the PCI bus address (pa). The CPU and PCI addresses may not be the same.
	 * The CPU sees only CPU addresses, while the device sees only PCI addresses.
	 * CPU address is used for the mmap (internal to the driver), and
	 * PCI address is the address passed to the DMA Controller in the device.
	 */
	switch (kmem_entry->type&PCILIB_KMEM_TYPE_MASK) {
	 case PCILIB_KMEM_TYPE_CONSISTENT:
	    retptr = pci_alloc_consistent( privdata->pdev, kmem_handle->size, &(kmem_entry->dma_handle) );
	    break;
	 case PCILIB_KMEM_TYPE_PAGE:
	    flags = GFP_KERNEL;
	    
	    if ((kmem_entry->type == PCILIB_KMEM_TYPE_DMA_S2C_PAGE)||(kmem_entry->type == PCILIB_KMEM_TYPE_DMA_C2S_PAGE))
		flags |= __GFP_DMA;

	    retptr = (void*)__get_free_pages(flags, get_order(PAGE_SIZE));
	    kmem_entry->dma_handle = 0;
	    kmem_handle->size = PAGE_SIZE;
	    
	    if (retptr) {
	        if (kmem_entry->type == PCILIB_KMEM_TYPE_DMA_S2C_PAGE) {
		    kmem_entry->direction = PCI_DMA_TODEVICE;
    		    kmem_entry->dma_handle = pci_map_single(privdata->pdev, retptr, PAGE_SIZE, PCI_DMA_TODEVICE);
		    if (pci_dma_mapping_error(privdata->pdev, kmem_entry->dma_handle)) {
			free_page((unsigned long)retptr);
			goto kmem_alloc_mem_fail;
		    }
		} else if (kmem_entry->type == PCILIB_KMEM_TYPE_DMA_C2S_PAGE) {
		    kmem_entry->direction = PCI_DMA_FROMDEVICE;
    		    kmem_entry->dma_handle = pci_map_single(privdata->pdev, retptr, PAGE_SIZE, PCI_DMA_FROMDEVICE);
		    if (pci_dma_mapping_error(privdata->pdev, kmem_entry->dma_handle)) {
			free_page((unsigned long)retptr);
			goto kmem_alloc_mem_fail;
		    
		    }
		}
	    }
	    
	    break;
	 default:
	    goto kmem_alloc_mem_fail;
	}
	
	
	if (retptr == NULL)
		goto kmem_alloc_mem_fail;

	kmem_entry->size = kmem_handle->size;
	kmem_entry->cpua = (unsigned long)retptr;
	kmem_handle->pa = (unsigned long)(kmem_entry->dma_handle);

	kmem_entry->mode = 1;
	if (kmem_handle->flags&KMEM_FLAG_REUSE) {
	    kmem_entry->mode |= KMEM_MODE_REUSABLE;
	    if (kmem_handle->flags&KMEM_FLAG_EXCLUSIVE) kmem_entry->mode |= KMEM_MODE_EXCLUSIVE;
	    if (kmem_handle->flags&KMEM_FLAG_PERSISTENT) kmem_entry->mode |= KMEM_MODE_PERSISTENT;
	}
	
	kmem_entry->refs = 0;
	if (kmem_handle->flags&KMEM_FLAG_HW) {
	    pcidriver_module_get(privdata);

	    kmem_entry->refs |= KMEM_REF_HW;
	}

        kmem_handle->flags = 0;
	
	set_pages_reserved_compat(kmem_entry->cpua, kmem_entry->size);

	/* Add the kmem_entry to the list of the device */
	spin_lock( &(privdata->kmemlist_lock) );
	list_add_tail( &(kmem_entry->list), &(privdata->kmem_list) );
	spin_unlock( &(privdata->kmemlist_lock) );

	return 0;

kmem_alloc_mem_fail:
		kfree(kmem_entry);
kmem_alloc_entry_fail:
		return -ENOMEM;
}

static int pcidriver_kmem_free_check(pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle, pcidriver_kmem_entry_t *kmem_entry) {
	if ((kmem_handle->flags & KMEM_FLAG_FORCE) == 0) {
	    if (kmem_entry->mode&KMEM_MODE_COUNT)
		kmem_entry->mode -= 1;

	    if (kmem_handle->flags&KMEM_FLAG_HW) {
		if (kmem_entry->refs&KMEM_REF_HW) 
		    pcidriver_module_put(privdata);

		kmem_entry->refs &= ~KMEM_REF_HW;
	    }
	
	    if (kmem_handle->flags&KMEM_FLAG_PERSISTENT)
		kmem_entry->mode &= ~KMEM_MODE_PERSISTENT;
	
	    if (kmem_handle->flags&KMEM_FLAG_REUSE) 
		return 0;

	    if (kmem_entry->refs) {
		kmem_entry->mode += 1;
		mod_info("can't free referenced kmem_entry\n");
		return -EBUSY;
	    }
	
	    if (kmem_entry->mode & KMEM_MODE_PERSISTENT) {
		kmem_entry->mode += 1;
		mod_info("can't free persistent kmem_entry\n");
		return -EBUSY;
	    }

	    if (((kmem_entry->mode&KMEM_MODE_EXCLUSIVE)==0)&&(kmem_entry->mode&KMEM_MODE_COUNT)&&((kmem_handle->flags&KMEM_FLAG_EXCLUSIVE)==0)) 
		return 0;
	} else {
	    if (kmem_entry->refs&KMEM_REF_HW)
		    pcidriver_module_put(privdata);
		
	    while (!atomic_add_negative(-1, &(privdata->refs))) pcidriver_module_put(privdata);
	    atomic_inc(&(privdata->refs));
		
	}
	
	return 1;
}

static int pcidriver_kmem_free_use(pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle)
{
	int err;
	int failed = 0;
	struct list_head *ptr, *next;
	pcidriver_kmem_entry_t *kmem_entry;

	/* iterate safely over the entries and delete them */
	list_for_each_safe(ptr, next, &(privdata->kmem_list)) {
		kmem_entry = list_entry(ptr, pcidriver_kmem_entry_t, list);
		if (kmem_entry->use == kmem_handle->use) {
		    err = pcidriver_kmem_free_check(privdata, kmem_handle, kmem_entry);
		    if (err > 0)
			pcidriver_kmem_free_entry(privdata, kmem_entry); 		/* spin lock inside! */
		    else
			failed = 1;
		}
	}
	
	if (failed) {
		mod_info("Some kmem_entries for use %lx are still referenced\n", kmem_handle->use);
		return -EBUSY;
	}	

	return 0;
}

/**
 *
 * Called via sysfs, frees kernel memory and the corresponding management structure
 *
 */
int pcidriver_kmem_free( pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle )
{
	int err;
	pcidriver_kmem_entry_t *kmem_entry;

	if (kmem_handle->flags&KMEM_FLAG_MASS) {
	    kmem_handle->flags &= ~KMEM_FLAG_MASS;
	    return pcidriver_kmem_free_use(privdata, kmem_handle);
	}
	
	/* Find the associated kmem_entry for this buffer */
	if ((kmem_entry = pcidriver_kmem_find_entry(privdata, kmem_handle)) == NULL)
		return -EINVAL;					/* kmem_handle is not valid */

	err = pcidriver_kmem_free_check(privdata, kmem_handle, kmem_entry);
	
	if (err > 0)
		return pcidriver_kmem_free_entry(privdata, kmem_entry);
	
	return err;
}

/**
 *
 * Called when cleaning up, frees all kernel memory and their corresponding management structure
 *
 */
int pcidriver_kmem_free_all(pcidriver_privdata_t *privdata)
{
//	int failed = 0;
	struct list_head *ptr, *next;
	pcidriver_kmem_entry_t *kmem_entry;

	/* iterate safely over the entries and delete them */
	list_for_each_safe(ptr, next, &(privdata->kmem_list)) {
		kmem_entry = list_entry(ptr, pcidriver_kmem_entry_t, list);
		/*if (kmem_entry->refs)
			failed = 1;
		else*/
			pcidriver_kmem_free_entry(privdata, kmem_entry); 		/* spin lock inside! */
	}
/*	
	if (failed) {
		mod_info("Some kmem_entries are still referenced\n");
		return -EBUSY;
	}	
*/
	return 0;
}


/**
 *
 * Synchronize memory to/from the device (or in both directions).
 *
 */
int pcidriver_kmem_sync_entry( pcidriver_privdata_t *privdata, pcidriver_kmem_entry_t *kmem_entry, int direction)
{
	if (kmem_entry->direction == PCI_DMA_NONE)
		return -EINVAL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11)
	switch (direction) {
		case PCILIB_KMEM_SYNC_TODEVICE:
			pci_dma_sync_single_for_device( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, kmem_entry->direction );
			break;
		case PCILIB_KMEM_SYNC_FROMDEVICE:
			pci_dma_sync_single_for_cpu( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, kmem_entry->direction );
			break;
		case PCILIB_KMEM_SYNC_BIDIRECTIONAL:
			pci_dma_sync_single_for_device( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, kmem_entry->direction );
			pci_dma_sync_single_for_cpu( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, kmem_entry->direction );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#else
	switch (direction) {
		case PCILIB_KMEM_SYNC_TODEVICE:
			pci_dma_sync_single( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, kmem_entry->direction );
			break;
		case PCILIB_KMEM_SYNC_FROMDEVICE:
			pci_dma_sync_single( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, kmem_entry->direction );
			break;
		case PCILIB_KMEM_SYNC_BIDIRECTIONAL:
			pci_dma_sync_single( privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, kmem_entry->direction );
			break;
		default:
			return -EINVAL;				/* wrong direction parameter */
	}
#endif

	return 0;	/* success */
}

/**
 *
 * Synchronize memory to/from the device (or in both directions).
 *
 */
int pcidriver_kmem_sync( pcidriver_privdata_t *privdata, kmem_sync_t *kmem_sync )
{
	pcidriver_kmem_entry_t *kmem_entry;

	/* Find the associated kmem_entry for this buffer */
	if ((kmem_entry = pcidriver_kmem_find_entry(privdata, &(kmem_sync->handle))) == NULL)
		return -EINVAL;					/* kmem_handle is not valid */

	return pcidriver_kmem_sync_entry(privdata, kmem_entry, kmem_sync->dir);
}

/**
 *
 * Free the given kmem_entry and its memory.
 *
 */
int pcidriver_kmem_free_entry(pcidriver_privdata_t *privdata, pcidriver_kmem_entry_t *kmem_entry)
{
	pcidriver_sysfs_remove(privdata, &(kmem_entry->sysfs_attr));

	/* Go over the pages of the kmem buffer, and mark them as not reserved */
#if 0
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
	/*
	 * This code is DISABLED.
	 * Apparently, it is not needed to unreserve them. Doing so here
	 * hangs the machine. Why?
	 *
	 * Uhm.. see links:
	 *
	 * http://lwn.net/Articles/161204/
	 * http://lists.openfabrics.org/pipermail/general/2007-March/034101.html
	 *
	 * I insist, this should be enabled, but doing so hangs the machine.
	 * Literature supports the point, and there is even a similar problem (see link)
	 * But this is not the case. It seems right to me. but obviously is not.
	 *
	 * Anyway, this goes away in kernel >=2.6.15.
	 */
	unsigned long start = __pa(kmem_entry->cpua) >> PAGE_SHIFT;
	unsigned long end = __pa(kmem_entry->cpua + kmem_entry->size) >> PAGE_SHIFT;
	unsigned long i;
	for(i=start;i<end;i++) {
		struct page *kpage = pfn_to_page(i);
		ClearPageReserved(kpage);
	}
#endif
#endif

	/* Release DMA memory */
	switch (kmem_entry->type&PCILIB_KMEM_TYPE_MASK) {
	 case PCILIB_KMEM_TYPE_CONSISTENT:
	    pci_free_consistent( privdata->pdev, kmem_entry->size, (void *)(kmem_entry->cpua), kmem_entry->dma_handle );
	    break;
	 case PCILIB_KMEM_TYPE_PAGE:
	    if (kmem_entry->dma_handle) {
		if (kmem_entry->type == PCILIB_KMEM_TYPE_DMA_S2C_PAGE) {
		    pci_unmap_single(privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_TODEVICE);
		} else if (kmem_entry->type == PCILIB_KMEM_TYPE_DMA_C2S_PAGE) {
		    pci_unmap_single(privdata->pdev, kmem_entry->dma_handle, kmem_entry->size, PCI_DMA_FROMDEVICE);
		}
	    }
	    free_page((unsigned long)kmem_entry->cpua);
	    break;
	}


	/* Remove the kmem list entry */
	spin_lock( &(privdata->kmemlist_lock) );
	list_del( &(kmem_entry->list) );
	spin_unlock( &(privdata->kmemlist_lock) );

	/* Release kmem_entry memory */
	kfree(kmem_entry);

	return 0;
}

/**
 *
 * Find the corresponding kmem_entry for the given kmem_handle.
 *
 */
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry(pcidriver_privdata_t *privdata, kmem_handle_t *kmem_handle)
{
	struct list_head *ptr;
	pcidriver_kmem_entry_t *entry, *result = NULL;

	/* should I implement it better using the handle_id? */

	spin_lock(&(privdata->kmemlist_lock));
	list_for_each(ptr, &(privdata->kmem_list)) {
		entry = list_entry(ptr, pcidriver_kmem_entry_t, list);

		if (entry->id == kmem_handle->handle_id) {
			result = entry;
			break;
		}
	}

	spin_unlock(&(privdata->kmemlist_lock));
	return result;
}

/**
 *
 * find the corresponding kmem_entry for the given id.
 *
 */
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry_id(pcidriver_privdata_t *privdata, int id)
{
	struct list_head *ptr;
	pcidriver_kmem_entry_t *entry, *result = NULL;

	spin_lock(&(privdata->kmemlist_lock));
	list_for_each(ptr, &(privdata->kmem_list)) {
		entry = list_entry(ptr, pcidriver_kmem_entry_t, list);

		if (entry->id == id) {
			result = entry;
			break;
		}
	}

	spin_unlock(&(privdata->kmemlist_lock));
	return result;
}

/**
 *
 * find the corresponding kmem_entry for the given use and item.
 *
 */
pcidriver_kmem_entry_t *pcidriver_kmem_find_entry_use(pcidriver_privdata_t *privdata, unsigned long use, unsigned long item)
{
	struct list_head *ptr;
	pcidriver_kmem_entry_t *entry, *result = NULL;

	spin_lock(&(privdata->kmemlist_lock));
	list_for_each(ptr, &(privdata->kmem_list)) {
		entry = list_entry(ptr, pcidriver_kmem_entry_t, list);

		if ((entry->use == use)&&(entry->item == item)&&(entry->mode&KMEM_MODE_REUSABLE)) {
			result = entry;
			break;
		}
	}

	spin_unlock(&(privdata->kmemlist_lock));
	return result;
}


void pcidriver_kmem_mmap_close(struct vm_area_struct *vma) {
    unsigned long vma_size;
    pcidriver_kmem_entry_t *kmem_entry = (pcidriver_kmem_entry_t*)vma->vm_private_data;
    if (kmem_entry) {
/*
	if (kmem_entry->id == 0) {
	    mod_info("refs: %p %p %lx\n", vma, vma->vm_private_data, kmem_entry->refs);
	    mod_info("kmem_size: %lu vma_size: %lu, s: %lx, e: %lx\n", kmem_entry->size, (vma->vm_end - vma->vm_start), vma->vm_start, vma->vm_end);
	}
*/

	vma_size = (vma->vm_end - vma->vm_start);
	
	if (kmem_entry->refs&KMEM_REF_COUNT) {
	    kmem_entry->refs -= vma_size / PAGE_SIZE;
	}
    }
}

static struct vm_operations_struct pcidriver_kmem_mmap_ops = {
    .close = pcidriver_kmem_mmap_close
};

/**
 *
 * mmap() kernel memory to userspace.
 *
 */
int pcidriver_mmap_kmem(pcidriver_privdata_t *privdata, struct vm_area_struct *vma)
{
	unsigned long vma_size;
	pcidriver_kmem_entry_t *kmem_entry;
	int ret;

	mod_info_dbg("Entering mmap_kmem\n");

	/* FIXME: Is this really right? Always just the latest one? Can't we identify one? */
	/* Get latest entry on the kmem_list */
	kmem_entry = pcidriver_kmem_find_entry_id(privdata, privdata->kmem_cur_id);
	if (!kmem_entry) {
		mod_info("Trying to mmap a kernel memory buffer without creating it first!\n");
		return -EFAULT;
	}

	mod_info_dbg("Got kmem_entry with id: %d\n", kmem_entry->id);

	/* Check sizes */
	vma_size = (vma->vm_end - vma->vm_start);
	
	if ((vma_size > kmem_entry->size) &&
		((kmem_entry->size < PAGE_SIZE) && (vma_size != PAGE_SIZE))) {
		mod_info("kem_entry size(%lu) and vma size do not match(%lu)\n", kmem_entry->size, vma_size);
		return -EINVAL;
	}

	/* reference counting */
	if ((kmem_entry->mode&KMEM_MODE_EXCLUSIVE)&&(kmem_entry->refs&KMEM_REF_COUNT)) {
		mod_info("can't make second mmaping for exclusive kmem_entry\n");
		return -EBUSY;
	}
	if (((kmem_entry->refs&KMEM_REF_COUNT) + (vma_size / PAGE_SIZE)) > KMEM_REF_COUNT) {
		mod_info("maximal amount of references is reached by kmem_entry\n");
		return -EBUSY;
	}
	
	kmem_entry->refs += vma_size / PAGE_SIZE;

	vma->vm_flags |= (VM_RESERVED);

#ifdef pgprot_noncached
	// This is coherent memory, so it must not be cached.
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

	mod_info_dbg("Mapping address %08lx / PFN %08lx\n",
			virt_to_phys((void*)kmem_entry->cpua),
			page_to_pfn(virt_to_page((void*)kmem_entry->cpua)));

	ret = remap_pfn_range_cpua_compat(
					vma,
					vma->vm_start,
					kmem_entry->cpua,
					(vma_size < kmem_entry->size)?vma_size:kmem_entry->size,
					vma->vm_page_prot );

	if (ret) {
		mod_info("kmem remap failed: %d (%lx)\n", ret,kmem_entry->cpua);
		kmem_entry->refs -= 1;
		return -EAGAIN;
	}

	vma->vm_ops = &pcidriver_kmem_mmap_ops;
	vma->vm_private_data = (void*)kmem_entry;
	
	return ret;
}
