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

The kernel shell accepts `help`, `clear`, `echo <text>`, `info`, and `halt`.
Serial diagnostics use COM1 when available but do not prevent boot when no
virtual serial device is configured.

Run the complete local gate with:

```sh
make verify
```

This rebuilds the kernel and ISO, boots the ISO in headless QEMU, and sends the
boot and interrupt initialization checks through the serial log. It requires
`xorriso` and `qemu-system-x86_64`; the same gate runs in GitHub Actions.
Interactive keyboard commands should be tested in VMware until a deterministic
automated input backend is added.