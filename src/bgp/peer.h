// ==================================================================
// @(#)peer.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @auhtor Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 24/11/2002
// $Id: peer.h,v 1.18 2009-03-24 14:29:05 bqu Exp $
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

#ifndef __PEER_H__
#define __PEER_H__

#include <libgds/stream.h>

#include <bgp/types.h>

extern char * SESSION_STATES[SESSION_STATE_MAX];

// ----- Peer flags -----
/* The peer is a route-reflector client */
#define PEER_FLAG_RR_CLIENT     0x01

/* The next-hop of the routes received from this peer is set to the
   local-peer when the route is re-advertised */
#define PEER_FLAG_NEXT_HOP_SELF 0x02

/* The peer is virtual, i.e. there is no real BGP session with a peer
   router */
#define PEER_FLAG_VIRTUAL       0x04

/* This option prevents the Adj-RIB-in of the virtual peer to be
   cleared when the session is closed. Moreover, the routes already
   available in the Adj-RIB-in will be taken into account by the
   decision process when the session with the virtual peer is
   restarted. This option is only available to virtual peers. */
#define PEER_FLAG_SOFT_RESTART  0x08

/* This flag is set if this session was configured automatically */ 
#define PEER_FLAG_AUTOCONF      0x10

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_peer_create ------------------------------------------
  bgp_peer_t * bgp_peer_create(uint16_t asn,
			       net_addr_t addr,
			       bgp_router_t * router);
  // ----- bgp_peer_destroy -----------------------------------------
  void bgp_peer_destroy(bgp_peer_t ** peer_ref);
  
  // ----- bgp_peer_flag_set ----------------------------------------
  void bgp_peer_flag_set(bgp_peer_t * peer, uint8_t flag, int state);
  // ----- bgp_peer_flag_get ----------------------------------------
  int bgp_peer_flag_get(bgp_peer_t * peer, uint8_t flag);
  // ----- bgp_peer_set_nexthop -------------------------------------
  int bgp_peer_set_nexthop(bgp_peer_t * peer, net_addr_t next_hop);
  // -----[ bgp_peer_set_source ]------------------------------------
  int bgp_peer_set_source(bgp_peer_t * peer, net_addr_t src_addr);

  ///////////////////////////////////////////////////////////////////
  // BGP FILTERS
  ///////////////////////////////////////////////////////////////////
  
  // -----[ bgp_peer_set_filter ]------------------------------------
  void bgp_peer_set_filter(bgp_peer_t * peer, bgp_filter_dir_t dir,
			   bgp_filter_t * filter);
  // -----[ bgp_peer_filter_get ]------------------------------------
  bgp_filter_t * bgp_peer_filter_get(bgp_peer_t * peer,
				     bgp_filter_dir_t dir);

  ///////////////////////////////////////////////////////////////////
  // BGP SESSION MANAGEMENT FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_peer_session_ok --------------------------------------
  int bgp_peer_session_ok(bgp_peer_t * peer);
  // ----- bgp_peer_session_refresh ---------------------------------
  void bgp_peer_session_refresh(bgp_peer_t * peer);
  // ----- peer_open_session ----------------------------------------
  int bgp_peer_open_session(bgp_peer_t * peer);
  // ----- peer_close_session ---------------------------------------
  int bgp_peer_close_session(bgp_peer_t * peer);
  // ----- peer_rescan_adjribin -------------------------------------
  void peer_rescan_adjribin(bgp_peer_t * peer, int clear);

  ///////////////////////////////////////////////////////////////////
  // BGP MESSAGE HANDLING
  ///////////////////////////////////////////////////////////////////

  // ----- bgp_peer_announce_route ----------------------------------
  void bgp_peer_announce_route(bgp_peer_t * peer, bgp_route_t * route);
  // ----- bgp_peer_withdraw_prefix ---------------------------------
  void bgp_peer_withdraw_prefix(bgp_peer_t * peer, ip_pfx_t prefix);
  // ----- bgp_peer_handle_message ----------------------------------
  int bgp_peer_handle_message(bgp_peer_t * peer, bgp_msg_t * msg);
  
  // ----- bgp_peer_route_eligible ----------------------------------
  int bgp_peer_route_eligible(bgp_peer_t * peer, bgp_route_t * route);
  // ----- bgp_peer_route_feasible ----------------------------------
  int bgp_peer_route_feasible(bgp_peer_t * peer, bgp_route_t * route);


  ///////////////////////////////////////////////////////////////////
  // INFORMATION RETRIEVAL
  ///////////////////////////////////////////////////////////////////
  
  // ----- bgp_peer_dump_id -----------------------------------------
  void bgp_peer_dump_id(gds_stream_t * stream, bgp_peer_t * peer);
  // ----- bgp_peer_dump --------------------------------------------
  void bgp_peer_dump(gds_stream_t * stream, bgp_peer_t * peer);
  // -----[ bgp_peer_show_info ]---------------------------------------
  void bgp_peer_show_info(gds_stream_t * stream, bgp_peer_t * peer);
  // ----- bgp_peer_dump_adjrib -------------------------------------
  void bgp_peer_dump_adjrib(gds_stream_t * stream, bgp_peer_t * peer,
				   ip_pfx_t prefix, bgp_rib_dir_t dir);
  // ----- bgp_peer_dump_filters ---------------------------------
  void bgp_peer_dump_filters(gds_stream_t * stream, bgp_peer_t * peer,
			     bgp_filter_dir_t dir);
  // -----[ bgp_peer_set_record_stream ]-----------------------------
  int bgp_peer_set_record_stream(bgp_peer_t * peer,
				 gds_stream_t * stream);
  // -----[ bgp_peer_send_enabled ]----------------------------------
  int bgp_peer_send_enabled(bgp_peer_t * peer);


  ///////////////////////////////////////////////////////////////////
  // WALTON DRAFT (EXPERIMENTAL)
  ///////////////////////////////////////////////////////////////////
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  void bgp_peer_withdraw_prefix_walton(bgp_peer_t * peer, ip_pfx_t prefix,
				       net_addr_t * next_hop);
  // -----[ bgp_peer_set_walton_limit ]------------------------------
  void bgp_peer_set_walton_limit(bgp_peer_t * peer,
				 unsigned int walton_limit);
  // -----[ bgp_peer_get_walton_limit ]------------------------------
  uint16_t bgp_peer_get_walton_limit(bgp_peer_t * peer);
#endif


#ifdef __cplusplus
}
#endif

#endif
