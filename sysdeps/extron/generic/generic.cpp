#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <mlibc/tcb.hpp>

// ----------------------------------------------------------------
// 1. Raw Syscall Wrappers (Using your x86_64 inline asm)
// ----------------------------------------------------------------

// Make sure these match your kernel's syscall numbers
#define SYS_READ        0
#define SYS_WRITE       1
#define SYS_SLEEP       2
#define SYS_PROC_DUMP   3
#define SYS_ANON_ALLOC  4
#define SYS_ANON_FREE   5
#define SYS_TCB_SET     6   /* set FS base for TLS (mlibc sys_tcb_set) */
#define SYS_EXIT        7   /* terminate current process */


extern "C" int main(int argc, char **argv);
// .tbss is 0x1a0 bytes — allocate that before the TCB
// TLS layout on x86_64: [TLS block][TCB]
// FS points to TCB, TLS accessed at FS - offset

#define TLS_SIZE 0x200  // slightly larger than 0x1a0 for safety

static uint8_t tls_storage[TLS_SIZE + sizeof(Tcb)];

extern "C" void __mlibc_start_main(uintptr_t *sp) {
    // Set up TLS/TCB
    memset(tls_storage, 0, sizeof(tls_storage));
    Tcb *tcb = reinterpret_cast<Tcb*>(tls_storage + TLS_SIZE);
    tcb->selfPointer = tcb;
    mlibc::sys_tcb_set(tcb);

    // Run constructors (parse_exec_stack runs here, needs valid stack)
    extern void (*__CTOR_LIST__[])();
    extern void (*__CTOR_END__[])();
    for (void (**ctor)() = __CTOR_LIST__; ctor < __CTOR_END__; ctor++)
        (*ctor)();

    // Parse argc/argv from initial stack
    int argc = (int)*sp;
    char **argv = (char**)(sp + 1);

    int ret = main(argc, argv);
    mlibc::sys_exit(ret);
    __builtin_unreachable();
}


static inline long syscall1(long n, long a1) {
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "r10", "r8", "r9", "memory");
    return ret;
}

static inline long syscall2(long n, long a1, long a2) {
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "r10", "r8", "r9", "memory");
    return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "r10", "r8", "r9", "memory");
    return ret;
}

// ----------------------------------------------------------------
// 2. mlibc Required Sysdeps
// ----------------------------------------------------------------



namespace mlibc {

// --- Panic & Logging (Crucial for debugging early boot) ---

void sys_libc_log(const char *message) {
    // Write directly to stdout (FD 1). 
    // We calculate length manually since we can't use strlen yet.
    size_t len = 0;
    while (message[len]) len++;
    syscall3(SYS_WRITE, 1, (long)message, len);
}

void sys_libc_panic() {
    sys_libc_log("\n[mlibc] FATAL PANIC! Halting user process.\n");
    syscall1(SYS_EXIT, 1);
    while (1); // Should never reach here
}

// --- Memory Management ---

int sys_anon_allocate(size_t size, void **pointer) {
    long ret = syscall1(SYS_ANON_ALLOC, size);
    if (ret < 0) {
        return -ret; // Return positive errno
    }
    *pointer = (void*)ret;
    return 0; // 0 means success
}

int sys_anon_free(void *pointer, size_t size) {
    long ret = syscall2(SYS_ANON_FREE, (long)pointer, size);
    if (ret < 0) return -ret;
    return 0;
}

// --- Threading & Execution ---

int sys_tcb_set(void *pointer) {
    long ret = syscall1(SYS_TCB_SET, (long)pointer);
    if (ret < 0) return -ret;
    return 0;
}

void sys_exit(int status) {
    syscall1(SYS_EXIT, status);
    while (1);
}

// --- Basic I/O ---

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
    long ret = syscall3(SYS_READ, fd, (long)buf, count);
    if (ret < 0) {
        return -ret;
    }
    *bytes_read = ret;
    return 0;
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
    long ret = syscall3(SYS_WRITE, fd, (long)buf, count);
    if (ret < 0) {
        return -ret;
    }
    *bytes_written = ret;
    return 0;
}

// ----------------------------------------------------------------
// 3. Stubbing out the rest
// ----------------------------------------------------------------
// mlibc will compile against these but fail safely if called.

int sys_open(const char *pathname, int flags, mode_t mode, int *fd) {
    sys_libc_log("[mlibc stub] sys_open called\n");
    return ENOSYS;
}

int sys_close(int fd) {
    return ENOSYS;
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
    return ENOSYS;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd, off_t offset, void **window) {
    sys_libc_log("[mlibc stub] sys_vm_map called\n");
    return ENOSYS; // You will implement this later for file-backed mapping
}

int sys_vm_unmap(void *pointer, size_t size) {
    long ret = syscall2(SYS_ANON_FREE, (long)pointer, size);
    if (ret < 0) return -ret;
    return 0;
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
    (void)pointer; (void)expected; (void)time;
    return ENOSYS;
}
int sys_futex_wake(int *pointer) {
    (void)pointer;
    return ENOSYS;
}

int sys_clock_get(int clock, time_t *secs, long *nanos) {
    *secs = 0;
    *nanos = 0;
    return 0;
}

int sys_ioctl(int fd, unsigned long request, void *arg, int *result) {
    (void)fd; (void)request; (void)arg; (void)result;
    return ENOSYS;
}

// Add any other functions mlibc complains about during linking as ENOSYS stubs here...

int sys_isatty(int fd) {
    if (fd == 0 || fd == 1 || fd == 2)
        return 0; // 0 = yes, it is a tty
    return ENOTTY;
}



} // namespace mlibc

// Outside the mlibc namespace
extern "C" int ioctl(int fd, unsigned long request, ...) {
    (void)fd; (void)request;
    return -ENOSYS;
}