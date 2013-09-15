// ==================================================================
// @(#)rt_filter.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/06/2008
// $Id: rt_filter.h,v 1.2 2009-03-24 16:26:53 bqu Exp $
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

#ifndef __NET_RT_FILTER_H__
#define __NET_RT_FILTER_H__

#include <net/prefix.h>
#include <net/iface.h>
#include <net/net_types.h>

// -----[ rt_filter_t ]----------------------------------------------
/**
 * This structure defines a filter to remove routes from the routing
 * table. The possible criteria are as follows:
 * - prefix  : exact match of destination prefix (NULL => match any)
 * - iface   : exact match of outgoing link (NULL => match any)
 * - gateway : exact match of gateway (NULL => match any)
 * - type    : exact match of type (mandatory)
 * All the given criteria have to match a route in order for it to be
 * deleted.
 */
typedef struct {
  ip_pfx_t         * prefix;
  net_iface_t      * oif;
  net_addr_t       * gateway;
  net_route_type_t   type;
  int                matches;
  ptr_array_t      * del_list;
} rt_filter_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ rt_filter_create ]-----------------------------------------
  rt_filter_t * rt_filter_create();
  // -----[ rt_filter_destroy ]----------------------------------------
  void rt_filter_destroy(rt_filter_t ** filter_ptr);
  // -----[ rt_filter_matches ]----------------------------------------
  int rt_filter_matches(rt_info_t * rtinfo, rt_filter_t * filter);


#ifdef __cplusplus
}
#endif

#endif /* __NET_RT_FILTER_H__ */
