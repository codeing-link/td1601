


INCDIR += -I$(KERNELDIR)/rhino/arch/include
INCDIR += -I$(KERNELDIR)/rhino/core/include
ifeq ($(CONFIG_KERNEL_PWR_MGMT), y)
INCDIR += -I$(KERNELDIR)/rhino/common
INCDIR += -I$(KERNELDIR)/rhino/pwrmgmt
endif
INCDIR += -I$(LIBSDIR)/mm
INCDIR += -I$(LIBSDIR)/trace/include

KERNEL_CSRC += $(KERNELDIR)/rhino/driver/systick.c
KERNEL_CSRC += $(KERNELDIR)/rhino/driver/hook_impl.c
KERNEL_CSRC += $(KERNELDIR)/rhino/driver/hook_weak.c
KERNEL_CSRC += $(KERNELDIR)/rhino/driver/yoc_impl.c

KERNEL_CSRC += $(KERNELDIR)/rhino/adapter/csi_rhino.c

KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_buf_queue.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_dyn_mem_proc.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_err.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_event.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_idle.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_mm.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_mm_debug.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_mm_blk.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_mm_region.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_mm_firstfit.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_mm_bestfit.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_ringbuf.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_mutex.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_obj.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_pend.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_queue.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_sched.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_sem.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_stats.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_sys.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_task.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_task_sem.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_tick.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_time.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_timer.c
KERNEL_CSRC += $(KERNELDIR)/rhino/core/k_workqueue.c

KERNEL_CSRC += $(KERNELDIR)/rhino/common/k_atomic.c
KERNEL_CSRC += $(KERNELDIR)/rhino/common/k_ffs.c

ifeq ($(CONFIG_KERNEL_PWR_MGMT), y)
KERNEL_CSRC += $(KERNELDIR)/rhino/board/board_cpu_pwr.c
KERNEL_CSRC += $(KERNELDIR)/rhino/board/board_cpu_pwr_systick.c
KERNEL_CSRC += $(KERNELDIR)/rhino/board/board_cpu_pwr_timer.c

KERNEL_CSRC += $(KERNELDIR)/rhino/pwrmgmt/cpu_pwr_hal_lib.c
KERNEL_CSRC += $(KERNELDIR)/rhino/pwrmgmt/cpu_pwr_lib.c
KERNEL_CSRC += $(KERNELDIR)/rhino/pwrmgmt/cpu_pwr_show.c
KERNEL_CSRC += $(KERNELDIR)/rhino/pwrmgmt/cpu_tickless.c
endif

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
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_16gpr/cpu_impl.c
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_16gpr/mct_sched.c
KERNEL_SSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_16gpr/tspend/port_s.S
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_16gpr/port_c.c
endif

ifeq ($(CONFIG_CPU_E906), y)
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_32gpr/cpu_impl.c
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_32gpr/mct_sched.c
KERNEL_SSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_32gpr/tspend/port_s.S
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32_32gpr/port_c.c
endif

ifeq ($(CONFIG_CPU_E906F), y)
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32f_32gpr/cpu_impl.c
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32f_32gpr/mct_sched.c
KERNEL_SSRC += $(KERNELDIR)/rhino/arch/riscv/rv32f_32gpr/tspend/port_s.S
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32f_32gpr/port_c.c
endif

ifeq ($(CONFIG_CPU_E906FD), y)
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32fd_32gpr/cpu_impl.c
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32fd_32gpr/mct_sched.c
KERNEL_SSRC += $(KERNELDIR)/rhino/arch/riscv/rv32fd_32gpr/tspend/port_s.S
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/riscv/rv32fd_32gpr/port_c.c
endif

ifeq ($(CONFIG_CPU_CM0), y)
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/arm/cm0/cpu_impl.c
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/arm/cm0/mct_sched.c
KERNEL_SSRC += $(KERNELDIR)/rhino/arch/arm/cm0/port_s.S
KERNEL_CSRC += $(KERNELDIR)/rhino/arch/arm/cm0/port_c.c
endif

KERNEL_CSRC += $(KERNELDIR)/rhino/os_port.c
