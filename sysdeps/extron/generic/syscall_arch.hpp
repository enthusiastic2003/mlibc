#ifndef EXTRON_SYSCALL_ARCH_HPP
#define EXTRON_SYSCALL_ARCH_HPP

// Raw Extron syscall wrappers. Convention (kernel/arch/*/include/arch/
// exceptions.h): syscall number + up to three arguments, result in the first
// argument register.
//   aarch64: x8 = number, x0-x2 = args, `svc #0`
//   x86-64 : rax = number, rdi/rsi/rdx = args, `syscall` (clobbers rcx, r11)
// Self-contained on purpose: ld.so uses these before libc exists.

#if defined(__aarch64__)

static inline long extron_syscall0(long n) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0");
    __asm__ volatile ("svc #0" : "=r"(x0) : "r"(x8) : "memory", "cc");
    return x0;
}
static inline long extron_syscall1(long n, long a1) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x8) : "memory", "cc");
    return x0;
}
static inline long extron_syscall2(long n, long a1, long a2) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x8), "r"(x1) : "memory", "cc");
    return x0;
}
static inline long extron_syscall3(long n, long a1, long a2, long a3) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a1;
    register long x1 __asm__("x1") = a2;
    register long x2 __asm__("x2") = a3;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory", "cc");
    return x0;
}

// AArch64 TLS (variant I): the ABI thread pointer is 16 bytes before the TLS
// block, i.e. just below the end of the Tcb.
#define EXTRON_TP_FROM_TCB(tcb, tcb_size) (reinterpret_cast<char *>(tcb) + (tcb_size) - 0x10)

#elif defined(__x86_64__)

static inline long extron_syscall0(long n) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n) : "rcx", "r11", "memory", "cc");
    return r;
}
static inline long extron_syscall1(long n, long a1) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1) : "rcx", "r11", "memory", "cc");
    return r;
}
static inline long extron_syscall2(long n, long a1, long a2) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2) : "rcx", "r11", "memory", "cc");
    return r;
}
static inline long extron_syscall3(long n, long a1, long a2, long a3) {
    long r;
    __asm__ volatile ("syscall" : "=a"(r) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory", "cc");
    return r;
}

// x86-64 TLS (variant II): FS base points at the Tcb itself (its first word
// is the self pointer), with the TLS block laid out below it.
#define EXTRON_TP_FROM_TCB(tcb, tcb_size) (reinterpret_cast<char *>(tcb))

#else
#error "unsupported architecture for the extron sysdeps"
#endif

#endif
