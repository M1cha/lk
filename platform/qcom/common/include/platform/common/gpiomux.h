#ifndef PLATFORM_COMMON_GPIOMUX_H
#define PLATFORM_COMMON_GPIOMUX_H

typedef enum {
    GPIOMUX_DRV_2MA = 0,
    GPIOMUX_DRV_4MA,
    GPIOMUX_DRV_6MA,
    GPIOMUX_DRV_8MA,
    GPIOMUX_DRV_10MA,
    GPIOMUX_DRV_12MA,
    GPIOMUX_DRV_14MA,
    GPIOMUX_DRV_16MA,
} gpiomux_drv_t;

typedef enum {
    GPIOMUX_FUNC_GPIO = 0,
    GPIOMUX_FUNC_1,
    GPIOMUX_FUNC_2,
    GPIOMUX_FUNC_3,
    GPIOMUX_FUNC_4,
    GPIOMUX_FUNC_5,
    GPIOMUX_FUNC_6,
    GPIOMUX_FUNC_7,
    GPIOMUX_FUNC_8,
    GPIOMUX_FUNC_9,
    GPIOMUX_FUNC_A,
    GPIOMUX_FUNC_B,
    GPIOMUX_FUNC_C,
    GPIOMUX_FUNC_D,
    GPIOMUX_FUNC_E,
    GPIOMUX_FUNC_F,
} gpiomux_func_t;

typedef enum {
    GPIOMUX_PULL_NONE = 0,
    GPIOMUX_PULL_DOWN,
    GPIOMUX_PULL_KEEPER,
    GPIOMUX_PULL_UP,
} gpiomux_pull_t;

typedef enum {
    GPIOMUX_IN = 0,
    GPIOMUX_OUT_HIGH,
    GPIOMUX_OUT_LOW,
} gpiomux_dir_t;

void gpiomux_tlmm_config(unsigned gpio, gpiomux_func_t func, gpiomux_dir_t dir, gpiomux_pull_t pull, gpiomux_drv_t drv);
void gpiomux_set_direction(unsigned gpio, gpiomux_dir_t dir);
int gpiomux_get(unsigned gpio);

#endif
