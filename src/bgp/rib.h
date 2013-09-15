// ==================================================================
// @(#)rib.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 04/12/2002
// $Id: rib.h,v 1.8 2009-03-24 15:50:17 bqu Exp $
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

#ifndef __BGP_RIB_H__
#define __BGP_RIB_H__

#include <net/prefix.h>
#include <bgp/types.h>

#define RIB_OPTION_ALIAS 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- rib_create -----------------------------------------------
  bgp_rib_t * rib_create(uint8_t options);
  // ----- rib_destroy ----------------------------------------------
  void rib_destroy(bgp_rib_t ** rib_ref);
  // ----- rib_find_best --------------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  bgp_route_ts * rib_find_best(bgp_rib_t * rib, ip_pfx_t prefix);
  bgp_route_t * rib_find_one_best(bgp_rib_t * rib, ip_pfx_t prefix);
#else
  bgp_route_t * rib_find_best(bgp_rib_t * rib, ip_pfx_t prefix);
#endif
  // ----- rib_find_exact -------------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  bgp_route_ts * rib_find_exact(bgp_rib_t * rib, ip_pfx_t prefix);
  bgp_route_t * rib_find_one_exact(bgp_rib_t * rib, ip_pfx_t prefix,
				   net_addr_t *next_hop);
#else
  bgp_route_t * rib_find_exact(bgp_rib_t * rib, ip_pfx_t prefix);
#endif
  // ----- rib_add_route --------------------------------------------
  int rib_add_route(bgp_rib_t * rib, bgp_route_t * route);
  // ----- rib_replace_route ----------------------------------------
  int rib_replace_route(bgp_rib_t * rib, bgp_route_t * route);
  // ----- rib_remove_route -----------------------------------------
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  int rib_remove_route(bgp_rib_t * rib, ip_pfx_t prefix,
		       net_addr_t * next_hop);
#else
  int rib_remove_route(bgp_rib_t * rib, ip_pfx_t prefix);
#endif
  // ----- rib_for_each ---------------------------------------------
  int rib_for_each(bgp_rib_t * rib, FRadixTreeForEach fForEach,
		   void * context);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_RIB_H__ */
