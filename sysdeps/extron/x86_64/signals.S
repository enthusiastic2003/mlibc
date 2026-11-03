.section .text
.global __mlibc_signal_restore
.type __mlibc_signal_restore, @function
__mlibc_signal_restore:
	mov $38, %eax              // SYS_SIGRETURN
	syscall
	ud2

// Cancellation-aware syscall: the (begin, end) labels let the signal path
// tell whether a thread was interrupted before the syscall instruction ran.
.global __mlibc_do_asm_cp_syscall
.global __mlibc_syscall_begin
.global __mlibc_syscall_end
.type __mlibc_do_asm_cp_syscall, @function
__mlibc_do_asm_cp_syscall:
	/* rdi=number, rsi/rdx/rcx = args 1-3 (cp_syscall3). */
	mov %rdi, %rax
	mov %rsi, %rdi
	mov %rdx, %rsi
	mov %rcx, %rdx
	mov %fs:0x30, %r11         // Tcb::cancelBits
__mlibc_syscall_begin:
	and $((1 << 0) | (1 << 2)), %r11
	cmp $((1 << 0) | (1 << 2)), %r11
	je 1f
	syscall
__mlibc_syscall_end:
	ret
1:
	call __mlibc_do_cancel
	hlt

.section .note.GNU-stack,"",@progbits
