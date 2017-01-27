LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	dev/interrupt/arm_gic

ENABLE_QCOM_TIMER := true
ENABLE_QCOM_CLOCK := true
ENABLE_QCOM_CLOCK_PLL := true
ENABLE_QCOM_CLOCK_LOCAL := true
ENABLE_QCOM_USB := true
ENABLE_QCOM_UARTDM := true
ENABLE_QCOM_GPIOMUX := true
ENABLE_QCOM_MMC := true
ENABLE_QCOM_DEV_SSBI := true
ENABLE_QCOM_DEV_PMIC_PM8921 := true

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/clock.c

MEMBASE := 0x80000000
MEMSIZE ?= 0x10000000 # 256MB
KERNEL_LOAD_OFFSET := 0x08f00000

ARCH := arm
ARM_CPU := cortex-a15

include make/module.mk

include platform/qcom/common/rules.mk
