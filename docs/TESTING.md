# NovaOS Testing

## Current results

The repository was inspected on 2026-10-29. Clang 18, GCC 13, GNU Make, and
GNU ld are present. Network access to the official Limine repository works.
`make clean && make kernel` passed with no warnings. The resulting file is an
ELF64 x86-64 executable with entry point `0xffffffff80001080`; its two
nonempty `PT_LOAD` segments are in the fixed higher half at
`0xffffffff80000000` and `0xffffffff80002000`, with `0x1000` alignment. Its
Limine request sections were confirmed with `readelf` and `nm`.

`make iso` downloaded the pinned Limine v12.9.6 package and built its host tool,
then stopped because `xorriso` is not installed. `qemu-system-x86_64` is also
not installed. Therefore no ISO or runtime boot result is claimed from this
container yet.

## Milestone 1 procedure

After installing the dependencies in [BUILDING.md](BUILDING.md):

1. Run `make clean && make kernel` and confirm the ELF links without warnings.
2. Run `make iso` and confirm `build/NovaOS.iso` exists.
3. Run `make run` and confirm the guest displays `Welcome to NovaOS`.
4. Attach the same ISO to a VMware x86-64 virtual machine and record the
   firmware mode and result.

The kernel output is written by NovaOS directly to VGA text memory. A Linux
installation or desktop application inside the guest is not involved.