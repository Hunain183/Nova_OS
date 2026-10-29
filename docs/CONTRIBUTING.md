# Contributing to NovaOS

Keep `main` buildable. Use feature branches for independent kernel, graphics,
or documentation work, and keep shared headers and interfaces small.

Before opening a change, run `make clean && make kernel`. Once host tools are
installed, also run `make iso`; runtime changes should be checked in QEMU and
VMware where practical. Do not add host-only APIs, Linux kernel headers, or
code that writes to physical disks.