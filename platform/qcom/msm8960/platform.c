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
#include <debug.h>
#include <trace.h>

#include <dev/uart.h>
#include <dev/pm8921.h>
#include <dev/ssbi.h>
#include <arch.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <platform.h>
#include <dev/interrupt/arm_gic.h>
#include <platform/qcom-platform.h>
#include <platform/common/timer.h>
#include <platform/common/uartdm.h>
#include <platform/common/clock.h>
#include <platform/common/gpiomux.h>
#include <platform/common/mmc.h>

#include <arch/arm.h>
#include <arch/arm/mmu.h>

static pm8921_dev_t pmic;
static struct mmc_device *dev;
static unsigned mmc_sdc_base[] = { MSM_SDC1_BASE, MSM_SDC2_BASE, MSM_SDC3_BASE, MSM_SDC4_BASE };

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    /* sdram */
    {
        .phys = SDRAM_BASE,
        .virt = KERNEL_BASE,
        .size = MEMSIZE,
        .flags = 0,
        .name = "memory"
    },

    /* peripherals */
    {
        .phys = PERIPHERAL_BASE_PHYS,
        .virt = PERIPHERAL_BASE_VIRT,
        .size = PERIPHERAL_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals"
    },

    /* imem */
    {
        .phys = IMEM_BASE_PHYS,
        .virt = IMEM_BASE_VIRT,
        .size = IMEM_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "imem"
    },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t arena = {
    .name = "sdram",
    .base = SDRAM_BASE,
    .size = MEMSIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};

void platform_init_mmu_mappings(void)
{
}

void platform_early_init(void)
{
    // this allows us to use spin()
    qcom_timer_early_init(TMR_BASE);

    qcom_clocks_init();

    // Initialize PMIC driver
    pmic.read = (pm8921_read_func) & pa1_ssbi2_read_bytes;
    pmic.write = (pm8921_write_func) & pa1_ssbi2_write_bytes;
    pm8921_init(&pmic);

    arm_gic_init();
    qcom_timer_init(6750000);

    /* add the main memory arena */
    pmm_add_arena(&arena);
}

void platform_init(void)
{
    unsigned base_addr;
    unsigned char slot;

    // Trying Slot 1 first
    slot = 1;
    base_addr = mmc_sdc_base[slot - 1];
    if (!(dev = mmc_boot_main(slot, base_addr))) {
        // Trying Slot 3 next
        slot = 3;
        base_addr = mmc_sdc_base[slot - 1];
        if (!(dev = mmc_boot_main(slot, base_addr))) {
            dprintf(CRITICAL, "mmc init failed!");
            ASSERT(0);
        }
    }
}

void uartdm_platform_config(uint8_t port)
{
    char gsbi_uart_clk_id[64];
    char gsbi_p_clk_id[64];

    snprintf(gsbi_uart_clk_id, 64,"gsbi%u_uart_clk", port);
    clk_get_set_enable(gsbi_uart_clk_id, 1843200, 1);

    snprintf(gsbi_p_clk_id, 64,"gsbi%u_pclk", port);
    clk_get_set_enable(gsbi_p_clk_id, 0, 1);

    gpiomux_tlmm_config(62, GPIOMUX_FUNC_GPIO, GPIOMUX_OUT_HIGH, GPIOMUX_PULL_UP, GPIOMUX_DRV_8MA);

    // configure rx gpio
    gpiomux_tlmm_config(83, GPIOMUX_FUNC_1, GPIOMUX_IN, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA);
    // configure tx gpio
    gpiomux_tlmm_config(82, GPIOMUX_FUNC_2, GPIOMUX_OUT_LOW, GPIOMUX_PULL_NONE, GPIOMUX_DRV_8MA);
}

void platform_clock_init_mmc(uint32_t interface)
{
}

void platform_clock_config_mmc(uint32_t interface, uint32_t freq)
{
    char sdc_clk[64];
    unsigned rate;

    snprintf(sdc_clk, 64, "sdc%u_clk", interface);

    // Disalbe MCI_CLK before changing the sdcc clock
    mmc_boot_mci_clk_disable();

    switch (freq) {
        case MMC_CLK_400KHZ:
            rate = 144000;
            break;
        case MMC_CLK_48MHZ:
        // Max supported is 48MHZ
        case MMC_CLK_50MHZ:
            rate = 48000000;
            break;
        case MMC_CLK_96MHZ:
            rate = 96000000;
            break;
        default:
            ASSERT(0);
    };

    clk_get_set_enable(sdc_clk, rate, 1);

    // Enable MCI clk
    mmc_boot_mci_clk_enable();
}
