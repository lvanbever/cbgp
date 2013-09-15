// ==================================================================
// @(#)dp_rt.h
//
// This file contains the routines used to install/remove entries in
// the node's routing table when BGP routes are installed, removed or
// updated.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/01/2007
// $Id: dp_rt.h,v 1.3 2009-03-24 14:13:05 bqu Exp $
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

#ifndef __BGP_DP_RT_H__
#define __BGP_DP_RT_H__

#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_router_rt_add_route ----------------------------------
  void bgp_router_rt_add_route(bgp_router_t * router, bgp_route_t * route);
  // ----- bgp_router_rt_del_route ----------------------------------
  void bgp_router_rt_del_route(bgp_router_t * router, ip_pfx_t prefix);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_DP_RT_H__ */
