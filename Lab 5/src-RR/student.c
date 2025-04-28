/*
 * student.c
 * Multithreaded OS Simulation for CS 2200 and ECE 3058
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"
#define DEBUG 0

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;
static pthread_mutex_t active_queue; // tracks if queue is active
static pthread_cond_t has_process; // communicate when queue is active

// Time slice used to mark ongoing processes as pre-empted and moves to the end of the queue
static int timeslice = -1;
/*
    Queue backed by a Linked List
    Create a pointer to the beginning of the LL
    -- Allows for use to track the processes ready for execution
*/
static pcb_t* head;

/*
    Push(new process) : Take the inputted process and append to the LL
*/
static void push(pcb_t* process) {
    // since the queue is shared, we need to lock it down
    pthread_mutex_lock(&active_queue);
    if (!head) head = process; // init condition if LL is NULL
    else {
        // Alias to the head of the LL -- Variable to find the end of LL
        pcb_t* temp = head;
        // Iterate over LL so we can append the new process
        while (temp->next != NULL) {
            // update temp variable
            temp = temp->next;
        }
        // Append process to the end of the LL
        temp->next = process;
    }
    process->next = NULL; // to ensure it is appended at the end
    // Queue is now active, so any process waiting on it should start.
    pthread_cond_broadcast(&has_process);

    // Queue is unlocked
    pthread_mutex_unlock(&active_queue);
}

/*
    Pop() : Take the oldest item in the linked list, remove it from the LL, and return it
*/
static pcb_t* pop() {
    // create variable to save the process that will be removed
    pcb_t* temp = head;

    // lock active queue incase current thread does not have it
    pthread_mutex_lock(&active_queue);

    // update head of the LL
    if (temp) head = temp->next;

    // unlock active queue
    pthread_mutex_unlock(&active_queue);

    // return process
    return temp;
}


/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* 
     *    Update current & Use MUTEX to protect it-- why?? 
     *    Mutex should not be implemneted for reading but should be for writing\
     * 
     *  pthread functions needed:-
     *  
     *  pthread_create(top-level procedure, args): creates a new thread that starts 
     *  execution in top-level procedure with supplied args as actual parameters for 
     *  formal parameters specifiec in procedure prototype
     * 
     *  pthread_terminate(tid): terminates thread with id given by tid
     * 
     *  pthread_mutex_lock(mylock): when thread returns it has *mylock*; calling 
     *  thread blocks if the lock is in use currently by some other thread.
     * 
     *  pthread_mutex_trylock(mylock): call does not block calling thead; 
     *  instead it returns success if thead gets *mylock*;
     *  failure if the lock is in use currently by some other thread.
     * 
     *  pthread_mutex_unlock(mylock): if calling thread currently has *mylock* it is released; 
     *  error otherwise
     * 
     *  pthread_join(peer_thread_tid): calling thread blocks until thread given by *peer_thread_tid* terminates
     * 
     *  pthread_cond_wait(buf_not_empty, buflock): calling thread blocks on condition variable buf_not_empty; 
     *  library implicitly releases lock buflock; 
     *  error if lock is not currently help by calling thread.
     *  
     *  pthread_cond_signal(buf_not_empty): a thread (if any) waiting on condition variable *buf_not_empty* is woken_up; 
     *  awakened thread either is ready for execution if lock associated with it (in wait call) is currently available; 
     *  if not, thread is moved fro queue for condition variable to the appropriate lock queue.
    */

    // Task 1 -- Select and remove a runnable process the queue
    pcb_t* runable_process = pop();

    // ensure that the process is not NULL
    if (runable_process) {
        // set the current task to running
        runable_process->state = PROCESS_RUNNING;
    }
    // current is a shared variable for different threads -- meaning we need to ensure that it is marked as a critical code
    pthread_mutex_lock(&current_mutex);

    // update current to include the current process that is running
    current[cpu_id] = runable_process;

    // now that the current process is finished, we can unlock
    pthread_mutex_unlock(&current_mutex);

    // preform a context switch 
    context_switch(cpu_id, runable_process, timeslice);
}

/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     * 
     * pthread_cond_wait : block for a singal
     * pthread_cond_signal: signal one waiting thread
     * pthread_cond_broadcast: signal all waiting threads
     * 
     * pthread signaling will allow us to track if something has been read
     * allowing us to not use other methods such as polling
     */
    // Lock mutex incase active queue is locked [empty]
    pthread_mutex_lock(&active_queue);
    while (head == NULL) {
        // listen for backlog of processes to start
        pthread_cond_wait(&has_process, &active_queue);
    }
    // unlock this thread
    pthread_mutex_unlock(&active_queue);

    // schedule next process to run
    schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{   
    if (DEBUG) {
        printf("\n\nPREEMPT PROCESS -- Debug\n\n");
    }
    // lock 
    pthread_mutex_lock(&current_mutex);

    // change the current process state
    current[cpu_id]->state = PROCESS_READY;

    // Unlock
    pthread_mutex_unlock(&current_mutex);

    // Push into the queue for execution
    push(current[cpu_id]);

    // Schedule it to run
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    // Getting ready for an I/O request -- need to stall until we recieve I/O data
    pthread_mutex_lock(&current_mutex);

    // Set current thread into the waiting state.
    current[cpu_id]->state = PROCESS_WAITING;

    // Once the I/O is recieved, unlock
    pthread_mutex_unlock(&current_mutex);

    // need to schedule next process to run
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    // Terminating a process, need to lock since we are modifying shared data
    pthread_mutex_lock(&current_mutex);

    // Marked process as terminated
    current[cpu_id]->state = PROCESS_TERMINATED;

    // Unlock the thread
    pthread_mutex_unlock(&current_mutex);

    // Schedule the next process to run
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is LRTF, wake_up() may need
 *      to preempt the CPU with lower remaining time left to allow it to
 *      execute the process which just woke up with higher reimaing time.
 * 	However, if any CPU is currently running idle,
 * 	or all of the CPUs are running processes
 *      with a higher remaining time left than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    // Lock since we are modifying a shared var
    pthread_mutex_lock(&current_mutex);

    // Set the process to ready
    process->state = PROCESS_READY;

    // Put the process in the queue
    push(process);

    // Unlock mutex
    pthread_mutex_unlock(&current_mutex);

    // push process onto queue
    push(process);

    // for (int i = 0; i < sizeof(current); i++){
    //     if (DEBUG) {
    //         printf("\n\n Wake_up.preempt -- Debug \n\n");
    //     }
    //     if (process->pid == current[i]->pid) {
    //         preempt(i);
    //     }
    // }
    
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -l and -r command-line parameters.
 */
int main(int argc, char *argv[])
{
    unsigned int cpu_count;

    /* Parse command-line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -l | -r <time slice> ]\n"
            "    Default : FIFO Scheduler\n"
	    "         -l : Longest Remaining Time First Scheduler\n"
            "         -r : Round-Robin Scheduler\n\n");
        return -1;
    }
    cpu_count = strtoul(argv[1], NULL, 0);

    /* FIX ME - Add support for -l and -r parameters*/
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            timeslice = atoi(argv[i+1]);
        }
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    // Initialize head of queue
    head = NULL;

    // Allocated ative queue mutex
    pthread_mutex_init(&active_queue, NULL);

    // Allocate has_process cond
    pthread_cond_init(&has_process, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


