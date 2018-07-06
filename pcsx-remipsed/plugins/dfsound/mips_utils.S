/*====================================================================================
  mips_utils.S for PCSX ReMIPSed
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

#ifdef __MACH__
.data
.align 2
ptr_ChanBuf:   .word ESYM(ChanBuf)
#endif

.text
.align 2

.macro load_varadr reg var
  nop
.endm

#ifdef __ARM_NEON__

FUNCTION(mix_chan): // (int *SSumLR, int count, int lv, int rv)
  nop

FUNCTION(mix_chan_rvb): @ (int *SSumLR, int count, int lv, int rv, int *rvb)
  nop

#elif defined(HAVE_ARMV5)

FUNCTION(mix_chan): @ (int *SSumLR, int count, int lv, int rv)
  nop

FUNCTION(mix_chan_rvb): @ (int *SSumLR, int count, int lv, int rv, int *rvb)
  nop

#endif

