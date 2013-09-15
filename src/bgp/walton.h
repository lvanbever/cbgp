// ==================================================================
// @(#)walton.h
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 15/02/2006
// $Id: walton.h,v 1.5 2009-03-24 15:55:46 bqu Exp $
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

#ifndef __WALTON_H__
#define __WALTON_H__

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/array.h>

#include <bgp/as.h>
#include <bgp/peer.h>

#ifdef __cplusplus
extern "C" {
#endif

  // ----- bgp_router_walton_init ------------------------------------
  void bgp_router_walton_init(bgp_router_t * pRouter);
  // ----- bgp_router_walton_finalize --------------------------------
  void bgp_router_walton_finalize(bgp_router_t * pRouter);
  // ---- bgp_router_walton_peer_remove ------------------------------
  void bgp_router_walton_peer_remove(bgp_router_t * pRouter,
				     bgp_peer_t * pPeer);
  // ----- bgp_router_walton_unsynchronized_all ----------------------
  void bgp_router_walton_unsynchronized_all(bgp_router_t * pRouter);
  // ----- bgp_router_walton_peer_set --------------------------------
  int bgp_router_walton_peer_set(bgp_peer_t * pPeer,
				 unsigned int uWaltonLimit);
  // ----- bgp_router_walton_dp_disseminate --------------------------
  void bgp_router_walton_dp_disseminate(bgp_router_t * pRouter, 
					bgp_peers_t * pPeers,
					ip_pfx_t prefix,
					bgp_routes_t * pRoutes);
  // ---- bgp_router_walton_disseminate_select_peers -----------------
  void bgp_router_walton_disseminate_select_peers(bgp_router_t * pRouter, 
						  bgp_routes_t * pRoutes, 
						  uint16_t iNextHopCount);
  // ----- bgp_router_walton_decision_process_run -------------------
  int bgp_router_walton_decision_process_run(bgp_router_t * pRouter,
					     bgp_routes_t * pRoutes);

#ifdef __cplusplus
}
#endif

#endif // EXPERIMENTAL_WALTON

#endif //__WALTON_H__
