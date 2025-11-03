# ArchAngel OS

This is my attempt to build custom Operating System from scratch.

## Dependencies

```
gcc grub-pc-bin grub-efi-amd64-bin xorriso mtools
qemu # for virtual machine
```

## Build and Run

```
make
make run
```

### Progress

- [x] Printing on boot screen (startup screen).
- [x] Implementated `Global Descriptor Table (GDT)` to manage memory segments.

## References

- Websites:
  - https://wiki.osdev.org/
- Github:
  - https://github.com/mit-pdos/xv6-riscv
- Youtube
  - https://www.youtube.com/@writeyourownoperatingsystem
  - https://www.youtube.com/@xyve7
  - https://www.youtube.com/@YourAvgDev
