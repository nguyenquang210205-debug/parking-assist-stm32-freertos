# =============================================================================
#  Hỗ trợ đỗ xe thông minh - FreeRTOS trên STM32F103C8 (Blue Pill)
#  Project chính dùng Keil C (keil/ParkingAssist.uvprojx). Makefile này để:
#   - chạy unit test trên máy tính (make test)
#   - biên dịch thử bằng Arm GNU Toolchain (arm-none-eabi-gcc) nếu không có Keil
#
#  make            biên dịch -> build/parking_assist.elf / .bin / .hex
#  make flash      nạp qua ST-Link (st-flash)
#  make test       chạy unit test trên máy tính (gcc của máy)
#  make clean
# =============================================================================
TARGET   := parking_assist
BUILD    := build
CROSS    ?= arm-none-eabi-
CC       := $(CROSS)gcc
OBJCOPY  := $(CROSS)objcopy
SIZE     := $(CROSS)size

# ---- Nguồn theo từng tầng ----
APP_SRC  := $(wildcard app/src/*.c)
MID_SRC  := $(wildcard middleware/src/*.c)
DRV_SRC  := $(wildcard drivers/src/*.c)
RTOS_SRC := middleware/FreeRTOS/tasks.c middleware/FreeRTOS/queue.c \
            middleware/FreeRTOS/list.c middleware/FreeRTOS/timers.c \
            middleware/FreeRTOS/event_groups.c \
            middleware/FreeRTOS/portable/GCC/ARM_CM3/port.c \
            middleware/FreeRTOS/portable/MemMang/heap_4.c
SPL_SRC  := $(wildcard platform/StdPeriph_Driver/src/*.c) platform/cmsis/device/system_stm32f10x.c
ASM_SRC  := platform/startup/gcc/startup_stm32f10x_md.s
LDSCRIPT := platform/startup/gcc/stm32f103c8_flash.ld

INC := -Iapp/inc -Imiddleware/inc -Idrivers/inc \
       -Imiddleware/FreeRTOS/include -Imiddleware/FreeRTOS/portable/GCC/ARM_CM3 \
       -Iplatform/cmsis/core -Iplatform/cmsis/device -Iplatform/StdPeriph_Driver/inc

MCU     := -mcpu=cortex-m3 -mthumb
DEFS    := -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD
CFLAGS  := $(MCU) $(DEFS) $(INC) -std=gnu11 -Os -g3 -ffunction-sections -fdata-sections \
           -Wall -Wextra -Wshadow -Wno-unused-parameter
LDFLAGS := $(MCU) -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map \
           --specs=nano.specs --specs=nosys.specs -Wl,--print-memory-usage

OBJS := $(addprefix $(BUILD)/,$(APP_SRC:.c=.o) $(MID_SRC:.c=.o) $(DRV_SRC:.c=.o) \
                              $(RTOS_SRC:.c=.o) $(SPL_SRC:.c=.o) $(ASM_SRC:.s=.o))

# Mã của hãng (SPL, CMSIS): tắt cảnh báo để chỉ thấy cảnh báo của code nhóm
$(BUILD)/platform/%.o: platform/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -w -MMD -MP -c $< -o $@

.PHONY: all flash test clean

all: $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex
	@$(SIZE) $(BUILD)/$(TARGET).elf

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(MCU) -x assembler-with-cpp -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD)/$(TARGET).bin
	st-flash --reset write $< 0x08000000

# ---- Unit test chạy trên máy tính: chỉ các module thuần logic, không cần phần cứng ----
TEST_SRC := test/test_main.c app/src/parking_fsm.c app/src/dist_filter.c app/src/parking_view.c \
            app/src/cpu_view.c \
            middleware/src/font3x5.c middleware/src/font5x7.c middleware/src/xprintf.c
test:
	@mkdir -p $(BUILD)/test
	gcc -std=c11 -Wall -Wextra -Iapp/inc -Imiddleware/inc $(TEST_SRC) -o $(BUILD)/test/run_tests
	./$(BUILD)/test/run_tests

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)
