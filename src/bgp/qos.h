// ==================================================================
// @(#)qos.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/11/2002
// $Id: qos.h,v 1.4 2009-03-24 15:49:01 bqu Exp $
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

#ifndef __BGP_QOS_H__
#define __BGP_QOS_H__

#include <libgds/array.h>

#include <net/prefix.h>
#include <bgp/types.h>

#ifdef BGP_QOS

// Minimum delay information
typedef struct {
  net_link_delay_t tDelay;
  net_link_delay_t tMean;
  uint8_t uWeight;
} qos_delay_t;

// Maximum reservable bandwidth
typedef struct {
  uint32_t uBandwidth;
  uint32_t uMean;
  uint8_t uWeight;
} qos_bandwidth_t;

typedef struct {
  
} bgp_route_qos_t;

extern unsigned int BGP_OPTIONS_QOS_AGGR_LIMIT;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- qos_route_aggregate ----------------------------------------
  bgp_route_t * qos_route_aggregate(SPtrArray * routes,
			       bgp_route_t * best_route);
  // ----- qos_route_init_delay ---------------------------------------
  void qos_route_init_delay(bgp_route_t * route);
  // ----- qos_route_init_bandwidth -----------------------------------
  void qos_route_init_bandwidth(bgp_route_t * route);
  // ----- qos_route_update_delay -------------------------------------
  int qos_route_update_delay(bgp_route_t * route, net_link_delay_t delay);
  // ----- qos_route_update_bandwidth ---------------------------------
  int qos_route_update_bandwidth(bgp_route_t * route,
				 uint32_t uBandwidth);
  // ----- qos_route_delay_equals -------------------------------------
  int qos_route_delay_equals(bgp_route_t * route1, bgp_route_t * route2);
  // ----- qos_route_bandwidth_equals ---------------------------------
  int qos_route_bandwidth_equals(bgp_route_t * route1, bgp_route_t * route2);
  // ----- qos_advertise_to_peer --------------------------------------
  int qos_advertise_to_peer(bgp_router_t * router, bgp_peer_t * peer,
			    bgp_route_t * route);
  // ----- qos_decision_process_tie_break -----------------------------
  void qos_decision_process_tie_break(bgp_router_t * router,
				      SPtrArray * routes,
				      unsigned int num_routes);
  // ----- qos_decision_process ---------------------------------------
  int qos_decision_process(bgp_router_t * router,
			   bgp_peer_t * origin_peer,
			   ip_pfx_t prefix);

void _qos_test();

#ifdef __cplusplus
}
#endif

#endif /* BGP_QOS */

#endif /* __BGP_QOS_H__ */
