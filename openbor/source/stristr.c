/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2009 OpenBOR Team
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

char *stristr(const char *String, const char *Pattern)
{
	char *pptr, *sptr, *start;
    unsigned int slen, plen;
    for(start=(char *)String, pptr=(char *)Pattern, slen=strlen(String), plen=strlen(Pattern); slen>=plen; start++, slen--)
	{
		/* find start of pattern in string */
        while(toupper(*start) != toupper(*Pattern))
		{
			start++;
            slen--;
            /* if pattern longer than string */
            if (slen < plen) return(NULL);
        }
        sptr = start;
        pptr = (char *)Pattern;
        while(toupper(*sptr) == toupper(*pptr))
        {
			sptr++;
            pptr++;
			/* if end of pattern then pattern was found */
            if('\0' == *pptr) return (start);
        }
	}
    return(NULL);
}
