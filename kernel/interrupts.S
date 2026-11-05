.section .text

.macro ISR_NO_ERROR number
.global isr\number
.type isr\number, @function
isr\number:
    pushq $0
    pushq $\number
    jmp interrupt_common_stub
.endm

.macro ISR_ERROR number
.global isr\number
.type isr\number, @function
isr\number:
    pushq $\number
    jmp interrupt_common_stub
.endm

.macro IRQ number, vector
.global irq\number
.type irq\number, @function
irq\number:
    pushq $0
    pushq $\vector
    jmp interrupt_common_stub
.endm

.global interrupt_common_stub
.type interrupt_common_stub, @function
interrupt_common_stub:
    cld
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, %r12
    movq %rsp, %rdi
    andq $-16, %rsp
    call interrupt_dispatch
    movq %r12, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rax
    addq $16, %rsp
    iretq

ISR_NO_ERROR 0
ISR_NO_ERROR 1
ISR_NO_ERROR 2
ISR_NO_ERROR 3
ISR_NO_ERROR 4
ISR_NO_ERROR 5
ISR_NO_ERROR 6
ISR_NO_ERROR 7
ISR_ERROR 8
ISR_NO_ERROR 9
ISR_ERROR 10
ISR_ERROR 11
ISR_ERROR 12
ISR_ERROR 13
ISR_ERROR 14
ISR_NO_ERROR 15
ISR_NO_ERROR 16
ISR_ERROR 17
ISR_NO_ERROR 18
ISR_NO_ERROR 19
ISR_NO_ERROR 20
ISR_ERROR 21
ISR_ERROR 22
ISR_ERROR 23
ISR_ERROR 24
ISR_ERROR 25
ISR_ERROR 26
ISR_ERROR 27
ISR_ERROR 28
ISR_ERROR 29
ISR_ERROR 30
ISR_NO_ERROR 31

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
