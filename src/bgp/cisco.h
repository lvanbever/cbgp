// ==================================================================
// @(#)cisco.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/05/2007
// $Id: cisco.h,v 1.3 2009-03-24 14:11:11 bqu Exp $
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

#ifndef __BGP_CISCO_H__
#define __BGP_CISCO_H__

#include <stdio.h>

#include <libgds/stream.h>

// ----- Error codes -----
#define CISCO_SUCCESS          0
#define CISCO_ERROR_UNEXPECTED -1
#define CISCO_ERROR_FILE_OPEN  -2
#define CISCO_ERROR_NUM_FIELDS -3

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cisco_perror ]-------------------------------------------
  void cisco_perror(gds_stream_t * stream, int error);
  // -----[ cisco_parser ]-------------------------------------------
  int cisco_parser(FILE * stream);
  // -----[ cisco_load ]---------------------------------------------
  int cisco_load(const char * filename);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_CISCO_H__ */
