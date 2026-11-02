# Portable Windows Build

The cross-toolchain is installed outside the repository under
`%LOCALAPPDATA%\mini-os-toolchains` and is not added to VS Code.

1. Run `powershell -ExecutionPolicy Bypass -File .\tools\setup-portable-toolchain.ps1`.
   This downloads the Windows `i686-elf` GCC 15.2.0 archive (about 888 MB),
   checks its published SHA-256, and extracts it.
2. Install GNU Make and NASM separately, then add their `bin` directories to
   `PATH`. The repository's `set-env.ps1` can be updated with those paths if
   you want them loaded in each PowerShell session.
3. Run `make` to build the kernel, or `make iso` to build the test ISO.

The ISO is written to `dist\mini-os.iso`. To use a toolchain extracted
elsewhere, pass its root with `-ToolchainPath` to
`tools\build-windows.ps1`. Running `make` builds only the kernel; it does not
create a disk image or ISO.