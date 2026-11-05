# NovaOS

NovaOS is an educational x86-64 operating system built as a freestanding
kernel and booted by Limine. The project is intentionally being developed in small milestones; the current
checkpoint is the second milestone with a bootable terminal shell.

## Current checkpoint

Milestone 2 provides:

- A freestanding x86-64 C kernel linked as an ELF executable.
- A Limine protocol base-revision request and HHDM request.
- A framebuffer terminal with VGA fallback, scrolling, and screen clearing.
- A PIC/IDT PS/2 keyboard driver with Enter, Backspace, Space, and Shift
  support.
- An interactive shell with `help`, `clear`, `about`, `version`, `echo`,
  `reboot`, and `shutdown` commands.
- A reproducible `build/NovaOS.iso` target using pinned Limine v12.9.6 assets.

The ISO is meant to run directly on virtual hardware. It does not require a
Linux installation inside the guest.

## Quick start

On Ubuntu, install the host tools listed in [docs/BUILDING.md](docs/BUILDING.md),
then run:

```sh
make iso
make run
```

`make kernel` is the tool-independent compile and link check. `make verify`
also builds the ISO and checks boot initialization in headless QEMU.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the boot design and
[docs/ROADMAP.md](docs/ROADMAP.md) for milestone status.

## GitHub Actions ISO build

The workflow at `.github/workflows/build-iso.yml` builds the kernel and runs
the repository's `make iso` target on Ubuntu 24.04 for every push to `main` and
every pull request targeting `main`. It also supports manual execution.

To start a manual build:

1. Open the repository on GitHub and select **Actions**.
2. Select **Build NovaOS ISO**.
3. Select **Run workflow**, choose the branch, and confirm.

After a successful run, open the workflow run and download the
`NovaOS-ISO` artifact. Extract `NovaOS.iso`, attach it as a CD/DVD image to an
x86-64 VMware virtual machine, and boot the VM. Verify that it displays
`NovaOS>`, then type `help` to list the supported commands. The workflow builds
and packages the ISO; local `make verify` additionally checks boot
initialization in QEMU but does not exercise interactive VMware input.