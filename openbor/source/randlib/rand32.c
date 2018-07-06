/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include "rand32.h"

unsigned long seed = 1234567890;

unsigned int rand32(void)
{
  unsigned long long t = seed;
  t *= 1103515245ull;
  t += 12345ull;
  seed = t;
  return (t >> 16) & 0xFFFFFFFF;
}

void srand32(int n) { seed = n; }

