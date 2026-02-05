/**
 * ClaudeOS System Call Interface - syscall.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: INT 0x80 handler and system call implementations
 */

#include "types.h"
#include "syscall.h"
#include "idt.h"
#include "process.h"
#include "timer.h"
#include "vga.h"

/* Forward declarations for VFS functions */
extern int32_t fs_open(const char* path, int flags);
extern int32_t fs_close(int32_t fd);
extern int32_t fs_read(int32_t fd, void* buf, uint32_t count);
extern int32_t fs_write(int32_t fd, const void* buf, uint32_t count);

/* String length helper */
static uint32_t str_len(const char* s) {
    uint32_t len = 0;
    while (s[len]) len++;
    return len;
}

/**
 * SYS_EXIT - Exit current process
 */
static int32_t do_sys_exit(int32_t status) {
    process_exit(status);
    return 0;  /* Never reached */
}

/**
 * SYS_READ - Read from file descriptor
 * Currently supports: STDIN (keyboard input)
 */
static int32_t do_sys_read(int32_t fd, void* buf, uint32_t count) {
    if (!buf || count == 0) {
        return SYSCALL_EINVAL;
    }

    if (fd == STDIN_FD) {
        /* Read from keyboard - for now just return 0 (no input) */
        /* In a full implementation, this would block until input available */
        return 0;
    }

    /* Try VFS for other file descriptors */
    /* Note: VFS integration would go here */
    return SYSCALL_EBADF;
}

/**
 * SYS_WRITE - Write to file descriptor
 * Currently supports: STDOUT, STDERR (VGA output)
 */
static int32_t do_sys_write(int32_t fd, const void* buf, uint32_t count) {
    if (!buf || count == 0) {
        return SYSCALL_EINVAL;
    }

    if (fd == STDOUT_FD || fd == STDERR_FD) {
        /* Write to VGA display */
        const char* str = (const char*)buf;
        for (uint32_t i = 0; i < count; i++) {
            if (str[i] == '\0') break;
            vga_putchar(str[i]);
        }
        return count;
    }

    /* Try VFS for other file descriptors */
    return SYSCALL_EBADF;
}

/**
 * SYS_GETPID - Get current process ID
 */
static int32_t do_sys_getpid(void) {
    process_t* proc = process_current();
    if (proc) {
        return proc->pid;
    }
    return 0;  /* Kernel context */
}

/**
 * SYS_SLEEP - Sleep for specified milliseconds
 */
static int32_t do_sys_sleep(uint32_t ms) {
    if (ms == 0) {
        /* Yield CPU instead of sleeping for 0ms */
        process_yield();
    } else {
        /* Use timer-based sleep */
        timer_sleep_ms(ms);
    }
    return SYSCALL_SUCCESS;
}

/**
 * SYS_YIELD - Voluntarily yield CPU
 */
static int32_t do_sys_yield(void) {
    process_yield();
    return SYSCALL_SUCCESS;
}

/**
 * SYS_UPTIME - Get system uptime in seconds
 */
static int32_t do_sys_uptime(void) {
    return timer_get_uptime_seconds();
}

/**
 * SYS_GETTIME - Get system time (ticks since boot)
 */
static int32_t do_sys_gettime(void) {
    /* Return low 32 bits of tick count */
    return (int32_t)(timer_get_ticks() & 0xFFFFFFFF);
}

/**
 * System call dispatch table
 */
typedef int32_t (*syscall_fn_t)(uint32_t, uint32_t, uint32_t);

static syscall_fn_t syscall_table[SYS_MAX] = {
    [SYS_EXIT]    = (syscall_fn_t)do_sys_exit,
    [SYS_READ]    = (syscall_fn_t)do_sys_read,
    [SYS_WRITE]   = (syscall_fn_t)do_sys_write,
    [SYS_GETPID]  = (syscall_fn_t)do_sys_getpid,
    [SYS_SLEEP]   = (syscall_fn_t)do_sys_sleep,
    [SYS_YIELD]   = (syscall_fn_t)do_sys_yield,
    [SYS_FORK]    = NULL,  /* Not implemented */
    [SYS_EXEC]    = NULL,  /* Not implemented */
    [SYS_WAIT]    = NULL,  /* Not implemented */
    [SYS_OPEN]    = NULL,  /* TODO: VFS integration */
    [SYS_CLOSE]   = NULL,  /* TODO: VFS integration */
    [SYS_STAT]    = NULL,  /* TODO: VFS integration */
    [SYS_MKDIR]   = NULL,  /* TODO: VFS integration */
    [SYS_RMDIR]   = NULL,  /* TODO: VFS integration */
    [SYS_UNLINK]  = NULL,  /* TODO: VFS integration */
    [SYS_CHDIR]   = NULL,  /* TODO: VFS integration */
    [SYS_GETCWD]  = NULL,  /* TODO: VFS integration */
    [SYS_GETTIME] = (syscall_fn_t)do_sys_gettime,
    [SYS_UPTIME]  = (syscall_fn_t)do_sys_uptime,
};

/**
 * Main system call handler
 * Called from INT 0x80 ISR
 */
int32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    /* Validate syscall number */
    if (syscall_num >= SYS_MAX) {
        return SYSCALL_EINVAL;
    }

    /* Get handler from dispatch table */
    syscall_fn_t handler = syscall_table[syscall_num];
    if (!handler) {
        return SYSCALL_ENOTSUP;
    }

    /* Call the handler */
    return handler(arg1, arg2, arg3);
}

/**
 * INT 0x80 interrupt handler
 * Assembly stub calls this with registers saved
 */
static void syscall_interrupt_handler(void) {
    /* Note: In a real implementation, we would get registers from the stack frame
     * passed by the ISR stub. For now, this is a placeholder that demonstrates
     * the concept. The actual register access would be:
     *
     * EAX = syscall number
     * EBX = arg1
     * ECX = arg2
     * EDX = arg3
     *
     * Return value goes in EAX
     */
}

/**
 * Initialize system call interface
 */
void syscall_init(void) {
    /* Register INT 0x80 handler
     * Note: This requires adding isr128 to the ISR stubs
     * For now, we register a placeholder handler
     */
    register_interrupt_handler(0x80, syscall_interrupt_handler);

    vga_puts("[KERNEL] System call interface initialized (INT 0x80)\n");
}

/*
 * User-space system call wrappers
 * These use inline assembly to invoke INT 0x80
 * DISABLED: These require a true 32-bit compiler or cross-compiler
 */
#ifdef ENABLE_USERSPACE_SYSCALLS

void sys_exit(int32_t status) {
    __asm__ volatile (
        "mov $0, %%eax\n"   /* SYS_EXIT = 0 */
        "mov %0, %%ebx\n"   /* status in EBX */
        "int $0x80\n"
        :
        : "r"(status)
        : "eax", "ebx"
    );
}

int32_t sys_read(int32_t fd, void* buf, uint32_t count) {
    int32_t result;
    __asm__ volatile (
        "mov $1, %%eax\n"   /* SYS_READ = 1 */
        "mov %1, %%ebx\n"   /* fd in EBX */
        "mov %2, %%ecx\n"   /* buf in ECX */
        "mov %3, %%edx\n"   /* count in EDX */
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(result)
        : "r"(fd), "r"(buf), "r"(count)
        : "eax", "ebx", "ecx", "edx"
    );
    return result;
}

int32_t sys_write(int32_t fd, const void* buf, uint32_t count) {
    int32_t result;
    __asm__ volatile (
        "mov $2, %%eax\n"   /* SYS_WRITE = 2 */
        "mov %1, %%ebx\n"   /* fd in EBX */
        "mov %2, %%ecx\n"   /* buf in ECX */
        "mov %3, %%edx\n"   /* count in EDX */
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(result)
        : "r"(fd), "r"(buf), "r"(count)
        : "eax", "ebx", "ecx", "edx"
    );
    return result;
}

int32_t sys_getpid(void) {
    int32_t result;
    __asm__ volatile (
        "mov $3, %%eax\n"   /* SYS_GETPID = 3 */
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(result)
        :
        : "eax"
    );
    return result;
}

int32_t sys_sleep(uint32_t ms) {
    int32_t result;
    __asm__ volatile (
        "mov $4, %%eax\n"   /* SYS_SLEEP = 4 */
        "mov %1, %%ebx\n"   /* ms in EBX */
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(result)
        : "r"(ms)
        : "eax", "ebx"
    );
    return result;
}

int32_t sys_yield(void) {
    int32_t result;
    __asm__ volatile (
        "mov $5, %%eax\n"   /* SYS_YIELD = 5 */
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(result)
        :
        : "eax"
    );
    return result;
}

int32_t sys_uptime(void) {
    int32_t result;
    __asm__ volatile (
        "mov $18, %%eax\n"  /* SYS_UPTIME = 18 */
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(result)
        :
        : "eax"
    );
    return result;
}

uint64_t sys_gettime(void) {
    uint32_t low;
    __asm__ volatile (
        "mov $17, %%eax\n"  /* SYS_GETTIME = 17 */
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(low)
        :
        : "eax"
    );
    return (uint64_t)low;
}

#endif /* ENABLE_USERSPACE_SYSCALLS */
