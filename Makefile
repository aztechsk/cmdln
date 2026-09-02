.DEFAULT_GOAL := all

TOP := $(abspath ..)
include $(TOP)/mk/common.mk

PROJECT := cmdln
OUT     := $(BUILD_ROOT)/$(PROJECT)
TARGET  := $(OUT)/$(PROJECT).a

SRCS := src/cmdln.c
OBJS := $(OUT)/cmdln.o
DEPS := $(OUT)/cmdln.d

SYS_INCLUDES := \
	-isystem$(TOP)/freertos/src/inc

USER_INCLUDES := \
	-I$(TOP)/inc \
	-I$(TOP)/ucdrv/src \
	-I$(TOP)/sys/src

WARNINGS := \
	-Wall \
	-Wextra \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wshadow \
	-Wpointer-arith \
	-Wbad-function-cast \
	-Wcast-align \
	-Wcast-qual \
	-Wjump-misses-init \
	-Wno-unused-parameter \
	-Wundef

OPT_FLAGS := -O2
CC1_FLAGS := $(CW_CC1_PRE_FLAGS) $(SYS_INCLUDES) $(USER_INCLUDES) $(CW_COMMON_DEFS)
CC1_POST_FLAGS := $(CW_CC1_BASE_FLAGS) $(WARNINGS) $(OPT_FLAGS) $(CW_CODEGEN_FLAGS)
AS_FLAGS := --traditional-format $(USER_INCLUDES) $(CPU_AS_FLAGS)

.PHONY: all clean
all: check-toolchain $(TARGET)

$(TARGET): $(OBJS)
	@echo "  AR      $@"
	@rm -f "$@"
	$(AR) -rcs "$@" $(OBJS)

$(OUT)/cmdln.o: src/cmdln.c | $(OUT)
	@echo "  CC      $<"
	$(CC1) $(CC1_FLAGS) -MD "$(OUT)/cmdln.d" -MQ "$@" $(CC1_POST_FLAGS) "$<" -o "$(OUT)/cmdln.asm"
	$(AS) $(AS_FLAGS) "$(OUT)/cmdln.asm" -o "$@"
	@rm -f "$(OUT)/cmdln.asm"

$(OUT):
	@mkdir -p "$@"

clean:
	@rm -rf "$(OUT)"

-include $(DEPS)
