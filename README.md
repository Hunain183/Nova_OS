# NovaOS

NovaOS is an educational x86-64 operating system built as a freestanding
kernel and booted by Limine. The project is intentionally being developed in
small milestones; the current checkpoint is the first bootable kernel.

## Current checkpoint

Milestone 1 provides:

- A freestanding x86-64 C kernel linked as an ELF executable.
- A Limine protocol base-revision request and HHDM request.
- A direct kernel entry point that displays `Welcome to NovaOS` in VGA text
	memory.
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

The current container lacks `xorriso` and QEMU, so ISO creation and runtime
testing must be performed after those tools are installed. `make kernel` is the
tool-independent compile and link check.

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
x86-64 VMware virtual machine, and boot the VM. For the current VGA-text
milestone, configure the VM for legacy BIOS mode and verify that it displays
`Welcome to NovaOS`. The workflow builds and packages the ISO; it does not
claim VMware or QEMU runtime success.