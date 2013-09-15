// ==================================================================
// @(#)routes_list.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/02/2004
// $Id: routes_list.h,v 1.7 2009-03-24 15:53:12 bqu Exp $
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

#ifndef __ROUTES_LIST_H__
#define __ROUTES_LIST_H__

#include <libgds/array.h>

#define ROUTES_LIST_OPTION_REF 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- routes_list_create ---------------------------------------
  bgp_routes_t * routes_list_create(uint8_t options);
  // ----- routes_list_destroy --------------------------------------
  void routes_list_destroy(bgp_routes_t ** routes_ref);
  // ----- routes_list_append ---------------------------------------
  void routes_list_append(bgp_routes_t * routes, bgp_route_t * route);
  // ----- routes_list_remove_at ------------------------------------
  void routes_list_remove_at(bgp_routes_t * routes, unsigned int index);
  // ----- routes_list_dump -----------------------------------------
  void routes_list_dump(gds_stream_t * stream, bgp_routes_t * routes);
  // -----[ routes_list_for_each ]-----------------------------------
  int routes_list_for_each(bgp_routes_t * routes,
			   gds_array_foreach_f foreach,
			   void * ctx);

  // -----[ bgp_routes_size ]----------------------------------------
  static inline unsigned int bgp_routes_size(bgp_routes_t * routes) {
    return ptr_array_length(routes);
  }
  // -----[ bgp_routes_at ]------------------------------------------
  static inline bgp_route_t * bgp_routes_at(bgp_routes_t * routes,
					    unsigned int index) {
    return routes->data[index];
  }
  
#ifdef __cplusplus
}
#endif

#endif /* __ROUTES_LIST_H__ */
