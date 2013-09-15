// ==================================================================
// @(#)auto-config.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/11/2005
// $Id: auto-config.c,v 1.11 2009-03-24 13:56:45 bqu Exp $
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <bgp/auto-config.h>

#include <assert.h>

#include <libgds/stream.h>

#include <bgp/as.h>
#include <bgp/peer.h>
#include <net/iface.h>
#include <net/network.h>
#include <net/node.h>

#define AUTO_CONFIG_LINK_WEIGHT 1

// -----[ bgp_auto_config_session ]----------------------------------
/**
 * This function creates a BGP session between the given router and
 * a remote router.
 *
 * The function first checks if the remote address corresponds to an
 * existing node. If not, the node is created. Then, the function
 * checks if a direct link exists between the router and the remote
 * node. If not, an unidirectional link is created. The IGP weight
 * associated with the link is AUTO_CONFIG_LINK_WEIGHT. In addition,
 * the link will not be advertised in the IGP. The reason for this is
 * that we do not want that further recomputation of the domain's IGP
 * routes take this link into account.
 *
 * The next operation for the function is now to check if the router
 * has a route towards the remote node. If not the function will add
 * in the router a static route towards the remote node. In addition,
 * if there was no route, the 'next-hop-self' option will be used on
 * the new peering session since the remote node will not be reachable
 * from other routers in the domain.
 *
 * Finaly, the function will check if the remote node supports the BGP
 * protocol. If not, support for the BGP protocol will not be added,
 * but the 'virtual' option will be used on the new peering
 * session. The new peering session can now be added. If the remote
 * node supports BGP, the BGP session must be added on both
 * sides. Otherwise, it must only be added on the local router's side.
 */
int bgp_auto_config_session(bgp_router_t * router,
			    net_addr_t tRemoteAddr,
			    uint16_t uRemoteAS,
			    bgp_peer_t ** peer_ref)
{
  net_node_t * node;
  net_iface_t * iface;
  ip_pfx_t prefix;
  int iUseNextHopSelf= 0;
  bgp_peer_t * peer;
  net_error_t error;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "AUTO-CONFIG ");
    bgp_router_dump_id(gdsdebug, router);
    stream_printf(gdsdebug, " NEIGHBOR AS%d:", uRemoteAS);
    ip_address_dump(gdsdebug, tRemoteAddr);
    stream_printf(gdsdebug, "\n");
  }

  // (1). If node does not exist, create it.
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "PHASE (1) CHECK NODE EXISTENCE\n");
  node= network_find_node(network_get_default(), tRemoteAddr);
  if (node == NULL) {
    error= node_create(tRemoteAddr, &node, NODE_OPTIONS_LOOPBACK);
    if (error != ESUCCESS)
      return error;
    error= network_add_node(network_get_default(), node);
    if (error != ESUCCESS)
      return error;
  }
  
  // (2). If there is no direct link to the node, create it. The
  // reason for this choice is that we create the neighbors based on a
  // RIB dump and the neighbors are supposed to be connected over
  // single-hop eBGP sessions.
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "PHASE (2) CHECK LINK EXISTENCE\n");
  iface= node_find_iface(router->node, net_iface_id_addr(node->rid));
  if (iface == NULL) {
    
    // Create the link in one direction only. IGP weight is set
    // to AUTO_CONFIG_LINK_WEIGHT. The IGP_ADV flag is also removed
    // from the new link.
    assert(net_link_create_rtr(router->node, node, UNIDIR, &iface)
	   == ESUCCESS);
    assert(net_iface_set_metric(iface, 0, AUTO_CONFIG_LINK_WEIGHT, UNIDIR)
	   == ESUCCESS);
  }

  // (3). Check if there is a route towards the remote node.
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "PHASE (3) CHECK ROUTE EXISTENCE\n");
  if (node_rt_lookup(router->node, tRemoteAddr) == NULL) {
    // We should also add a route towards this node. Let's
    // assume that 'next-hop-self' will be used for this session
    // and add a static route towards the next-hop. The link is
    // therefore not advertised within the IGP domain.
    prefix.network= tRemoteAddr;
    prefix.mask= 32;
    assert(node_rt_add_route_link(router->node, prefix, iface,
				  tRemoteAddr, AUTO_CONFIG_LINK_WEIGHT,
				  NET_ROUTE_STATIC) == 0);
    iUseNextHopSelf= 1;
  }
  
  // Add a new BGP session.
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "PHASE (4) ADD BGP SESSION ");
  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    bgp_router_dump_id(gdsdebug, router);
    stream_printf(gdsdebug, " --> ");
    ip_address_dump(gdsdebug, tRemoteAddr);
    stream_printf(gdsdebug, "\n");
  }
  if (bgp_router_add_peer(router, uRemoteAS, tRemoteAddr, &peer) != 0) {
    STREAM_ERR(STREAM_LEVEL_FATAL, "ERROR: could not create peer\n");
    abort();
  }
  bgp_peer_flag_set(peer, PEER_FLAG_AUTOCONF, 1);
  
  // If peer does not support BGP, create it virtual. Otherwise, also
  // create the session in the remote BGP router.
  if (protocols_get(node->protocols, NET_PROTOCOL_BGP) == NULL)
    bgp_peer_flag_set(peer, PEER_FLAG_VIRTUAL, 1);
  else {
    // TODO: we should create the BGP session in the reverse
    // direction...
    STREAM_ERR(STREAM_LEVEL_FATAL, "ERROR: Code not implemented\n");
    abort();
  }
  
  // Set the next-hop-self flag (this is required since the
  // next-hop is not advertised in the IGP).
  if (iUseNextHopSelf)
    bgp_peer_flag_set(peer, PEER_FLAG_NEXT_HOP_SELF, 1);
  
  // Open the BGP session.
  assert(bgp_peer_open_session(peer) == 0);

  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "DONE :-)\n");

  *peer_ref= peer;

  return ESUCCESS;
}
