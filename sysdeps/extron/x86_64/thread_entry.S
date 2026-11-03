.section .text
// A new thread starts here with %rsp -> { entry, user_arg, tcb, 0 }
// (prepared by sys_prepare_stack()).
.global __mlibc_start_thread
.type __mlibc_start_thread, @function
__mlibc_start_thread:
	xor %ebp, %ebp
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rax
	call __mlibc_enter_thread
	hlt

.section .note.GNU-stack,"",@progbits
