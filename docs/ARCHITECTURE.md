# NovaOS Architecture

## Milestone 1 boot path

The firmware loads Limine from the ISO. Limine reads `boot/limine.conf`, loads
`boot/NovaOS.elf`, satisfies the requests embedded in the ELF, and transfers
control to `kmain`. The kernel uses the Limine HHDM response to translate the
physical VGA text buffer address `0xb8000` into the mapped address it writes.

The kernel is linked at the fixed x86-64 higher-half base
`0xffffffff80000000`, as required for non-relocatable Limine ELF images. Its
loadable segments use page alignment and separate executable/read-only
permissions. Limine allocates the physical image and maps it at the linked
higher-half virtual addresses. The kernel uses the x86-64 System V ABI expected
by the Limine protocol. The compiler emits no libc calls, uses no Linux
headers, and does not enable the x86-64 red zone because later interrupt
handlers will need the stack to remain safe across asynchronous entry.

## Module boundaries

- `kernel/main.c`: first kernel entry and VGA milestone output.
- `include/limine.h`: only the Limine protocol declarations currently needed.
- `linker.ld`: ELF entry point, load address, and retained request sections.
- `Makefile`: freestanding compilation, Limine asset retrieval, and ISO layout.

Future architecture-specific CPU, interrupt, memory, driver, shell, and
graphics modules will be added only after this boot checkpoint is verified.