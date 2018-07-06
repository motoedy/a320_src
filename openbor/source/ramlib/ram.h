/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#ifndef RAM_H
#define RAM_H

void setSystemRam();
unsigned long getSystemRam();
unsigned long getFreeRam();
unsigned long getUsedRam();
void getRamStatus();

#endif
