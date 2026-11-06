# NovaOS Architecture

## Milestones 3-5 boot, memory, and scheduling path

The firmware loads Limine from the ISO. Limine reads `boot/limine.conf`, loads
`boot/NovaOS.elf`, satisfies the requests embedded in the ELF, and transfers
control to `kmain`. The kernel requests both HHDM and framebuffer responses. It renders a small
interactive terminal into a 32-bit RGB framebuffer when one is available,
which supports UEFI VMware boots. BIOS boots without a framebuffer fall back
to the HHDM-mapped VGA text buffer at `0xb8000`. The terminal supports
newlines, scrolling, backspace, and clearing. Its shell also exposes timer
status, exception information, and interrupt checks.

Limine supplies the initial flat 64-bit code and data descriptors, so NovaOS
uses the bootloader's valid GDT rather than reloading segment selectors during
early boot. `kernel/interrupts.c` creates and loads the IDT using the current
code selector. CPU exceptions use vectors 0 through 31 and hardware IRQs use
the remapped PIC vectors 32 through 47.

`kernel/interrupts.S` normalizes exception error codes and saves all general
purpose registers before calling the C dispatcher. The dispatcher routes IRQ0
to the PIT timer and IRQ1 to the keyboard, then sends the appropriate PIC EOI.
`drivers/timer.c` programs PIT channel 0 for 100 Hz and maintains a 64-bit
tick counter. Exception handlers print the vector, name, error code, and RIP,
then halt because continuing after an unrecoverable CPU exception is unsafe.

The Limine memory-map response is initialized before the terminal or interrupts.
`mm/pmm.c` uses a 4 KiB bitmap: all pages begin reserved, only
`LIMINE_MEMMAP_USABLE` ranges are released, and the bitmap's own pages are
reserved again. Bootloader, executable/module, framebuffer, ACPI, and other
non-usable regions therefore cannot be allocated. The bitmap is accessed
through the Limine HHDM; no new paging or virtual-memory manager is introduced.

`sched/scheduler.c` implements cooperative round-robin scheduling. Each task
gets one PMM-allocated page through the HHDM and starts from a synthetic,
ABI-aligned context frame. `kernel/context.S` switches the callee-saved
registers and stack pointer. The timer remains an accounting device only;
tasks call `scheduler_yield`, so this milestone does not claim preemptive
scheduling or timer-driven task switches.

The kernel is linked at the fixed x86-64 higher-half base
`0xffffffff80000000`, as required for non-relocatable Limine ELF images. Its
loadable segments use page alignment and separate executable/read-only
permissions. Limine allocates the physical image and maps it at the linked
higher-half virtual addresses. The kernel uses the x86-64 System V ABI expected
by the Limine protocol. The compiler emits no libc calls, uses no Linux
headers, and does not enable the x86-64 red zone because later interrupt
handlers will need the stack to remain safe across asynchronous entry.

## Module boundaries

- `kernel/main.c`: kernel entry and serial output.
- `kernel/terminal.c`: framebuffer terminal rendering, VGA fallback, scrolling,
  and clearing.
- `kernel/interrupts.c`: IDT setup, exception reporting, and IRQ dispatch.
- `kernel/interrupts.S`: exception and hardware IRQ entry stubs.
- `drivers/pic.c`: legacy 8259 PIC remapping, masking, and EOI signaling.
- `drivers/timer.c`: PIT setup and system tick accounting.
- `mm/pmm.c`: Limine memory-map validation and bitmap physical page allocator.
- `sched/scheduler.c`: task control blocks, cooperative round-robin scheduling,
  PMM-backed stacks, and demonstration tasks.
- `kernel/context.S`: callee-saved register and stack context switching.
- `drivers/keyboard.c`: bounded PS/2 setup, explicit set-1 scancode
  translation, and keyboard input dispatch.
- `shell/shell.c`: fixed-size command input and shell command execution.
- `include/limine.h`, `include/interrupts.h`, `include/pic.h`,
  `include/timer.h`, `include/pmm.h`, `include/scheduler.h`,
  `include/terminal.h`, `include/keyboard.h`, and `include/shell.h`: protocol
  and module interfaces.
- `linker.ld`: ELF entry point, load address, and retained request sections.
- `Makefile`: freestanding compilation, Limine asset retrieval, and ISO layout.

The `exception-test` shell command deliberately triggers divide error and
halts the kernel. It is never invoked during normal boot.