


INCDIR += -I$(ROOTDIR)/libs/mm
INCDIR += -I$(KERNELDIR)/freertos/include
INCDIR += -I$(KERNELDIR)/freertos/FreeRTOS/Source/include

KERNEL_CSRC += $(KERNELDIR)/freertos/adapter/csi_freertos.c
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/timers.c
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/tasks.c
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/queue.c
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/list.c
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/event_groups.c
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/MemMang/heap_4.c
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/stream_buffer.c

ifeq ($(CONFIG_SUPPORT_TSPEND), y)
IS_TSPEND=tspend
else
IS_TSPEND=non_tspend
endif

ifeq ($(CONFIG_SYSTEM_SECURE), y)
IS_SECURE=security
else
IS_SECURE=non_security
endif

ifeq ($(CONFIG_CPU_RV32EMC)$(CONFIG_CPU_RV32EC)$(CONFIG_CPU_RV32I)$(CONFIG_CPU_RV32IAC)$(CONFIG_CPU_RV32IM)$(CONFIG_CPU_RV32IMAC)$(CONFIG_CPU_RV32IMAFC)$(CONFIG_CPU_E902), y)
INCDIR += -I$(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32_16gpr/tspend
KERNEL_SSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32_16gpr/tspend/cpu_task_sw.S
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32_16gpr/port.c
endif

ifeq ($(CONFIG_CPU_E906), y)
INCDIR += -I$(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32_32gpr/tspend
KERNEL_SSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32_32gpr/tspend/cpu_task_sw.S
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32_32gpr/port.c
endif

ifeq ($(CONFIG_CPU_E906F), y)
INCDIR += -I$(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32f_32gpr/tspend
KERNEL_SSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32f_32gpr/tspend/cpu_task_sw.S
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32f_32gpr/port.c
endif

ifeq ($(CONFIG_CPU_E906FD), y)
INCDIR += -I$(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32fd_32gpr/tspend
KERNEL_SSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32fd_32gpr/tspend/cpu_task_sw.S
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv32fd_32gpr/port.c
endif

ifeq ($(CONFIG_CPU_C906), y)
INCDIR += -I$(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv64/tspend
KERNEL_SSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv64/tspend/cpu_task_sw.S
KERNEL_CSRC += $(KERNELDIR)/freertos/FreeRTOS/Source/portable/GCC/riscv/rv64/port.c
endif
