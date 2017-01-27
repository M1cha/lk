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
#include <err.h>
#include <stdio.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/common/gsbi.h>
#include <platform/clock.h>
#include <platform/qcom-platform.h>

#define LOCAL_TRACE 0

#define UARTDM_MR1    (0x00)
#define UARTDM_MR2    (0x04)
#define UARTDM_CSR    (0x08)
#define UARTDM_SR     (0x08)
#define UARTDM_CR     (0x10)
#define UARTDM_ISR    (0x14)
#define UARTDM_IMR    (0x14)
#define UARTDM_NCF_TX (0x40)
#define UARTDM_TF     (0x70)
#define UARTDM_RF     (0x70)
#define UARTDM_DMRX   (0x34)
#define UARTDM_TFWR   (0x1c)
#define UARTDM_RFWR   (0x20)
#define UARTDM_IPR    (0x18)
#define UARTDM_IRDA   (0x38)
#define UARTDM_HCR    (0x24)
#define UARTDM_DMEN   (0x3c)

#define UARTREG(base, reg)  (*REG32((base)  + (reg)))

#define UARTDM_SR_RXRDY            (1 << 0)
#define UARTDM_SR_TXRDY            (1 << 2)
#define UARTDM_SR_TXEMT            (1 << 3)
#define UARTDM_SR_UART_OVERRUN     (1 << 4)

#define UARTDM_TXLEV               (1 << 0)
#define UARTDM_TX_READY            (1 << 7)
#define UARTDM_RXLEV               (1 << 4)
#define UARTDM_RXSTALE             (1 << 3)

#define UARTDM_CR_CMD_RESET_RX            (1 << 4)
#define UARTDM_CR_CMD_RESET_TX            (2 << 4)
#define UARTDM_CR_CMD_RESET_ERR           (3 << 4)
#define UARTDM_CR_CMD_RESET_STALE_INT     (8 << 4)
#define UARTDM_CR_CMD_RESET_TX_ERR        (10 << 4)
#define UARTDM_CR_CMD_RESET_TX_READY      (3 << 8)
#define UARTDM_CR_CMD_FORCE_STALE         (4 << 8)
#define UARTDM_CR_CMD_STALE_EVENT_ENABLE  (5 << 8)
#define UARTDM_CR_CMD_STALE_EVENT_DISABLE (6 << 8)
#define UARTDM_STALE_TIMEOUT_LSB (0x0f)

#define UARTDM_RXFS           (0x50)
#define UARTDM_RXFS_BUF_SHIFT (0x07)
#define UARTDM_RXFS_BUF_MASK  (0x07)

#define UARTDM_CR_RX_ENABLE        (1 << 0)
#define UARTDM_CR_TX_ENABLE        (1 << 2)

// UART Parity Mode
enum UARTDM_PARITY_MODE {
    UARTDM_NO_PARITY,
    UARTDM_ODD_PARITY,
    UARTDM_EVEN_PARITY,
    UARTDM_SPACE_PARITY
};

// UART Stop Bit Length
enum UARTDM_STOP_BIT_LEN {
    UARTDM_SBL_9_16,
    UARTDM_SBL_1,
    UARTDM_SBL_1_9_16,
    UARTDM_SBL_2
};

// UART Bits per Char
enum UARTDM_BITS_PER_CHAR {
    UARTDM_5_BPS,
    UARTDM_6_BPS,
    UARTDM_7_BPS,
    UARTDM_8_BPS
};

static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;
static uintptr_t uart_bases[256];
static uint8_t default_port = 0;

static inline uintptr_t uart_to_ptr(unsigned int n)
{
    if (n==0)
        n = default_port;

    if (n>sizeof(uart_bases))
        return 0;

    return uart_bases[n];
}

static int uartdm_init_internal(uintptr_t base)
{
    // Configure UART mode registers MR1 and MR2
    // Hardware flow control isn't supported
    UARTREG(base, UARTDM_MR1) = 0;

    // 8-N-1 configuration: 8 data bits - No parity - 1 stop bit
    UARTREG(base, UARTDM_MR2) = UARTDM_NO_PARITY | (UARTDM_SBL_1 << 2) | (UARTDM_8_BPS << 4);

    // Configure Interrupt Mask register IMR
    UARTREG(base, UARTDM_IMR) = UARTDM_TX_READY | UARTDM_TXLEV | UARTDM_RXLEV | UARTDM_RXSTALE;

    // Configure Tx and Rx watermarks configuration registers
    // TX watermark value is set to 0 - interrupt is generated when
    // FIFO level is less than or equal to 0
    UARTREG(base, UARTDM_TFWR) = 0;

    // RX watermark value
    UARTREG(base, UARTDM_RFWR) = 0;

    // Configure Interrupt Programming Register
    // Set initial Stale timeout value
    UARTREG(base, UARTDM_IPR) = UARTDM_STALE_TIMEOUT_LSB;

    // Configure IRDA if required
    // Disabling IRDA mode
    UARTREG(base, UARTDM_IRDA) = 0;

    // Configure hunt character value in HCR register
    // Keep it in reset state
    UARTREG(base, UARTDM_HCR) = 0;

    // Issue soft reset command
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_RX;
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_TX;
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_ERR;
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_TX_ERR;
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_STALE_INT;

    // Enable/Disable Rx/Tx DM interfaces
    // Data Mover not currently utilized.
    UARTREG(base, UARTDM_DMEN) = 0;

    // Enable transmitter and receiver
    UARTREG(base, UARTDM_CR) = UARTDM_CR_RX_ENABLE;
    UARTREG(base, UARTDM_CR) = UARTDM_CR_TX_ENABLE;

    // Initialize Receive Path
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_STALE_EVENT_DISABLE;
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_RESET_STALE_INT;
    UARTREG(base, UARTDM_DMRX) = 0xFFFFFF;
    UARTREG(base, UARTDM_CR) = UARTDM_CR_CMD_STALE_EVENT_ENABLE;

    return NO_ERROR;
}

void uartdm_init(uint8_t port, uintptr_t gsbi_base, uintptr_t base)
{
    // platform config
    uartdm_platform_config(port);

    // Configure GSBI for UART_DM protocol.
    // I2C on 2 ports, UART (without HS flow control) on the other 2.
    if (gsbi_base) {
        writel(GSBI_PROTOCOL_CODE_I2C_UART << GSBI_CTRL_REG_PROTOCOL_CODE_S, GSBI_CTRL_REG(gsbi_base));
        DSB;
    }

    // Configure clock selection register for tx and rx rates.
    // Selecting 115.2k for both RX and TX.
    UARTREG(base, UARTDM_CSR) = UART_DM_CLK_RX_TX_BIT_RATE;
    DSB;

    // Intialize UART_DM
    uartdm_init_internal(base);

    // use the first initialized port as default
    if (default_port==0)
        default_port = port;

    // store port base
    uart_bases[port] = base;
}

int uart_putc(int port, char c)
{
    uintptr_t base = uart_to_ptr(port);
    if (!base)
        return -1;

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

    if (!base)
        return -1;

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
