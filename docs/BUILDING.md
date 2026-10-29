# Building NovaOS

## Ubuntu dependencies

Install the host tools with the Ubuntu package manager:

```sh
sudo apt update
sudo apt install clang lld make curl xorriso qemu-system-x86
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