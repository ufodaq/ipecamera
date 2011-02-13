#ifndef _PCIDRIVER_COMMON_H
#define _PCIDRIVER_COMMON_H

/*************************************************************************/
/* Private data types and structures */

/* Define an entry in the kmem list (this list is per device) */
/* This list keeps references to the allocated kernel buffers */
typedef struct {
	int id;
	struct list_head list;
	dma_addr_t dma_handle;
	unsigned long cpua;
	unsigned long size;
	struct class_device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_kmem_entry_t;

/* Define an entry in the umem list (this list is per device) */
/* This list keeps references to the SG lists for each mapped userspace region */
typedef struct {
	int id;
	struct list_head list;
	unsigned int nr_pages;		/* number of pages for this user memeory area */
	struct page **pages;		/* list of pointers to the pages */
	unsigned int nents;			/* actual entries in the scatter/gatter list (NOT nents for the map function, but the result) */
	struct scatterlist *sg;		/* list of sg entries */
	struct class_device_attribute sysfs_attr;	/* initialized when adding the entry */
} pcidriver_umem_entry_t;

/* Hold the driver private data */
typedef struct  {
	dev_t devno;						/* device number (major and minor) */
	struct pci_dev *pdev;				/* PCI device */
	struct class_device *class_dev;		/* Class device */
	struct cdev cdev;					/* char device struct */
	int mmap_mode;						/* current mmap mode */
	int mmap_area;						/* current PCI mmap area */

#ifdef ENABLE_IRQ
	int irq_enabled;					/* Non-zero if IRQ is enabled */
	int irq_count;						/* Just an IRQ counter */

	wait_queue_head_t irq_queues[ PCIDRIVER_INT_MAXSOURCES ];
										/* One queue per interrupt source */
	atomic_t irq_outstanding[ PCIDRIVER_INT_MAXSOURCES ];
										/* Outstanding interrupts per queue */
	volatile unsigned int *bars_kmapped[6];		/* PCI BARs mmapped in kernel space */

#endif
	
	spinlock_t kmemlist_lock;			/* Spinlock to lock kmem list operations */
	struct list_head kmem_list;			/* List of 'kmem_list_entry's associated with this device */
	atomic_t kmem_count;				/* id for next kmem entry */

	spinlock_t umemlist_lock;			/* Spinlock to lock umem list operations */
	struct list_head umem_list;			/* List of 'umem_list_entry's associated with this device */
	atomic_t umem_count;				/* id for next umem entry */

	
} pcidriver_privdata_t;

/* Identifies the mpRACE-1 boards */
#define MPRACE1_VENDOR_ID 0x10b5
#define MPRACE1_DEVICE_ID 0x9656

/* Identifies the PCI-X Test boards */
#define PCIXTEST_VENDOR_ID 0x10dc
#define PCIXTEST_DEVICE_ID 0x0156

/* Identifies the PCIe-PLDA Test board */
#define PCIEPLDA_VENDOR_ID 0x1556
#define PCIEPLDA_DEVICE_ID 0x1100

/* Identifies the PCIe-ABB Test board */
#define PCIEABB_VENDOR_ID 0x10dc
#define PCIEABB_DEVICE_ID 0x0153

/* Identifies the PCI-X PROGRAPE4 */
#define PCIXPG4_VENDOR_ID 0x1679
#define PCIXPG4_DEVICE_ID 0x0001

/* Identifies the PCI-64 PROGRAPE4 */
#define PCI64PG4_VENDOR_ID 0x1679
#define PCI64PG4_DEVICE_ID 0x0005

/* Identifies the PCI-E Xilinx ML605 */
#define PCIE_XILINX_VENDOR_ID 0x10ee
#define PCIE_ML605_DEVICE_ID 0x04a0

/*************************************************************************/
/* Some nice defines that make code more readable */
/* This is to print nice info in the log */

#ifdef DEBUG
 #define mod_info( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
 #define mod_info_dbg( args... ) \
    do { printk( KERN_INFO "%s - %s : ", MODNAME , __FUNCTION__ );\
    printk( args ); } while(0)
#else
 #define mod_info( args... ) \
    do { printk( KERN_INFO "%s: ", MODNAME );\
    printk( args ); } while(0)
 #define mod_info_dbg( args... ) 
#endif

#define mod_crit( args... ) \
    do { printk( KERN_CRIT "%s: ", MODNAME );\
    printk( args ); } while(0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif
