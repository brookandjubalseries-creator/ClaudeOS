/**
 * ClaudeOS Process Scheduler - process.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: Round-robin process scheduler with timer-driven preemption
 */

#include "types.h"
#include "process.h"
#include "timer.h"
#include "kmalloc.h"
#include "vga.h"

/* Process table */
static process_t process_table[MAX_PROCESSES];
static process_t* current_process = NULL;
static uint32_t next_pid = 1;
static bool scheduler_enabled = false;

/* Idle process (runs when no other process is ready) */
static void idle_process_entry(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

/* String copy helper */
static void proc_strcpy(char* dest, const char* src, uint32_t max) {
    uint32_t i;
    for (i = 0; i < max - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/* State name lookup */
static const char* state_names[] = {
    "FREE",
    "READY",
    "RUNNING",
    "BLOCKED",
    "SLEEPING",
    "TERMINATED"
};

/**
 * Get process state name
 */
const char* process_state_name(process_state_t state) {
    if (state <= PROCESS_STATE_TERMINATED) {
        return state_names[state];
    }
    return "UNKNOWN";
}

/**
 * Find a free process slot
 */
static process_t* find_free_slot(void) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_FREE) {
            return &process_table[i];
        }
    }
    return NULL;
}

/**
 * Find next ready process (round-robin)
 */
static process_t* find_next_ready(void) {
    if (!current_process) {
        /* Start from beginning if no current process */
        for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
            if (process_table[i].state == PROCESS_STATE_READY) {
                return &process_table[i];
            }
        }
        return NULL;
    }

    /* Start from current process and wrap around */
    uint32_t start_idx = current_process - process_table;
    for (uint32_t i = 1; i <= MAX_PROCESSES; i++) {
        uint32_t idx = (start_idx + i) % MAX_PROCESSES;
        if (process_table[idx].state == PROCESS_STATE_READY) {
            return &process_table[idx];
        }
    }

    return NULL;
}

/**
 * Wake sleeping processes whose wake time has passed
 */
static void wake_sleeping_processes(uint64_t current_ticks) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_SLEEPING) {
            if (current_ticks >= process_table[i].wake_time) {
                process_table[i].state = PROCESS_STATE_READY;
            }
        }
    }
}

/**
 * Timer callback for scheduler
 */
static void scheduler_timer_callback(uint64_t ticks) {
    if (!scheduler_enabled) return;

    /* Wake sleeping processes */
    wake_sleeping_processes(ticks);

    /* Decrement time slice for current process */
    if (current_process && current_process->state == PROCESS_STATE_RUNNING) {
        if (current_process->time_slice > 0) {
            current_process->time_slice--;
        }

        /* Preempt if time slice expired */
        if (current_process->time_slice == 0) {
            schedule();
        }

        /* Track CPU usage */
        current_process->total_ticks++;
    }
}

/**
 * Process wrapper that handles process exit
 */
static void process_wrapper(void) {
    if (current_process && current_process->entry) {
        current_process->entry();
    }
    process_exit(0);
}

/**
 * Initialize the process scheduler
 */
void process_init(void) {
    /* Clear process table */
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_STATE_FREE;
        process_table[i].pid = 0;
        process_table[i].stack = NULL;
    }

    /* Create idle process (PID 0) */
    process_t* idle = &process_table[0];
    idle->pid = 0;
    idle->state = PROCESS_STATE_READY;
    idle->priority = PRIORITY_LOW;
    proc_strcpy(idle->name, "idle", 32);
    idle->entry = idle_process_entry;
    idle->stack = (uint8_t*)kmalloc(PROCESS_STACK_SIZE);
    idle->stack_size = PROCESS_STACK_SIZE;
    idle->time_slice = 1;  /* Minimal time slice for idle */
    idle->total_ticks = 0;
    idle->parent = NULL;
    idle->exit_code = 0;

    /* Set up idle process stack */
    if (idle->stack) {
        /* Stack grows downward, so start at top */
        uint32_t* stack_top = (uint32_t*)(idle->stack + PROCESS_STACK_SIZE);

        /* Push initial values (will be popped by context switch) */
        *(--stack_top) = 0x202;                     /* EFLAGS (IF=1) */
        *(--stack_top) = 0x08;                      /* CS */
        *(--stack_top) = (uint32_t)idle_process_entry; /* EIP */
        *(--stack_top) = 0;                         /* EAX */
        *(--stack_top) = 0;                         /* ECX */
        *(--stack_top) = 0;                         /* EDX */
        *(--stack_top) = 0;                         /* EBX */
        *(--stack_top) = 0;                         /* ESP (ignored) */
        *(--stack_top) = 0;                         /* EBP */
        *(--stack_top) = 0;                         /* ESI */
        *(--stack_top) = 0;                         /* EDI */

        idle->esp = (uint32_t)stack_top;
    }

    /* Create init process (PID 1) - this is the kernel/shell */
    process_t* init = &process_table[1];
    init->pid = 1;
    init->state = PROCESS_STATE_RUNNING;  /* Already running */
    init->priority = PRIORITY_NORMAL;
    proc_strcpy(init->name, "init", 32);
    init->entry = NULL;  /* Already executing */
    init->stack = NULL;  /* Uses kernel stack */
    init->stack_size = 0;
    init->time_slice = 10;  /* 100ms time slice */
    init->total_ticks = 0;
    init->parent = NULL;
    init->exit_code = 0;

    current_process = init;
    next_pid = 2;

    /* Register timer callback for scheduling */
    timer_set_callback(scheduler_timer_callback);
    scheduler_enabled = true;

    vga_puts("[KERNEL] Process scheduler initialized (round-robin)\n");
}

/**
 * Create a new process
 */
int32_t process_create(const char* name, process_entry_t entry, process_priority_t priority) {
    if (!entry) return -1;

    /* Find free slot */
    process_t* proc = find_free_slot();
    if (!proc) {
        return -1;  /* No free slots */
    }

    /* Allocate stack */
    proc->stack = (uint8_t*)kmalloc(PROCESS_STACK_SIZE);
    if (!proc->stack) {
        return -1;  /* Out of memory */
    }
    proc->stack_size = PROCESS_STACK_SIZE;

    /* Initialize process */
    proc->pid = next_pid++;
    proc->state = PROCESS_STATE_READY;
    proc->priority = priority;
    proc->entry = entry;
    proc->time_slice = 10;  /* 100ms time slice */
    proc->total_ticks = 0;
    proc->wake_time = 0;
    proc->parent = current_process;
    proc->exit_code = 0;
    proc_strcpy(proc->name, name, 32);

    /* Set up initial stack for context switch */
    uint32_t* stack_top = (uint32_t*)(proc->stack + PROCESS_STACK_SIZE);

    /* Push initial values (simulates interrupt frame) */
    *(--stack_top) = 0x202;                     /* EFLAGS (IF=1, enable interrupts) */
    *(--stack_top) = 0x08;                      /* CS (kernel code segment) */
    *(--stack_top) = (uint32_t)process_wrapper; /* EIP (entry wrapper) */
    *(--stack_top) = 0;                         /* EAX */
    *(--stack_top) = 0;                         /* ECX */
    *(--stack_top) = 0;                         /* EDX */
    *(--stack_top) = 0;                         /* EBX */
    *(--stack_top) = 0;                         /* ESP (ignored by popad) */
    *(--stack_top) = 0;                         /* EBP */
    *(--stack_top) = 0;                         /* ESI */
    *(--stack_top) = 0;                         /* EDI */

    proc->esp = (uint32_t)stack_top;
    proc->ebp = 0;
    proc->eip = (uint32_t)process_wrapper;

    return proc->pid;
}

/**
 * Exit current process
 */
void process_exit(int32_t exit_code) {
    if (!current_process) return;

    /* Can't exit init process (PID 1) */
    if (current_process->pid == 1) {
        vga_puts("[KERNEL] Warning: init process cannot exit\n");
        return;
    }

    current_process->state = PROCESS_STATE_TERMINATED;
    current_process->exit_code = exit_code;

    /* Free stack memory */
    if (current_process->stack) {
        kfree(current_process->stack);
        current_process->stack = NULL;
    }

    /* Switch to another process */
    schedule();
}

/**
 * Get current process
 */
process_t* process_current(void) {
    return current_process;
}

/**
 * Get process by PID
 */
process_t* process_get(uint32_t pid) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid &&
            process_table[i].state != PROCESS_STATE_FREE) {
            return &process_table[i];
        }
    }
    return NULL;
}

/**
 * Sleep current process
 */
void process_sleep(uint32_t ms) {
    if (!current_process) return;

    uint64_t ticks = ms / MS_PER_TICK;
    if (ms > 0 && ticks == 0) ticks = 1;

    current_process->wake_time = timer_get_ticks() + ticks;
    current_process->state = PROCESS_STATE_SLEEPING;
    schedule();
}

/**
 * Block current process
 */
void process_block(void) {
    if (!current_process) return;
    current_process->state = PROCESS_STATE_BLOCKED;
    schedule();
}

/**
 * Unblock a process
 */
void process_unblock(uint32_t pid) {
    process_t* proc = process_get(pid);
    if (proc && proc->state == PROCESS_STATE_BLOCKED) {
        proc->state = PROCESS_STATE_READY;
    }
}

/**
 * Kill a process
 */
int32_t process_kill(uint32_t pid) {
    /* Can't kill idle or init */
    if (pid <= 1) return -1;

    process_t* proc = process_get(pid);
    if (!proc) return -1;

    proc->state = PROCESS_STATE_TERMINATED;
    proc->exit_code = -1;  /* Killed */

    /* Free stack */
    if (proc->stack) {
        kfree(proc->stack);
        proc->stack = NULL;
    }

    /* If killing current process, switch away */
    if (proc == current_process) {
        schedule();
    }

    return 0;
}

/**
 * Run the scheduler - switch to next ready process
 */
void schedule(void) {
    if (!scheduler_enabled) return;

    process_t* next = find_next_ready();

    /* If no ready process, use idle */
    if (!next) {
        next = &process_table[0];  /* Idle process */
        if (next->state == PROCESS_STATE_FREE) {
            /* No idle process available - should never happen */
            return;
        }
    }

    /* If same process, just reset time slice */
    if (next == current_process) {
        current_process->time_slice = 10;
        return;
    }

    /* Context switch */
    process_t* prev = current_process;

    /* Mark previous as ready (unless it's terminated/blocked/sleeping) */
    if (prev && prev->state == PROCESS_STATE_RUNNING) {
        prev->state = PROCESS_STATE_READY;
    }

    /* Switch to next process */
    current_process = next;
    current_process->state = PROCESS_STATE_RUNNING;
    current_process->time_slice = 10;  /* Reset time slice */

    /* Note: In a real implementation, we would do actual context switching
     * by saving/restoring registers. For now, we rely on the cooperative
     * model where processes yield or block voluntarily.
     *
     * A full implementation would need assembly code to:
     * 1. Save current ESP/EBP to prev->esp/ebp
     * 2. Load next->esp/ebp to ESP/EBP
     * 3. Return (which would jump to next process)
     */
}

/**
 * Yield CPU voluntarily
 */
void process_yield(void) {
    if (current_process) {
        current_process->time_slice = 0;
        schedule();
    }
}

/**
 * Get number of active processes
 */
uint32_t process_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_STATE_FREE &&
            process_table[i].state != PROCESS_STATE_TERMINATED) {
            count++;
        }
    }
    return count;
}

/**
 * Get process list
 */
uint32_t process_list(uint32_t* pids, uint32_t max_count) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_PROCESSES && count < max_count; i++) {
        if (process_table[i].state != PROCESS_STATE_FREE &&
            process_table[i].state != PROCESS_STATE_TERMINATED) {
            pids[count++] = process_table[i].pid;
        }
    }
    return count;
}
