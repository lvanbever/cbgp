// ==================================================================
// @(#)as.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// $Id: as.h,v 1.35 2009-03-24 13:55:54 bqu Exp $
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

#ifndef __BGP_ROUTER_H__
#define __BGP_ROUTER_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/types.h>
#include <libgds/list.h>

#include <bgp/route-input.h>
#include <bgp/types.h>
#include <net/network.h>

// ----- Global BGP options --------
#define BGP_OPT_AUTO_CREATE         0x01
#define BGP_OPT_RIB_OUT             0x02
#define BGP_OPT_VRIB_OUT            0x04
#define BGP_OPT_EXT_BEST            0x08
#define BGP_OPT_WALTON_CONV_ON_BEST 0x10

// ----- BGP Router Load RIB Options -----
#define BGP_ROUTER_LOAD_OPTIONS_SUMMARY  0x01  /* Display a summary (stderr) */
#define BGP_ROUTER_LOAD_OPTIONS_FORCE    0x02  /* Force the route to load */
#define BGP_ROUTER_LOAD_OPTIONS_AUTOCONF 0x04  /* Create non-existing peers */

extern const net_protocol_def_t PROTOCOL_BGP;


// -----[ FBGPMsgListener ]-----
typedef void (*FBGPMsgListener)(net_msg_t * msg, void * ctx);

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // OPTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ bgp_options_flag_set ]-----------------------------------
  void bgp_options_flag_set(uint8_t flag);
  // -----[ bgp_options_flag_reset ]---------------------------------
  void bgp_options_flag_reset(uint8_t flag);
  // -----[ bgp_options_flag_isset ]---------------------------------
  int bgp_options_flag_isset(uint8_t flag);
  // -----[ bgp_options_set_local_pref ]-----------------------------
  void bgp_options_set_local_pref(uint32_t local_pref);
  // -----[ bgp_options_get_local_pref ]-----------------------------
  uint32_t bgp_options_get_local_pref();


  ///////////////////////////////////////////////////////////////////
  // BGP ROUTER MANAGEMENT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ bgp_add_router ]-----------------------------------------
  net_error_t bgp_add_router(uint16_t asn, net_node_t * node,
			     bgp_router_t ** router_ref);

  // ----- bgp_router_create ----------------------------------------
  int bgp_router_create(uint16_t asn, net_node_t * node,
			bgp_router_t ** router_ref);
  // ----- bgp_router_destroy ---------------------------------------
  void bgp_router_destroy(bgp_router_t ** router_ref);
  // ----- bgp_router_get_name --------------------------------------
  char * bgp_router_get_name(bgp_router_t * router);
  // ----- bgp_router_add_peer --------------------------------------
  int bgp_router_add_peer(bgp_router_t * router, uint16_t remote_asn,
			  net_addr_t addr, bgp_peer_t ** peer_ref);
  // ----- bgp_router_find_peer -------------------------------------
  bgp_peer_t * bgp_router_find_peer(bgp_router_t * router, net_addr_t addr);
  // ----- bgp_router_peer_set_filter -------------------------------
  int bgp_router_peer_set_filter(bgp_router_t * router, net_addr_t addr,
				 bgp_filter_dir_t dir,
				 bgp_filter_t * filter);
  // ----- bgp_router_add_network -----------------------------------
  int bgp_router_add_network(bgp_router_t * router, ip_pfx_t prefix);
  // ----- bgp_router_del_network -----------------------------------
  int bgp_router_del_network(bgp_router_t * router, ip_pfx_t prefix);
  // ----- as_add_qos_network ---------------------------------------
  int as_add_qos_network(bgp_router_t * router, ip_pfx_t prefix,
			 net_link_delay_t delay);
  // ----- bgp_router_start -----------------------------------------
  int bgp_router_start(bgp_router_t * router);
  // ----- bgp_router_stop ------------------------------------------
  int bgp_router_stop(bgp_router_t * router);
  // ----- bgp_router_reset -----------------------------------------
  int bgp_router_reset(bgp_router_t * router);

  // ----- bgp_router_dump_adj_rib_in -------------------------------
  int bgp_router_dump_adj_rib_in(FILE * stream, bgp_router_t * router,
				 ip_pfx_t prefix);
  // ----- bgp_router_dump_rt_dp_rule -------------------------------
  int bgp_router_dump_rt_dp_rule(FILE * stream, bgp_router_t * router,
				 ip_pfx_t prefix);
  // ----- bgp_router_info ------------------------------------------
  void bgp_router_info(gds_stream_t * stream, bgp_router_t * router);
  // -----[ bgp_router_clear_rib ]-------------------------------
  int bgp_router_clear_rib(bgp_router_t * router);
  // -----[ bgp_router_clear_adj_rib ]-------------------------------
  int bgp_router_clear_adj_rib(bgp_router_t * router);
  
  // ----- bgp_router_decision_process_dop --------------------------
  /*void bgp_router_decision_process_dop(bgp_router_t * router,
    SPtrArray * pRoutes);*/
  // ----- bgp_router_decision_process_disseminate_to_peer ----------
  void bgp_router_decision_process_disseminate_to_peer(bgp_router_t * router,
						       ip_pfx_t prefix,
						       bgp_route_t * route,
						       bgp_peer_t * peer);
  // ----- bgp_router_decision_process_disseminate ------------------
  void bgp_router_decision_process_disseminate(bgp_router_t * router,
					       ip_pfx_t prefix,
					       bgp_route_t * route);
  // ----- bgp_router_get_best_routes -------------------------------
  bgp_routes_t * bgp_router_get_best_routes(bgp_router_t * router,
					    ip_pfx_t prefix);
  // ----- bgp_router_get_feasible_routes ---------------------------
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * router,
						ip_pfx_t prefix,
						uint8_t uEBGPRoute);
#else
  bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * router,
						ip_pfx_t prefix);
#endif
  // ----- bgp_router_decision_process ------------------------------
  int bgp_router_decision_process(bgp_router_t * router,
				  bgp_peer_t * origin_peer,
				  ip_pfx_t prefix);
  // ----- bgp_router_handle_message --------------------------------
  int bgp_router_handle_message(simulator_t * sim,
				void * router,
				net_msg_t * msg);
  // ----- bgp_router_ecomm_red_process -----------------------------
  int bgp_router_ecomm_red_process(bgp_peer_t * peer, bgp_route_t * route);
  // ----- bgp_router_dump_id ---------------------------------------
  void bgp_router_dump_id(gds_stream_t * stream, bgp_router_t * router);
  
  // -----[ bgp_router_rerun ]---------------------------------------
  int bgp_router_rerun(bgp_router_t * router, ip_pfx_t prefix);
  // -----[ bgp_router_peer_readv_prefix ]---------------------------
  int bgp_router_peer_readv_prefix(bgp_router_t * router,
				   bgp_peer_t * peer,
				   ip_pfx_t prefix);
  
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  // ----- bgp_router_walton_peer_set -------------------------------
  int bgp_router_walton_peer_set(bgp_peer_t * peer, unsigned int uWaltonLimit);
#endif
  
  // ----- bgp_router_scan_rib --------------------------------------
  int bgp_router_scan_rib(bgp_router_t * router);
  // ----- bgp_router_dump_networks ---------------------------------
  void bgp_router_dump_networks(gds_stream_t * stream,
				       bgp_router_t * router);
  // ----- bgp_router_networks_for_each -----------------------------
  int bgp_router_networks_for_each(bgp_router_t * router,
				   gds_array_foreach_f foreach,
				   void * ctx);
  // ----- bgp_router_dump_peers ------------------------------------
  void bgp_router_dump_peers(gds_stream_t * stream, bgp_router_t * router);
  // ----- bgp_router_peers_for_each --------------------------------
  int bgp_router_peers_for_each(bgp_router_t * router,
				gds_array_foreach_f foreach,
				void * ctx);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  int bgp_router_peer_rib_out_remove(bgp_router_t * router,
				     bgp_peer_t * peer,
				     ip_pfx_t prefix,
				     net_addr_t * next_hop);
#else
  int bgp_router_peer_rib_out_remove(bgp_router_t * router,
				     bgp_peer_t * peer,
				     ip_pfx_t prefix);
#endif
  // ----- bgp_router_dump_rib --------------------------------------
  void bgp_router_dump_rib(gds_stream_t * stream, bgp_router_t * router);
  // ----- bgp_router_dump_rib_address ------------------------------
  void bgp_router_dump_rib_address(gds_stream_t * stream,
				   bgp_router_t * router,
				   net_addr_t addr);
  // ----- bgp_router_dump_rib_prefix -------------------------------
  void bgp_router_dump_rib_prefix(gds_stream_t * stream,
				  bgp_router_t * router,
				  ip_pfx_t prefix);
  // ----- bgp_router_dump_adjrib -----------------------------------
  void bgp_router_dump_adjrib(gds_stream_t * stream,
			      bgp_router_t * router,
			      bgp_peer_t * peer, ip_pfx_t prefix,
			      bgp_rib_dir_t dir);

  ///////////////////////////////////////////////////////////////////
  // LOAD FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_router_load_rib --------------------------------------
  int bgp_router_load_rib(bgp_router_t * router, const char * filename,
			  bgp_input_type_t format, uint8_t options);

  ///////////////////////////////////////////////////////////////////
  // MISCELLANEOUS FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ bgp_router_show_stats ]----------------------------------
  void bgp_router_show_stats(gds_stream_t * stream, bgp_router_t * router);
  // -----[ bgp_router_show_routes_info ]----------------------------
  void bgp_router_show_routes_info(gds_stream_t * stream,
				   bgp_router_t * router,
				   ip_dest_t dest);
  // -----[ bgp_router_set_msg_listener ]----------------------------
  void bgp_router_set_msg_listener(FBGPMsgListener f, void * p);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTER_H__ */
