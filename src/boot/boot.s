.section .multiboot
.align 4
.long 0x1BADB002             # magic
.long 0x00000000             # flags
.long -(0x1BADB002 + 0x0)   # checksum

.section .text
.global _start
.type _start, @function
_start:
    cli
    mov $stack_top, %esp
    call kernel_main
    hlt

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:
