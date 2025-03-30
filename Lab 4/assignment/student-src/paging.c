#include "paging.h"
#include "page_splitting.h"
#include "swapops.h"
#include "stats.h"

 /* The frame table pointer. You will set this up in system_init. */
fte_t *frame_table;

/*  --------------------------------- PROBLEM 2 --------------------------------------
    In this problem, you will initialize the frame table.

    The frame table will be located at physical address 0 in our simulated
    memory. You will first assign the frame_table global variable to point to
    this location in memory. You should zero out the frame table, in case for
    any reason physical memory is not clean.

    You should then mark the first entry in the frame table as protected. We do
    this because we do not want our free frame allocator to give out the frame
    used by the frame table.

    HINTS:
        You will need to use the following global variables:
        - mem: Simulated physical memory already allocated for you.
        - PAGE_SIZE: The size of one page.
        You will need to initialize (set) the following global variable:
        - frame_table: a pointer to the first entry of the frame table

    -----------------------------------------------------------------------------------
*/
void system_init(void) {
    /*
     * 1. Set the frame table pointer to point to the first frame in physical
     * memory. Zero out the memory used by the frame table.
     *
     * Address "0" in memory will be used for the frame table. This table will
     * contain n frame table entries (fte_t), where n is the number of
     * frames in memory. The frame table will be useful later if we need to
     * evict pages during page faults.
     */
    frame_table = (fte_t*) mem; /*  Sets the initial frame to the address base of
                                    the memory.
                                */ 
    // for (int i = 0; i <= NUM_FRAMES; i++) { // Loop to clear out memory 
    //     frame_table->mapped = 0;
    //     frame_table->process = 0;
    //     frame_table->protected = 0;
    //     frame_table->referenced = 0;
    //     frame_table->vpn = 0;
    // }
    memset(frame_table, 0, PAGE_SIZE);  /* Built in C function that allows us to fill a block of memory 
                                           with a specified value
                                        */
    /*
     * 2. Mark the first frame table entry as protected.
     *
     * The frame table contains entries for all of physical memory,
     * however, there are some frames we never want to evict.
     * We mark these special pages as "protected" to indicate this.
     */
    frame_table[0].protected = 1; // Protect the first frame in order to prevent eviction
}

/*  --------------------------------- PROBLEM 3 --------------------------------------
    This function gets called every time a new process is created.
    You will need to allocate a new page table for the process in memory using the
    free_frame function so that the process can store its page mappings. Then, you
    will need to store the PFN of this page table in the process's PCB.

    HINTS:
        - Look at the pcb_t struct defined in pagesim.h to know what to set inside.
        - You are not guaranteed that the memory returned by the free frame allocator
        is empty - an existing frame could have been evicted for our new page table.
        - As in the previous problem, think about whether we need to mark any entries
        in the frame_table as protected after we allocate memory for our page table.
    -----------------------------------------------------------------------------------
*/
void proc_init(pcb_t *proc) {
    /*
     * 1. Call the free frame allocator (free_frame) to return a free frame for
     * this process's page table. You should zero-out the memory.
     */
    pfn_t proc_frame = free_frame();   // Create a new frame for a new process 
    memset(mem + (proc_frame * PAGE_SIZE), 0, PAGE_SIZE);   // Clear out data that occupied this memory space for new process data
    /*
     * 2. Update the process's PCB with the frame number
     * of the newly allocated page table.
     *
     * Additionally, mark the frame's frame table entry as protected. You do not
     * want your page table to be accidentally evicted.
     */
    proc->saved_ptbr = proc_frame;  // Add new process to the list of active processes
    frame_table[proc_frame].protected = 1;  // Protect the new process from eviction
}

/*  --------------------------------- PROBLEM 4 --------------------------------------
    Swaps the currently running process on the CPU to another process.

    Every process has its own page table, as you allocated in proc_init. You will
    need to tell the processor to use the new process's page table.

    HINTS:
        - Look at the global variables defined in pagesim.h. You may be interested in
        the definition of pcb_t as well.
    -----------------------------------------------------------------------------------
 */
void context_switch(pcb_t *proc) {
    PTBR = proc->saved_ptbr;    /*  Simulator needs access to the current process that is running.
                                    This is accomplished though proc_init, therefor we need to ensure
                                    that the PTBR follows.
                                */
}

/*  --------------------------------- PROBLEM 5 --------------------------------------
    Takes an input virtual address and returns the data from the corresponding
    physical memory address. The steps to do this are:

    1) Translate the virtual address into a physical address using the page table.
    2) Go into the memory and read/write the data at the translated address.

    Parameters:
        1) address     - The virtual address to be translated.
        2) rw          - 'r' if the access is a read, 'w' if a write
        3) data        - One byte of data to write to our memory, if the access is a write.
                         This byte will be NULL on read accesses.

    Return:
        The data at the address if the access is a read, or
        the data we just wrote if the access is a write.

    HINTS:
        - You will need to use the macros we defined in Problem 1 in this function.
        - You will need to access the global PTBR value. This will tell you where to
        find the page table. Be very careful when you think about what this register holds!
        - On a page fault, simply call the page_fault function defined in page_fault.c.
        You may assume that the pagefault handler allocated a page for your address
        in the page table after it returns.
        - Make sure to set the referenced bit in the frame table entry since we accessed the page.
        - Make sure to set the dirty bit in the page table entry if it's a write.
        - Make sure to update the stats variables correctly (see stats.h)
    -----------------------------------------------------------------------------------
 */
uint8_t mem_access(vaddr_t address, char rw, uint8_t data) {


    /* Split the address and find the page table entry */
    vpn_t virtual_pg_num = vaddr_vpn(address);     // Find the VPN
    uint16_t offset = vaddr_offset(address);       // Find the Offset

    pte_t* entry_ptr = (pte_t*) (mem + (PAGE_SIZE * PTBR)); // Create base entry for current process
    entry_ptr += virtual_pg_num;                            // Set entry point to current VPN


    /* If an entry is invalid, just page fault to allocate a page for the page table. */
    if (!(entry_ptr->valid)) page_fault(address);   // If the current entyr is invalid, preform a page_fault

    /* Set the "referenced" bit to reduce the page's likelihood of eviction */
    frame_table[entry_ptr->pfn].referenced = 1;     // Setting the referece bit in order to reduce the eviction probability

    /*
        The physical address will be constructed like this:
        -------------------------------------
        |     PFN    |      Offset          |
        -------------------------------------
        where PFN is the value stored in the page table entry.
        We need to calculate the number of bits are in the offset.

        Create the physical address using your offset and the page
        table entry.
    */
    uint8_t* phys_addr = (uint8_t*) (mem + entry_ptr->pfn * PAGE_SIZE + offset);    // Translate into physical address for write case

    /* Either read or write the data to the physical address
       depending on 'rw' */
    if (rw == 'r') {
        stats.reads++;
        return *phys_addr;
    } else {
        stats.writes++;
        *phys_addr = data;       // preform write
        entry_ptr->dirty = 1;   // Due to data write, set dirty bit for entry
        return data;            // return data
    }
}

/*  --------------------------------- PROBLEM 8 --------------------------------------
    When a process exits, you need to free any pages previously occupied by the
    process. Otherwise, every time you closed and re-opened Microsoft Word, it
    would gradually eat more and more of your computer's usable memory.

    To free a process, you must clear the "mapped" bit on every page the process
    has mapped. If the process has swapped any pages to disk, you must call
    swap_free() using the page table entry pointer as a parameter.

    You must also clear the "protected" bits for the page table itself.
    -----------------------------------------------------------------------------------
*/
void proc_cleanup(pcb_t *proc) {
    /* Look up the process's page table */
    pte_t* proc_entry = (pte_t*) (mem + proc->saved_ptbr * PAGE_SIZE);
    /* Iterate the page table and clean up each valid page */
    for (size_t i = 0; i < NUM_PAGES; i++) {
        if (proc_entry[i].valid) {
            frame_table[proc_entry[i].pfn].mapped = 0;
        }
        if (swap_exists(&proc_entry[i])) swap_free(&proc_entry[i]);

    }

    /* Free the page table itself in the frame table */

}
