/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <reg.h>
#include <stdio.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/qcom-msm8960.h>

#define LOCAL_TRACE 0

#define UARTDM_SR     (0x08)
#define UARTDM_CR     (0x10)
#define UARTDM_ISR    (0x14)
#define UARTDM_NCF_TX (0x40)
#define UARTDM_TF     (0x70)
#define UARTDM_RF     (0x70)
#define UARTDM_DMRX   (0x34)

#define UARTREG(base, reg)  (*REG32((base)  + (reg)))

#define UARTDM_SR_RXRDY            (1 << 0)
#define UARTDM_SR_TXRDY            (1 << 2)
#define UARTDM_SR_TXEMT            (1 << 3)
#define UARTDM_SR_UART_OVERRUN     (1 << 4)
#define UARTDM_TX_READY            (1 << 7)

#define UARTDM_CR_CMD_RESET_ERR           (3 << 4)
#define UARTDM_CR_CMD_RESET_TX_READY      (3 << 8)
#define UARTDM_CR_CMD_STALE_EVENT_ENABLE  (80 << 4)
#define UARTDM_CR_CMD_FORCE_STALE         (4 << 8)
#define UARTDM_CR_CMD_RESET_STALE_INT     (8 << 4)

#define UARTDM_RXFS           (0x50)
#define UARTDM_RXFS_BUF_SHIFT (0x07)
#define UARTDM_RXFS_BUF_MASK  (0x07)

static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

static inline uintptr_t uart_to_ptr(unsigned int n)
{
    switch (n) {
        default:
        case 0:
            return UART_BASE;
    }
}

void uart_init(void)
{
}

void uart_init_early(void)
{
}

int uart_putc(int port, char c)
{
    uintptr_t base = uart_to_ptr(port);

    // Check if transmit FIFO is empty. If not we'll wait for TX_READY interrupt.
    if (!(UARTREG(base, UARTDM_SR) & UARTDM_SR_TXEMT)) {
        while (!(UARTREG(base, UARTDM_ISR) & UARTDM_TX_READY))
            ;
    }

    // Check for Overrun error. We'll just reset Error Status
    if (UARTREG(base, UARTDM_SR) & UARTDM_SR_UART_OVERRUN) {
        UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_ERR;
    }

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    // Write number of characters to be written
    UARTREG(base, UARTDM_NCF_TX) = 1;

    // Clear TX_READY interrupt
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_TX_READY;

    // Wait till TX FIFO has space
    while (!(UARTREG(base, UARTDM_SR) & UARTDM_SR_TXRDY))
        ;

    // TX FIFO has space. Write the chars
    UARTREG(base, UARTDM_TF)  = c;

    spin_unlock_irqrestore(&lock, state);

    return 1;
}

int uart_getc(int port, bool wait)
{
    int c;
    static u32 slop;
    static int count;
    unsigned char *sp = (unsigned char *)&slop;
    uintptr_t base = uart_to_ptr(port);

    do {
        // previous read had more than one char
        if (count) {
            c = sp[sizeof(slop) - count];
            count--;
        }

        // FIFO is empty
        else if (!(UARTREG(base, UARTDM_SR) & UARTDM_SR_RXRDY)) {
            // If RX packing buffer has less than a word, force stale to push contents into RX FIFO
            count = UARTREG(base, UARTDM_RXFS);
            count = (count >> UARTDM_RXFS_BUF_SHIFT) & UARTDM_RXFS_BUF_MASK;
            if (count) {
                UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_FORCE_STALE;
                slop = UARTREG(base, UARTDM_RF);
                c = sp[0];
                count--;
                UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_STALE_INT;
                UARTREG(base, UARTDM_DMRX) = 0xFFFFFF;
                UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_STALE_EVENT_ENABLE;
            } else {
                c = -1;
            }
        }

        // FIFO has a word
        else {
            // Check for Overrun error. We'll just reset Error Status
            if (UARTREG(base, UARTDM_SR) & UARTDM_SR_UART_OVERRUN) {
                UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_ERR;
            }

            slop = UARTREG(base, UARTDM_RF);
            c = sp[0];
            count = sizeof(slop) - 1;
        }
    } while (wait && c==-1);

    return c;
}

void uart_flush_tx(int port)
{
}

void uart_flush_rx(int port)
{
}

void uart_init_port(int port, uint baud)
{
}


