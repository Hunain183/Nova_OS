# Building NovaOS

## Ubuntu dependencies

Install the host tools with the Ubuntu package manager:

```sh
sudo apt update
sudo apt install build-essential binutils clang curl file xorriso qemu-system-x86
```

VMware is an additional test environment and is not required for compilation.
Do not write an ISO to a physical disk.

## Build commands

From the repository root:

```sh
make clean
make kernel
make iso
make run
```

`make iso` downloads the pinned Limine v12.9.6 binary package into `build/`,
compiles its portable host tool, creates the hybrid BIOS/UEFI ISO with
`xorriso`, and runs `limine bios-install` on the generated image. The expected
artifact is `build/NovaOS.iso`.

`make debug` starts QEMU paused with a GDB server on TCP port 1234. QEMU is not
silently substituted for VMware; `make run` is only an additional emulator
check.

The kernel shell prompt is `NovaOS>`. It accepts `help`, `clear`, `about`,
`version`, `echo <text>`, `reboot`, `shutdown`, `ticks`, `uptime`,
`meminfo`, `memmap`, `alloc`, `free`, `ps`, `tasktest`, `scheduler`, `kill`,
`exceptions`, and `inttest`. `exception-test` is a destructive test: it
deliberately triggers divide error and halts the kernel, so run it last.
Use the keyboard letters, number row, space, Enter, Backspace, and Shift keys.
Serial diagnostics use COM1 when available but do not prevent boot when no
virtual serial device is configured.

Run the complete local gate with:

```sh
make verify
```

This rebuilds the kernel and ISO, boots the ISO in headless QEMU, and sends the
boot and interrupt initialization checks through the serial log. It requires
`xorriso` and `qemu-system-x86_64`; the same gate runs in GitHub Actions.
To test interactively in VMware, attach `build/NovaOS.iso` as the VM's CD/DVD,
boot it, and type `help`, `about`, `version`, `echo Hello NovaOS`,
`echo This is a test`, `ticks`, `uptime`, `exceptions`, and `inttest`.
Confirm that the tick count increases, uptime eventually increases, spaces are
preserved, and Backspace still works. Then test `meminfo`, `memmap`, `alloc`,
`free`, `tasktest`, `ps`, `scheduler`, and `kill` with an eligible task ID.
Test `reboot` next and `exception-test` last because they intentionally restart
or halt the VM.