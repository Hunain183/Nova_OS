#include <stdint.h>

#include "interrupts.h"
#include "keyboard.h"
#include "pic.h"
#include "terminal.h"
#include "timer.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];

#define DECLARE_ISR(number) extern void isr##number(void)
#define DECLARE_IRQ(number) extern void irq##number(void)

DECLARE_ISR(0); DECLARE_ISR(1); DECLARE_ISR(2); DECLARE_ISR(3);
DECLARE_ISR(4); DECLARE_ISR(5); DECLARE_ISR(6); DECLARE_ISR(7);
DECLARE_ISR(8); DECLARE_ISR(9); DECLARE_ISR(10); DECLARE_ISR(11);
DECLARE_ISR(12); DECLARE_ISR(13); DECLARE_ISR(14); DECLARE_ISR(15);
DECLARE_ISR(16); DECLARE_ISR(17); DECLARE_ISR(18); DECLARE_ISR(19);
DECLARE_ISR(20); DECLARE_ISR(21); DECLARE_ISR(22); DECLARE_ISR(23);
DECLARE_ISR(24); DECLARE_ISR(25); DECLARE_ISR(26); DECLARE_ISR(27);
DECLARE_ISR(28); DECLARE_ISR(29); DECLARE_ISR(30); DECLARE_ISR(31);
DECLARE_IRQ(0); DECLARE_IRQ(1); DECLARE_IRQ(2); DECLARE_IRQ(3);
DECLARE_IRQ(4); DECLARE_IRQ(5); DECLARE_IRQ(6); DECLARE_IRQ(7);
DECLARE_IRQ(8); DECLARE_IRQ(9); DECLARE_IRQ(10); DECLARE_IRQ(11);
DECLARE_IRQ(12); DECLARE_IRQ(13); DECLARE_IRQ(14); DECLARE_IRQ(15);

static const char *exception_names[] = {
    "Divide Error", "Debug", "Non-Maskable Interrupt", "Breakpoint",
    "Overflow", "Bound Range Exceeded", "Invalid Opcode",
    "Device Not Available", "Double Fault", "Coprocessor Segment Overrun",
    "Invalid TSS", "Segment Not Present", "Stack-Segment Fault",
    "General Protection Fault", "Page Fault", "Reserved",
    "x87 Floating-Point Exception", "Alignment Check", "Machine Check",
    "SIMD Floating-Point Exception", "Virtualization Exception",
    "Control Protection Exception", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Hypervisor Injection Exception",
    "VMM Communication Exception", "Security Exception", "Reserved",
};

static void idt_set_gate(uint8_t vector, void (*handler)(void),
                         uint16_t selector) {
    uint64_t address = (uint64_t)handler;
    idt[vector].offset_low = (uint16_t)address;
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].attributes = 0x8e;
    idt[vector].offset_mid = (uint16_t)(address >> 16);
    idt[vector].offset_high = (uint32_t)(address >> 32);
    idt[vector].reserved = 0;
}

static void write_decimal(uint64_t value) {
    char digits[20];
    uint8_t length = 0;
    if (value == 0) {
        terminal_putchar('0');
        return;
    }
    while (value != 0) {
        digits[length++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (length != 0) {
        terminal_putchar(digits[--length]);
    }
}

static void write_hex(uint64_t value) {
    static const char digits[] = "0123456789ABCDEF";
    terminal_write("0x");
    for (int8_t shift = 60; shift >= 0; shift -= 4) {
        terminal_putchar(digits[(value >> shift) & 0xf]);
    }
}

static void exception_halt(const struct interrupt_frame *frame) {
    terminal_write("\nCPU EXCEPTION: ");
    terminal_write(interrupt_exception_name(frame->vector));
    terminal_write(" (#");
    write_decimal(frame->vector);
    terminal_write(")\nError code: ");
    write_hex(frame->error_code);
    terminal_write("\nInstruction pointer: ");
    write_hex(frame->rip);
    terminal_write("\nSystem halted.\n");
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

const char *interrupt_exception_name(uint64_t vector) {
    if (vector < sizeof(exception_names) / sizeof(exception_names[0])) {
        return exception_names[vector];
    }
    return "Unknown Exception";
}

void interrupt_dispatch(struct interrupt_frame *frame) {
    if (frame->vector < 32) {
        exception_halt(frame);
    }
    if (frame->vector >= 32 && frame->vector < 48) {
        uint8_t irq = (uint8_t)(frame->vector - 32);
        if (irq == 0) {
            timer_interrupt();
        } else if (irq == 1) {
            keyboard_interrupt_handler();
        }
        pic_send_eoi(irq);
    }
}

void interrupt_test_divide_error(void) {
    __asm__ volatile (
        "xor %%eax, %%eax\n"
        "xor %%edx, %%edx\n"
        "xor %%ecx, %%ecx\n"
        "div %%ecx"
        :
        :
        : "rax", "rcx", "rdx");
}

int interrupts_init(void) {
    void (*exceptions[])(void) = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
        isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
    };
    void (*irqs[])(void) = {
        irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
        irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15,
    };
    uint16_t code_segment;
    __asm__ volatile ("mov %%cs, %0" : "=r"(code_segment));

    for (uint16_t index = 0; index < 256; index++) {
        idt[index] = (struct idt_entry){0};
    }
    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_gate(vector, exceptions[vector], code_segment);
    }
    for (uint8_t irq = 0; irq < 16; irq++) {
        idt_set_gate((uint8_t)(32 + irq), irqs[irq], code_segment);
    }
    struct idt_pointer pointer = {
        .limit = sizeof(idt) - 1,
        .base = (uint64_t)idt,
    };
    __asm__ volatile ("lidt %0" : : "m"(pointer));

    pic_initialize();
    timer_initialize();
    if (!keyboard_initialize()) {
        return 0;
    }
    __asm__ volatile ("sti");
    return 1;
}