#include "vga.h"
#include "net.h"

void outb(unsigned short port, unsigned char val);
unsigned char inb(unsigned short port);

void net_init() {
    // COM1 0x3F8 для SLIP
    outb(0x3F8 + 1, 0x00); // Disable interrupts
    outb(0x3F8 + 3, 0x80); // Enable DLAB
    outb(0x3F8 + 0, 0x03); // Divisor 3 = 38400 baud
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03); // 8 bits, no parity, 1 stop
    outb(0x3F8 + 2, 0xC7); // Enable FIFO
    outb(0x3F8 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

void net_send(char* msg) {
    while (*msg) {
        while ((inb(0x3F8 + 5) & 0x20) == 0);
        outb(0x3F8, *msg++);
    }
}

void net_ping_kndr() {
    vga_draw_string(0, 0, "PING KNDR... NAHUY POSHEL", 12);
    net_send("PING KNDR\n");
}
