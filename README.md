## mini-os

Test with <https://nadchif.github.io/in-browser-virtual-machine/>

QEMU launch targets configure an E1000 NIC with user-mode networking. On boot,
mini-os requests an IPv4 lease via DHCP; use `net ip` in the shell to display
the assigned address, `net mac` to display the NIC address, or
`net ping <IPv4 address>` to send one ICMP echo request. Ping currently accepts
numeric IPv4 addresses only. `test video` reports mini-os's supported VGA
text mode and the display mode reported by the firmware.

Run `exec music.cfg` in the shell to play the Nokia ringtone.

The QEMU `run` and `run-iso` targets route the PC speaker to the host audio
backend. If the default audio backend is unavailable on your host, override
it with a supported QEMU `QEMU_AUDIO_FLAGS` value.
