# heavy-yayca-kernel / тяжёлая-яйца-kernel

<img width="772" height="516" alt="image" src="https://github.com/user-attachments/assets/bc9710ac-e457-4cac-b6fd-75898643ab33" />

### What is this?
`heavy-yayca-kernel` is a bare-metal x86 32-bit monolithic kernel written from scratch in C + ASM. 

It boots via GRUB, runs in QEMU, and has its own shell, RAMFS, VGA textmode GUI, and the `YAYCA++ v3` scripting language built into the kernel.

This is not Linux. This is a pizdec OS for educational purposes and suffering.

### Build & Run
```bash
git clone https://github.com/USERNAME/heavy-yayca-kernel.git
cd heavy-yayca-kernel
make clean && make
make qemu
