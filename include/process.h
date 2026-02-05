/**
 * ClaudeOS Process Scheduler - process.h
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: Process management and scheduling
 */

#ifndef _CLAUDEOS_PROCESS_H
#define _CLAUDEOS_PROCESS_H

#include "types.h"

/* Maximum number of processes */
#define MAX_PROCESSES   64

/* Process stack size (4KB) */
#define PROCESS_STACK_SIZE  4096

/* Process states */
typedef enum {
    PROCESS_STATE_FREE = 0,     /* Process slot is free */
    PROCESS_STATE_READY,        /* Ready to run */
    PROCESS_STATE_RUNNING,      /* Currently executing */
    PROCESS_STATE_BLOCKED,      /* Waiting for I/O or event */
    PROCESS_STATE_SLEEPING,     /* Sleeping until wake_time */
    PROCESS_STATE_TERMINATED    /* Finished, waiting for cleanup */
} process_state_t;

/* Process priority levels */
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_REALTIME = 3
} process_priority_t;

/* CPU register state for context switching */
typedef struct {
    /* Pushed by interrupt stub */
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy;     /* Ignored by popad */
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    /* Pushed by CPU on interrupt */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;           /* Only present on privilege change */
    uint32_t ss;            /* Only present on privilege change */
} __attribute__((packed)) cpu_registers_t;

/* Process control block (PCB) */
typedef struct process {
    uint32_t pid;                   /* Process ID */
    process_state_t state;          /* Current state */
    process_priority_t priority;    /* Process priority */

    /* Saved CPU state */
    uint32_t esp;                   /* Saved stack pointer */
    uint32_t ebp;                   /* Saved base pointer */
    uint32_t eip;                   /* Saved instruction pointer */

    /* Stack */
    uint8_t* stack;                 /* Stack memory */
    uint32_t stack_size;            /* Stack size in bytes */

    /* Scheduling info */
    uint64_t wake_time;             /* Tick count to wake (if sleeping) */
    uint32_t time_slice;            /* Ticks remaining in time slice */
    uint64_t total_ticks;           /* Total CPU ticks used */

    /* Process info */
    char name[32];                  /* Process name */
    struct process* parent;         /* Parent process */
    int32_t exit_code;              /* Exit code (if terminated) */

    /* Entry point for new processes */
    void (*entry)(void);            /* Process entry function */
} process_t;

/* Process entry point function type */
typedef void (*process_entry_t)(void);

/**
 * Initialize the process scheduler
 */
void process_init(void);

/**
 * Create a new process
 * @param name Process name (for identification)
 * @param entry Entry point function
 * @param priority Process priority
 * @return Process ID, or -1 on failure
 */
int32_t process_create(const char* name, process_entry_t entry, process_priority_t priority);

/**
 * Exit the current process
 * @param exit_code Exit status code
 */
void process_exit(int32_t exit_code);

/**
 * Get current running process
 * @return Pointer to current process, or NULL if none
 */
process_t* process_current(void);

/**
 * Get process by PID
 * @param pid Process ID
 * @return Pointer to process, or NULL if not found
 */
process_t* process_get(uint32_t pid);

/**
 * Put current process to sleep
 * @param ms Milliseconds to sleep
 */
void process_sleep(uint32_t ms);

/**
 * Block current process (e.g., waiting for I/O)
 */
void process_block(void);

/**
 * Unblock a process (make it ready)
 * @param pid Process ID to unblock
 */
void process_unblock(uint32_t pid);

/**
 * Terminate a process by PID
 * @param pid Process ID to kill
 * @return 0 on success, -1 on failure
 */
int32_t process_kill(uint32_t pid);

/**
 * Run the scheduler
 * Called by timer interrupt to switch processes
 */
void schedule(void);

/**
 * Yield CPU to next process
 * Voluntary context switch
 */
void process_yield(void);

/**
 * Get process count
 * @return Number of active processes
 */
uint32_t process_count(void);

/**
 * Get process list for 'ps' command
 * @param pids Array to fill with PIDs
 * @param max_count Maximum entries to return
 * @return Number of processes returned
 */
uint32_t process_list(uint32_t* pids, uint32_t max_count);

/**
 * Get process state name as string
 * @param state Process state
 * @return State name string
 */
const char* process_state_name(process_state_t state);

#endif /* _CLAUDEOS_PROCESS_H */
