## mini-os

Test with <https://nadchif.github.io/in-browser-virtual-machine/>

### Newlib smoke test

Build the optional i686-elf GCC/Newlib toolchain, then build and boot the sample:

```sh
bash tools/build_i686_elf_newlib.sh
make newlib-smoke
make run
```

The sample is added to the initrd when present and exercises Newlib `printf`,
`malloc`, and the supported file syscalls. Run `newlib-smoke` from the shell
to return to the prompt after it completes. This validates a narrow static
Newlib target; it is not full POSIX conformance or a general ELF process loader.
