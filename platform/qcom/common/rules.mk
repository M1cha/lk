LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/fixed_point

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c

ifeq ($(ENABLE_QCOM_TIMER),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/timer.c
endif

ifeq ($(ENABLE_QCOM_CLOCK),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/clock.c
endif

ifeq ($(ENABLE_QCOM_CLOCK_PLL),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/clock_pll.c
endif

ifeq ($(ENABLE_QCOM_CLOCK_LOCAL),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/clock-local.c
endif

ifeq ($(ENABLE_QCOM_USB),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/hsusb.c
endif

ifeq ($(ENABLE_QCOM_UARTDM),true)
MODULE_SRCS += \
	$(LOCAL_DIR)/uart_dm.c
endif

ifeq ($(ENABLE_QCOM_DEV_SSBI),true)
MODULE_DEPS += \
	$(LOCAL_DIR)/dev/ssbi
endif

ifeq ($(ENABLE_QCOM_DEV_PMIC_PM8921),true)
MODULE_DEPS += \
	$(LOCAL_DIR)/dev/pmic/pm8921
endif

include make/module.mk
