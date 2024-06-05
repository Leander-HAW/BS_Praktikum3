/**
 * @file vmaccess.c
 * @author Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * @date 2010
 * @brief The access functions to virtual memory.
 */

#include "vmaccess.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#include "syncdataexchange.h"
#include "vmem.h"
#include "debug.h"
#include "error.h"

/*
 * static variables
 */

static struct vmem_struct *vmem = NULL; //!< Reference to virtual memory

/**
 * The progression of time is simulated by the counter g_count, which is incremented by 
 * vmaccess on each memory access. The memory manager will be informed by a command, whenever 
 * a fixed period of time has passed. Hence the memory manager must be informed, whenever 
 * g_count % TIME_WINDOW == 0. 
 * Based on this information, memory manager will update aging information
 */

static int g_count = 0;    //!< global acces counter as quasi-timestamp - will be increment by each memory access
static int shm_id = -1; 
#define TIME_WINDOW   20

/**
 *****************************************************************************************
 *  @brief      This function setup the connection to virtual memory.
 *              The virtual memory has to be created by mmanage.c module.
 *
 *  @return     void
 ****************************************************************************************/
static void vmem_init(void) {

    //printf("vmem_init\n");
    /* Create System V shared memory */

    /* We are only using the shm, don't set the IPC_CREAT flag */
	key_t shm_key = ftok(SHMKEY, SHMPROCID);
	TEST_AND_EXIT_ERRNO(shm_key == -1, "ftok failed!");
	shm_id = shmget(shm_key, sizeof(struct vmem_struct), 0664);
	
	if (shm_id == -1){
		fprintf(stderr, "Shared memory from old run might still exists\n");
		fprintf(stderr, "   Use ipcs -ma for checking shared memory ressources\n");
		fprintf(stderr, "   Use ipcrm for deleting shared memory ressources\n");
	}
	
    /* Attach shared memory to vmem (virtual memory) */
	TEST_AND_EXIT_ERRNO(shm_id == -1, "shmget failed!");
	PRINT_DEBUG((stderr, "shmget successfuly allocated %lu bytes\n", sizeof(struct msg)));
	vmem = (struct vmem_struct *) shmat(shm_id, NULL, 0);
	TEST_AND_EXIT_ERRNO(vmem == (struct vmem_struct *) -1, "Error attaching shared memory");
	PRINT_DEBUG((stderr, "Shared memory successfuly attached\n"));
}

static void send_message(int cmd, int val) {
    //printf("sending msg: %d, val: %d\n", cmd, val);
    struct msg message;
    message.cmd = cmd;
    message.value = val;
    message.g_count = g_count;
    message.ref = g_count + val;
    sendMsgToMmanager(message);
}

/**
 *****************************************************************************************
 *  @brief      This function puts a page into memory (if required). Ref Bit of page table
 *              entry will be updated.
 *              If the time window handle by g_count has reached, the window window message
 *              will be send to the memory manager. 
 *              To keep conform with this log files, g_count must be increased before 
 *              the time window will be checked.
 *              vmem_read and vmem_write call this function.
 *
 *  @param      page The page that stores the contents of this address will be 
 *              put in (if required).
 * 
 *  @return     void
 ****************************************************************************************/
static void inc_gcount() {
    g_count++;
    if ((g_count % TIME_WINDOW) == 0) {
        send_message(CMD_TIME_INTER_VAL, g_count);
    }
}
static void vmem_put_page_into_mem(int page) {
	TEST_AND_EXIT_ERRNO(page > VMEM_NPAGES, "Page out of bounds!");
    // check ob page(adresse) ist im vmem
    if (vmem->pt[page].flags & PTF_PRESENT) {
        return;
    }

    // wenn nicht page fault senden
    send_message(CMD_PAGEFAULT, page);
    vmem->pt[page].flags |= PTF_REF;
    inc_gcount();
}


unsigned char vmem_read(int address) {
	if (vmem == NULL) {
		vmem_init();
	}

	int page = address / VMEM_PAGESIZE;

    vmem_put_page_into_mem(page);

    struct pt_entry* pt = &vmem->pt[page];

    int frame = pt->frame;
    TEST_AND_EXIT_ERRNO(frame > VMEM_NFRAMES, "Frame out of bounds!");

	int offset = address % VMEM_PAGESIZE;
    
    //printf("vmem read %d %d \n", &vmem->pt[page], vmem->pt[page]);
    return vmem->mainMemory[frame * VMEM_PAGESIZE + offset];
}

void vmem_write(int address, unsigned char data) {
	if (vmem == NULL) {
		vmem_init();
	}

    //printf("vmem read\n");

	int page = address / (VMEM_PAGESIZE / sizeof(unsigned char));
    TEST_AND_EXIT_ERRNO(page > VMEM_NPAGES, "Page out of bounds!");

    vmem_put_page_into_mem(page);

    struct pt_entry* pt = &vmem->pt[page]; 

    int frame = pt->frame;
    TEST_AND_EXIT_ERRNO(frame > VMEM_NFRAMES, "Frame out of bounds!");

    pt->flags |= PTF_DIRTY;
	int offset = address % (VMEM_PAGESIZE / sizeof(unsigned char));
    vmem->mainMemory[frame * VMEM_PAGESIZE + offset] = data;
}
// EOF
