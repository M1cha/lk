LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

STM32_CHIP := stm32f207

PLATFORM := stm32f2xx

GLOBAL_DEFINES += \
	ENABLE_UART3=1 \
	TARGET_HAS_DEBUG_LED=1

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk

