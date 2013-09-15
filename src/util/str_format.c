// ==================================================================
// @(#)str_format.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/02/2008
// $Id: str_format.c,v 1.2 2009-03-24 16:30:03 bqu Exp $
//
// C-BGP, BGP Routing Solver
// Copyright (C) 2002-2008 Bruno Quoitin
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
// 02111-1307  USA
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <util/str_format.h>

typedef enum {
  NORMAL, ESCAPED
} EFormatState;

// -----[ str_format_for_each ]--------------------------------------
int str_format_for_each(gds_stream_t * stream,
			FFormatForEach fFunction,
			void * pContext,
			const char * pcFormat)
{
  const char * pPos= pcFormat;
  EFormatState eState= NORMAL;
  int iResult;

  while ((pPos != NULL) && (*pPos != 0)) {
    switch (eState) {
    case NORMAL:
      if (*pPos == '%') {
	eState= ESCAPED;
      } else {
	stream_printf(stream, "%c", *pPos);
      }
      break;

    case ESCAPED:
      if (*pPos != '%') {
	iResult= fFunction(stream, pContext, *pPos);
	if (iResult != 0)
	  return iResult;
      } else
	stream_printf(stream, "%");
      eState= NORMAL;
      break;

    default:
      abort();
    }
    pPos++;
  }
  // only NORMAL is a final state for the FSM
  if (eState != NORMAL)
    return -1;
  return 0;
}

