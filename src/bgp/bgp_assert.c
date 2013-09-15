// ==================================================================
// @(#)bgp_assert.c
//
// @Author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 08/03/2004
// $Id: bgp_assert.c,v 1.18 2009-03-24 14:08:04 bqu Exp $
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

#include <libgds/array.h>
#include <libgds/trie.h>
#include <libgds/types.h>

#include <net/network.h>
#include <net/protocol.h>
#include <bgp/as.h>
#include <bgp/bgp_assert.h>
#include <bgp/attr/path.h>
#include <bgp/peer.h>
#include <bgp/record-route.h>
#include <bgp/rib.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>

// -----[ _build_router_list_rtfe ]----------------------------------
static int _build_router_list_rtfe(uint32_t key, uint8_t key_len,
				   void * item, void * ctx)
{
  ptr_array_t * rl= (ptr_array_t *) ctx;
  net_node_t * node= (net_node_t *) item;
  net_protocol_t * protocol;

  protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
  if (protocol != NULL)
    ptr_array_append(rl, protocol->handler);
  return 0;
}

// ----- build_router_list ------------------------------------------
static ptr_array_t * build_router_list()
{
  ptr_array_t * rl= ptr_array_create_ref(0);
  network_t * network= network_get_default();

  // Build list of BGP routers
  trie_for_each(network->nodes, _build_router_list_rtfe, rl);
  return rl;
}

// ----- bgp_assert_reachability ------------------------------------
/**
 * This function checks that all the advertised networks are reachable
 * from all the BGP routers.
 */
int bgp_assert_reachability()
{
  ptr_array_t * rl;
  int iIndexSrc, iIndexDst, iIndexNet;
  bgp_router_t * routerSrc, * routerDst;
  bgp_path_t * path;
  bgp_route_t * route;
  int result= 0;

  rl= build_router_list();

  // All routers as source...
  for (iIndexSrc= 0; iIndexSrc < ptr_array_length(rl); iIndexSrc++) {
    routerSrc= (bgp_router_t *) rl->data[iIndexSrc];

    // All routers as destination...
    for (iIndexDst= 0; iIndexDst < ptr_array_length(rl); iIndexDst++) {
      routerDst= (bgp_router_t *) rl->data[iIndexDst];
      
      if (routerSrc != routerDst) {

	// For all advertised networks in the destination router
	for (iIndexNet= 0;
	     iIndexNet < ptr_array_length(routerDst->local_nets);
	     iIndexNet++) {
	  route= (bgp_route_t *) routerDst->local_nets->data[iIndexNet];

	  // Check BGP reachability
	  if (bgp_record_route(routerSrc, route->prefix, &path, 0)) {
	    stream_printf(gdsout, "Assert: ");
	    bgp_router_dump_id(gdsout, routerSrc);
	    stream_printf(gdsout, " can not reach ");
	    ip_prefix_dump(gdsout, route->prefix);
	    stream_printf(gdsout, "\n");
	    result= -1;
	  }
	  path_destroy(&path);

	}
	
      }
    }
  }

  ptr_array_destroy(&rl);

  return result;
}

// ----- bgp_assert_peerings ----------------------------------------
/**
 * This function checks that all defined peerings are valid, i.e. the
 * peers exist and are reachable.
 */
int bgp_assert_peerings()
{
  unsigned int index, peer_index;
  ptr_array_t * rl;
  bgp_router_t * router;
  int result= 0;
  //net_node_t * node;
  //SNetProtocol * protocol;
  bgp_peer_t * peer;
  int iBadPeerings= 0;

  rl= build_router_list();

  // For all BGP instances...
  for (index= 0; index < ptr_array_length(rl); index++) {
    router= (bgp_router_t *) rl->data[index];

    stream_printf(gdsout, "check router ");
    bgp_router_dump_id(gdsout, router);
    stream_printf(gdsout, "\n");

    // For all peerings...
    for (peer_index= 0; peer_index < ptr_array_length(router->peers);
	 peer_index++) {
      peer= (bgp_peer_t *) router->peers->data[peer_index];

      stream_printf(gdsout, "\tcheck peer ");
      bgp_peer_dump_id(gdsout, peer);
      stream_printf(gdsout, "\n");

      // Check existence of BGP peer
      /*node= network_find_node(peer->addr);
      if (node != NULL) {

	// Check support for BGP protocol
	pProtocol= protocols_get(node->pProtocols, NET_PROTOCOL_BGP);
	if (pProtocol != NULL) {*/
	  
	  // Check for reachability
	  if (!bgp_peer_session_ok(peer)) {
	    stream_printf(gdsout, "Assert: ");
	    bgp_router_dump_id(gdsout, router);
	    stream_printf(gdsout, "'s peer ");
	    ip_address_dump(gdsout, peer->addr);
	    stream_printf(gdsout, " is not reachable\n");
	    result= -1;
	    iBadPeerings++;
	  }

	  /*} else {
	  fprintf(stdout, "Assert: ");
	  ip_address_dump(stdout, router->node->addr);
	  fprintf(stdout, "'s peer ");
	  ip_address_dump(stdout, peer->addr);
	  fprintf(stdout, " does not support BGP\n");
	  result= -1;
	  iBadPeerings++;
	}

      } else {
	fprintf(stdout, "Assert: ");
	ip_address_dump(stdout, router->node->addr);
	fprintf(stdout, "'s peer ");
	ip_address_dump(stdout, peer->addr);
	fprintf(stdout, " does not exist\n");
	result= -1;
	iBadPeerings++;
	}*/
      
    }
  }

  ptr_array_destroy(&rl);

  return result;
}

// ----- bgp_assert_full_mesh ---------------------------------------
int bgp_assert_full_mesh(asn_t asn)
{
  int result= 0;

  return result;
}

// ----- bgp_assert_sessions ----------------------------------------
int bgp_assert_sessions()
{
  int result= 0;

  return result;
}

// ----- bgp_router_assert_best -------------------------------------
/**
 * This function checks that the router has a best route towards the
 * given prefix. In addition, the function checks that this route has
 * the given next-hop.
 *
 * Return: 0 on success, -1 on failure
 */
int bgp_router_assert_best(bgp_router_t * router, ip_pfx_t prefix,
			   net_addr_t next_hop)
{
  bgp_route_t * route;

  // Get the best route
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  route= rib_find_one_exact(router->loc_rib, prefix, NULL);
#else
  route= rib_find_exact(router->loc_rib, prefix);
#endif

  // Check that it exists
  if (route == NULL)
    return -1;

  // Check the next-hop
  if (route_get_nexthop(route) != next_hop)
    return -1;

  return 0;
}

// ----- bgp_router_assert_feasible ---------------------------------
/**
 * This function checks that the router has a route towards the given
 * prefix with the given next-hop.
 *
 * Return: 0 on success, -1 on failure
 */
int bgp_router_assert_feasible(bgp_router_t * router, ip_pfx_t prefix,
			       net_addr_t next_hop)
{
  bgp_routes_t * routes;
  bgp_route_t * route;
  unsigned int index;
  int result= -1;

  // Get the feasible routes
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  routes = bgp_router_get_feasible_routes(router, prefix, 0);
#else
  routes= bgp_router_get_feasible_routes(router, prefix);
#endif

  // Find a route with the given next-hop
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);
    if (route_get_nexthop(route) == next_hop) {
      result= 0;
      break;
    }
  }

  routes_list_destroy(&routes);

  return result;
}
