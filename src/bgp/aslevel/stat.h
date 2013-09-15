// ==================================================================
// @(#)as-level-stat.h
//
// Provide structures and functions to report some AS-level
// topologies statistics.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/06/2007
// $Id: stat.h,v 1.1 2009-03-24 13:39:08 bqu Exp $
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

#ifndef __BGP_ASLEVEL_STAT_H__
#define __BGP_ASLEVEL_STAT_H__

#include <libgds/stream.h>

#include <bgp/aslevel/as-level.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ aslevel_stat_degree ]------------------------------------
  void aslevel_stat_degree(gds_stream_t * stream, as_level_topo_t * topo,
			   int cumulated, int normalized,
			   int inverse);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ASLEVEL_STAT_H__ */
