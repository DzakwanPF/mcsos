.section .text
.global x86_64_syscall_int80_stub
.type   x86_64_syscall_int80_stub, @function
.extern mcsos_syscall_dispatch_frame

x86_64_syscall_int80_stub:
    cld
    subq $64, %rsp
    movq %rax,  0(%rsp)
    movq %rdi,  8(%rsp)
    movq %rsi, 16(%rsp)
    movq %rdx, 24(%rsp)
    movq %r10, 32(%rsp)
    movq %r8,  40(%rsp)
    movq %r9,  48(%rsp)
    movq $0,   56(%rsp)
    movq %rsp, %rdi
    call mcsos_syscall_dispatch_frame
    movq 56(%rsp), %rax
    addq $64, %rsp
    iretq
.size x86_64_syscall_int80_stub, . - x86_64_syscall_int80_stub
