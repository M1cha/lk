LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/app.c

EXTRA_LINKER_SCRIPTS += $(LOCAL_DIR)/app.ld

include make/module.mk
