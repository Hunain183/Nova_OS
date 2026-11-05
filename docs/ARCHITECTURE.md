# NovaOS Architecture

## Milestone 2 boot path

The firmware loads Limine from the ISO. Limine reads `boot/limine.conf`, loads
`boot/NovaOS.elf`, satisfies the requests embedded in the ELF, and transfers
control to `kmain`. The kernel requests both HHDM and framebuffer responses. It renders a small
interactive terminal into a 32-bit RGB framebuffer when one is available,
which supports UEFI VMware boots. BIOS boots without a framebuffer fall back
to the HHDM-mapped VGA text buffer at `0xb8000`. The terminal supports
newlines, scrolling, backspace, and clearing. Its shell accepts `help`,
`clear`, `about`, `version`, `echo`, `reboot`, and `shutdown`.

The kernel is linked at the fixed x86-64 higher-half base
`0xffffffff80000000`, as required for non-relocatable Limine ELF images. Its
loadable segments use page alignment and separate executable/read-only
permissions. Limine allocates the physical image and maps it at the linked
higher-half virtual addresses. The kernel uses the x86-64 System V ABI expected
by the Limine protocol. The compiler emits no libc calls, uses no Linux
headers, and does not enable the x86-64 red zone because later interrupt
handlers will need the stack to remain safe across asynchronous entry.

## Module boundaries

- `kernel/main.c`: kernel entry, serial output, and PIC/IDT setup.
- `kernel/terminal.c`: framebuffer terminal rendering, VGA fallback, scrolling,
  and clearing.
- `kernel/interrupts.S`: keyboard IRQ entry stub.
- `drivers/keyboard.c`: bounded PS/2 controller setup, scancode translation,
  and keyboard interrupt dispatch.
- `shell/shell.c`: fixed-size command input and shell command execution.
- `include/limine.h`, `include/terminal.h`, `include/keyboard.h`, and
  `include/shell.h`: protocol and module interfaces.
- `linker.ld`: ELF entry point, load address, and retained request sections.
- `Makefile`: freestanding compilation, Limine asset retrieval, and ISO layout.

Future architecture-specific CPU, interrupt, memory, driver, shell, and
graphics modules will be added only after this boot checkpoint is verified.