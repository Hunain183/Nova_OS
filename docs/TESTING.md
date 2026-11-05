# NovaOS Testing

## Current results

The Milestone 3 kernel builds with Clang, GNU ld, and GNU Make without
warnings. `make verify` builds the Limine ISO and boots it in headless QEMU;
the serial log confirms serial setup and interrupt initialization. Interactive
keyboard, timer commands, and deliberate exception testing require a VM test.

## Milestone 3 procedure

After installing the dependencies in [BUILDING.md](BUILDING.md):

1. Run `make clean && make kernel` and confirm the ELF links without warnings.
2. Run `make iso` and confirm `build/NovaOS.iso` exists.
3. Run `make run` and confirm the guest displays `NovaOS>`.
4. Attach the same ISO to a VMware x86-64 virtual machine.
5. Test `help`, `clear`, `about`, `version`, `echo Hello NovaOS`, and
	`echo This is a test`; verify spaces, Enter, and Backspace.
6. Run `ticks` twice and confirm the values increase; run `uptime` after a
	delay and confirm seconds increase.
7. Run `exceptions` and `inttest` and confirm they return to the prompt.
8. Test `reboot` next because it intentionally restarts the VM.
9. Boot again and run `exception-test` last; confirm the exception report
	shows Divide Error, vector 0, an error code, an instruction pointer, and a
	halted kernel.

The kernel output is written by NovaOS directly to the Limine framebuffer or
VGA text memory. A Linux installation or desktop application inside the guest
is not involved.