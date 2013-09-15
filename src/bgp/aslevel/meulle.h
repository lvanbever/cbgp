// ==================================================================
// @(#)meulle.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/10/2007
// $Id: meulle.h,v 1.2 2009-06-25 14:31:03 bqu Exp $
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

#ifndef __BGP_ASLEVEL_MEULLE_H__
#define __BGP_ASLEVEL_MEULLE_H__

#include <stdio.h>

#include <libgds/stream.h>

#include <net/prefix.h>
#include <net/network.h>

#include <bgp/aslevel/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ meulle_parser ]-----------------------------------------
  int meulle_parser(FILE * file, as_level_topo_t * topo,
		    int * line_number);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_MEULLE_H__ */

