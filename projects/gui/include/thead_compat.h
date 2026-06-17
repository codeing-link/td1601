/*
 * thead_compat.h - T-Head (Xuantie E906FD) assembler compatibility shim
 *
 * Force-included into every C/asm translation unit via the Makefile:
 *     COMPAT := -include include/thead_compat.h
 *
 * Purpose
 * -------
 * The vendor sources under components/ (read-only) use T-Head *bare* mnemonics
 * for the cache-maintenance instructions and *bare* private CSR names that
 * upstream / xPack binutils does not recognise:
 *
 *   - cache ops:  dcache.call / dcache.ciall / dcache.cipa / dcache.cpa /
 *                 dcache.iall / dcache.ipa / icache.iall / icache.ipa
 *   - private CSRs: mxstatus(0x7c0) / mhcr(0x7c1) / mhint(0x7c5) / mtvt(0x307)
 *
 * The bare mnemonics appear in TWO places:
 *   1. pure assembly files  (.S, __ASSEMBLER__ defined)
 *   2. inline asm inside C   (e.g. csi_rv32_gcc.h: __ASM volatile("dcache.ipa %0"))
 *
 * So the GAS .macro / .set definitions must reach the assembler in BOTH cases.
 * We therefore emit them:
 *   - directly, when assembling a .S file
 *   - via a file-scope asm() block, when compiling C (so they land in the same
 *     assembly stream that the inline asm is later emitted into)
 *
 * The macros rewrite the bare cache mnemonics into the binutils xtheadcmo
 * "th.dcache.*" / "th.icache.*" encodings (requires -march=...._xtheadcmo).
 *
 * REQUIRED while building with upstream/xPack binutils. Only droppable when
 * switching back to the official T-Head Xuantie GCC.
 */

#ifndef THEAD_COMPAT_H
#define THEAD_COMPAT_H

/* The raw GAS directives, shared by the .S path and the C file-scope asm(). */
#define THEAD_COMPAT_ASM \
    ".set mxstatus, 0x7c0\n" \
    ".set mhcr,     0x7c1\n" \
    ".set mhint,    0x7c5\n" \
    ".set mtvt,     0x307\n" \
    /* whole-cache / index ops (no register operand) */ \
    ".macro dcache.call\n th.dcache.call\n.endm\n" \
    ".macro dcache.ciall\n th.dcache.ciall\n.endm\n" \
    ".macro dcache.iall\n th.dcache.iall\n.endm\n" \
    ".macro icache.iall\n th.icache.iall\n.endm\n" \
    /* address ops (single register operand) */ \
    ".macro dcache.cpa rs1\n th.dcache.cpa \\rs1\n.endm\n" \
    ".macro dcache.cipa rs1\n th.dcache.cipa \\rs1\n.endm\n" \
    ".macro dcache.ipa rs1\n th.dcache.ipa \\rs1\n.endm\n" \
    ".macro icache.ipa rs1\n th.icache.ipa \\rs1\n.endm\n"

#ifdef __ASSEMBLER__

/* ---- private CSR name aliases (bare names used by vendor asm) ---- */
.set mxstatus, 0x7c0
.set mhcr,     0x7c1
.set mhint,    0x7c5
.set mtvt,     0x307

/* ---- cache-maintenance instruction shims (whole-cache / index) ---- */
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

/* ---- cache-maintenance instruction shims (by address) ---- */
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

#else  /* compiling C/C++ : inject the same directives into the asm stream */

__asm__(THEAD_COMPAT_ASM);

#endif /* __ASSEMBLER__ */

#endif /* THEAD_COMPAT_H */
