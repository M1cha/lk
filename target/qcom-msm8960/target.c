#include <stdlib.h>
#include <platform/common/mmc.h>
#include <platform/common/uartdm.h>
#include <platform/qcom-platform.h>

void target_early_init(void)
{
    uartdm_init(7, PERIPHERAL_BASE_VIRT+0x16500000, PERIPHERAL_BASE_VIRT+0x16540000);
}

void target_mmc_caps(struct mmc_host *host)
{
    host->caps.ddr_mode = 1;
    host->caps.hs200_mode = 1;
    host->caps.bus_width = MMC_BOOT_BUS_WIDTH_8_BIT;
    host->caps.hs_clk_rate = MMC_CLK_96MHZ;
}
