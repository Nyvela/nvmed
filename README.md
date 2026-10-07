# nvmed

An userspace NVME driver made for Nyvela kernel

## Building standalone

```sh
make
```

`USERLIB_PATH` in `nvmed.conf` points at `../Nyvela/include`, which is where
`<nyvela/user/syslib.h>` comes from. Override it if the kernel lives elsewhere.

## Layout

`linker.ld` places the image at `0x400000`, the same base as the init program and
the shell. Nyvela enters a flat binary at its link base, so `_start` has to land
at offset 0 of `nvmed.bin`.
