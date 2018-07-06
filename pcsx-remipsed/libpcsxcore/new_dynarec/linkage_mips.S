/*====================================================================================
  linkage_mips.S for PCSX ReMIPSed
  Copyright (c) 2016 Steward Fu <g9313716@yuntech.edu.tw>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License 
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
====================================================================================*/
#include "mips_features.h"
#include "linkage_offsets.h"

.bss
.align  4
.global dynarec_local
.type  dynarec_local, %object
.size  dynarec_local, LO_dynarec_local_size
dynarec_local:
.space  LO_dynarec_local_size

#define DRC_VAR_(name, vname, size_) vname = dynarec_local + LO_##name; \
  .global vname; \
  .type  vname, %object; \
  .size  vname, size_

#define DRC_VAR(name, size_) DRC_VAR_(name, ESYM(name), size_)

DRC_VAR(next_interupt, 4)
DRC_VAR(cycle_count, 4)
DRC_VAR(last_count, 4)
DRC_VAR(pending_exception, 4)
DRC_VAR(stop, 4)
DRC_VAR(invc_ptr, 4)
DRC_VAR(address, 4)
DRC_VAR(psxRegs, LO_psxRegs_end - LO_psxRegs)

// psxRegs
DRC_VAR(reg, 128)
DRC_VAR(lo, 4)
DRC_VAR(hi, 4)
DRC_VAR(reg_cop0, 128)
DRC_VAR(reg_cop2d, 128)
DRC_VAR(reg_cop2c, 128)
DRC_VAR(pcaddr, 4)
// @DRC_VAR(code, 4)
// @DRC_VAR(cycle, 4)
// @DRC_VAR(interrupt, 4)
// @DRC_VAR(intCycle, 256)

DRC_VAR(rcnts, 7*4*4)
DRC_VAR(mem_rtab, 4)
DRC_VAR(mem_wtab, 4)
DRC_VAR(psxH_ptr, 4)
DRC_VAR(zeromem_ptr, 4)
DRC_VAR(inv_code_start, 4)
DRC_VAR(inv_code_end, 4)
DRC_VAR(branch_target, 4)
DRC_VAR(scratch_buf_ptr, 4)
// @DRC_VAR(align0, 12) // unused/alignment
DRC_VAR(mini_ht, 256)
DRC_VAR(restore_candidate, 512)

// unused
DRC_VAR(FCR0, 4)
DRC_VAR(FCR31, 4)

//.syntax unified
.text
.align  2

.macro load_varadr reg var
  //ldr  \reg, =\var
  nop
.endm

.macro load_varadr_ext reg var
  nop
.endm

.macro mov_16 reg imm
  nop
.endm

.macro mov_24 reg imm
  nop
.endm

.macro dyna_linker_main
  nop
.endm

FUNCTION(dyna_linker):
  nop
  .size  dyna_linker, .-dyna_linker

FUNCTION(exec_pagefault):
  nop
  .size  exec_pagefault, .-exec_pagefault

// Special dynamic linker for the case 
// where a page fault may occur in a branch delay slot
FUNCTION(dyna_linker_ds):
  nop
  .size  dyna_linker_ds, .-dyna_linker_ds

  .align  2
FUNCTION(jump_vaddr_r0):
  nop
  .size  jump_vaddr_r0, .-jump_vaddr_r0
FUNCTION(jump_vaddr_r1):
  nop
  .size  jump_vaddr_r1, .-jump_vaddr_r1
FUNCTION(jump_vaddr_r2):
  nop
  .size  jump_vaddr_r2, .-jump_vaddr_r2
FUNCTION(jump_vaddr_r3):
  nop
  .size  jump_vaddr_r3, .-jump_vaddr_r3
FUNCTION(jump_vaddr_r4):
  nop
  .size  jump_vaddr_r4, .-jump_vaddr_r4
FUNCTION(jump_vaddr_r5):
  nop
  .size  jump_vaddr_r5, .-jump_vaddr_r5
FUNCTION(jump_vaddr_r6):
  nop
  .size  jump_vaddr_r6, .-jump_vaddr_r6
FUNCTION(jump_vaddr_r8):
  nop
  .size  jump_vaddr_r8, .-jump_vaddr_r8
FUNCTION(jump_vaddr_r9):
  nop
  .size  jump_vaddr_r9, .-jump_vaddr_r9
FUNCTION(jump_vaddr_r10):
  nop
  .size  jump_vaddr_r10, .-jump_vaddr_r10
FUNCTION(jump_vaddr_r12):
  nop
  .size  jump_vaddr_r12, .-jump_vaddr_r12
FUNCTION(jump_vaddr_r7):
  nop
  .size  jump_vaddr_r7, .-jump_vaddr_r7
FUNCTION(jump_vaddr):
  nop
  .size  jump_vaddr, .-jump_vaddr

  .align  2
FUNCTION(verify_code_ds):
  nop
FUNCTION(verify_code_vm):
FUNCTION(verify_code):
  nop
  .size  verify_code, .-verify_code
  .size  verify_code_vm, .-verify_code_vm

  .align  2
FUNCTION(cc_interrupt):
  nop
  .size  cc_interrupt, .-cc_interrupt

  .align  2
FUNCTION(do_interrupt):
  nop
  .size  do_interrupt, .-do_interrupt

  .align  2
FUNCTION(fp_exception):
  nop
  .size  fp_exception, .-fp_exception
  .align  2
FUNCTION(fp_exception_ds):
  nop
  .size  fp_exception_ds, .-fp_exception_ds

  .align  2
FUNCTION(jump_syscall):
  nop
  .size  jump_syscall, .-jump_syscall
  .align  2

  .align  2
FUNCTION(jump_syscall_hle):
  nop
  .size  jump_syscall_hle, .-jump_syscall_hle

  .align  2
FUNCTION(jump_hlecall):
  nop
  .size  jump_hlecall, .-jump_hlecall

  .align  2
FUNCTION(jump_intcall):
  nop
  .size  jump_hlecall, .-jump_hlecall

  .align  2
FUNCTION(new_dyna_leave):
  nop
  .size  new_dyna_leave, .-new_dyna_leave

  .align  2
FUNCTION(invalidate_addr_r0):
  nop
  .size  invalidate_addr_r0, .-invalidate_addr_r0
  .align  2
FUNCTION(invalidate_addr_r1):
  nop
  .size  invalidate_addr_r1, .-invalidate_addr_r1
  .align  2
FUNCTION(invalidate_addr_r2):
  nop
  .size  invalidate_addr_r2, .-invalidate_addr_r2
  .align  2
FUNCTION(invalidate_addr_r3):
  nop
  .size  invalidate_addr_r3, .-invalidate_addr_r3
  .align  2
FUNCTION(invalidate_addr_r4):
  nop
  .size  invalidate_addr_r4, .-invalidate_addr_r4
  .align  2
FUNCTION(invalidate_addr_r5):
  nop
  .size  invalidate_addr_r5, .-invalidate_addr_r5
  .align  2
FUNCTION(invalidate_addr_r6):
  nop
  .size  invalidate_addr_r6, .-invalidate_addr_r6
  .align  2
FUNCTION(invalidate_addr_r7):
  nop
  .size  invalidate_addr_r7, .-invalidate_addr_r7
  .align  2
FUNCTION(invalidate_addr_r8):
  nop
  .size  invalidate_addr_r8, .-invalidate_addr_r8
  .align  2
FUNCTION(invalidate_addr_r9):
  nop
  .size  invalidate_addr_r9, .-invalidate_addr_r9
  .align  2
FUNCTION(invalidate_addr_r10):
  nop
  .size  invalidate_addr_r10, .-invalidate_addr_r10
  .align  2
FUNCTION(invalidate_addr_r12):
  nop
  .size  invalidate_addr_r12, .-invalidate_addr_r12
  .align  2
invalidate_addr_call:
  nop
  .size  invalidate_addr_call, .-invalidate_addr_call

  .align  2
FUNCTION(new_dyna_start):
  nop
  .size  new_dyna_start, .-new_dyna_start

.align  2
.macro pcsx_read_mem readop tab_shift
  nop
.endm

FUNCTION(jump_handler_read8):
  nop
FUNCTION(jump_handler_read16):
  nop
FUNCTION(jump_handler_read32):
  nop

.macro pcsx_write_mem wrtop tab_shift
  nop
.endm

FUNCTION(jump_handler_write8):
  nop
FUNCTION(jump_handler_write16):
  nop
FUNCTION(jump_handler_write32):
  nop
FUNCTION(jump_handler_write_h):
  nop

FUNCTION(jump_handle_swl):
  nop
FUNCTION(jump_handle_swr):
  nop

.macro rcntx_read_mode0 num
  nop
.endm

FUNCTION(rcnt0_read_count_m0):
  nop
FUNCTION(rcnt1_read_count_m0):
  nop
FUNCTION(rcnt2_read_count_m0):
  nop
FUNCTION(rcnt0_read_count_m1):
  nop
FUNCTION(rcnt1_read_count_m1):
  nop
FUNCTION(rcnt2_read_count_m1):
  nop
