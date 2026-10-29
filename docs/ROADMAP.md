# NovaOS Roadmap

- [x] Milestone 1 source: Limine entry, linker script, welcome message, ISO
  recipe.
- [x] Milestone 1 runtime verification in QEMU (automated); VMware remains a
  manual compatibility target.
- [x] Milestone 2 terminal, keyboard driver, bounded shell parser.
- [x] Milestone 3 basic IRQ setup for keyboard input.
- [ ] Milestone 4 physical-page allocator and kernel heap.
- [ ] Milestone 5 user mode, processes, cooperative tasks, then a scheduler.
- [ ] Milestone 6 persistent filesystem and file-backed shell commands.
- [ ] Milestone 7 system calls, applications, and process isolation.
- [ ] Milestone 8 networking, graphics, and optional desktop shell.
- [ ] Milestone 9 integration test report and final documentation.

Every milestone must pass `make verify` before it is considered complete.