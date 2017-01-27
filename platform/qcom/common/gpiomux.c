#include <stdio.h>
#include <reg.h>
#include <arch/arm.h>
#include <platform/qcom-platform.h>
#include <platform/common/gpiomux.h>
#include <platform/common/defines.h>

void gpiomux_tlmm_config(unsigned gpio, gpiomux_func_t func,
                      gpiomux_dir_t dir, gpiomux_pull_t pull,
                      gpiomux_drv_t drv)
{
    uint32_t val = 0;

    val |= pull;
    val |= func << 2;
    val |= drv << 6;
    if (func==GPIOMUX_FUNC_GPIO && dir>GPIOMUX_IN) {
        // set direction to output
        val |= BIT1(9);

        // set output value
        writel(dir==GPIOMUX_OUT_HIGH?BIT1(1):0, GPIO_IN_OUT_ADDR(gpio));
    }

    writel(val, GPIO_CONFIG_ADDR(gpio));

    DSB;

    return;
}

void gpiomux_set_direction(unsigned gpio, gpiomux_dir_t dir)
{
    uint32_t val = readl(GPIO_CONFIG_ADDR(gpio));

    if (dir>GPIOMUX_IN) {
        // set direction to output
        val |= BIT1(9);

        // set output value
        writel(dir==GPIOMUX_OUT_HIGH?BIT1(1):0, GPIO_IN_OUT_ADDR(gpio));
    } else {
        // set direction to input
        val &= ~(BIT1(9));
    }

    writel(val, GPIO_CONFIG_ADDR(gpio));

    DSB;
}

int gpiomux_get(unsigned gpio)
{
    int rc = GPIO_IN_OUT_ADDR(gpio) & BIT1(0);
    DSB;
    return rc;
}
