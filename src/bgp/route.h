// ==================================================================
// @(#)route.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/11/2002
// $Id: route.h,v 1.16 2009-03-24 15:51:30 bqu Exp $
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

#ifndef __BGP_ROUTE_H__
#define __BGP_ROUTE_H__

#include <libgds/array.h>

#include <stdio.h>
#include <stdlib.h>

#include <bgp/route_reflector.h>
#include <bgp/types.h>

// ----- BGP Routes Output Formats -----
#define BGP_ROUTES_OUTPUT_CISCO     0
#define BGP_ROUTES_OUTPUT_MRT_ASCII 1
#define BGP_ROUTES_OUTPUT_CUSTOM    2
#define BGP_ROUTES_OUTPUT_DEFAULT   BGP_ROUTES_OUTPUT_CISCO

// Route flags
#define ROUTE_FLAG_FEASIBLE   0x0001 /* The route is feasible,
					i.e. the next-hop is reachable
					and the route has passed the
					input filters */
#define ROUTE_FLAG_ELIGIBLE   0x0002 /* The route is eligible, i.e. it
					has passed the input filters
				      */
#define ROUTE_FLAG_BEST       0x0004 /* The route is in the Loc-RIB
					and is installed in the
					routing table */
#define ROUTE_FLAG_ADVERTISED 0x0008 /* The route has been advertised
				      */
#define ROUTE_FLAG_DP_IGP     0x0010 /* The route has been chosen
					based on the IGP cost to the
					next-hop */
#define ROUTE_FLAG_INTERNAL   0x0020 /* The route is internal
					(i.e. added with the 'add
					network' statement */

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
#define ROUTE_FLAG_EXTERNAL_BEST 0x0040
#endif

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
#define ROUTE_FLAG_WALTON_BEST 0x0080 /* This flag is used to tell a neighbor
					 which was the best route selected by
					 the sender router. Then it is possible
					 to do the convergence as if it was the
					 normal BGP behavior without taking into
					 account the others routes received
					 from a specified neighbor.*/
#endif

/* QoS flags */
#define ROUTE_FLAG_BEST_EBGP  0x0100 /* best eBGP route */
#define ROUTE_FLAG_AGGR       0x0200 /* The route is a member of an
					aggregated route */

/* RR flags */
#define ROUTE_FLAG_RR_CLIENT  0x1000 /* The route was learned from an
					RR client */

#define ROUTE_PREF_DEFAULT 100
#define ROUTE_MED_MISSING  MAX_UINT32_T
#define ROUTE_MED_DEFAULT  0

#define ROUTE_SHOW_CISCO  0
#define ROUTE_SHOW_MRT    1
#define ROUTE_SHOW_CUSTOM 2


#ifdef __cplusplus
extern "C" {
#endif

  // ----- route_create ---------------------------------------------
  bgp_route_t * route_create(ip_pfx_t prefix, bgp_peer_t * peer,
			     net_addr_t next_hop, bgp_origin_t origin);
  // -----[ route_create_nlri ]--------------------------------------
  bgp_route_t * route_create_nlri(bgp_nlri_t nlri, bgp_peer_t * peer,
				  net_addr_t next_hop, bgp_origin_t origin,
				  bgp_attr_t * attr);
  // ----- route_destroy --------------------------------------------
  void route_destroy(bgp_route_t ** route_ref);
  // ----- route_flag_set -------------------------------------------
  void route_flag_set(bgp_route_t * route, uint16_t flag, int state);
  // ----- route_flag_get -------------------------------------------
  int route_flag_get(bgp_route_t * route, uint16_t flag);
  // ----- route_set_nexthop ----------------------------------------
  void route_set_nexthop(bgp_route_t * route, net_addr_t next_hop);
  // ----- route_get_nexthop ----------------------------------------
  net_addr_t route_get_nexthop(bgp_route_t * route);
  
  // ----- route_peer_set -------------------------------------------
  void route_peer_set(bgp_route_t * route, bgp_peer_t * peer);
  // ----- route_peer_get -------------------------------------------
  bgp_peer_t * route_peer_get(bgp_route_t * route);
  
  // ----- route_set_origin -----------------------------------------
  void route_set_origin(bgp_route_t * route, bgp_origin_t origin);
  // ----- route_get_origin -----------------------------------------
  bgp_origin_t route_get_origin(bgp_route_t * route);
  
  // -----[ route_get_path ]-----------------------------------------
  bgp_path_t * route_get_path(bgp_route_t * route);
  // -----[ route_set_path ]-----------------------------------------
  void route_set_path(bgp_route_t * route, bgp_path_t * path);
  // ----- route_path_prepend ---------------------------------------
  int route_path_prepend(bgp_route_t * route, asn_t asn, uint8_t amount);
  // -----[ route_path_rem_private ]---------------------------------
  int route_path_rem_private(bgp_route_t * route);
  // ----- route_path_contains --------------------------------------
  int route_path_contains(bgp_route_t * route, asn_t asn);
  // ----- route_path_length ----------------------------------------
  int route_path_length(bgp_route_t * route);
  // ----- route_path_last_as ---------------------------------------
  int route_path_last_as(bgp_route_t * route);
  
  // ----- route_set_comm -------------------------------------------
  void route_set_comm(bgp_route_t * route, bgp_comms_t * comms);
  // ----- route_comm_append ----------------------------------------
  int route_comm_append(bgp_route_t * route, bgp_comm_t comm);
  // ----- route_comm_strip -----------------------------------------
  void route_comm_strip(bgp_route_t * route);
  // ----- route_comm_remove ----------------------------------------
  void route_comm_remove(bgp_route_t * route, bgp_comm_t comm);
  // ----- route_comm_contains --------------------------------------
  int route_comm_contains(bgp_route_t * route, bgp_comm_t comm);
  
  // ----- route_ecomm_append ---------------------------------------
  int route_ecomm_append(bgp_route_t * route, bgp_ecomm_t * comm);
  // ----- route_ecomm_strip_non_transitive -------------------------
  void route_ecomm_strip_non_transitive(bgp_route_t * route);
  
  // ----- route_localpref_set --------------------------------------
  void route_localpref_set(bgp_route_t * route, uint32_t pref);
  // ----- route_localpref_get --------------------------------------
  uint32_t route_localpref_get(bgp_route_t * route);
  
  // ----- route_med_clear ------------------------------------------
  void route_med_clear(bgp_route_t * route);
  // ----- route_med_set --------------------------------------------
  void route_med_set(bgp_route_t * route, uint32_t med);
  // ----- route_med_get --------------------------------------------
  uint32_t route_med_get(bgp_route_t * route);
  
  // ----- route_originator_set -------------------------------------
  void route_originator_set(bgp_route_t * route, net_addr_t originator);
  // ----- route_originator_get -------------------------------------
  int route_originator_get(bgp_route_t * route, net_addr_t * originator);
  // ----- route_originator_clear -----------------------------------
  void route_originator_clear(bgp_route_t * route);
  
  // ----- route_cluster_list_set -----------------------------------
  void route_cluster_list_set(bgp_route_t * route);
  // ----- route_cluster_list_append --------------------------------
  void route_cluster_list_append(bgp_route_t * route,
				 bgp_cluster_id_t cluster_id);
  // ----- route_cluster_list_clear ---------------------------------
  void route_cluster_list_clear(bgp_route_t * route);
  // ----- route_cluster_list_contains ------------------------------
  int route_cluster_list_contains(bgp_route_t * route,
				  bgp_cluster_id_t cluster_id);
  
  // ----- route_copy -----------------------------------------------
  bgp_route_t * route_copy(bgp_route_t * route);
  // ----- route_equals ---------------------------------------------
  int route_equals(bgp_route_t * route1, bgp_route_t * route2);


  ///////////////////////////////////////////////////////////////////
  // DUMP FUNCTIONS
  ///////////////////////////////////////////////////////////////////
  
  // -----[ route_str2mode ]-----------------------------------------
  int route_str2mode(const char * str, uint8_t * mode);
  // ----- route_dump -----------------------------------------------
  void route_dump(gds_stream_t * stream, bgp_route_t * route);
  // ----- route_dump_cisco -----------------------------------------
  void route_dump_cisco(gds_stream_t * stream, bgp_route_t * route);
  // ----- route_dump_mrt -------------------------------------------
  void route_dump_mrt(gds_stream_t * stream, bgp_route_t * route);
  // ----- route_dump_custom ----------------------------------------
  void route_dump_custom(gds_stream_t * stream, bgp_route_t * route,
			 const char * format);


  ///////////////////////////////////////////////////////////////////
  // OPTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ route_set_show_mode ]------------------------------------
  void route_set_show_mode(uint8_t mode, const char * format);


  ///////////////////////////////////////////////////////////////////
  // INITIALIZATION & FINALIZATION
  ///////////////////////////////////////////////////////////////////
  
  // -----[ _bgp_route_destroy ]-------------------------------------
  void _bgp_route_destroy();

  
#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_H__ */
