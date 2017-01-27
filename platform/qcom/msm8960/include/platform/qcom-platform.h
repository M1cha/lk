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
#pragma once

#define SDRAM_BASE 0x80000000

#define PERIPHERAL_BASE_PHYS (0x00100000)
#define PERIPHERAL_BASE_VIRT (0xc0000000UL) // -1GB
#define PERIPHERAL_SIZE      (0x28000000)

#define IMEM_BASE_PHYS (0x2a000000)
#define IMEM_BASE_VIRT (PERIPHERAL_BASE_VIRT+PERIPHERAL_SIZE)
#define IMEM_SIZE      (MB)

#define CPUPRIV_BASE   (PERIPHERAL_BASE_VIRT + 0x01f00000)
#define UART_BASE      (PERIPHERAL_BASE_VIRT + 0x16540000)
#define TMR_BASE       (CPUPRIV_BASE + 0x0a000)

#define MSM_CLK_CTL_BASE      (PERIPHERAL_BASE_VIRT + 0x00800000)
#define MSM_MMSS_CLK_CTL_BASE (PERIPHERAL_BASE_VIRT + 0x03f00000)
#define MSM_USB_BASE          (PERIPHERAL_BASE_VIRT + 0x12400000)
#define MSM_PA1_SSBI2_BASE    (PERIPHERAL_BASE_VIRT + 0x00400000)
#define MSM_PA2_SSBI2_BASE    (PERIPHERAL_BASE_VIRT + 0x00b00000)

#define GIC_PPI_START 16
#define GIC_SPI_START 32
#define INT_DEBUG_TIMER_EXP     (GIC_PPI_START + 1)
#define GSBI7_UARTDM_IRQ        (GIC_SPI_START + 158)
#define USB1_HS_IRQ             (GIC_SPI_START + 100)
#define INT_USB_HS              (USB1_HS_IRQ)

void qcom_timer_early_init(addr_t _control_base);
void qcom_timer_init(uint32_t freq);
void qcom_clocks_init(void);
