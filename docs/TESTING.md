# NovaOS Testing

## Current results

The modular Milestone 2 kernel builds with Clang, GNU ld, and GNU Make without
warnings. `make verify` builds the Limine ISO and boots it in headless QEMU;
the serial log confirms both serial setup and interrupt initialization. The
framebuffer terminal path and keyboard command behavior still require an
interactive VMware test.

## Milestone 2 procedure

After installing the dependencies in [BUILDING.md](BUILDING.md):

1. Run `make clean && make kernel` and confirm the ELF links without warnings.
2. Run `make iso` and confirm `build/NovaOS.iso` exists.
3. Run `make run` and confirm the guest displays `NovaOS>`.
4. Attach the same ISO to a VMware x86-64 virtual machine.
5. Test `help`, `clear`, `about`, `version`, `echo hello`, and `shutdown`.
6. Test `reboot` last because it intentionally restarts the VM.

The kernel output is written by NovaOS directly to the Limine framebuffer or
VGA text memory. A Linux installation or desktop application inside the guest
is not involved.