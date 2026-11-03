// x86-64 crt1 for Extron: the kernel (kernel/proc/exec.c build_arg_stack())
// enters _start with a SysV x86-64 process stack: argc at (%rsp), argv[],
// NULL, envp[], NULL, auxv. Hand the raw stack pointer to __mlibc_start_main(),
// which calls __dlapi_enter() before touching anything else.
.file "crt1.S"
.section .text
.global _start
.type _start, @function
_start:
    xor  %ebp, %ebp              // terminate the frame-pointer chain
    mov  %rsp, %rdi              // raw stack pointer, per mlibc's docs
    lea  main(%rip), %rsi        // reference main so its archive member is pulled in
    and  $-16, %rsp
    call __mlibc_start_main
    hlt
.size _start, . - _start
.section .note.GNU-stack,"",@progbits
