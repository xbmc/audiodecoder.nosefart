/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
** MA 02110-1301, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** log.c
**
** Error logging functions
** $Id: log.c,v 1.2 2003/12/05 15:55:01 f1rmb Exp $
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdarg.h>
#include "types.h"
#include "log.h"


#ifdef OSD_LOG
#include "osd.h"
#endif

#if defined(OSD_LOG) && !defined(NOFRENDO_DEBUG)
#error NOFRENDO_DEBUG must be defined as well as OSD_LOG
#endif

/* Note that all of these functions will be empty if
** debugging is not enabled.
*/
#ifdef NOFRENDO_DEBUG
static FILE *errorlog;
#endif

int EXPORT log_init(void)
{
#ifdef NOFRENDO_DEBUG
#ifdef OSD_LOG
   /* Initialize an OSD logging system */
   osd_loginit();
#endif /* OSD_LOG */
   errorlog = fopen("errorlog.txt", "wt");
   if (NULL == errorlog)
      return (-1);
#endif /* NOFRENDO_DEBUG */
   return 0;
}

void EXPORT log_shutdown(void)
{
#ifdef NOFRENDO_DEBUG
   /* Snoop around for unallocated blocks */
   mem_checkblocks();
   mem_checkleaks();
#ifdef OSD_LOG
   osd_logshutdown();
#endif /* OSD_LOG */
   fclose(errorlog);
#endif /* NOFRENDO_DEBUG */
}

void EXPORT log_print(const char *string)
{
#ifdef NOFRENDO_DEBUG
#ifdef OSD_LOG
   osd_logprint(string);
#endif /* OSD_LOG */
   /* Log it to disk, as well */
   fputs(string, errorlog);
#else
   (void)string;
#endif /* NOFRENDO_DEBUG */
}

void EXPORT log_printf(const char *format, ... )
{
#ifdef NOFRENDO_DEBUG
#ifdef OSD_LOG
   char buffer[1024 + 1];
#endif /* OSD_LOG */
   va_list arg;

   va_start(arg, format);

#ifdef OSD_LOG
   vsprintf(buffer, format, arg);
   osd_logprint(buffer);
#endif /* OSD_LOG */
   vfprintf(errorlog, format, arg);
   va_end(arg);
#else
   (void)format;
#endif /* NOFRENDO_DEBUG */
}

/*
** $Log: log.c,v $
** Revision 1.2  2003/12/05 15:55:01  f1rmb
** cleanup phase II. use xprintf when it's relevant, use xine_xmalloc when it's relevant too. Small other little fix (can't remember). Change few internal function prototype because it xine_t pointer need to be used if some xine's internal sections. NOTE: libdvd{nav,read} is still too noisy, i will take a look to made it quit, without invasive changes. To be continued...
**
** Revision 1.1  2003/01/08 07:04:35  tmmm
** initial import of Nosefart sources
**
** Revision 1.5  2000/06/26 04:55:33  matt
** minor change
**
** Revision 1.4  2000/06/09 15:12:25  matt
** initial revision
**
*/
