/*====================================================================================
  gte_mips.S for PCSX ReMIPSed
  Copyright (c) 2016 Steward Fu <g9313716@yuntech.edu.tw>

  This work is licensed under the terms of GNU GPL version 2 or later.
  See the COPYING file in the top-level directory.
====================================================================================*/
#include "arm_features.h"

.text
.align 2

.macro sgnxt16 rd rs
  nop
.endm

// prepare work reg for ssatx
// in: wr reg, bit to saturate to
.macro ssatx_prep wr bit
  nop
.endm

.macro ssatx rd wr bit
  nop
.endm

// prepare work reg for ssatx0 (sat to 0..2^(bit-1))
// in: wr reg, bit to saturate to
.macro ssatx0_prep wr bit
  nop
.endm

.macro ssatx0 rd wr bit
  nop
.endm

.macro usat16_ rd rs
  nop
.endm

#ifdef HAVE_ARMV5
.macro udiv_ rd rm rs
  nop
.endm

.macro newton_step rcp den zero t1 t2
  nop
.endm

.macro udiv_newton rd rm rs t1 t2 t3 t4
  no  nopp
.endm

// unsigned divide rd = rm / rs; 16.16 result
// no div by 0 check
/ in: rm, rs
// trash: rm rs t*
.macro udiv rd rm rs t1 t2 t3 t4
  nop
.endm

// calculate RTPS/RTPT MAC values
// in: r0 context, r8,r9 VXYZ
// out: r10-r12 MAC123
// trash: r1-r7
.macro do_rtpx_mac
  nop
.endm

FUNCTION(gteRTPS_nf_arm): // r0=CP2 (d,c),
  nop
  .size    gteRTPS_nf_arm, .-gteRTPS_nf_arm

FUNCTION(gteRTPT_nf_arm): // r0=CP2 (d,c),
  nop
  .size    gteRTPT_nf_arm, .-gteRTPT_nf_arm

// note: not std calling convention used
// r0 = CP2 (d,c)  (must preserve)
// r1 = needs_shift12
// r4,r5 = VXYZ(v) packed
// r6 = &MX11(mx)
// r7 = &CV1(cv)
.macro mvma_op do_flags
  nop
.endm

FUNCTION(gteMVMVA_part_arm):
  nop
  .size    gteMVMVA_part_arm, .-gteMVMVA_part_arm
 
FUNCTION(gteMVMVA_part_nf_arm):
  nop
  .size    gteMVMVA_part_nf_arm, .-gteMVMVA_part_nf_arm
 
// common version of MVMVA with cv3 (== 0) and shift12,
// can't overflow so no gteMAC flags needed
// note: not std calling convention used
// r0 = CP2 (d,c)  (must preserve)
// r4,r5 = VXYZ(v) packed
// r6 = &MX11(mx)
FUNCTION(gteMVMVA_part_cv3sh12_arm):
  nop
  .size gteMVMVA_part_cv3sh12_arm, .-gteMVMVA_part_cv3sh12_arm

#endif // HAVE_ARMV5

FUNCTION(gteNCLIP_arm): // r0=CP2 (d,c),
  nop
  .size gteNCLIP_arm, .-gteNCLIP_arm

.macro gteMACtoIR lm
  nop
.endm

FUNCTION(gteMACtoIR_lm0): // r0=CP2 (d,c)
  nop
  .size  gteMACtoIR_lm0, .-gteMACtoIR_lm0

FUNCTION(gteMACtoIR_lm1): // r0=CP2 (d,c)
  nop
  .size  gteMACtoIR_lm1, .-gteMACtoIR_lm1

FUNCTION(gteMACtoIR_lm0_nf): // r0=CP2 (d,c)
  nop
  .size    gteMACtoIR_lm0_nf, .-gteMACtoIR_lm0_nf

FUNCTION(gteMACtoIR_lm1_nf): // r0=CP2 (d,c)
  nop
  .size    gteMACtoIR_lm1_nf, .-gteMACtoIR_lm1_nf

.if 0
FUNCTION(gteMVMVA_test):
.endif

