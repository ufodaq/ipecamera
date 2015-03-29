#define _POSIX_C_SOURCE 200112L
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <alloca.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <signal.h>

#include <getopt.h>

#include <fastwriter.h>

#include "pcitool/sysinfo.h"
#include "pcitool/formaters.h"

//#include "pci.h"
#include "tools.h"
#include "kernel.h"
#include "error.h"

/* defines */
#define MAX_KBUF 14
//#define BIGBUFSIZE (512*1024*1024)
#define BIGBUFSIZE (1024*1024)


#define DEFAULT_FPGA_DEVICE "/dev/fpga0"

#define LINE_WIDTH 80
#define SEPARATOR_WIDTH 2
#define BLOCK_SEPARATOR_WIDTH 2
#define BLOCK_SIZE 8
#define BENCHMARK_ITERATIONS 128
#define STATUS_MESSAGE_INTERVAL	5	/* seconds */


#define isnumber pcilib_isnumber
#define isxnumber pcilib_isxnumber
#define isnumber_n pcilib_isnumber_n
#define isxnumber_n pcilib_isxnumber_n

typedef uint8_t access_t;

typedef enum {
    GRAB_MODE_GRAB = 1,
    GRAB_MODE_TRIGGER = 2
} GRAB_MODE;

typedef enum {
    MODE_INVALID,
    MODE_INFO,
    MODE_LIST,
    MODE_BENCHMARK,
    MODE_READ,
    MODE_READ_REGISTER,
    MODE_WRITE,
    MODE_WRITE_REGISTER,
    MODE_RESET,
    MODE_GRAB,
    MODE_START_DMA,
    MODE_STOP_DMA,
    MODE_LIST_DMA,
    MODE_LIST_DMA_BUFFERS,
    MODE_READ_DMA_BUFFER,
    MODE_ENABLE_IRQ,
    MODE_DISABLE_IRQ,
    MODE_ACK_IRQ,
    MODE_WAIT_IRQ,
    MODE_ALLOC_KMEM,
    MODE_LIST_KMEM,
    MODE_READ_KMEM,
    MODE_FREE_KMEM
} MODE;

typedef enum {
    ACCESS_BAR,
    ACCESS_DMA,
    ACCESS_FIFO, 
    ACCESS_CONFIG
} ACCESS_MODE;

typedef enum {
    FLAG_MULTIPACKET = 1,
    FLAG_WAIT = 2
} FLAGS;


typedef enum {
    FORMAT_DEFAULT = 0,
    FORMAT_RAW,
    FORMAT_HEADER,
    FORMAT_RINGFS
} FORMAT;

typedef enum {
    PARTITION_UNKNOWN,
    PARTITION_RAW,
    PARTITION_EXT4,
    PARTITION_NULL
} PARTITION;

typedef enum {
    OPT_DEVICE = 'd',
    OPT_MODEL = 'm',
    OPT_BAR = 'b',
    OPT_ACCESS = 'a',
    OPT_ENDIANESS = 'e',
    OPT_SIZE = 's',
    OPT_OUTPUT = 'o',
    OPT_TIMEOUT = 't',
    OPT_INFO = 'i',
    OPT_LIST = 'l',
    OPT_READ = 'r',
    OPT_WRITE = 'w',
    OPT_GRAB = 'g',
    OPT_QUIETE = 'q',
    OPT_HELP = 'h',
    OPT_RESET = 128,
    OPT_BENCHMARK,
    OPT_TRIGGER,
    OPT_DATA_TYPE,
    OPT_EVENT,
    OPT_TRIGGER_RATE,
    OPT_TRIGGER_TIME,
    OPT_RUN_TIME,
    OPT_FORMAT,
    OPT_BUFFER,
    OPT_THREADS,
    OPT_LIST_DMA,
    OPT_LIST_DMA_BUFFERS,
    OPT_READ_DMA_BUFFER,
    OPT_START_DMA,
    OPT_STOP_DMA,
    OPT_ENABLE_IRQ,
    OPT_DISABLE_IRQ,
    OPT_ACK_IRQ,
    OPT_WAIT_IRQ,
    OPT_ITERATIONS,
    OPT_ALLOC_KMEM,
    OPT_LIST_KMEM,
    OPT_FREE_KMEM,
    OPT_READ_KMEM,
    OPT_BLOCK_SIZE,
    OPT_ALIGNMENT,
    OPT_TYPE,
    OPT_FORCE,
    OPT_VERIFY,
    OPT_WAIT,
    OPT_MULTIPACKET,
    OPT_VERBOSE
} OPTIONS;

static struct option long_options[] = {
    {"device",			required_argument, 0, OPT_DEVICE },
    {"model",			required_argument, 0, OPT_MODEL },
    {"bar",			required_argument, 0, OPT_BAR },
    {"access",			required_argument, 0, OPT_ACCESS },
    {"endianess",		required_argument, 0, OPT_ENDIANESS },
    {"size",			required_argument, 0, OPT_SIZE },
    {"output",			required_argument, 0, OPT_OUTPUT },
    {"timeout",			required_argument, 0, OPT_TIMEOUT },
    {"iterations",		required_argument, 0, OPT_ITERATIONS },
    {"info",			no_argument, 0, OPT_INFO },
    {"list",			no_argument, 0, OPT_LIST },
    {"reset",			no_argument, 0, OPT_RESET },
    {"benchmark",		optional_argument, 0, OPT_BENCHMARK },
    {"read",			optional_argument, 0, OPT_READ },
    {"write",			optional_argument, 0, OPT_WRITE },
    {"grab",			optional_argument, 0, OPT_GRAB },
    {"trigger",			optional_argument, 0, OPT_TRIGGER },
    {"data",			required_argument, 0, OPT_DATA_TYPE },
    {"event",			required_argument, 0, OPT_EVENT },
    {"run-time",		required_argument, 0, OPT_RUN_TIME },
    {"trigger-rate",		required_argument, 0, OPT_TRIGGER_RATE },
    {"trigger-time",		required_argument, 0, OPT_TRIGGER_TIME },
    {"format",			required_argument, 0, OPT_FORMAT },
    {"buffer",			optional_argument, 0, OPT_BUFFER },
    {"threads",			optional_argument, 0, OPT_THREADS },
    {"start-dma",		required_argument, 0, OPT_START_DMA },
    {"stop-dma",		optional_argument, 0, OPT_STOP_DMA },
    {"list-dma-engines",	no_argument, 0, OPT_LIST_DMA },
    {"list-dma-buffers",	required_argument, 0, OPT_LIST_DMA_BUFFERS },
    {"read-dma-buffer",		required_argument, 0, OPT_READ_DMA_BUFFER },
    {"enable-irq",		optional_argument, 0, OPT_ENABLE_IRQ },
    {"disable-irq",		optional_argument, 0, OPT_DISABLE_IRQ },
    {"acknowledge-irq",		optional_argument, 0, OPT_ACK_IRQ },
    {"wait-irq",		optional_argument, 0, OPT_WAIT_IRQ },
    {"list-kernel-memory",	optional_argument, 0, OPT_LIST_KMEM },
    {"read-kernel-memory",	required_argument, 0, OPT_READ_KMEM },
    {"alloc-kernel-memory",	required_argument, 0, OPT_ALLOC_KMEM },
    {"free-kernel-memory",	required_argument, 0, OPT_FREE_KMEM },
    {"type",			required_argument, 0, OPT_TYPE },
    {"block-size",		required_argument, 0, OPT_BLOCK_SIZE },
    {"alignment",		required_argument, 0, OPT_ALIGNMENT },
    {"quiete",			no_argument, 0, OPT_QUIETE },
    {"verbose",			optional_argument, 0, OPT_VERBOSE },
    {"force",			no_argument, 0, OPT_FORCE },
    {"verify",			no_argument, 0, OPT_VERIFY },
    {"multipacket",		no_argument, 0, OPT_MULTIPACKET },
    {"wait",			no_argument, 0, OPT_WAIT },
    {"help",			no_argument, 0, OPT_HELP },
    { 0, 0, 0, 0 }
};


void Usage(int argc, char *argv[], const char *format, ...) {
    if (format) {
	va_list ap;
    
	va_start(ap, format);
	printf("Error %i: ", errno);
	vprintf(format, ap);
	printf("\n");
	va_end(ap);
    
        printf("\n");
    }


    printf(
"Usage:\n"
" %s <mode> [options] [hex data]\n"
"  Modes:\n"
"   -i				- Device Info\n"
"   -l[l]			- List (detailed) Data Banks & Registers\n"
"   -r <addr|reg|dmaX>		- Read Data/Register\n"
"   -w <addr|reg|dmaX>		- Write Data/Register\n"
"   --benchmark <barX|dmaX>	- Performance Evaluation\n"
"   --reset			- Reset board\n"
"   --help			- Help message\n"
"\n"
"  Event Modes:\n"
"   --trigger [event]		- Trigger Events\n"
"   -g [event]			- Grab Events\n"
"\n"
"  IRQ Modes:\n"
"   --enable-irq [type]		- Enable IRQs\n"
"   --disable-irq [type]	- Disable IRQs\n"
"   --acknowledge-irq <source>	- Clean IRQ queue\n"
"   --wait-irq <source>		- Wait for IRQ\n"

"  DMA Modes:\n"
"   --start-dma <num>[r|w]	- Start specified DMA engine\n"
"   --stop-dma [num[r|w]]	- Stop specified engine or DMA subsystem\n"
"   --list-dma-engines		- List active DMA engines\n"
"   --list-dma-buffers <dma>	- List buffers for specified DMA engine\n"
"   --read-dma-buffer <dma:buf>	- Read the specified buffer\n"
"\n"
"  Kernel Modes:\n"
"   --list-kernel-memory [use] 	- List kernel buffers\n"
"   --read-kernel-memory <blk>	- Read the specified block of the kernel memory\n"
"				  block is specified as: use:block_number\n"
"   --alloc-kernel-memory <use>	- Allocate kernel buffers (DANGEROUS)\n"
"   --free-kernel-memory <use>	- Cleans lost kernel space buffers (DANGEROUS)\n"
"	dma			- Remove all buffers allocated by DMA subsystem\n"
"	#number			- Remove all buffers with the specified use id\n"
"\n"
"  Addressing:\n"
"   -d <device>			- FPGA device (/dev/fpga0)\n"
"   -m <model>			- Memory model (autodetected)\n"
"	pci			- Plain\n"
"	ipecamera		- IPE Camera\n"
"   -b <bank>			- PCI bar, Register bank, or DMA channel\n"
"\n"
"  Options:\n"
"   -s <size>			- Number of words (default: 1)\n"
"   -a [fifo|dma|config]<bits>	- Access type and bits per word (default: 32)\n"
"   -e <l|b>			- Endianess Little/Big (default: host)\n"
"   -o <file>			- Append output to file (default: stdout)\n"
"   -t <timeout|unlimited> 	- Timeout in microseconds\n"
"   --check			- Verify write operations\n"
"\n"
"  Event Options:\n"
"   --event <evt>		- Specifies event for trigger and grab modes\n"
"   --data <type>		- Data type to request for the events\n"
"   --run-time <us>		- Limit time to grab/trigger events\n"
"   -t <timeout|unlimited>	- Timeout to stop if no events triggered\n"
"   --trigger-rate <tps>		- Generate tps triggers per second\n"
"   --trigger-time <us>		- Specifies delay between triggers (us)\n"
"   -s <num|unlimited> 		- Number of events to grab and trigger\n"
"   --format [type]		- Specifies how event data should be stored\n"
"	raw			- Just write all events sequentially\n"
"	add_header		- Prefix events with 512 bit header:\n"
"				  event(64), data(64), nope(64), size(64)\n"
"				  seqnum(64), offset(64), timestamp(128)\n"
//"	ringfs			- Write to RingFS\n"
"   --buffer [size]		- Request data buffering, size in MB\n"
"   --threads [num]		- Allow multithreaded processing\n"
"\n"
"  DMA Options:\n"
"   --multipacket		- Read multiple packets\n"
"   --wait			- Wait until data arrives\n"
"\n"
"  Kernel Options:\n"
"   --type <type>		- Type of kernel memory to allocate\n"
"   	consistent		- Consistent memory\n"
"   	s2c			- DMA S2C (write) memory\n"
"   	c2s			- DMA C2S (read) memory\n"
"   --page-size <size>		- Size of kernel buffer in bytes (default: page)\n"
"   -s <size>			- Number of buffers to allocate (default: 1)\n"
"   --allignment <alignment>	- Buffer alignment (default: page)\n"
"\n"
"  Information:\n"
"   --verbose [level]		- Announce details of ongoing operations\n"
"   -q				- Quiete mode (suppress warnings)\n"
"\n"
"  Data:\n"
"	Data can be specified as sequence of hexdecimal number or\n"
"	a single value prefixed with '*'. In this case it will be\n"
"	replicated the specified amount of times\n"
"\n\n",
argv[0]);

    exit(0);
}

static int StopFlag = 0;

static void signal_exit_handler(int signo) { 
    if (++StopFlag > 2)
	exit(-1);
}


void Error(const char *format, ...) {
    va_list ap;
    
    va_start(ap, format);
    printf("Error %i: ", errno);
    vprintf(format, ap);
    if (errno) printf("\n errno: %s", strerror(errno));
    printf("\n\n");
    va_end(ap);
    
    exit(-1);
}

void Silence(const char *format, ...) {
}

void List(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, int details) {
    int i,j;
    pcilib_register_bank_description_t *banks;
    pcilib_register_description_t *registers;
    pcilib_event_description_t *events;
    pcilib_event_data_type_description_t *types;

    const pcilib_board_info_t *board_info = pcilib_get_board_info(handle);
    const pcilib_dma_info_t *dma_info = pcilib_get_dma_info(handle);
    
    for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	if (board_info->bar_length[i] > 0) {
	    printf(" BAR %d - ", i);

	    switch ( board_info->bar_flags[i]&IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:  printf(" IO"); break;
		case IORESOURCE_MEM: printf("MEM"); break;
		case IORESOURCE_IRQ: printf("IRQ"); break;
		case IORESOURCE_DMA: printf("DMA"); break;
	    }
	    
	    if (board_info->bar_flags[i]&IORESOURCE_MEM_64) printf("64");
	    else printf("32");
	    
	    printf(", Start: 0x%08lx, Length: 0x%8lx, Flags: 0x%08lx\n", board_info->bar_start[i], board_info->bar_length[i], board_info->bar_flags[i] );
	}
    }
    printf("\n");
    
    if ((dma_info)&&(dma_info->engines)) {
	printf("DMA Engines: \n");
	for (i = 0; dma_info->engines[i]; i++) {
	    pcilib_dma_engine_description_t *engine = dma_info->engines[i];
	    printf(" DMA %2d ", engine->addr);
	    switch (engine->direction) {
		case PCILIB_DMA_FROM_DEVICE:
		    printf("C2S");
		break;
		case PCILIB_DMA_TO_DEVICE:
		    printf("S2C");
		break;
		case PCILIB_DMA_BIDIRECTIONAL:
		    printf("BI ");
		break;
	    }
	    printf(" - Type: ");
	    switch (engine->type) {
		case PCILIB_DMA_TYPE_BLOCK:
		    printf("Block");
		break;
		case PCILIB_DMA_TYPE_PACKET:
		    printf("Packet");
		break;
		default:
		    printf("Unknown");
	    }
	    
	    printf(", Address Width: %02lu bits\n", engine->addr_bits);
	}
	printf("\n");
    }

    if ((bank)&&(bank != (char*)-1)) banks = NULL;
    else banks = model_info->banks;
    
    if (banks) {
	printf("Banks: \n");
	for (i = 0; banks[i].access; i++) {
	    printf(" 0x%02x %s", banks[i].addr, banks[i].name);
	    if ((banks[i].description)&&(banks[i].description[0])) {
		printf(": %s", banks[i].description);
	    }
	    printf("\n");
	}
	printf("\n");
    }
    
    if (bank == (char*)-1) registers = NULL;
    else registers = model_info->registers;
    
    if (registers) {
        pcilib_register_bank_addr_t bank_addr = 0;
	if (bank) {
	    pcilib_register_bank_t bank_id = pcilib_find_bank(handle, bank);
	    pcilib_register_bank_description_t *b = model_info->banks + bank_id;

	    bank_addr = b->addr;
	    if (b->description) printf("%s:\n", b->description);
	    else if (b->name) printf("Registers of bank %s:\n", b->name);
	    else printf("Registers of bank 0x%x:\n", b->addr);
	} else {
	    printf("Registers: \n");
	}
	for (i = 0; registers[i].bits; i++) {
	    const char *mode;
	    
	    if ((bank)&&(registers[i].bank != bank_addr)) continue;
	    if (registers[i].type == PCILIB_REGISTER_BITS) {
		if (!details) continue;
		
		if (registers[i].bits > 1) {
		    printf("    [%2u:%2u] - %s\n", registers[i].offset, registers[i].offset + registers[i].bits, registers[i].name);
		} else {
		    printf("    [   %2u] - %s\n", registers[i].offset, registers[i].name);
		}
		
		continue;
	    }

	    if (registers[i].mode == PCILIB_REGISTER_RW) mode = "RW";
	    else if (registers[i].mode == PCILIB_REGISTER_R) mode = "R ";
	    else if (registers[i].mode == PCILIB_REGISTER_W) mode = " W";
	    else mode = "  ";
	    
	    printf(" 0x%02x (%2i %s) %s", registers[i].addr, registers[i].bits, mode, registers[i].name);
	    if ((details > 0)&&(registers[i].description)&&(registers[i].description[0])) {
		printf(": %s", registers[i].description);
	    }
	    printf("\n");

	}
	printf("\n");
    }

    if (bank == (char*)-1) events = NULL;
    else {
	events = model_info->events;
	types = model_info->data_types;
    }

    if (events) {
	printf("Events: \n");
	for (i = 0; events[i].name; i++) {
	    printf(" %s", events[i].name);
	    if ((events[i].description)&&(events[i].description[0])) {
		printf(": %s", events[i].description);
	    }
	    
	    if (types) {
		for (j = 0; types[j].name; j++) {
		    if (types[j].evid & events[i].evid) {
			printf("\n    %s", types[j].name);
			if ((types[j].description)&&(types[j].description[0])) {
			    printf(": %s", types[j].description);
			}
		    }
		}
	    }
	}
	printf("\n");
    }
}

void Info(pcilib_t *handle, pcilib_model_description_t *model_info) {
    const pcilib_board_info_t *board_info = pcilib_get_board_info(handle);

    printf("Vendor: %x, Device: %x, Bus: %x, Slot: %x, Function: %x\n", board_info->vendor_id, board_info->device_id, board_info->bus, board_info->slot, board_info->func);
    printf(" Interrupt - Pin: %i, Line: %i\n", board_info->interrupt_pin, board_info->interrupt_line);
    List(handle, model_info, (char*)-1, 0);
}


#define BENCH_MAX_DMA_SIZE 4 * 1024 * 1024
#define BENCH_MAX_FIFO_SIZE 1024 * 1024

int Benchmark(pcilib_t *handle, ACCESS_MODE mode, pcilib_dma_engine_addr_t dma, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, size_t iterations) {
    int err;
    int i, j, errors;
    void *data, *buf, *check;
    void *fifo = NULL;
    struct timeval start, end;
    unsigned long time;
    size_t size, min_size, max_size;
    double mbs_in, mbs_out, mbs;
    size_t irqs;
    
    const pcilib_board_info_t *board_info = pcilib_get_board_info(handle);
    
    if (mode == ACCESS_CONFIG)
	Error("No benchmarking of configuration space acess is allowed");

    if (mode == ACCESS_DMA) {
	if (n) {
	    min_size = n * access;
	    max_size = n * access;
	} else {
	    min_size = 1024;
	    max_size = BENCH_MAX_DMA_SIZE;
	}
	
        for (size = min_size; size <= max_size; size *= 4) {
	    mbs_in = pcilib_benchmark_dma(handle, dma, addr, size, iterations, PCILIB_DMA_FROM_DEVICE);
	    mbs_out = pcilib_benchmark_dma(handle, dma, addr, size, iterations, PCILIB_DMA_TO_DEVICE);
	    mbs = pcilib_benchmark_dma(handle, dma, addr, size, iterations, PCILIB_DMA_BIDIRECTIONAL);
	    err = pcilib_wait_irq(handle, 0, 0, &irqs);
	    if (err) irqs = 0;
	    
	    printf("%8zu KB - ", size / 1024);
	    
	    printf("RW: ");
	    if (mbs < 0) printf("failed ...   ");
	    else printf("%8.2lf MB/s", mbs);

	    printf(", R: ");
	    if (mbs_in < 0) printf("failed ...   ");
	    else printf("%8.2lf MB/s", mbs_in);

	    printf(", W: ");
	    if (mbs_out < 0) printf("failed ...   ");
	    else printf("%8.2lf MB/s", mbs_out);
	    
	    if (irqs) {
	        printf(", IRQs: %lu", irqs);
	    }

	    printf("\n");
	}
	
	return 0;
    }

    if (bar == PCILIB_BAR_INVALID) {
	unsigned long maxlength = 0;
	

	for (i = 0; i < PCILIB_MAX_BANKS; i++) {
	    if ((addr >= board_info->bar_start[i])&&((board_info->bar_start[i] + board_info->bar_length[i]) >= (addr + access))) {
		bar = i;
		break;
	    }

	    if (board_info->bar_length[i] > maxlength) {
		maxlength = board_info->bar_length[i];
		bar = i;
	    }
	}
	
	if (bar < 0) Error("Data banks are not available");
    }
    
    if (n) {
	if ((mode == ACCESS_BAR)&&(n * access > board_info->bar_length[bar])) Error("The specified size (%i) exceeds the size of bar (%i)", n * access, board_info->bar_length[bar]);

	min_size = n * access;
	max_size = n * access;
    } else {
	min_size = access;
	if (mode == ACCESS_BAR) max_size = board_info->bar_length[bar];
	else max_size = BENCH_MAX_FIFO_SIZE;
    }
    
    err = posix_memalign( (void**)&buf, 256, max_size );
    if (!err) err = posix_memalign( (void**)&check, 256, max_size );
    if ((err)||(!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", max_size);

    data = pcilib_map_bar(handle, bar);
    if (!data) Error("Can't map bar %i", bar);

    if (mode == ACCESS_FIFO) {
        fifo = data + (addr - board_info->bar_start[bar]) + (board_info->bar_start[bar] & pcilib_get_page_mask());
//	pcilib_resolve_register_address(handle, bar, addr);
	if (!fifo) Error("Can't resolve address (%lx) in bar (%u)", addr, bar);
    }

    if (mode == ACCESS_FIFO)
	printf("Transfer time (Bank: %i, Fifo: %lx):\n", bar, addr);
    else
	printf("Transfer time (Bank: %i):\n", bar);
	
    for (size = min_size ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_memcpy(buf, data, size);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		for (j = 0; j < (size/access); j++) {
		    pcilib_memcpy(buf + j * access, fifo, access);
		}
	    }
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8zu bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
	
	fflush(0);

	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_memcpy(data, buf, size);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		for (j = 0; j < (size/access); j++) {
		    pcilib_memcpy(fifo, buf + j * access, access);
		}
	    }
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s\n", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
    }
    
    pcilib_unmap_bar(handle, bar, data);

    printf("\n\nOpen-Transfer-Close time: \n");
    
    for (size = 4 ; size < max_size; size *= 8) {
	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_read(handle, bar, 0, size, buf);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_read_fifo(handle, bar, addr, access, size / access, buf);
	    }
	}
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf("%8zu bytes - read: %8.2lf MB/s", size, 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
	
	fflush(0);

	gettimeofday(&start,NULL);
	if (mode == ACCESS_BAR) {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_write(handle, bar, 0, size, buf);
	    }
	} else {
	    for (i = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_write_fifo(handle, bar, addr, access, size / access, buf);
	    }
	}
	
	gettimeofday(&end,NULL);

	time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	printf(", write: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));

	if (mode == ACCESS_BAR) {
	    gettimeofday(&start,NULL);
	    for (i = 0, errors = 0; i < BENCHMARK_ITERATIONS; i++) {
		pcilib_write(handle, bar, 0, size, buf);
		pcilib_read(handle, bar, 0, size, check);
		if (memcmp(buf, check, size)) ++errors;
	    }
	    gettimeofday(&end,NULL);

	    time = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
	    printf(", write-verify: %8.2lf MB/s", 1000000. * size * BENCHMARK_ITERATIONS / (time * 1024. * 1024.));
	    if (errors) printf(", errors: %u of %u", errors, BENCHMARK_ITERATIONS);
	}
	printf("\n");
    }
    
    printf("\n\n");

    free(check);
    free(buf);
    
    return 0;
}

#define pci2host16(endianess, value) endianess?

/*
typedef struct {
    size_t size;
    void *data;
    size_t pos;

    int multi_mode;
} DMACallbackContext;

static int DMACallback(void *arg, pcilib_dma_flags_t flags, size_t bufsize, void *buf) {
    DMACallbackContext *ctx = (DMACallbackContext*)arg;
    
    if ((ctx->pos + bufsize > ctx->size)||(!ctx->data)) {
	ctx->size *= 2;
	ctx->data = realloc(ctx->data, ctx->size);
	if (!ctx->data) {
	    Error("Allocation of %i bytes of memory have failed", ctx->size);
	    return 0;
	}
    }
    
    memcpy(ctx->data + ctx->pos, buf, bufsize);
    ctx->pos += bufsize;

    if (flags & PCILIB_DMA_FLAG_EOP) return 0;
    return 1;
}
*/


int ReadData(pcilib_t *handle, ACCESS_MODE mode, FLAGS flags, pcilib_dma_engine_addr_t dma, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, int endianess, size_t timeout, FILE *o) {
    void *buf;
    int i, err;
    size_t ret, bytes;
    size_t size = n * abs(access);
    int block_width, blocks_per_line;
    int numbers_per_block, numbers_per_line; 
    pcilib_dma_engine_t dmaid;
    pcilib_dma_flags_t dma_flags = 0;

    int fd;
    char stmp[256];
    struct stat st;
    const pcilib_board_info_t *board_info;

    numbers_per_block = BLOCK_SIZE / access;

    block_width = numbers_per_block * ((access * 2) +  SEPARATOR_WIDTH);
    blocks_per_line = (LINE_WIDTH - 10) / (block_width +  BLOCK_SEPARATOR_WIDTH);
    if ((blocks_per_line > 1)&&(blocks_per_line % 2)) --blocks_per_line;
    numbers_per_line = blocks_per_line * numbers_per_block;

    if (size) {
	buf = malloc(size);
        if (!buf) Error("Allocation of %zu bytes of memory has failed", size);
    } else {
	buf = NULL;
    }
    
    switch (mode) {
      case ACCESS_DMA:
        if (timeout == (size_t)-1) timeout = PCILIB_DMA_TIMEOUT;
    
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_FROM_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (%lu) is specified", dma);
	
	if (flags&FLAG_MULTIPACKET) dma_flags |= PCILIB_DMA_FLAG_MULTIPACKET;
	if (flags&FLAG_WAIT) dma_flags |= PCILIB_DMA_FLAG_WAIT;
	
	if (size) {
	    err = pcilib_read_dma_custom(handle, dmaid, addr, size, dma_flags, timeout, buf, &bytes);
	    if (err) Error("Error (%i) is reported by DMA engine", err);
	} else {
	    dma_flags |= PCILIB_DMA_FLAG_IGNORE_ERRORS;
	    
	    size = 2048; bytes = 0;
	    do {
		size *= 2;
		buf = realloc(buf, size);
		if (!buf) Error("Allocation of %zu bytes of memory has failed", size);
	        err = pcilib_read_dma_custom(handle, dmaid, addr, size - bytes, dma_flags, timeout, buf + bytes, &ret);
		bytes += ret;
		
		if ((!err)&&(flags&FLAG_MULTIPACKET)) {
		    err = PCILIB_ERROR_TOOBIG;
		    if ((flags&FLAG_WAIT)==0) timeout = 0;
		}
	    } while (err == PCILIB_ERROR_TOOBIG);
	}

	if ((err)&&(err != PCILIB_ERROR_TIMEOUT)) {
	    Error("Error (%i) during DMA read", err);
	}
	if (bytes <= 0) {
	    pcilib_warning("No data is returned by DMA engine");
	    return 0;
	}
	size = bytes;
	n = bytes / abs(access);
	addr = 0;
      break;
      case ACCESS_FIFO:
	pcilib_read_fifo(handle, bar, addr, access, n, buf);
	addr = 0;
      break;
      case ACCESS_CONFIG:
	board_info = pcilib_get_board_info(handle);
	sprintf(stmp, "/sys/bus/pci/devices/0000:%02x:%02x.%1x/config", board_info->bus, board_info->slot, board_info->func);
	fd = open(stmp, O_RDONLY);

	if ((!fd)||(fstat(fd, &st))) Error("Can't open %s", stmp);
	
        if (st.st_size < addr) 
    	    Error("Access beyond the end of PCI configuration space");
    	
	if (st.st_size < (addr + size)) {
	    n = (st.st_size - addr) / abs(access);
	    size = n * abs(access);
	    if (!n) Error("Access beyond the end of PCI configuration space");
	}
	
	lseek(fd, addr, SEEK_SET);
	ret = read(fd, buf, size);
	if (ret == (size_t)-1) Error("Error reading %s", stmp);
	
	if (ret < size) {
	    size = ret;
	    n = ret / abs(access);
	}
	
	close(fd);
      break;
      default:
	pcilib_read(handle, bar, addr, size, buf);
    }
    
    if (endianess) pcilib_swap(buf, buf, abs(access), n);

    if (o) {
	printf("Writting output (%zu bytes) to file (append to the end)...\n", n * abs(access));
	fwrite(buf, abs(access), n, o);
    } else {
	for (i = 0; i < n; i++) {
	    if (i) {
		if (i%numbers_per_line == 0) printf("\n");
		else {
		    printf("%*s", SEPARATOR_WIDTH, "");
		    if (i%numbers_per_block == 0) printf("%*s", BLOCK_SEPARATOR_WIDTH, "");
		}
	    }
	    
	    if (i%numbers_per_line == 0) printf("%8lx:  ", addr + i * abs(access));

	    switch (access) {
		case 1: printf("%0*hhx", access * 2, ((uint8_t*)buf)[i]); break;
		case 2: printf("%0*hx", access * 2, ((uint16_t*)buf)[i]); break;
		case 4: printf("%0*x", access * 2, ((uint32_t*)buf)[i]); break;
		case 8: printf("%0*lx", access * 2, ((uint64_t*)buf)[i]); break;
	    }
	}
	printf("\n\n");
    }

    
    free(buf);
    return 0;
}




int ReadRegister(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, const char *reg) {
    int i;
    int err;
    const char *format;

    pcilib_register_bank_t bank_id;
    pcilib_register_bank_addr_t bank_addr = 0;

    pcilib_register_value_t value;
    
    if (reg) {
	pcilib_register_t regid = pcilib_find_register(handle, bank, reg);
        bank_id = pcilib_find_bank_by_addr(handle, model_info->registers[regid].bank);
        format = model_info->banks[bank_id].format;
        if (!format) format = "%lu";

        err = pcilib_read_register_by_id(handle, regid, &value);
    //    err = pcilib_read_register(handle, bank, reg, &value);
        if (err) printf("Error reading register %s\n", reg);
        else {
	    printf("%s = ", reg);
	    printf(format, value);
	    printf("\n");
	}
    } else {
	    // Adding DMA registers
	pcilib_get_dma_info(handle);	
    
	if (model_info->registers) {
	    if (bank) {
		bank_id = pcilib_find_bank(handle, bank);
		bank_addr = model_info->banks[bank_id].addr;
	    }
	    
	    printf("Registers:\n");
	    for (i = 0; model_info->registers[i].bits; i++) {
		if ((model_info->registers[i].mode & PCILIB_REGISTER_R)&&((!bank)||(model_info->registers[i].bank == bank_addr))&&(model_info->registers[i].type != PCILIB_REGISTER_BITS)) { 
		    bank_id = pcilib_find_bank_by_addr(handle, model_info->registers[i].bank);
		    format = model_info->banks[bank_id].format;
		    if (!format) format = "%lu";

		    err = pcilib_read_register_by_id(handle, i, &value);
		    if (err) printf(" %s = error reading value", model_info->registers[i].name);
	    	    else {
			printf(" %s = ", model_info->registers[i].name);
			printf(format, value);
		    }

		    printf(" [");
		    printf(format, model_info->registers[i].defvalue);
		    printf("]");
		    printf("\n");
		}
	    }
	} else {
	    printf("No registers");
	}
	printf("\n");
    }
    
    return 0;
}

#define WRITE_REGVAL(buf, n, access, o) {\
    uint##access##_t tbuf[n]; \
    for (i = 0; i < n; i++) { \
	tbuf[i] = (uint##access##_t)buf[i]; \
    } \
    fwrite(tbuf, access/8, n, o); \
}

int ReadRegisterRange(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, uintptr_t addr, long addr_shift, size_t n, FILE *o) {
    int err;
    int i;

    pcilib_register_bank_description_t *banks = model_info->banks;
    pcilib_register_bank_t bank_id = pcilib_find_bank(handle, bank);

    if (bank_id == PCILIB_REGISTER_BANK_INVALID) {
	if (bank) Error("Invalid register bank is specified (%s)", bank);
	else Error("Register bank should be specified");
    }

    int access = banks[bank_id].access / 8;
//    int size = n * abs(access);
    int block_width, blocks_per_line;
    int numbers_per_block, numbers_per_line; 
    
    numbers_per_block = BLOCK_SIZE / access;

    block_width = numbers_per_block * ((access * 2) +  SEPARATOR_WIDTH);
    blocks_per_line = (LINE_WIDTH - 6) / (block_width +  BLOCK_SEPARATOR_WIDTH);
    if ((blocks_per_line > 1)&&(blocks_per_line % 2)) --blocks_per_line;
    numbers_per_line = blocks_per_line * numbers_per_block;


    pcilib_register_value_t buf[n];
    err = pcilib_read_register_space(handle, bank, addr, n, buf);
    if (err) Error("Error reading register space for bank \"%s\" at address %lx, size %lu", bank?bank:"default", addr, n);


    if (o) {
	printf("Writting output (%zu bytes) to file (append to the end)...\n", n * abs(access));
	switch (access) {
	    case 1: WRITE_REGVAL(buf, n, 8, o) break;
	    case 2: WRITE_REGVAL(buf, n, 16, o) break;
	    case 4: WRITE_REGVAL(buf, n, 32, o) break;
	    case 8: WRITE_REGVAL(buf, n, 64, o) break;
	}
    } else {
	for (i = 0; i < n; i++) {
	    if (i) {
		if (i%numbers_per_line == 0) printf("\n");
		else {
		    printf("%*s", SEPARATOR_WIDTH, "");
		    if (i%numbers_per_block == 0) printf("%*s", BLOCK_SEPARATOR_WIDTH, "");
		}
	    }
	    
	    if (i%numbers_per_line == 0) printf("%4lx:  ", addr + 4 * i - addr_shift);
	    printf("%0*lx", access * 2, (unsigned long)buf[i]);
	}
	printf("\n\n");
    }
    
    return 0;
}

int WriteData(pcilib_t *handle, ACCESS_MODE mode, pcilib_dma_engine_addr_t dma, pcilib_bar_t bar, uintptr_t addr, size_t n, access_t access, int endianess, char ** data, int verify) {
    int read_back = 0;
    void *buf, *check;
    int res = 0, i, err;
    int size = n * abs(access);
    size_t ret;
    pcilib_dma_engine_t dmaid;

    if (mode == ACCESS_CONFIG)
	Error("Writting to PCI configuration space is not supported");

    err = posix_memalign( (void**)&buf, 256, size );
    if (!err) err = posix_memalign( (void**)&check, 256, size );
    if ((err)||(!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", size);

    for (i = 0; i < n; i++) {
	switch (access) {
	    case 1: res = sscanf(data[i], "%hhx", ((uint8_t*)buf)+i); break;
	    case 2: res = sscanf(data[i], "%hx", ((uint16_t*)buf)+i); break;
	    case 4: res = sscanf(data[i], "%x", ((uint32_t*)buf)+i); break;
	    case 8: res = sscanf(data[i], "%lx", ((uint64_t*)buf)+i); break;
	    default: Error("Unexpected data size (%lu)", access);
	}
	if ((res != 1)||(!isxnumber(data[i]))) Error("Can't parse data value at poition %i, (%s) is not valid hex number", i, data[i]);
    }

    if (endianess) pcilib_swap(buf, buf, abs(access), n);

    switch (mode) {
      case ACCESS_DMA:
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_TO_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (%lu) is specified", dma);
	err = pcilib_write_dma(handle, dmaid, addr, size, buf, &ret);
	if ((err)||(ret != size)) {
	    if (err == PCILIB_ERROR_TIMEOUT) Error("Timeout writting the data to DMA"); 
	    else if (err) Error("DMA engine returned a error while writing the data");
	    else if (!ret) Error("No data is written by DMA engine");
	    else Error("Only %lu bytes of %lu is written by DMA engine", ret, size);
	}
      break;
      case ACCESS_FIFO:
	pcilib_write_fifo(handle, bar, addr, access, n, buf);
      break;
      default:
	pcilib_write(handle, bar, addr, size, buf);
	if (verify) {
	    pcilib_read(handle, bar, addr, size, check);
	    read_back = 1;
	}
    }

    if ((read_back)&&(memcmp(buf, check, size))) {
	printf("Write failed: the data written and read differ, the foolowing is read back:\n");
        if (endianess) pcilib_swap(check, check, abs(access), n);
	ReadData(handle, mode, 0, dma, bar, addr, n, access, endianess, (size_t)-1, NULL);
	exit(-1);
    }

    free(check);
    free(buf);
    
    return 0;
}

int WriteRegisterRange(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, uintptr_t addr, long addr_shift, size_t n, char ** data) {
    pcilib_register_value_t *buf, *check;
    int res, i, err;
    unsigned long value;
    int size = n * sizeof(pcilib_register_value_t);

    err = posix_memalign( (void**)&buf, 256, size );
    if (!err) err = posix_memalign( (void**)&check, 256, size );
    if ((err)||(!buf)||(!check)) Error("Allocation of %i bytes of memory have failed", size);

    for (i = 0; i < n; i++) {
	res = sscanf(data[i], "%lx", &value);
	if ((res != 1)||(!isxnumber(data[i]))) Error("Can't parse data value at poition %i, (%s) is not valid hex number", i, data[i]);
	buf[i] = value;
    }

    err = pcilib_write_register_space(handle, bank, addr, n, buf);
    if (err) Error("Error writting register space for bank \"%s\" at address %lx, size %lu", bank?bank:"default", addr, n);
    
    err = pcilib_read_register_space(handle, bank, addr, n, check);
    if (err) Error("Error reading register space for bank \"%s\" at address %lx, size %lu", bank?bank:"default", addr, n);

    if (memcmp(buf, check, size)) {
	printf("Write failed: the data written and read differ, the foolowing is read back:\n");
	ReadRegisterRange(handle, model_info, bank, addr, addr_shift, n, NULL);
	exit(-1);
    }

    free(check);
    free(buf);
    
    return 0;

}

int WriteRegister(pcilib_t *handle, pcilib_model_description_t *model_info, const char *bank, const char *reg, char ** data) {
    int err;

    unsigned long val;
    pcilib_register_value_t value;

    const char *format = NULL;

    pcilib_register_t regid = pcilib_find_register(handle, bank, reg);
    if (regid == PCILIB_REGISTER_INVALID) Error("Can't find register (%s) from bank (%s)", reg, bank?bank:"autodetected");

/*
    pcilib_register_bank_t bank_id;
    pcilib_register_bank_addr_t bank_addr;

    bank_id = pcilib_find_bank_by_addr(handle, model_info->registers[regid].bank);
    if (bank_id == PCILIB_REGISTER_BANK_INVALID) Error("Can't find bank of the register (%s)", reg);
    format = model_info->banks[bank_id].format;
    if (!format) format = "%lu";
*/

    if (isnumber(*data)) {
	if (sscanf(*data, "%li", &val) != 1) {
	    Error("Can't parse data value (%s) is not valid decimal number", *data);
	}

	format = "%li";
    } else if (isxnumber(*data)) {
	if (sscanf(*data, "%lx", &val) != 1) {
	    Error("Can't parse data value (%s) is not valid decimal number", *data);
	}
	
	format = "0x%lx";
    } else {
	    Error("Can't parse data value (%s) is not valid decimal number", *data);
    }

    value = val;
    
    err = pcilib_write_register(handle, bank, reg, value);
    if (err) Error("Error writting register %s\n", reg);

    if ((model_info->registers[regid].mode&PCILIB_REGISTER_RW) == PCILIB_REGISTER_RW) {
	err = pcilib_read_register(handle, bank, reg, &value);
	if (err) Error("Error reading back register %s for verification\n", reg);
    
	if (val != value) {
	    Error("Failed to write register %s: %lu is written and %lu is read back", reg, val, value);
	} else {
	    printf("%s = ", reg);
	    printf(format, value);
	    printf("\n");
	}
    } else {
        printf("%s is written\n ", reg);
    }
    
    return 0;
}

typedef struct {
    pcilib_t *handle;
    pcilib_event_t event;
    pcilib_event_data_type_t data;

    fastwriter_t *writer;

    int verbose;
    pcilib_timeout_t timeout;
    size_t run_time;
    size_t trigger_time;    
    size_t max_triggers;
    pcilib_event_flags_t flags;
    FORMAT format;
    
    volatile int event_pending;			/**< Used to detect that we have read previously triggered event */
    volatile int trigger_thread_started;	/**< Indicates that trigger thread is ready and we can't procced to start event recording */
    volatile int started;			/**< Indicates that recording is started */
    
    volatile int run_flag;
    volatile int writing_flag;

    struct timeval first_frame;    
    struct timeval last_frame;
    size_t last_num;
    
    size_t trigger_failed;
    size_t trigger_count;
    size_t event_count;
    size_t incomplete_count;
    size_t broken_count;
    size_t missing_count;
    size_t storage_count;
    
    struct timeval start_time;
    struct timeval stop_time;
} GRABContext;

int GrabCallback(pcilib_event_id_t event_id, pcilib_event_info_t *info, void *user) {
    int err = 0;
    void *data;
    size_t size;
    
    GRABContext *ctx = (GRABContext*)user;
    pcilib_t *handle = ctx->handle;

    gettimeofday(&ctx->last_frame, NULL);

    if (!ctx->event_count) {
	memcpy(&ctx->first_frame, &ctx->last_frame, sizeof(struct timeval));
    }

    ctx->event_pending = 0;
    ctx->event_count++;
    
    if (ctx->last_num)
	ctx->missing_count += (info->seqnum - ctx->last_num) - 1;
    ctx->last_num = info->seqnum;
    
    if (info->flags&PCILIB_EVENT_INFO_FLAG_BROKEN) {
	ctx->incomplete_count++;
	return PCILIB_STREAMING_CONTINUE;
    }
    
    switch (ctx->format) {
     case FORMAT_DEFAULT:
	data = pcilib_get_data(handle, event_id, PCILIB_EVENT_DATA, &size);
	break;
     default:
	data = pcilib_get_data(handle, event_id, PCILIB_EVENT_RAW_DATA, &size);
    }
        
    if (!data) {
	ctx->broken_count++;
	return PCILIB_STREAMING_CONTINUE;
    }

    if (ctx->format == FORMAT_HEADER) {
	uint64_t header[8];
	header[0] = info->type;
	header[1] = ctx->data;
	header[2] = 0;
	header[3] = size;
	header[4] = info->seqnum;
	header[5] = info->offset;
	memcpy(header + 6, &info->timestamp, 16);
	
	err = fastwriter_push(ctx->writer, 64, header);
    }

    if (!err) 
	err = fastwriter_push(ctx->writer, size, data);

    if (err) {
	fastwriter_cancel(ctx->writer);

	if (err != EWOULDBLOCK)
	    Error("Storage error %i", err);

	ctx->storage_count++;
	pcilib_return_data(handle, event_id, ctx->data, data);
	return PCILIB_STREAMING_CONTINUE;
    }

    err = pcilib_return_data(handle, event_id, ctx->data, data);
    if (err) {
	ctx->missing_count++;
	fastwriter_cancel(ctx->writer);
	return PCILIB_STREAMING_CONTINUE;
    }

    err = fastwriter_commit(ctx->writer);
    if (err) Error("Error commiting data to storage, Error: %i", err);
    
    return PCILIB_STREAMING_CONTINUE;
}

int raw_data(pcilib_event_id_t event_id, pcilib_event_info_t *info, pcilib_event_flags_t flags, size_t size, void *data, void *user) {
    int err;

    GRABContext *ctx = (GRABContext*)user;
//    pcilib_t *handle = ctx->handle;


    if ((info)&&(info->seqnum != ctx->last_num)) {
        gettimeofday(&ctx->last_frame, NULL);
	if (!ctx->event_count) {
	    memcpy(&ctx->first_frame, &ctx->last_frame, sizeof(struct timeval));
	}

	ctx->event_count++;
	ctx->missing_count += (info->seqnum - ctx->last_num) - 1;
	ctx->last_num = info->seqnum;
    }

    err = fastwriter_push_data(ctx->writer, size, data);
    if (err) {
	if (err == EWOULDBLOCK) Error("Storage is not able to handle the data stream, buffer overrun");
	Error("Storage error %i", err);
    }
    
    return PCILIB_STREAMING_CONTINUE;
}


void *Trigger(void *user) {
    int err;
    struct timeval start;

    GRABContext *ctx = (GRABContext*)user;
    size_t trigger_time = ctx->trigger_time;
    size_t max_triggers = ctx->max_triggers;
    
    ctx->trigger_thread_started = 1;
    ctx->event_pending = 1;

    while (!ctx->started) ;

    gettimeofday(&start, NULL);
    do {
        err = pcilib_trigger(ctx->handle, ctx->event, 0, NULL);
        if (err) ctx->trigger_failed++;
	if ((++ctx->trigger_count == max_triggers)&&(max_triggers)) break;
	
	if (trigger_time) {
	    pcilib_add_timeout(&start, trigger_time);
	    if ((ctx->stop_time.tv_sec)&&(pcilib_timecmp(&start, &ctx->stop_time)>0)) break;
	    pcilib_sleep_until_deadline(&start);
	}  else {
	    while ((ctx->event_pending)&&(ctx->run_flag)) usleep(10);
	    ctx->event_pending = 1;
	}
    } while (ctx->run_flag);

    ctx->trigger_thread_started = 0;

    return NULL;
}

void GrabStats(GRABContext *ctx, struct timeval *end_time) {
    int verbose;
    pcilib_timeout_t duration, fps_duration;
    struct timeval cur;
    double fps = 0, good_fps = 0;
    size_t total, good, pending = 0;

    verbose = ctx->verbose;
    
    if (end_time) {
	if (verbose++) {
	    printf("-------------------------------------------------------------------------------\n");
	}
    } else {
	gettimeofday(&cur, NULL);
	end_time = &cur;
    }

//    if ((ctx->event_count + ctx->missing_count) == 0) 
//	return;
    
    duration = pcilib_timediff(&ctx->start_time, end_time);
    fps_duration = pcilib_timediff(&ctx->first_frame, &ctx->last_frame);
    
    if (ctx->trigger_count) {
	total = ctx->trigger_count;
	pending = ctx->trigger_count - ctx->event_count - ctx->missing_count - ctx->trigger_failed;
    } else {
	total = ctx->event_count + ctx->missing_count;
    }
    
    good = ctx->event_count - ctx->broken_count - ctx->incomplete_count - ctx->storage_count;

    if (ctx->event_count > 1) {
	fps = (ctx->event_count - 1) / (1.*fps_duration/1000000);
    }
    
    if (good > 1) {
	good_fps = (good - 1) / (1.*fps_duration/1000000);
    }

    printf("Run: ");
    PrintTime(duration);    
    
    if (ctx->trigger_count) {
	printf(", Triggers: ");
	PrintNumber(ctx->trigger_count);
    }
    
    printf(", Captured: ");
    PrintNumber(ctx->event_count);
    printf(" FPS %5.0lf", fps);

    if ((ctx->flags&PCILIB_EVENT_FLAG_RAW_DATA_ONLY) == 0) {
	printf(", Stored: ");
	PrintNumber(good);
	printf(" FPS %5.0lf", good_fps);
    }

    printf("\n");    

    if (verbose > 2) {
	if (ctx->trigger_count) {
	    printf("Trig: ");
	    PrintNumber(ctx->trigger_count);
	    printf(" Issued: ");
	    PrintNumber(ctx->trigger_count - ctx->trigger_failed);
	    printf(" (");
	    PrintPercent(ctx->trigger_count - ctx->trigger_failed, ctx->trigger_count);
	    printf("%%) Failed: ");
	    PrintNumber(ctx->trigger_failed);
	    printf( " (");
	    PrintPercent(ctx->trigger_failed, ctx->trigger_count);
	    printf( "%%); Pending: ");
	    PrintNumber(pending);
	    printf( " (");
	    PrintPercent(pending, ctx->trigger_count);
	    printf( "%%)\n");
	}
	
	if (ctx->flags&PCILIB_EVENT_FLAG_RAW_DATA_ONLY) {
	    printf("Captured: ");
	    PrintNumber(good);
	} else {
	    printf("Good: ");
	    PrintNumber(good);
	    printf(", Dropped: ");
    	    PrintNumber(ctx->storage_count);
	    printf(", Bad: ");
	    PrintNumber(ctx->incomplete_count);
	    printf(", Empty: ");
    	    PrintNumber(ctx->broken_count);
	}
	printf(", Lost: ");
	PrintNumber(ctx->missing_count);
	printf("\n");
    }

    if (verbose > 1) {
	if (ctx->flags&PCILIB_EVENT_FLAG_RAW_DATA_ONLY) {
	    printf("Captured: ");
	    PrintPercent(good, total);
	} else {
	    printf("Good: ");
	    PrintPercent(good, total);
	    printf("%% Dropped: ");
	    PrintPercent(ctx->storage_count, total);
	    printf("%% Bad: ");
	    PrintPercent(ctx->incomplete_count, total);
	    printf("%% Empty: ");
	    PrintPercent(ctx->broken_count, total);
	}

	printf("%% Lost: ");
	PrintPercent(ctx->missing_count, total);
        printf("%%");
	printf("\n");
    }
}

void StorageStats(GRABContext *ctx) {
    int err;
    fastwriter_stats_t st;

    pcilib_timeout_t duration;
    struct timeval cur;

    gettimeofday(&cur, NULL);
    duration = pcilib_timediff(&ctx->start_time, &cur);

    
    err = fastwriter_get_stats(ctx->writer, &st);
    if (err) return;

    printf("Wrote ");
    PrintSize(st.written);
    printf(" of ");
    PrintSize(st.commited);
    printf(" at ");
    PrintSize(1000000.*st.written / duration);
    printf("/s, %6.2lf%% ", 100.*st.buffer_used / st.buffer_size);
    printf(" of ");
    PrintSize(st.buffer_size);
    printf(" buffer (%6.2lf%% max)\n", 100.*st.buffer_max / st.buffer_size);
}

void *Monitor(void *user) {
    struct timeval deadline;
    struct timeval nextinfo;
    
    GRABContext *ctx = (GRABContext*)user;
    int verbose = ctx->verbose;
    pcilib_timeout_t timeout = ctx->timeout;
    
    
    if (timeout == PCILIB_TIMEOUT_INFINITE) timeout = 0;

//    while (!ctx->started);
    
    if (timeout) {
	memcpy(&deadline, (struct timeval*)&ctx->last_frame, sizeof(struct timeval));
	pcilib_add_timeout(&deadline, timeout);
    }
    
    if (verbose > 0) {
	pcilib_calc_deadline(&nextinfo, STATUS_MESSAGE_INTERVAL*1000000);
    }
    
    while (ctx->run_flag) {
	if (StopFlag) {
	    pcilib_stop(ctx->handle, PCILIB_EVENT_FLAG_STOP_ONLY);
	    break;
	}
	
	if (timeout) {
	    if (pcilib_calc_time_to_deadline(&deadline) == 0) {
		memcpy(&deadline, (struct timeval*)&ctx->last_frame, sizeof(struct timeval));
		pcilib_add_timeout(&deadline, timeout);
		
		if (pcilib_calc_time_to_deadline(&deadline) == 0) {
		    pcilib_stop(ctx->handle, PCILIB_EVENT_FLAG_STOP_ONLY);
		    break;
		}
	    }
	}
	
	if (verbose > 0) {
	    if (pcilib_calc_time_to_deadline(&nextinfo) == 0) {
		GrabStats(ctx, NULL);
		StorageStats(ctx);
		pcilib_calc_deadline(&nextinfo, STATUS_MESSAGE_INTERVAL*1000000);
	    }
	}
	
	usleep(100000);
    }
    
    pcilib_calc_deadline(&nextinfo, STATUS_MESSAGE_INTERVAL*1000000);
    while (ctx->writing_flag) {
        if (pcilib_calc_time_to_deadline(&nextinfo) == 0) {
	    if (verbose >= 0) StorageStats(ctx);
	    pcilib_calc_deadline(&nextinfo, STATUS_MESSAGE_INTERVAL*1000000);
	}
    
	usleep(100000);
    }
    
    return NULL;
}

int TriggerAndGrab(pcilib_t *handle, GRAB_MODE grab_mode, const char *evname, const char *data_type, size_t num, size_t run_time, size_t trigger_time, pcilib_timeout_t timeout, PARTITION partition, FORMAT format, size_t buffer_size, size_t threads, int verbose, const char *output) {
    int err;
    GRABContext ctx;
//    void *data = NULL;
//    size_t size, written;

    pcilib_event_t event;
    pcilib_event_t listen_events;
    pcilib_event_data_type_t data;

    pthread_t monitor_thread;
    pthread_t trigger_thread;
    pthread_attr_t attr;
    struct sched_param sched;

    struct timeval end_time;
    pcilib_event_flags_t flags;

    if (evname) {
	event = pcilib_find_event(handle, evname);
	if (event == PCILIB_EVENT_INVALID) 
	    Error("Can't find event (%s)", evname);

	listen_events = event;
    } else {
	listen_events = PCILIB_EVENTS_ALL;
	event = PCILIB_EVENT0;
    }
    
    if (data_type) {
	data = pcilib_find_event_data_type(handle, event, data_type);
	if (data == PCILIB_EVENT_DATA_TYPE_INVALID)
	    Error("Can't find data type (%s)", data_type);
    } else {
	data = PCILIB_EVENT_DATA;
    }
    
    memset(&ctx, 0, sizeof(GRABContext));
    
    ctx.handle = handle;
    ctx.event = event;
    ctx.data = data;
    ctx.run_time = run_time;
    ctx.timeout = timeout;
    ctx.format = format;

    if (grab_mode&GRAB_MODE_GRAB) ctx.verbose = verbose;
    else ctx.verbose = 0;
    
    if (grab_mode&GRAB_MODE_GRAB) {
	ctx.writer =  fastwriter_init(output, 0);
	if (!ctx.writer)
	    Error("Can't initialize fastwritter library");

	fastwriter_set_buffer_size(ctx.writer, buffer_size);
	
	err = fastwriter_open(ctx.writer, output, 0);
	if (err)
	    Error("Error opening file (%s), Error: %i\n", output, err);

	ctx.writing_flag = 1;
    }

    ctx.run_flag = 1;

    flags = PCILIB_EVENT_FLAGS_DEFAULT;
    
    if (data == PCILIB_EVENT_RAW_DATA) {
	if (format == FORMAT_RAW) {
	    flags |= PCILIB_EVENT_FLAG_RAW_DATA_ONLY;
	}
    } else {
	flags |= PCILIB_EVENT_FLAG_PREPROCESS;
    }
    
    ctx.flags = flags;

//    printf("Limits: %lu %lu %lu\n", num, run_time, timeout);
    pcilib_configure_autostop(handle, num, run_time);
    
    if (flags&PCILIB_EVENT_FLAG_RAW_DATA_ONLY) {
	pcilib_configure_rawdata_callback(handle, &raw_data, &ctx);
    }
    
    if (flags&PCILIB_EVENT_FLAG_PREPROCESS) {
	pcilib_configure_preprocessing_threads(handle, threads);
    }
    
    if (grab_mode&GRAB_MODE_TRIGGER) {
	if (trigger_time) {
	    if ((timeout)&&(trigger_time * 2 > timeout)) {
		timeout = 2 * trigger_time;
		ctx.timeout = timeout;
	    }
	} else {
		// Otherwise, we will trigger next event after previous one is read
	    if (((grab_mode&GRAB_MODE_GRAB) == 0)||(flags&PCILIB_EVENT_FLAG_RAW_DATA_ONLY)) trigger_time = PCILIB_TRIGGER_TIMEOUT;
	}
	
	ctx.max_triggers = num;
	ctx.trigger_count = 0;
	ctx.trigger_time = trigger_time;

	    // We don't really care if RT priority is imposible
	pthread_attr_init(&attr);
	if (!pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
	    sched.sched_priority = sched_get_priority_min(SCHED_FIFO);
	    pthread_attr_setschedparam(&attr, &sched);
	}
    
	    // Start triggering thread and wait until it is schedulled
	if (pthread_create(&trigger_thread, &attr, Trigger, (void*)&ctx))
	    Error("Error spawning trigger thread");

	while (!ctx.trigger_thread_started) usleep(10);
    }

    gettimeofday(&ctx.start_time, NULL);

    if (grab_mode&GRAB_MODE_GRAB) {
	err = pcilib_start(handle, listen_events, flags);
	if (err) Error("Failed to start event engine, error %i", err);
    }
    
    ctx.started = 1;
    
    if (run_time) {
	ctx.stop_time.tv_usec = ctx.start_time.tv_usec + run_time%1000000;
	if (ctx.stop_time.tv_usec > 999999) {
	    ctx.stop_time.tv_usec -= 1000000;
	    __sync_synchronize();
	    ctx.stop_time.tv_sec = ctx.start_time.tv_sec + 1 + run_time / 1000000;
	} else {
	    __sync_synchronize();
	    ctx.stop_time.tv_sec = ctx.start_time.tv_sec + run_time / 1000000;
	}
    }
    
    memcpy(&ctx.last_frame, &ctx.start_time, sizeof(struct timeval));
    if (pthread_create(&monitor_thread, NULL, Monitor, (void*)&ctx))
	Error("Error spawning monitoring thread");

    if (grab_mode&GRAB_MODE_GRAB) {
	err = pcilib_stream(handle, &GrabCallback, &ctx);
	if (err) Error("Error streaming events, error %i", err);
    }
    
    ctx.run_flag = 0;

    if (grab_mode&GRAB_MODE_TRIGGER) {
	while (ctx.trigger_thread_started) usleep(10);
    }
    
    if (grab_mode&GRAB_MODE_GRAB) {
        pcilib_stop(handle, PCILIB_EVENT_FLAGS_DEFAULT);
    }

    gettimeofday(&end_time, NULL);

    if (grab_mode&GRAB_MODE_TRIGGER) {
	pthread_join(trigger_thread, NULL);
    }
    

    if (grab_mode&GRAB_MODE_GRAB) {
	if (verbose >= 0)
	    printf("Grabbing is finished, flushing results....\n");
    
	err = fastwriter_close(ctx.writer);
	if (err) Error("Storage problems, error %i", err);
    }

    ctx.writing_flag = 0;

    pthread_join(monitor_thread, NULL);

    if ((grab_mode&GRAB_MODE_GRAB)&&(verbose>=0)) {
	GrabStats(&ctx, &end_time);
	StorageStats(&ctx);
    }

    fastwriter_destroy(ctx.writer);

    return 0;
}

int StartStopDMA(pcilib_t *handle,  pcilib_model_description_t *model_info, pcilib_dma_engine_addr_t dma, pcilib_dma_direction_t dma_direction, int start) {
    int err;
    pcilib_dma_engine_t dmaid;
    
    if (dma == PCILIB_DMA_ENGINE_ADDR_INVALID) {
        const pcilib_dma_info_t *dma_info = pcilib_get_dma_info(handle);

        if (start) Error("DMA engine should be specified");

	for (dmaid = 0; dma_info->engines[dmaid]; dmaid++) {
	    err = pcilib_start_dma(handle, dmaid, 0);
	    if (err) Error("Error starting DMA Engine (%s %i)", ((dma_info->engines[dmaid]->direction == PCILIB_DMA_FROM_DEVICE)?"C2S":"S2C"), dma_info->engines[dmaid]->addr);
	    err = pcilib_stop_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
	    if (err) Error("Error stopping DMA Engine (%s %i)", ((dma_info->engines[dmaid]->direction == PCILIB_DMA_FROM_DEVICE)?"C2S":"S2C"), dma_info->engines[dmaid]->addr);
	}
	
	return 0;
    }
    
    if (dma_direction&PCILIB_DMA_FROM_DEVICE) {
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_FROM_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (C2S %lu) is specified", dma);
	
	if (start) {
	    err = pcilib_start_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error starting DMA engine (C2S %lu)", dma);
	} else {
	    err = pcilib_start_dma(handle, dmaid, 0);
    	    if (err) Error("Error starting DMA engine (C2S %lu)", dma);
	    err = pcilib_stop_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error stopping DMA engine (C2S %lu)", dma);
	}
    }
    
    if (dma_direction&PCILIB_DMA_TO_DEVICE) {
	dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_TO_DEVICE, dma);
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("Invalid DMA engine (S2C %lu) is specified", dma);
	
	if (start) {
	    err = pcilib_start_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error starting DMA engine (S2C %lu)", dma);
	} else {
	    err = pcilib_start_dma(handle, dmaid, 0);
    	    if (err) Error("Error starting DMA engine (S2C %lu)", dma);
	    err = pcilib_stop_dma(handle, dmaid, PCILIB_DMA_FLAG_PERSISTENT);
    	    if (err) Error("Error stopping DMA engine (S2C %lu)", dma);
	}
    }

    return 0;
}


typedef struct {
    pcilib_kmem_use_t use;
    
    int referenced;
    int hw_lock;
    int reusable;
    int persistent;
    int open;
    
    size_t count;
    size_t size;
} kmem_use_info_t;

#define MAX_USES 64

pcilib_kmem_use_t ParseUse(const char *use) {
    unsigned long utmp;

    if (use) {
        if ((!isxnumber(use))||(sscanf(use, "%lx", &utmp) != 1)) Error("Invalid use (%s) is specified", use);

	if (strlen(use) < 5)
	    return PCILIB_KMEM_USE(PCILIB_KMEM_USE_USER,utmp);
	else
	    return utmp;
    }

    Error("Kernel memory use is not specified");
    return 0;
}

size_t FindUse(size_t *n_uses, kmem_use_info_t *uses, pcilib_kmem_use_t use) {
    size_t i, n = *n_uses;
    
    if (uses[n - 1].use == use) return n - 1;

    for (i = 1; i < (n - 1); i++) {
	if (uses[i].use == use) return i;
    }
    
    if (n == MAX_USES) return 0;

    uses[n].use = use;
    return (*n_uses)++;
}


kmem_use_info_t *GetUse(size_t n_uses, kmem_use_info_t *uses, pcilib_kmem_use_t use) {
    size_t i;
    for (i = 0; i < n_uses; i++) {
	if (uses[i].use == use) {
	    if (uses[i].count) return uses + i;
	    else return NULL;
	}
    }
    return NULL;
}


int ParseKMEM(pcilib_t *handle, const char *device, size_t *uses_number, kmem_use_info_t *uses) {
    DIR *dir;
    struct dirent *entry;
    const char *pos;
    char sysdir[256];
    char fname[256];
    char info[256];

    size_t useid, n_uses = 1;	// Use 0 is for others

    memset(uses, 0, sizeof(uses));
    
    pos = strrchr(device, '/');
    if (pos) ++pos;
    else pos = device;
    
    snprintf(sysdir, 255, "/sys/class/fpga/%s", pos);

    dir = opendir(sysdir);
    if (!dir) Error("Can't open directory (%s)", sysdir);
    
    while ((entry = readdir(dir)) != NULL) {
	FILE *f;
	unsigned long use = 0;
	unsigned long size = 0;
	unsigned long refs = 0;
	unsigned long mode = 0;
	unsigned long hwref = 0;
	
	if (strncmp(entry->d_name, "kbuf", 4)) continue;
	if (!isnumber(entry->d_name+4)) continue;
	
	snprintf(fname, 255, "%s/%s", sysdir, entry->d_name);
	f = fopen(fname, "r");
	if (!f) Error("Can't access file (%s)", fname);

	while(!feof(f)) {
	    fgets(info, 256, f);
	    if (!strncmp(info, "use:", 4)) use = strtoul(info+4, NULL, 16);
	    if (!strncmp(info, "size:", 5)) size = strtoul(info+5, NULL, 10);
	    if (!strncmp(info, "refs:", 5)) refs = strtoul(info+5, NULL, 10);
	    if (!strncmp(info, "mode:", 5)) mode = strtoul(info+5, NULL, 16);
	    if (!strncmp(info, "hw ref:", 7)) hwref = strtoul(info+7, NULL, 10);
	}
	fclose(f);

	useid = FindUse(&n_uses, uses, use);
	uses[useid].count++;
	uses[useid].size += size;
	if (refs) uses[useid].referenced = 1;
	if (hwref) uses[useid].hw_lock = 1;
	if (mode&KMEM_MODE_REUSABLE) uses[useid].reusable = 1;
	if (mode&KMEM_MODE_PERSISTENT) uses[useid].persistent = 1;
	if (mode&KMEM_MODE_COUNT) uses[useid].open = 1;
    }
    closedir(dir);

    *uses_number = n_uses;

    return 0;
}

int ListKMEM(pcilib_t *handle, const char *device) {
    int err;
    char stmp[256];

    size_t i, useid, n_uses;	
    kmem_use_info_t uses[MAX_USES];

    err = ParseKMEM(handle, device, &n_uses, uses);
    if (err) Error("Failed to parse kernel memory information provided through sysfs");

    if ((n_uses == 1)&&(uses[0].count == 0)) {
	printf("No kernel memory is allocated\n");
	return 0;
    }
    
    printf("Use      Type                  Count      Total Size       REF       Mode \n");
    printf("--------------------------------------------------------------------------------\n");
    for (useid = 0; useid < n_uses; useid++) {
	if (useid + 1 == n_uses) {
	    if (!uses[0].count) continue;
	    i = 0;
	} else i = useid + 1;
	
	printf("%08x  ", uses[i].use);
	if (!i) printf("All Others         ");
	else if ((uses[i].use >> 16) == PCILIB_KMEM_USE_DMA_RING) printf("DMA%u %s Ring      ", uses[i].use&0x7F, ((uses[i].use&0x80)?"S2C":"C2S"));
	else if ((uses[i].use >> 16) == PCILIB_KMEM_USE_DMA_PAGES) printf("DMA%u %s Pages     ", uses[i].use&0x7F, ((uses[i].use&0x80)?"S2C":"C2S"));
	else if ((uses[i].use >> 16) == PCILIB_KMEM_USE_USER)	printf("User %04x         ", uses[i].use&0xFFFF);
	else printf ("                   ");
	printf("  ");
	printf("% 6lu", uses[i].count);
	printf("     ");
	printf("% 10s", GetPrintSize(stmp, uses[i].size));
	printf("      ");
	if (uses[i].referenced&&uses[i].hw_lock) printf("HW+SW");
	else if (uses[i].referenced) printf("   SW");
	else if (uses[i].hw_lock) printf("HW   ");
	else printf("  -  ");
	printf("      ");
	if (uses[i].persistent) printf("Persistent");
	else if (uses[i].open) printf("Open      ");
	else if (uses[i].reusable) printf("Reusable  ");
	else printf("Closed    ");
	printf("\n");
    }
    printf("--------------------------------------------------------------------------------\n");
    printf("REF - Software/Hardware Reference, MODE - Reusable/Persistent/Open\n");


    return 0;
}

int DetailKMEM(pcilib_t *handle, const char *device, const char *use, size_t block) {
    int err;
    size_t i, n;
    pcilib_kmem_handle_t *kbuf;
    pcilib_kmem_use_t useid = ParseUse(use); 

    size_t n_uses;	
    kmem_use_info_t uses[MAX_USES];
    kmem_use_info_t *use_info;
    
    if (block == (size_t)-1) {
        err = ParseKMEM(handle, device, &n_uses, uses);
	if (err) Error("Failed to parse kernel memory information provided through sysfs");
	use_info = GetUse(n_uses, uses, useid);
	if (!use_info) Error("No kernel buffers is allocated for the specified use (%lx)", useid);

	i = 0;    
	n = use_info->count;
    } else {
	i = block;
	n = block + 1;
    }
    
    kbuf = pcilib_alloc_kernel_memory(handle, 0, n, 0, 0, useid, PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_TRY);
    if (!kbuf) {
	Error("Allocation of kernel buffer (use %lx, count %lu) is failed\n", useid, n);
	return 0;
    }

    printf("Buffer         Address          Hardware Address          Bus Address\n");
    printf("--------------------------------------------------------------------------------\n");
    for (; i < n; i++) {
	void *data = pcilib_kmem_get_block_ua(handle, kbuf, i);
	uintptr_t pa = pcilib_kmem_get_block_pa(handle, kbuf, i);
	uintptr_t ba = pcilib_kmem_get_block_ba(handle, kbuf, i);
	printf("%6lu     %16p     %16lx       %16lx\n", i, data, pa, ba);
    }
    printf("\n");

    pcilib_free_kernel_memory(handle, kbuf, KMEM_FLAG_REUSE);

    return 0;
}


int ReadKMEM(pcilib_t *handle, const char *device, pcilib_kmem_use_t useid, size_t block, size_t max_size, FILE *o) {
    int err;
    void *data;
    size_t size;
    pcilib_kmem_handle_t *kbuf;

    if (block == (size_t)-1) block = 0;

    kbuf = pcilib_alloc_kernel_memory(handle, 0, block + 1, 0, 0, useid, PCILIB_KMEM_FLAG_REUSE|PCILIB_KMEM_FLAG_TRY);
    if (!kbuf) {
	Error("The specified kernel buffer is not allocated\n");
	return 0;
    }

    err = pcilib_kmem_sync_block(handle, kbuf, PCILIB_KMEM_SYNC_FROMDEVICE, block);
    if (err) {
	pcilib_free_kernel_memory(handle, kbuf, KMEM_FLAG_REUSE);
	Error("The synchronization of kernel buffer has failed\n");
	return 0;
    }

    data = pcilib_kmem_get_block_ua(handle, kbuf, block);
    if (data) {
	size = pcilib_kmem_get_block_size(handle, kbuf, block);
	if ((max_size)&&(size > max_size)) size = max_size;
	
	fwrite(data, 1, size, o?o:stdout);
    } else {
	pcilib_free_kernel_memory(handle, kbuf, KMEM_FLAG_REUSE);
	Error("The specified block is not existing\n");
	return 0;
    }

    pcilib_free_kernel_memory(handle, kbuf, KMEM_FLAG_REUSE);

    return 0;
}

int AllocKMEM(pcilib_t *handle, const char *device, const char *use, const char *type, size_t size, size_t block_size, size_t alignment) {
    pcilib_kmem_type_t ktype = PCILIB_KMEM_TYPE_PAGE;
    pcilib_kmem_flags_t flags = KMEM_FLAG_REUSE;
    pcilib_kmem_handle_t *kbuf;
    pcilib_kmem_use_t useid = ParseUse(use);

    long page_size = sysconf(_SC_PAGESIZE);

    if (type) {
	if (!strcmp(type, "consistent")) ktype = PCILIB_KMEM_TYPE_CONSISTENT;
	else if (!strcmp(type, "c2s")) ktype = PCILIB_KMEM_TYPE_DMA_C2S_PAGE;
	else if (!strcmp(type, "s2c")) ktype = PCILIB_KMEM_TYPE_DMA_S2C_PAGE;
	else Error("Invalid memory type (%s) is specified", type);
    } 
    
    if ((block_size)&&(ktype != PCILIB_KMEM_TYPE_CONSISTENT))
	Error("Selected memory type does not allow custom size");
    
    kbuf = pcilib_alloc_kernel_memory(handle, ktype, size, (block_size?block_size:page_size), (alignment?alignment:page_size), useid, flags|KMEM_FLAG_PERSISTENT);
    if (!kbuf) Error("Allocation of kernel memory has failed");

    pcilib_free_kernel_memory(handle, kbuf, flags);

    return 0;
}

int FreeKMEM(pcilib_t *handle, const char *device, const char *use, int force) {
    int err;
    int i;

    pcilib_kmem_use_t useid;

    pcilib_kmem_flags_t flags = PCILIB_KMEM_FLAG_HARDWARE|PCILIB_KMEM_FLAG_PERSISTENT|PCILIB_KMEM_FLAG_EXCLUSIVE; 
    if (force) flags |= PCILIB_KMEM_FLAG_FORCE; // this will ignore mmap locks as well.

    if (!strcasecmp(use, "dma")) {
	for (i = 0; i < PCILIB_MAX_DMA_ENGINES; i++) {
	    err = pcilib_clean_kernel_memory(handle, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_RING, i), flags);
	    if (err) Error("Error cleaning DMA%i C2S Ring buffer", i);
	    err = pcilib_clean_kernel_memory(handle, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_RING, 0x80|i), flags);
	    if (err) Error("Error cleaning DMA%i S2C Ring buffer", i);
	    err = pcilib_clean_kernel_memory(handle, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_PAGES, i), flags);
	    if (err) Error("Error cleaning DMA%i C2S Page buffers", i);
	    err = pcilib_clean_kernel_memory(handle, PCILIB_KMEM_USE(PCILIB_KMEM_USE_DMA_PAGES, 0x80|i), flags);
	    if (err) Error("Error cleaning DMA%i S2C Page buffers", i);
	}
	
	return 0;
    }

    useid = ParseUse(use);
    err = pcilib_clean_kernel_memory(handle, useid, flags);
    if (err) Error("Error cleaning kernel buffers for use (0x%lx)", useid);

    return 0;
}

int ListDMA(pcilib_t *handle, const char *device, pcilib_model_description_t *model_info) {
    int err;
    
    DIR *dir;
    struct dirent *entry;
    const char *pos;
    char sysdir[256];
    char fname[256];
    char info[256];
    char stmp[256];

    pcilib_dma_engine_t dmaid;
    pcilib_dma_engine_status_t status;
    
    pos = strrchr(device, '/');
    if (pos) ++pos;
    else pos = device;
    
    snprintf(sysdir, 255, "/sys/class/fpga/%s", pos);

    dir = opendir(sysdir);
    if (!dir) Error("Can't open directory (%s)", sysdir);
    
    printf("DMA Engine     Status      Total Size         Buffer Ring (1st used - 1st free)\n");
    printf("--------------------------------------------------------------------------------\n");
    while ((entry = readdir(dir)) != NULL) {
	FILE *f;
	unsigned long use = 0;
//	unsigned long size = 0;
//	unsigned long refs = 0;
	unsigned long mode = 0;
//	unsigned long hwref = 0;
	
	if (strncmp(entry->d_name, "kbuf", 4)) continue;
	if (!isnumber(entry->d_name+4)) continue;
	
	snprintf(fname, 255, "%s/%s", sysdir, entry->d_name);
	f = fopen(fname, "r");
	if (!f) Error("Can't access file (%s)", fname);

	while(!feof(f)) {
	    fgets(info, 256, f);
	    if (!strncmp(info, "use:", 4)) use = strtoul(info+4, NULL, 16);
//	    if (!strncmp(info, "size:", 5)) size = strtoul(info+5, NULL, 10);
//	    if (!strncmp(info, "refs:", 5)) refs = strtoul(info+5, NULL, 10);
	    if (!strncmp(info, "mode:", 5)) mode = strtoul(info+5, NULL, 16);
//	    if (!strncmp(info, "hw ref:", 7)) hwref = strtoul(info+7, NULL, 10);
	}
	fclose(f);
	
	if ((mode&(KMEM_MODE_REUSABLE|KMEM_MODE_PERSISTENT|KMEM_MODE_COUNT)) == 0) continue;	// closed
	if ((use >> 16) != PCILIB_KMEM_USE_DMA_RING) continue;

	if (use&0x80) {
	    dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_TO_DEVICE, use&0x7F);
	} else {
	    dmaid = pcilib_find_dma_by_addr(handle, PCILIB_DMA_FROM_DEVICE, use&0x7F);
	}
	
	if (dmaid == PCILIB_DMA_ENGINE_INVALID) continue;
	
	
	printf("DMA%lu %s         ", use&0x7F, (use&0x80)?"S2C":"C2S");
        err = pcilib_start_dma(handle, dmaid, 0);
        if (err) {
    	    printf("-- Wrong state, start is failed\n");
	    continue;
	}
	
	err = pcilib_get_dma_status(handle, dmaid, &status, 0, NULL);
	if (err) {
	    printf("-- Wrong state, failed to obtain status\n");
	    pcilib_stop_dma(handle, dmaid, 0);
	    continue;
	}

	pcilib_stop_dma(handle, dmaid, 0);
	
	if (status.started) printf("S");
	else printf(" ");
	
	if (status.ring_head == status.ring_tail) printf(" ");
	else printf("D");

	printf("        ");
	printf("% 10s", GetPrintSize(stmp, status.ring_size * status.buffer_size));
	
	printf("         ");
	printf("%zu - %zu (of %zu)", status.ring_tail, status.ring_head, status.ring_size);

	printf("\n");
	
    }
    closedir(dir);

    printf("--------------------------------------------------------------------------------\n");
    printf("S - Started, D - Data in buffers\n");

    return 0;
}

int ListBuffers(pcilib_t *handle, const char *device, pcilib_model_description_t *model_info, pcilib_dma_engine_addr_t dma, pcilib_dma_direction_t dma_direction) {
    int err;
    size_t i;
    pcilib_dma_engine_t dmaid;
    pcilib_dma_engine_status_t status;
    pcilib_dma_buffer_status_t *buffer;
    char stmp[256];

    dmaid = pcilib_find_dma_by_addr(handle, dma_direction, dma);
    if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("The specified DMA engine is not found");

    err = pcilib_start_dma(handle, dmaid, 0);
    if (err) Error("Error starting the specified DMA engine");

    err = pcilib_get_dma_status(handle, dmaid, &status, 0, NULL);
    if (err) Error("Failed to obtain status of the specified DMA engine");

    buffer = (pcilib_dma_buffer_status_t*)malloc(status.ring_size*sizeof(pcilib_dma_buffer_status_t));
    if (!buffer) Error("Failed to allocate memory for status buffer");

    err = pcilib_get_dma_status(handle, dmaid, &status, status.ring_size, buffer);
    if (err) Error("Failed to obtain extended status of the specified DMA engine");


    printf("Buffer      Status      Total Size         \n");
    printf("--------------------------------------------------------------------------------\n");

    for (i = 0; i < status.ring_size; i++) {
	printf("%8zu    ", i);
        printf("%c%c %c%c ", buffer[i].used?'U':' ',  buffer[i].error?'E':' ', buffer[i].first?'F':' ', buffer[i].last?'L':' ');
	printf("% 10s", GetPrintSize(stmp, buffer[i].size));
	printf("\n");
    }

    printf("--------------------------------------------------------------------------------\n");
    printf("U - Used, E - Error, F - First block, L - Last Block\n");

    free(buffer);

    pcilib_stop_dma(handle, dmaid, 0);

    return 0;
}

int ReadBuffer(pcilib_t *handle, const char *device, pcilib_model_description_t *model_info, pcilib_dma_engine_addr_t dma, pcilib_dma_direction_t dma_direction, size_t block, FILE *o) {
    int err;
    pcilib_dma_engine_t dmaid;
    pcilib_dma_engine_status_t status;
    pcilib_dma_buffer_status_t *buffer;
    size_t size;

    dmaid = pcilib_find_dma_by_addr(handle, dma_direction, dma);
    if (dmaid == PCILIB_DMA_ENGINE_INVALID) Error("The specified DMA engine is not found");

    err = pcilib_start_dma(handle, dmaid, 0);
    if (err) Error("Error starting the specified DMA engine");
    
    err = pcilib_get_dma_status(handle, dmaid, &status, 0, NULL);
    if (err) Error("Failed to obtain status of the specified DMA engine");
    
    buffer = (pcilib_dma_buffer_status_t*)malloc(status.ring_size*sizeof(pcilib_dma_buffer_status_t));
    if (!buffer) Error("Failed to allocate memory for status buffer");

    err = pcilib_get_dma_status(handle, dmaid, &status, status.ring_size, buffer);
    if (err) Error("Failed to obtain extended status of the specified DMA engine");

    if (block == (size_t)-1) {
	// get current 
    }

    size = buffer[block].size;

    free(buffer);

    pcilib_stop_dma(handle, dmaid, 0);

    return ReadKMEM(handle, device, ((dma&0x7F)|((dma_direction == PCILIB_DMA_TO_DEVICE)?0x80:0x00))|(PCILIB_KMEM_USE_DMA_PAGES<<16), block, size, o);
}


int EnableIRQ(pcilib_t *handle, pcilib_model_description_t *model_info, pcilib_irq_type_t irq_type) {
    int err;

    err = pcilib_enable_irq(handle, irq_type, 0);
    if (err) {
	if ((err != PCILIB_ERROR_NOTSUPPORTED)&&(err != PCILIB_ERROR_NOTAVAILABLE))
	    Error("Error enabling IRQs");
    }
    
    return err;
}

int DisableIRQ(pcilib_t *handle, pcilib_model_description_t *model_info, pcilib_irq_type_t irq_type) {
    int err;
    
    err = pcilib_disable_irq(handle, 0);
    if (err) {
	if ((err != PCILIB_ERROR_NOTSUPPORTED)&&(err != PCILIB_ERROR_NOTAVAILABLE))
	    Error("Error disabling IRQs");
    }
    
    return err;
}

int AckIRQ(pcilib_t *handle, pcilib_model_description_t *model_info, pcilib_irq_hw_source_t irq_source) {
    pcilib_clear_irq(handle, irq_source);
    return 0;
}

int WaitIRQ(pcilib_t *handle, pcilib_model_description_t *model_info, pcilib_irq_hw_source_t irq_source, pcilib_timeout_t timeout) {
    int err;
    size_t count;

    err = pcilib_wait_irq(handle, irq_source, timeout, &count);
    if (err) {
	if (err == PCILIB_ERROR_TIMEOUT) Error("Timeout waiting for IRQ");
	else Error("Error waiting for IRQ");
    }

    return 0;
}


int main(int argc, char **argv) {
    int i;
    long itmp;
    size_t ztmp;
    unsigned char c;

    const char *stmp;
    const char *num_offset;

    int details = 0;
    int verbose = 0;
    int quiete = 0;
    int force = 0;
    int verify = 0;
    
    pcilib_model_t model = PCILIB_MODEL_DETECT;
    pcilib_model_description_t *model_info;
    MODE mode = MODE_INVALID;
    GRAB_MODE grab_mode = 0;
    size_t trigger_time = 0;
    size_t run_time = 0;
    size_t buffer = 0;
    size_t threads = 1;
    FORMAT format = FORMAT_DEFAULT;
    PARTITION partition = PARTITION_UNKNOWN;
    FLAGS flags = 0;
    const char *atype = NULL;
    const char *type = NULL;
    ACCESS_MODE amode = ACCESS_BAR;
    const char *fpga_device = DEFAULT_FPGA_DEVICE;
    pcilib_bar_t bar = PCILIB_BAR_DETECT;
    const char *addr = NULL;
    const char *reg = NULL;
    const char *bank = NULL;
    char **data = NULL;
    const char *event = NULL;
    const char *data_type = NULL;
    const char *dma_channel = NULL;
    const char *use = NULL;
    size_t block = (size_t)-1;
    pcilib_irq_type_t irq_type = PCILIB_IRQ_TYPE_ALL;
    pcilib_irq_hw_source_t irq_source =  PCILIB_IRQ_SOURCE_DEFAULT;
    pcilib_dma_direction_t dma_direction = PCILIB_DMA_BIDIRECTIONAL;
    pcilib_kmem_use_t useid = 0;
    
    pcilib_dma_engine_addr_t dma = PCILIB_DMA_ENGINE_ADDR_INVALID;
    long addr_shift = 0;
    uintptr_t start = -1;
    size_t block_size = 0;
    size_t size = 1;
    access_t access = 4;
//    int skip = 0;
    int endianess = 0;
    size_t timeout = 0;
    size_t alignment = 0;
    const char *output = NULL;
    FILE *ofile = NULL;
    size_t iterations = BENCHMARK_ITERATIONS;

    pcilib_t *handle;

    int size_set = 0;
    int timeout_set = 0;
//    int run_time_set = 0;

    while ((c = getopt_long(argc, argv, "hqilr::w::g::d:m:t:b:a:s:e:o:", long_options, NULL)) != (unsigned char)-1) {
	extern int optind;
	switch (c) {
	    case OPT_HELP:
		Usage(argc, argv, NULL);
	    break;
	    case OPT_INFO:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_INFO;
	    break;
	    case OPT_LIST:
		if (mode == MODE_LIST) details++;
		else if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_LIST;
	    break;
	    case OPT_RESET:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_RESET;
	    break;
	    case OPT_BENCHMARK:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_BENCHMARK;

		if (optarg) addr = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) addr = argv[optind++];
	    break;
	    case OPT_READ:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_READ;
		if (optarg) addr = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) addr = argv[optind++];
	    break;
	    case OPT_WRITE:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_WRITE;
		if (optarg) addr = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) addr = argv[optind++];
	    break;
	    case OPT_GRAB:
		if ((mode != MODE_INVALID)&&((mode != MODE_GRAB)||(grab_mode&GRAB_MODE_GRAB))) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_GRAB;
		grab_mode |= GRAB_MODE_GRAB;
		
		stmp = NULL;
		if (optarg) stmp = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) stmp = argv[optind++];

		if (stmp) {
		    if ((event)&&(strcasecmp(stmp,event))) Usage(argc, argv, "Redefinition of considered event");
		    event = stmp;
		}
	    break;
	    case OPT_TRIGGER:
		if ((mode != MODE_INVALID)&&((mode != MODE_GRAB)||(grab_mode&GRAB_MODE_TRIGGER))) Usage(argc, argv, "Multiple operations are not supported");

		mode = MODE_GRAB;
		grab_mode |= GRAB_MODE_TRIGGER;
		
		stmp = NULL;
		if (optarg) stmp = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) stmp = argv[optind++];

		if (stmp) {
		    if ((event)&&(strcasecmp(stmp,event))) Usage(argc, argv, "Redefinition of considered event");
		    event = stmp;
		}
	    break;
	    case OPT_LIST_DMA:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_LIST_DMA;
	    break;
	    case OPT_LIST_DMA_BUFFERS:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_LIST_DMA_BUFFERS;
		dma_channel = optarg;
	    break;
	    case OPT_READ_DMA_BUFFER:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_READ_DMA_BUFFER;

		num_offset = strchr(optarg, ':');

		if (num_offset) {
		    if (sscanf(num_offset + 1, "%zu", &block) != 1)
			Usage(argc, argv, "Invalid buffer is specified (%s)", num_offset + 1);

		    *(char*)num_offset = 0;
		} else block = (size_t)-1;
		
		dma_channel = optarg;
	    break;
	    case OPT_START_DMA:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_START_DMA;
		if (optarg) dma_channel = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) dma_channel = argv[optind++];
	    break;
	    case OPT_STOP_DMA:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_STOP_DMA;
		if (optarg) dma_channel = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) dma_channel = argv[optind++];
	    break;
	    case OPT_ENABLE_IRQ:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_ENABLE_IRQ;
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-'))  num_offset = argv[optind++];
		else num_offset = NULL;
		
		if (num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%li", &itmp) != 1))
			Usage(argc, argv, "Invalid IRQ source is specified (%s)", num_offset);

		    irq_type = itmp;
		}
	    break;
	    case OPT_DISABLE_IRQ:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_DISABLE_IRQ;
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-'))  num_offset = argv[optind++];
		else num_offset = NULL;
		
		if (num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%li", &itmp) != 1))
			Usage(argc, argv, "Invalid IRQ source is specified (%s)", num_offset);

		    irq_type = itmp;
		}
	    break;
	    case OPT_ACK_IRQ:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_ACK_IRQ;
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-'))  num_offset = argv[optind++];
		else num_offset = NULL;
		
		if (num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%li", &itmp) != 1))
			Usage(argc, argv, "Invalid IRQ source is specified (%s)", num_offset);

		    irq_source = itmp;
		}
	    break;
	    case OPT_WAIT_IRQ:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		
		mode = MODE_WAIT_IRQ;
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-'))  num_offset = argv[optind++];
		else num_offset = NULL;
		
		if (num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%li", &itmp) != 1))
			Usage(argc, argv, "Invalid IRQ source is specified (%s)", num_offset);

		    irq_source = itmp;
		}
	    break;
	    case OPT_LIST_KMEM:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		mode = MODE_LIST_KMEM;
		
		if (optarg) use = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-'))  use = argv[optind++];
		else use = NULL;
		
		if (use) {
		    num_offset = strchr(use, ':');

		    if (num_offset) {
			if (sscanf(num_offset + 1, "%zu", &block) != 1)
			    Usage(argc, argv, "Invalid block number is specified (%s)", num_offset + 1);

			*(char*)num_offset = 0;
		    }
		}
	    break;
	    case OPT_READ_KMEM:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		mode = MODE_READ_KMEM;

		num_offset = strchr(optarg, ':');

		if (num_offset) {
		    if (sscanf(num_offset + 1, "%zu", &block) != 1)
			Usage(argc, argv, "Invalid block number is specified (%s)", num_offset + 1);

		    *(char*)num_offset = 0;
		}
		
		use = optarg;
		useid = ParseUse(use);
    	    break;
	    case OPT_ALLOC_KMEM:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		mode = MODE_ALLOC_KMEM;

		if (optarg) use = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) use = argv[optind++];
	    break;
	    case OPT_FREE_KMEM:
		if (mode != MODE_INVALID) Usage(argc, argv, "Multiple operations are not supported");
		mode = MODE_FREE_KMEM;

		if (optarg) use = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) use = argv[optind++];
	    break;
	    case OPT_DEVICE:
		fpga_device = optarg;
	    break;
	    case OPT_MODEL:
		if (!strcasecmp(optarg, "pci")) model = PCILIB_MODEL_PCI;
		else if (!strcasecmp(optarg, "ipecamera")) model = PCILIB_MODEL_IPECAMERA;
		else if (!strcasecmp(optarg, "kapture")) model = PCILIB_MODEL_KAPTURE;
		else Usage(argc, argv, "Invalid memory model (%s) is specified", optarg);
	    break;
	    case OPT_BAR:
		bank = optarg;
//		if ((sscanf(optarg,"%li", &itmp) != 1)||(itmp < 0)||(itmp >= PCILIB_MAX_BANKS)) Usage(argc, argv, "Invalid data bank (%s) is specified", optarg);
//		else bar = itmp;
	    break;
	    case OPT_ALIGNMENT:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &alignment) != 1)) {
		    Usage(argc, argv, "Invalid alignment is specified (%s)", optarg);
		}
	    break;
	    case OPT_ACCESS:
		if (!strncasecmp(optarg, "fifo", 4)) {
		    atype = "fifo";
		    num_offset = optarg + 4;
		    amode = ACCESS_FIFO;
		} else if (!strncasecmp(optarg, "dma", 3)) {
		    atype = "dma";
		    num_offset = optarg + 3;
		    amode = ACCESS_DMA;
		} else if (!strncasecmp(optarg, "bar", 3)) {
		    atype = "plain";
		    num_offset = optarg + 3;
		    amode = ACCESS_BAR;
		} else if (!strncasecmp(optarg, "config", 6)) {
		    atype = "config";
		    num_offset = optarg + 6;
		    amode = ACCESS_CONFIG;
		} else if (!strncasecmp(optarg, "plain", 5)) {
		    atype = "plain";
		    num_offset = optarg + 5;
		    amode = ACCESS_BAR;
		} else {
		    num_offset = optarg;
		}

		if (*num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%li", &itmp) != 1))
			Usage(argc, argv, "Invalid access type (%s) is specified", optarg);

	    	    switch (itmp) {
			case 8: access = 1; break;
			case 16: access = 2; break;
			case 32: access = 4; break;
			case 64: access = 8; break;
			default: Usage(argc, argv, "Invalid data width (%s) is specified", num_offset);
		    }	
		}
	    break;
	    case OPT_SIZE:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &size) != 1)) {
		    if (strcasecmp(optarg, "unlimited"))
			Usage(argc, argv, "Invalid size is specified (%s)", optarg);
		    else
			size = 0;//(size_t)-1;
		}
			
		size_set = 1;
	    break;
	    case OPT_BLOCK_SIZE:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &block_size) != 1)) {
		    Usage(argc, argv, "Invalid size is specified (%s)", optarg);
		}
	    break;
	    case OPT_ENDIANESS:
		if ((*optarg == 'b')||(*optarg == 'B')) {
		    if (ntohs(1) == 1) endianess = 0;
		    else endianess = 1;
		} else if ((*optarg == 'l')||(*optarg == 'L')) {
		    if (ntohs(1) == 1) endianess = 1;
		    else endianess = 0;
		} else Usage(argc, argv, "Invalid endianess is specified (%s)", optarg);
		
	    break;
	    case OPT_TIMEOUT:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &timeout) != 1)) {
		    if (strcasecmp(optarg, "unlimited"))
			Usage(argc, argv, "Invalid timeout is specified (%s)", optarg);
		    else
			timeout = PCILIB_TIMEOUT_INFINITE;
		}
		timeout_set = 1;
	    break;
	    case OPT_OUTPUT:
		output = optarg;
	    break;
	    case OPT_ITERATIONS:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &iterations) != 1))
		    Usage(argc, argv, "Invalid number of iterations is specified (%s)", optarg);
	    break;
	    case OPT_EVENT:
		event = optarg;
	    break;
	    case OPT_TYPE:
		type = optarg;
	    break;
	    case OPT_DATA_TYPE:
		data_type = optarg;
	    break;
	    case OPT_RUN_TIME:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &run_time) != 1)) {
		    if (strcasecmp(optarg, "unlimited"))
			Usage(argc, argv, "Invalid run-time is specified (%s)", optarg);
		    else
			run_time = 0;
		}
//		run_time_set = 1;
	    break;
	    case OPT_TRIGGER_TIME:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &trigger_time) != 1))
		    Usage(argc, argv, "Invalid trigger-time is specified (%s)", optarg);
	    break;	    
	    case OPT_TRIGGER_RATE:
		if ((!isnumber(optarg))||(sscanf(optarg, "%zu", &ztmp) != 1))
		    Usage(argc, argv, "Invalid trigger-rate is specified (%s)", optarg);
		    
		    trigger_time = (1000000 / ztmp) + ((1000000 % ztmp)?1:0);
	    break;
	    case OPT_BUFFER:
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) num_offset = argv[optind++];
		else num_offset = NULL;
		
		if (num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%zu", &buffer) != 1))
			Usage(argc, argv, "Invalid buffer size is specified (%s)", num_offset);
		    buffer *= 1024 * 1024;
		} else {
		    buffer = get_free_memory();
		    if (buffer < 256) Error("Not enough free memory (%lz MB) for buffering", buffer / 1024 / 1024);
		    
		    buffer -= 128 + buffer/16;
		}
	    break;	   
	    case OPT_THREADS:
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-')) num_offset = argv[optind++];
		else num_offset = NULL;
		
		if (num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%zu", &threads) != 1))
			Usage(argc, argv, "Invalid threads number is specified (%s)", num_offset);
		} else {
		    threads = 0;
		}
	    break;	   
	    case OPT_FORMAT:
		if (!strcasecmp(optarg, "raw")) format =  FORMAT_RAW;
		else if (!strcasecmp(optarg, "add_header")) format =  FORMAT_HEADER;
//		else if (!strcasecmp(optarg, "ringfs")) format =  FORMAT_RINGFS;
		else if (strcasecmp(optarg, "default")) Error("Invalid format (%s) is specified", optarg);
	    break; 
	    case OPT_QUIETE:
		quiete = 1;
		verbose = -1;
	    break;
	    case OPT_VERBOSE:
		if (optarg) num_offset = optarg;
		else if ((optind < argc)&&(argv[optind][0] != '-'))  num_offset = argv[optind++];
		else num_offset = NULL;
		
		if (num_offset) {
		    if ((!isnumber(num_offset))||(sscanf(num_offset, "%i", &verbose) != 1))
			Usage(argc, argv, "Invalid verbosity level is specified (%s)", num_offset);
		} else {
		    verbose = 1;
		}
	    break;
	    case OPT_FORCE:
		force = 1;
	    break;
	    case OPT_VERIFY:
		verify = 1;
	    break;
	    case OPT_MULTIPACKET:
		flags |= FLAG_MULTIPACKET;
	    break;
	    case OPT_WAIT:
		flags |= FLAG_WAIT;
	    break;
	    default:
		Usage(argc, argv, "Unknown option (%s) with argument (%s)", optarg?argv[optind-2]:argv[optind-1], optarg?optarg:"(null)");
	}
    }

    if (mode == MODE_INVALID) {
	if (argc > 1) Usage(argc, argv, "Operation is not specified");
	else Usage(argc, argv, NULL);
    }

    pcilib_set_error_handler(&Error, quiete?Silence:NULL);

    handle = pcilib_open(fpga_device, model);
    if (handle < 0) Error("Failed to open FPGA device: %s", fpga_device);

    model = pcilib_get_model(handle);
    model_info = pcilib_get_model_description(handle);

    switch (mode) {
     case MODE_WRITE:
        if (((argc - optind) == 1)&&(*argv[optind] == '*')) {
    	    int vallen = strlen(argv[optind]);
    	    if (vallen > 1) {
		data = (char**)malloc(size * (vallen + sizeof(char*)));
		if (!data) Error("Error allocating memory for data array");

		for (i = 0; i < size; i++) {
		    data[i] = ((char*)data) + size * sizeof(char*) + i * vallen;
		    strcpy(data[i], argv[optind] + 1);
		}
	    } else {
		data = (char**)malloc(size * (9 + sizeof(char*)));
		if (!data) Error("Error allocating memory for data array");
		
		for (i = 0; i < size; i++) {
		    data[i] = ((char*)data) + size * sizeof(char*) + i * 9;
		    sprintf(data[i], "%x", i);
		}
	    }
        } else if ((argc - optind) == size) data = argv + optind;
        else Usage(argc, argv, "The %i data values is specified, but %i required", argc - optind, size);
     case MODE_READ:
        if (!addr) {
	    if (model == PCILIB_MODEL_PCI) {
		if ((amode != ACCESS_DMA)&&(amode != ACCESS_CONFIG)) 
		    Usage(argc, argv, "The address is not specified");
	    } else ++mode;
	}
     break;
     case MODE_START_DMA:
     case MODE_STOP_DMA:
     case MODE_LIST_DMA_BUFFERS:
     case MODE_READ_DMA_BUFFER:
        if ((dma_channel)&&(*dma_channel)) {
	    itmp = strlen(dma_channel) - 1;
	    if (dma_channel[itmp] == 'r') dma_direction = PCILIB_DMA_FROM_DEVICE;
	    else if (dma_channel[itmp] == 'w') dma_direction = PCILIB_DMA_TO_DEVICE;

	    if (dma_direction != PCILIB_DMA_BIDIRECTIONAL) itmp--;
	    
	    if (strncmp(dma_channel, "dma", 3)) num_offset = dma_channel;
	    else {
		num_offset = dma_channel + 3;
		itmp -= 3;
	    }
	    
	    if (bank) {
		if (strncmp(num_offset, bank, itmp)) Usage(argc, argv, "Conflicting DMA channels are specified in mode parameter (%s) and bank parameter (%s)", dma_channel, bank);
	    }
		 
	    if (!isnumber_n(num_offset, itmp))
		 Usage(argc, argv, "Invalid DMA channel (%s) is specified", dma_channel);

	    dma = atoi(num_offset);
	}
     break;
     default:
        if (argc > optind) Usage(argc, argv, "Invalid non-option parameters are supplied");
    }


    if (addr) {
	if ((!strncmp(addr, "dma", 3))&&((addr[3]==0)||isnumber(addr+3))) {
	    if ((atype)&&(amode != ACCESS_DMA)) Usage(argc, argv, "Conflicting access modes, the DMA read is requested, but access type is (%s)", type);
	    if (bank) {
		if ((addr[3] != 0)&&(strcmp(addr + 3, bank))) Usage(argc, argv, "Conflicting DMA channels are specified in read parameter (%s) and bank parameter (%s)", addr + 3, bank);
	    } else {
		if (addr[3] == 0) Usage(argc, argv, "The DMA channel is not specified");
	    }
	    dma = atoi(addr + 3);
	    amode = ACCESS_DMA;
	    addr = NULL;
	} else if ((!strncmp(addr, "bar", 3))&&((addr[3]==0)||isnumber(addr+3))) {
	    if ((atype)&&(amode != ACCESS_BAR)) Usage(argc, argv, "Conflicting access modes, the plain PCI read is requested, but access type is (%s)", type);
	    if ((addr[3] != 0)&&(strcmp(addr + 3, bank))) Usage(argc, argv, "Conflicting PCI bars are specified in read parameter (%s) and bank parameter (%s)", addr + 3, bank);
	    bar = atoi(addr + 3);
	    amode = ACCESS_BAR;
	    addr = NULL;
	} else if (!strcmp(addr, "config")) {
	    if ((atype)&&(amode != ACCESS_CONFIG)) Usage(argc, argv, "Conflicting access modes, the read of PCI configurataion space is requested, but access type is (%s)", type);
	    amode = ACCESS_CONFIG;
	    addr = NULL;
	} else if ((isxnumber(addr))&&(sscanf(addr, "%lx", &start) == 1)) {
		// check if the address in the register range
	    pcilib_register_range_t *ranges =  model_info->ranges;
	    
	    if (ranges) {
		for (i = 0; ranges[i].start != ranges[i].end; i++) 
		    if ((start >= ranges[i].start)&&(start <= ranges[i].end)) break;
	    		
		    // register access in plain mode
		if (ranges[i].start != ranges[i].end) {
		    pcilib_register_bank_t regbank = pcilib_find_bank_by_addr(handle, ranges[i].bank);
		    if (regbank == PCILIB_REGISTER_BANK_INVALID) Error("Configuration error: register bank specified in the address range is not found");
		    
		    bank = model_info->banks[regbank].name;
		    start += ranges[i].addr_shift;
		    addr_shift = ranges[i].addr_shift;
		    ++mode;
		}
	    }
	} else {
	    if (pcilib_find_register(handle, bank, addr) == PCILIB_REGISTER_INVALID) {
	        Usage(argc, argv, "Invalid address (%s) is specified", addr);
	    } else {
	        reg = addr;
		++mode;
	    }
	} 
    }
	
    if (mode == MODE_GRAB) {
	if (output) {
	    char fsname[128];
	    if (!get_file_fs(output, 127, fsname)) {
		if (!strcmp(fsname, "ext4")) partition = PARTITION_EXT4;
		else if (!strcmp(fsname, "raw")) partition = PARTITION_RAW;
	    }
	} else {
	    output = "/dev/null";
	    partition = PARTITION_NULL;
	}

	if (!timeout_set) {
	    if (run_time) timeout = PCILIB_TIMEOUT_INFINITE;
	    else timeout = PCILIB_EVENT_TIMEOUT;
	}
	
	if (!size_set) {
	    if (run_time) size = 0;
	}
    }
    
    if (mode != MODE_GRAB) {
	if (size == (size_t)-1)
	    Usage(argc, argv, "Unlimited size is not supported in selected operation mode");
    }
    

    if ((bank)&&(amode == ACCESS_DMA)) {
	if ((!isnumber(bank))||(sscanf(bank,"%li", &itmp) != 1)||(itmp < 0)) 
	    Usage(argc, argv, "Invalid DMA channel (%s) is specified", bank);
	else dma = itmp;
    } else if (bank) {
	switch (mode) {
	    case MODE_BENCHMARK:
	    case MODE_READ:
	    case MODE_WRITE:
		if ((!isnumber(bank))||(sscanf(bank,"%li", &itmp) != 1)||(itmp < 0)||(itmp >= PCILIB_MAX_BANKS)) 
		    Usage(argc, argv, "Invalid data bank (%s) is specified", bank);
		else bar = itmp;
	    break;
	    default:
		if (pcilib_find_bank(handle, bank) == PCILIB_REGISTER_BANK_INVALID)
		    Usage(argc, argv, "Invalid data bank (%s) is specified", bank);
	}
    }

    signal(SIGINT, signal_exit_handler);

    if ((mode != MODE_GRAB)&&(output)) {
	ofile = fopen(output, "a+");
	if (!ofile) {
	    Error("Failed to open file \"%s\"", output);
	}
    }    

    switch (mode) {
     case MODE_INFO:
        Info(handle, model_info);
     break;
     case MODE_LIST:
        List(handle, model_info, bank, details);
     break;
     case MODE_BENCHMARK:
        Benchmark(handle, amode, dma, bar, start, size_set?size:0, access, iterations);
     break;
     case MODE_READ:
	if (amode == ACCESS_DMA) {
	    ReadData(handle, amode, flags, dma, bar, start, size_set?size:0, access, endianess, timeout_set?timeout:(size_t)-1, ofile);
	} else if (amode == ACCESS_CONFIG) {
	    ReadData(handle, amode, flags, dma, bar, addr?start:0, (addr||size_set)?size:(256/abs(access)), access, endianess, (size_t)-1, ofile);
	} else if (addr) {
	    ReadData(handle, amode, flags, dma, bar, start, size, access, endianess, (size_t)-1, ofile);
	} else {
	    Error("Address to read is not specified");
	}
     break;
     case MODE_READ_REGISTER:
        if ((reg)||(!addr)) ReadRegister(handle, model_info, bank, reg);
	else ReadRegisterRange(handle, model_info, bank, start, addr_shift, size, ofile);
     break;
     case MODE_WRITE:
	WriteData(handle, amode, dma, bar, start, size, access, endianess, data, verify);
     break;
     case MODE_WRITE_REGISTER:
        if (reg) WriteRegister(handle, model_info, bank, reg, data);
	else WriteRegisterRange(handle, model_info, bank, start, addr_shift, size, data);
     break;
     case MODE_RESET:
        pcilib_reset(handle);
     break;
     case MODE_GRAB:
        TriggerAndGrab(handle, grab_mode, event, data_type, size, run_time, trigger_time, timeout, partition, format, buffer, threads, verbose, output);
     break;
     case MODE_LIST_DMA:
        ListDMA(handle, fpga_device, model_info);
     break;
     case MODE_LIST_DMA_BUFFERS:
        ListBuffers(handle, fpga_device, model_info, dma, dma_direction);
     break;
     case MODE_READ_DMA_BUFFER:
        ReadBuffer(handle, fpga_device, model_info, dma, dma_direction, block, ofile);
     break;
     case MODE_START_DMA:
        StartStopDMA(handle, model_info, dma, dma_direction, 1);
     break;
     case MODE_STOP_DMA:
        StartStopDMA(handle, model_info, dma, dma_direction, 0);
     break;
     case MODE_ENABLE_IRQ:
        EnableIRQ(handle, model_info, irq_type);
     break;
     case MODE_DISABLE_IRQ:
        DisableIRQ(handle, model_info, irq_type);
     break;
     case MODE_ACK_IRQ:
        AckIRQ(handle, model_info, irq_source);
     break;
     case MODE_WAIT_IRQ:
        WaitIRQ(handle, model_info, irq_source, timeout);
     break;
     case MODE_LIST_KMEM:
        if (use) DetailKMEM(handle, fpga_device, use, block);
        else ListKMEM(handle, fpga_device);
     break;
     case MODE_READ_KMEM:
        ReadKMEM(handle, fpga_device, useid, block, 0, ofile);
     break;
     case MODE_ALLOC_KMEM:
        AllocKMEM(handle, fpga_device, use, type, size, block_size, alignment);
     break;
     case MODE_FREE_KMEM:
        FreeKMEM(handle, fpga_device, use, force);
     break;
     case MODE_INVALID:
        break;
    }

    if (ofile) fclose(ofile);

    pcilib_close(handle);
    
    if (data != argv + optind) free(data);
}
