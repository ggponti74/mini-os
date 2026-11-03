## mini-os

Test with <https://nadchif.github.io/in-browser-virtual-machine/>

### Newlib smoke test

Build the optional i686-elf GCC/Newlib toolchain, then build the sample and
refresh the initrd:

```sh
bash tools/build_i686_elf_newlib.sh
make newlib-smoke
make iso
```

The sample is added to the initrd when present and exercises Newlib `printf`,
`sbrk`, `malloc`, `calloc`, `realloc`, and the supported file syscalls. Flat
applications receive a separate heap beginning after their linked image and
BSS, bounded by the reserved application memory region. This keeps Newlib
allocations out of the kernel heap. `make newlib-smoke` builds the executable
and refreshes the initrd; `make iso` then builds the test ISO with the sample
included. Run `newlib-smoke` from the OS shell to return to the prompt after it
completes. This validates a narrow static Newlib target; it is not full POSIX
conformance or a general ELF process loader.
