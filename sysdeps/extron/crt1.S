.file "crt1.S"
.section .text
.global _start
.type _start, @function
_start:
    xorq  %rbp, %rbp
    movq  %rsp, %rdi       # pass initial stack pointer
    andq  $~15, %rsp
    call  __mlibc_start_main
    ud2
    
.size _start, . - _start