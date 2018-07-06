/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   linkage_mips.s for PCSX                                                *
 *   Copyright (C) 2016-2016 Steward                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __MIPS_FEATURES_H__
#define __MIPS_FEATURES_H__

 /* global function/external symbol */
 #ifndef __MACH__
  #define ESYM(name) name
  #define FUNCTION(name) \
   .globl name; \
   .type name, %function; \
   name
  #define EXTRA_UNSAVED_REGS
 #else
  #define ESYM(name) _##name
  #define FUNCTION(name) \
   .globl ESYM(name); \
   name: \
   ESYM(name)
  // r7 is preserved, but add it for EABI alignment..
  #define EXTRA_UNSAVED_REGS r7, r9,
 #endif

#endif /* __MIPS_FEATURES_H__ */
