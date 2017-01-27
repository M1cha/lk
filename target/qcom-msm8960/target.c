#include <stdlib.h>
#include <platform/common/mmc.h>

void target_mmc_caps(struct mmc_host *host)
{
    host->caps.ddr_mode = 1;
    host->caps.hs200_mode = 1;
    host->caps.bus_width = MMC_BOOT_BUS_WIDTH_8_BIT;
    host->caps.hs_clk_rate = MMC_CLK_96MHZ;
}
