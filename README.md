## mini-os

Test with <https://nadchif.github.io/in-browser-virtual-machine/>

QEMU launch targets configure an E1000 NIC with user-mode networking. On boot,
mini-os requests an IPv4 lease via DHCP; use `net ip` in the shell to display
the assigned address, or `net mac` to display the NIC address.
