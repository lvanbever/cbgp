// ==================================================================
// @(#)dp_rules.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/11/2002
// $Id: dp_rules.h,v 1.10 2009-03-24 14:20:33 bqu Exp $
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

#ifndef __BGP_DP_RULES_H__
#define __BGP_DP_RULES_H__

#include <libgds/array.h>

#include <bgp/types.h>
#include <bgp/routes_list.h>

typedef enum {
  BGP_MED_TYPE_DETERMINISTIC,
  BGP_MED_TYPE_ALWAYS_COMPARE,
  BGP_MED_TYPE_MAX
} bgp_med_type_t;

// ----- FDPRule -----
/**
 * Defines a decision process rule. A rule takes as arguments a router
 * and a set of possible routes. It removes routes from the set.
 * Note: the int return value is not used yet. Decision process rules
 * must return 0 for now.
 */
typedef int (*FDPRule)(bgp_router_t * router, bgp_routes_t * routes);

struct dp_rule_t {
  char    * name;
  FDPRule   rule;
};

// -----[ decision process ]-----------------------------------------
#define DP_NUM_RULES 11
extern struct dp_rule_t DP_RULES[DP_NUM_RULES];

// ----- FDPRouteCompare -----
/**
 * Defines a route comparison function prototype. The function must
 * return:
 *   0 if routes are equal
 *   > 0 if route1 > route2
 *   < 0 if route1 < route2
 * Note: function of this type MUST be deterministic, i.e. the
 * ordering they define must be complete
 */
typedef int (*FDPRouteCompare)(bgp_router_t * router,
			       bgp_route_t * route1,
			       bgp_route_t * route2);

#ifdef __cplusplus
extern "C" {
#endif 

  ///////////////////////////////////////////////////////////////////
  // OPTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ dp_rules_options_set_med_type ]--------------------------
  void dp_rules_options_set_med_type(bgp_med_type_t med_type);
  // -----[ dp_rules_str2med_type ]----------------------------------
  net_error_t dp_rules_str2med_type(const char * str,
				    bgp_med_type_t * med_type);


  ///////////////////////////////////////////////////////////////////
  // RULES
  ///////////////////////////////////////////////////////////////////

  int dp_rule_no_selection(bgp_router_t * router, bgp_routes_t * routes);
  int dp_rule_lowest_nh(bgp_router_t * router, bgp_routes_t * routes);
  int dp_rule_lowest_path(bgp_router_t * router, bgp_routes_t * routes);
  // ----- bgp_dp_rule_local ----------------------------------------
  void bgp_dp_rule_local(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_highest_pref -------------------------------------
  int dp_rule_highest_pref(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_shortest_path ------------------------------------
  int dp_rule_shortest_path(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_lowest_origin ------------------------------------
  int dp_rule_lowest_origin(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_lowest_med ---------------------------------------
  int dp_rule_lowest_med(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_ebgp_over_ibgp -----------------------------------
  int dp_rule_ebgp_over_ibgp(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_nearest_next_hop ---------------------------------
  int dp_rule_nearest_next_hop(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_lowest_router_id ---------------------------------
  int dp_rule_lowest_router_id(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_shortest_cluster_list ----------------------------
  int dp_rule_shortest_cluster_list(bgp_router_t * router, bgp_routes_t * routes);
  // ----- dp_rule_lowest_neighbor_address --------------------------
  int dp_rule_lowest_neighbor_address(bgp_router_t * router, bgp_routes_t * routes);
  
  // ----- dp_rule_lowest_delay ---------------------------------------
  int dp_rule_lowest_delay(bgp_router_t * router, bgp_routes_t * routes);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_DP_RULES_H__ */
