## mini-os

Test with <https://nadchif.github.io/in-browser-virtual-machine/>

The shell supports directories with `mkdir`, `rmdir` (empty directories only),
and `cd`. `ls` lists the current directory; `cd` with no argument prints the
current path. File commands such as `cat`, `cp`, `mv`, `rm`, `touch`, `run`,
`exec`, and `hexdump` accept paths relative to the current directory or
absolute paths from `/`.

The `test cpu` command runs a 30-second CPU stress test and prints CPU usage
once per second. Pass a duration in seconds (1-300) to change the run time, for
example `test cpu 60`. The current CPU percentage is also shown in the status
bar.
