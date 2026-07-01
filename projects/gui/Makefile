# ============================================================================
# Makefile - TD1601 EVB (T-Head Xuantie E906FD, RV32 IMAFDC + ilp32d) on macOS
#
# Replaces the Windows CDK + T-Head private toolchain with the upstream
# xPack riscv-none-elf-gcc (which ships the rv32imafdc_zicsr/ilp32d multilib).
#
# Usage:
#   make                 # incremental build -> Obj/gui.elf / .bin / .ihex
#   make clean           # remove Obj/
#   make clean && make   # full rebuild
#   make TOOLCHAIN_DIR=/path/to/xpack    # override toolchain location
#
# After building, run `bash aft_build_macos.sh` to produce the flashable image.
# ============================================================================

# Default goal MUST be set before any other target/rule below, otherwise the
# first component static-lib rule (libchip_td1601.a) becomes the default and
# `make` with no args stops after building that one lib (never links the ELF).
.DEFAULT_GOAL := all

PROJECT  := gui

# ---- Toolchain (xPack riscv-none-elf-gcc; MUST have ilp32d multilib) -------
TOOLCHAIN_DIR ?= $(HOME)/opt/xpack-riscv/xpack-riscv-none-elf-gcc-14.3.0-1
CROSS    := $(TOOLCHAIN_DIR)/bin/riscv-none-elf-
CC       := $(CROSS)gcc
AS       := $(CROSS)gcc
AR       := $(CROSS)ar
OBJCOPY  := $(CROSS)objcopy
OBJDUMP  := $(CROSS)objdump
READELF  := $(CROSS)readelf
SIZE     := $(CROSS)size

# ---- Paths -----------------------------------------------------------------
PROJ_DIR := .
REPO     := ../..
COMP     := $(REPO)/components
BOARDS   := $(REPO)/boards

CHIP     := $(COMP)/chips/chip_td1601/v1.0
CONSOLE  := $(COMP)/console/v1.0
CSI      := $(COMP)/csi/v1.0
MINILIBC := $(COMP)/minilibc/v1.0
MM       := $(COMP)/mm/v1.0
BOARD    := $(BOARDS)/td1601_evb/v1.0

LDSCRIPT := $(BOARD)/gcc_flash.ld
OBJ      := Obj
LST      := Lst

# ---- Architecture / ABI ----------------------------------------------------
#  _zicsr     : aligns with xPack multilib name (rv32imafdc_zicsr/ilp32d)
#  _xtheadcmo : enables T-Head cache-maintenance instruction encodings
#  ilp32d     : double-precision hard float ABI (E906FD)
ARCH_FLAGS := -march=rv32imafdc_zicsr_xtheadcmo -mabi=ilp32d

# ---- T-Head assembler compatibility shim (force-included everywhere) -------
COMPAT := -include $(PROJ_DIR)/include/thead_compat.h

# ---- Include directories ---------------------------------------------------
INCLUDES := \
	-Iinclude \
	-Iinclude/sys \
	-Isrc \
	-Isrc/littlefs \
	-Isrc/TJpgDec \
	-I$(CHIP)/include \
	-I$(CHIP)/sys \
	-I$(CONSOLE) \
	-I$(CONSOLE)/console \
	-I$(CSI)/include \
	-I$(CSI)/include/core \
	-I$(CSI)/include/drv \
	-I$(MINILIBC)/include \
	-I$(MM)/include \
	-I$(BOARD)/include \
	-I$(COMP)

# ---- Preprocessor defines (from project package.yaml def_config) -----------
DEFINES := \
	-DCONFIG_KERNEL_NONE=1 \
	-DCONFIG_ARCH_INTERRUPTSTACK=16384 \
	-DCONFIG_DEBUG=1 \
	-DCONFIG_DEBUG_MODE=0 \
	-DCONFIG_XIP=1 \
	-DCONFIG_CHIP_TD1601=1 \
	-DCONFIG_SUPPORT_TSPEND=1 \
	-DCONFIG_CPU_E906FD=1

# ---- Common / C / ASM flags ------------------------------------------------
COMMON_FLAGS := $(ARCH_FLAGS) -Os -g $(COMPAT) $(INCLUDES) $(DEFINES) \
	-ffunction-sections -fdata-sections

CFLAGS  := $(COMMON_FLAGS) -Wall -std=gnu99
ASFLAGS := $(COMMON_FLAGS) -x assembler-with-cpp
DEPFLAGS = -MMD -MP

# ---- Link flags ------------------------------------------------------------
#  -nostartfiles : project provides its own startup.S / vectors / section init
#                  (do NOT add -nostdlib: we need libgcc/libc/libm)
#  --specs=nano.specs : link newlib-nano (much smaller printf/scanf footprint)
LDFLAGS := -nostartfiles --specs=nano.specs -Wl,--gc-sections \
	-T$(LDSCRIPT) $(ARCH_FLAGS) -L$(OBJ) -Wl,-zmax-page-size=1024 \
	-Wl,-Map=$(LST)/$(PROJECT).map

# ============================================================================
# Project sources (this directory - editable)
# ============================================================================
PROJ_SRCS := \
	src/main.c \
	src/board_init.c \
	src/gpio_toggle.c \
	src/ST77916.c \
	src/CST816.c \
	src/lfs_port.c \
	src/lfs_test.c \
	src/TJpgDec/tjpgd.c \
	src/jpg_display.c \
	src/img_1.c

# Files at project root (not under src/)
ROOT_SRCS := \
	chip_config.c

PROJ_OBJS := \
	$(OBJ)/src_main.o \
	$(OBJ)/src_board_init.o \
	$(OBJ)/src_gpio_toggle.o \
	$(OBJ)/src_ST77916.o \
	$(OBJ)/src_CST816.o \
	$(OBJ)/src_lfs_port.o \
	$(OBJ)/src_lfs_test.o \
	$(OBJ)/src_TJpgDec_tjpgd.o \
	$(OBJ)/src_jpg_display.o \
	$(OBJ)/src_img_1.o \
	$(OBJ)/chip_config.o

# ============================================================================
# Component: chip_td1601
# ============================================================================
CHIP_SRCS := \
	$(CHIP)/drivers/adc.c \
	$(CHIP)/drivers/dma.c \
	$(CHIP)/drivers/edma.c \
	$(CHIP)/drivers/efuse.c \
	$(CHIP)/drivers/etb.c \
	$(CHIP)/drivers/gpio.c \
	$(CHIP)/drivers/iic.c \
	$(CHIP)/drivers/irq_port.c \
	$(CHIP)/drivers/ospi.c \
	$(CHIP)/drivers/pinmux.c \
	$(CHIP)/drivers/power_manager.c \
	$(CHIP)/drivers/pwm.c \
	$(CHIP)/drivers/qspi.c \
	$(CHIP)/drivers/qspiflash_ops.c \
	$(CHIP)/drivers/rtc.c \
	$(CHIP)/drivers/rtc_alg.c \
	$(CHIP)/drivers/spi.c \
	$(CHIP)/drivers/spiflash.c \
	$(CHIP)/drivers/spiflash_ops.c \
	$(CHIP)/drivers/spiflash_vendor.c \
	$(CHIP)/drivers/tdm.c \
	$(CHIP)/drivers/timer.c \
	$(CHIP)/drivers/uart.c \
	$(CHIP)/drivers/wdt.c \
	$(CHIP)/drivers/wj_pmu_td1601.c \
	$(CHIP)/drivers/ll/cd_qspi_ll.c \
	$(CHIP)/drivers/ll/dw_iic_ll.c \
	$(CHIP)/drivers/ll/dw_ospi_ll.c \
	$(CHIP)/drivers/ll/dw_spi_ll.c \
	$(CHIP)/drivers/ll/dw_uart_ll.c \
	$(CHIP)/drivers/ll/wj_adc_ll.c \
	$(CHIP)/drivers/ll/wj_dma_ll.c \
	$(CHIP)/drivers/ll/wj_efuse_ll.c \
	$(CHIP)/drivers/ll/wj_etb_ll.c \
	$(CHIP)/drivers/ll/wj_rtc_ll.c \
	$(CHIP)/sys/cache.c \
	$(CHIP)/sys/clk.c \
	$(CHIP)/sys/devices.c \
	$(CHIP)/sys/irq.c \
	$(CHIP)/sys/power_manage.c \
	$(CHIP)/sys/pre_main.c \
	$(CHIP)/sys/reboot.c \
	$(CHIP)/sys/sys_clk.c \
	$(CHIP)/sys/system.c \
	$(CHIP)/sys/target_get.c \
	$(CHIP)/sys/tick.c \
	$(CHIP)/sys/trap_c.c \
	$(CHIP)/sys/weak.c

CHIP_ASM := \
	$(CHIP)/sys/startup.S \
	$(CHIP)/sys/vectors.S \
	$(CHIP)/sys/lpm_arch_reg_save.S

# object-name mapping:
#   $(CHIP)/drivers/foo.c       -> $(OBJ)/drivers_foo.o
#   $(CHIP)/drivers/ll/foo.c    -> $(OBJ)/drivers_ll_foo.o
#   $(CHIP)/sys/foo.c           -> $(OBJ)/sys_foo.o
#   $(CHIP)/sys/foo.S           -> $(OBJ)/sys_foo.o
CHIP_OBJS := \
	$(patsubst $(CHIP)/drivers/ll/%.c,$(OBJ)/drivers_ll_%.o,$(filter $(CHIP)/drivers/ll/%,$(CHIP_SRCS))) \
	$(patsubst $(CHIP)/drivers/%.c,$(OBJ)/drivers_%.o,$(filter-out $(CHIP)/drivers/ll/%,$(filter $(CHIP)/drivers/%,$(CHIP_SRCS)))) \
	$(patsubst $(CHIP)/sys/%.c,$(OBJ)/sys_%.o,$(filter $(CHIP)/sys/%,$(CHIP_SRCS))) \
	$(patsubst $(CHIP)/sys/%.S,$(OBJ)/sys_%.o,$(CHIP_ASM))

$(OBJ)/drivers_ll_%.o: $(CHIP)/drivers/ll/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/drivers_%.o: $(CHIP)/drivers/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/sys_%.o: $(CHIP)/sys/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/sys_%.o: $(CHIP)/sys/%.S
	@mkdir -p $(dir $@)
	$(AS) -c $(ASFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/libchip_td1601.a: $(CHIP_OBJS)
	$(AR) rcs $@ $^

# ============================================================================
# Component: console
# ============================================================================
CONSOLE_SRCS := $(CONSOLE)/src/sys_console.c
CONSOLE_OBJS := $(patsubst $(CONSOLE)/src/%.c,$(OBJ)/console_%.o,$(CONSOLE_SRCS))

$(OBJ)/console_%.o: $(CONSOLE)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/libconsole.a: $(CONSOLE_OBJS)
	$(AR) rcs $@ $^

# ============================================================================
# Component: csi
# ============================================================================
CSI_SRCS := $(CSI)/src/csi_ringbuffer.c
CSI_OBJS := $(patsubst $(CSI)/src/%.c,$(OBJ)/csi_%.o,$(CSI_SRCS))

$(OBJ)/csi_%.o: $(CSI)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/libcsi.a: $(CSI_OBJS)
	$(AR) rcs $@ $^

# ============================================================================
# Component: minilibc
# ============================================================================
MINILIBC_SRCS := \
	$(MINILIBC)/src/_init.c \
	$(MINILIBC)/src/clock_gettime.c \
	$(MINILIBC)/src/malloc.c \
	$(MINILIBC)/src/minilibc_port.c \
	$(MINILIBC)/src/printf.c \
	$(MINILIBC)/src/reboot.c \
	$(MINILIBC)/src/syslog.c
MINILIBC_OBJS := $(patsubst $(MINILIBC)/src/%.c,$(OBJ)/minilibc_%.o,$(MINILIBC_SRCS))

$(OBJ)/minilibc_%.o: $(MINILIBC)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/libminilibc.a: $(MINILIBC_OBJS)
	$(AR) rcs $@ $^

# ============================================================================
# Component: mm
# ============================================================================
MM_SRCS := \
	$(MM)/src/dq_addlast.c \
	$(MM)/src/dq_rem.c \
	$(MM)/src/lib_mallinfo.c \
	$(MM)/src/mm_addfreechunk.c \
	$(MM)/src/mm_free.c \
	$(MM)/src/mm_initialize.c \
	$(MM)/src/mm_leak.c \
	$(MM)/src/mm_malloc.c \
	$(MM)/src/mm_mallinfo.c \
	$(MM)/src/mm_size2ndx.c
MM_OBJS := $(patsubst $(MM)/src/%.c,$(OBJ)/mm_%.o,$(MM_SRCS))

$(OBJ)/mm_%.o: $(MM)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/libmm.a: $(MM_OBJS)
	$(AR) rcs $@ $^

# ============================================================================
# Project object rules
# ============================================================================
$(OBJ)/src_main.o: src/main.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_board_init.o: src/board_init.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_gpio_toggle.o: src/gpio_toggle.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_ST77916.o: src/ST77916.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_CST816.o: src/CST816.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_lfs_port.o: src/lfs_port.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_lfs_test.o: src/lfs_test.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_TJpgDec_tjpgd.o: src/TJpgDec/tjpgd.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_jpg_display.o: src/jpg_display.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/src_img_1.o: src/img_1.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

$(OBJ)/chip_config.o: chip_config.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) -o $@ $<

# ============================================================================
# Component: littlefs (v2.9.3)
# ============================================================================
LFS_DIR  := src/littlefs
LFS_SRCS := $(LFS_DIR)/lfs.c $(LFS_DIR)/lfs_util.c
LFS_OBJS := $(OBJ)/lfs_lfs.o $(OBJ)/lfs_lfs_util.o

$(OBJ)/lfs_lfs.o: $(LFS_DIR)/lfs.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) \
		-DLFS_NO_ASSERT -DLFS_NO_DEBUG -DLFS_NO_WARN \
		-o $@ $<

$(OBJ)/lfs_lfs_util.o: $(LFS_DIR)/lfs_util.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(DEPFLAGS) -MF $(@:.o=.d) \
		-DLFS_NO_ASSERT -DLFS_NO_DEBUG -DLFS_NO_WARN \
		-o $@ $<

$(OBJ)/liblittlefs.a: $(LFS_OBJS)
	$(AR) rcs $@ $^

# ============================================================================
# Link
# ============================================================================
LIBS := \
	$(OBJ)/libchip_td1601.a \
	$(OBJ)/libconsole.a \
	$(OBJ)/libcsi.a \
	$(OBJ)/libminilibc.a \
	$(OBJ)/libmm.a \
	$(OBJ)/liblittlefs.a

ELF  := $(OBJ)/$(PROJECT).elf
BIN  := $(OBJ)/$(PROJECT).bin
IHEX := $(OBJ)/$(PROJECT).ihex

.PHONY: all clean size
all: $(BIN) $(IHEX) size

$(ELF): $(PROJ_OBJS) $(LIBS)
	@mkdir -p $(LST)
	$(CC) $(PROJ_OBJS) \
		-Wl,--start-group \
		-lchip_td1601 -lconsole -lcsi -lminilibc -lmm -llittlefs \
		-Wl,--end-group \
		$(LDFLAGS) -o $@
	$(OBJDUMP) -d $@ > $(LST)/$(PROJECT).asm

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(IHEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

size: $(ELF)
	$(SIZE) $(ELF)

clean:
	rm -rf $(OBJ) $(LST)

# ---- Auto-generated dependency files ---------------------------------------
-include $(wildcard $(OBJ)/*.d)
