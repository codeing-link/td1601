#ifndef TD1601_XPACK_COMPAT_H
#define TD1601_XPACK_COMPAT_H
#define TD1601_COMPAT_ASM \
".set mxstatus, 0x7c0\n" \
".set mhcr, 0x7c1\n" \
".set mhint, 0x7c5\n" \
".set mtvt, 0x307\n" \
".macro dcache.call\n th.dcache.call\n.endm\n" \
".macro dcache.ciall\n th.dcache.ciall\n.endm\n" \
".macro dcache.iall\n th.dcache.iall\n.endm\n" \
".macro icache.iall\n th.icache.iall\n.endm\n" \
".macro dcache.cpa rs1\n th.dcache.cpa \\rs1\n.endm\n" \
".macro dcache.cipa rs1\n th.dcache.cipa \\rs1\n.endm\n" \
".macro dcache.ipa rs1\n th.dcache.ipa \\rs1\n.endm\n" \
".macro icache.ipa rs1\n th.icache.ipa \\rs1\n.endm\n"
#ifdef __ASSEMBLER__
.set mxstatus, 0x7c0
.set mhcr, 0x7c1
.set mhint, 0x7c5
.set mtvt, 0x307
.macro dcache.call
 th.dcache.call
.endm
.macro dcache.ciall
 th.dcache.ciall
.endm
.macro dcache.iall
 th.dcache.iall
.endm
.macro icache.iall
 th.icache.iall
.endm
.macro dcache.cpa rs1
 th.dcache.cpa \rs1
.endm
.macro dcache.cipa rs1
 th.dcache.cipa \rs1
.endm
.macro dcache.ipa rs1
 th.dcache.ipa \rs1
.endm
.macro icache.ipa rs1
 th.icache.ipa \rs1
.endm
#else
__asm__(TD1601_COMPAT_ASM);
#endif
#endif
