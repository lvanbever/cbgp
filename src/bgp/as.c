// ==================================================================
// @(#)as.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Pradeep Bangera (pradeep.bangera@imdea.org)
// @date 22/11/2002
// $Id: as.c,v 1.79 2009-06-25 14:25:36 bqu Exp $
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
// TO-DO LIST:
// - do not keep in local_nets a _copy_ of the BGP routes locally
//   originated. Store a reference instead => reduce memory footprint.
// - re-check IGP/BGP interaction
// - check initialization and comparison of Router-ID (it was
//   previously not exchanged in the OPEN message).
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/enumerator.h>
#include <libgds/list.h>
#include <libgds/stream.h>
#include <libgds/memory.h>
#include <libgds/str_util.h>
#include <libgds/tokenizer.h>

#include <bgp/as.h>
#include <bgp/auto-config.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/ecomm.h>
#include <bgp/attr/path.h>
#include <bgp/domain.h>
#include <bgp/dp_rt.h>
#include <bgp/dp_rules.h>
#include <bgp/filter/filter.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/peer-list.h>
#include <bgp/qos.h>
#include <bgp/rib.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>
#include <bgp/route-input.h>
#include <bgp/tie_breaks.h>
#include <bgp/walton.h>
#include <net/network.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/node.h>
#include <net/protocol.h>
#include <ui/output.h>
#include <util/str_format.h>


/////////////////////////////////////////////////////////////////////
//
// OPTIONS
//
/////////////////////////////////////////////////////////////////////

typedef struct _options_t {
  uint8_t           flags;
  FBGPMsgListener   listener;
  void            * listener_ctx;
  uint32_t          local_pref;
} _options_t;
static _options_t _default_options= {
  .flags       = 0,
  .listener    = NULL,
  .listener_ctx= NULL,
  .local_pref  = 0,
};

// -----[ bgp_options_flag_set ]-------------------------------------
void bgp_options_flag_set(uint8_t flag)
{
  _default_options.flags|= flag;
}

// -----[ bgp_options_flag_reset ]-----------------------------------
void bgp_options_flag_reset(uint8_t flag)
{
  _default_options.flags&= ~flag;
} 

// -----[ bgp_options_flag_isset ]-----------------------------------
int bgp_options_flag_isset(uint8_t flag)
{
  return (_default_options.flags & flag);
}

// -----[ bgp_options_set_local_pref ]-------------------------------
void bgp_options_set_local_pref(uint32_t local_pref)
{
  _default_options.local_pref= local_pref;
}

// -----[ bgp_options_get_local_pref ]-------------------------------
uint32_t bgp_options_get_local_pref()
{
  return _default_options.local_pref;
}


/////////////////////////////////////////////////////////////////////
//
// DEBUG
//
/////////////////////////////////////////////////////////////////////

#define BGP_ROUTER_DEBUG

#ifdef BGP_ROUTER_DEBUG
static int ___debug_for_each(gds_stream_t * stream, void * context,
			   char format)
{
  va_list * ap= (va_list*) context;
  char * format_str= "%?";
  net_addr_t addr;
  bgp_router_t * router;
  ip_pfx_t pfx;
  bgp_route_t * route;

  switch (format) {
  case 'a':
    addr= va_arg(*ap, net_addr_t);
    ip_address_dump(stream, addr);
    break;
  case 'p':
    pfx= va_arg(*ap, ip_pfx_t);
    ip_prefix_dump(stream, pfx);
    break;
  case 'o':
    route= va_arg(*ap, bgp_route_t *);
    route_dump(stream, route);
    break;
  case 'r':
    router= va_arg(*ap, bgp_router_t *);
    bgp_router_dump_id(stream, router);
    break;
  default:
    format_str[1]= format;
    stream_vprintf(stream, format_str, *ap);
  }
  return 0;
}
#endif /* BGP_ROUTER_DEBUG */

static inline
void ___debug(const char * format, ...)
{
#ifdef BGP_ROUTER_DEBUG
  va_list ap;

  va_start(ap, format);
  str_format_for_each(gdsout, ___debug_for_each, &ap, format);
  va_end(ap);
#endif /* BGP_ROUTER_DEBUG */
}


/////////////////////////////////////////////////////////////////////
//
// PROTOCOL HANDLER DEFINITION
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_proto_destroy ]---------------------------------------
static void _bgp_proto_destroy(void ** handler_ref)
{
  bgp_router_destroy((bgp_router_t **) handler_ref);
}

// -----[ _bgp_proto_dump_msg ]--------------------------------------
static void _bgp_proto_dump_msg(gds_stream_t * stream, net_msg_t * msg)
{
  bgp_msg_dump(stream, NULL, (bgp_msg_t *) msg->payload);
}

const net_protocol_def_t PROTOCOL_BGP= {
  .name= "bgp",
  .ops= {
    .handle      = bgp_router_handle_message,
    .destroy     = _bgp_proto_destroy,
    .dump_msg    = _bgp_proto_dump_msg,
    .destroy_msg = NULL,
    .copy_payload= NULL,
  }
};


/////////////////////////////////////////////////////////////////////
//
// BGP MESSAGE LISTENER
//
/////////////////////////////////////////////////////////////////////

// -----[ bgp_router_set_msg_listener ]------------------------------
void bgp_router_set_msg_listener(FBGPMsgListener listener, void * ctx)
{
  _default_options.listener= listener;
  _default_options.listener_ctx= ctx;
}

// -----[ _bgp_router_msg_listener ]---------------------------------
static inline void _bgp_router_msg_listener(net_msg_t * msg)
{
  if (_default_options.listener != NULL)
    _default_options.listener(msg, _default_options.listener_ctx);
}


/////////////////////////////////////////////////////////////////////
//
// BGP ROUTER OPERATIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_router_select_rid ]-----------------------------------
/**
 * The BGP Router-ID of this router should be the highest IP address
 * of the router with preference to loopback interfaces.
 * In the current C-BGP implementation, there is only one loopback
 * interface and its IP address is the node's identifier.
 */
static inline int _bgp_router_select_rid(net_node_t * node, net_addr_t * addr)
{
  return net_ifaces_get_highest(node->ifaces, addr);
}

// -----[ bgp_add_router ]-------------------------------------------
/**
 *
 */
int bgp_add_router(uint16_t asn, net_node_t * node,
		   bgp_router_t ** router_ref)
{
  int result;
  bgp_router_t * router;

  result= bgp_router_create(asn, node, &router);
  if (result != ESUCCESS)
    return result;

  // Register BGP protocol into the node
  result= node_register_protocol(node, NET_PROTOCOL_BGP, router);
  if (result != ESUCCESS)
    return result;

  if (router_ref != NULL)
    *router_ref= router;
  return ESUCCESS;
}

// -----[ bgp_router_create ]----------------------------------------
/**
 * Create a BGP router in the given node.
 *
 * Arguments:
 * - the AS-number of the domain the router will belong to
 * - the underlying node
 */
int bgp_router_create(uint16_t asn, net_node_t * node,
		      bgp_router_t ** router_ref)
{
  bgp_router_t * router;
  net_addr_t rid;
  int result;

  result= _bgp_router_select_rid(node, &rid);
  if (result != ESUCCESS)
    return result;

  router= (bgp_router_t*) MALLOC(sizeof(bgp_router_t));
  router->asn= asn;
  router->rid= rid;
  router->peers= bgp_peers_create();
  router->loc_rib= rib_create(0);
  router->local_nets= routes_list_create(ROUTES_LIST_OPTION_REF);
  router->cluster_id= router->rid;
  router->reflector= 0;

  // Reference to the node running this BGP router
  router->node= node;
  // Register the router into its AS
  bgp_domain_add_router(get_bgp_domain(asn), router);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_router_walton_init(router);
#endif 

  *router_ref= router;
  return ESUCCESS;
}

// -----[ bgp_router_destroy ]---------------------------------------
/**
 * Free the memory used by the given BGP router.
 */
void bgp_router_destroy(bgp_router_t ** router_ref)
{
  unsigned int index;
  bgp_route_t * route;

  if (*router_ref != NULL) {
    bgp_peers_destroy(&(*router_ref)->peers);
    rib_destroy(&(*router_ref)->loc_rib);
    for (index= 0; index < bgp_routes_size((*router_ref)->local_nets);
	index++) {
      route= bgp_routes_at((*router_ref)->local_nets, index);
      route_destroy(&route);
    }
    ptr_array_destroy(&(*router_ref)->local_nets);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    bgp_router_walton_finalize((*router_ref));
#endif
    FREE(*router_ref);
    *router_ref= NULL;
  }
}

// -----[ bgp_router_find_peer ]-------------------------------------
/**
 * Find a BGP neighbor in a BGP router. Lookup is done based on
 * neighbor's address.
 *
 * Returns:
 *   NULL in case the neighbor was not found.
 *   pointer to neighbor if it exists
 */
bgp_peer_t * bgp_router_find_peer(bgp_router_t * router, net_addr_t addr)
{
  return bgp_peers_find(router->peers, addr);
}

// -----[ bgp_router_add_peer ]--------------------------------------
/**
 * Add a BGP neighbor to a BGP router.
 *
 * Conditions:
 *   - router must not own peer's address.
 *   - same router must not exist
 *
 * Returns:
 *   ESUCCESS in case of success (also returns a pointer to the new
 *                                peer if ppeer != NULL)
 *   < 0 in case of failure
 */
int bgp_router_add_peer(bgp_router_t * router, uint16_t asn,
			net_addr_t addr, bgp_peer_t ** peer_ref)
{
  bgp_peer_t * new_peer;
  int result;

  // Router must not own peer's address
  if (node_has_address(router->node, addr))
    return EBGP_PEER_INVALID_ADDR;

  new_peer= bgp_peer_create(asn, addr, router);
  
  result= bgp_peers_add(router->peers, new_peer);
  if (result != ESUCCESS) {
    bgp_peer_destroy(&new_peer);
    return result;
  }

  if (peer_ref != NULL)
    *peer_ref= new_peer;

  return ESUCCESS;
}

// ----- bgp_router_peer_set_filter ---------------------------------
/**
 *
 */
int bgp_router_peer_set_filter(bgp_router_t * router, net_addr_t addr,
			       bgp_filter_dir_t dir, bgp_filter_t * filter)
{
  bgp_peer_t * peer;

  if ((peer= bgp_peers_find(router->peers, addr)) != NULL) {
    bgp_peer_set_filter(peer, dir, filter);
    return ESUCCESS;
  }
  return EBGP_PEER_UNKNOWN;
}

// -----[ _bgp_router_add_route ]------------------------------------
/**
 * Add a route in the Loc-RIB
 */
static inline int _bgp_router_add_route(bgp_router_t * router,
					bgp_route_t * route)
{
  routes_list_append(router->local_nets, route);
  rib_add_route(router->loc_rib, route_copy(route));
  bgp_router_decision_process_disseminate(router, route->prefix, route);
  return ESUCCESS;
}

// ----- bgp_router_add_network -------------------------------------
/**
 * This function adds a route towards the given prefix. This route
 * will be originated by this router.
 */
int bgp_router_add_network(bgp_router_t * router, ip_pfx_t prefix)
{
  bgp_route_t * route;
  unsigned int index;

  // Check that a route with the same prefix does not already exist
  for (index= 0; index < bgp_routes_size(router->local_nets); index++) {
    route= bgp_routes_at(router->local_nets, index);
    if (!ip_prefix_cmp(&route->prefix, &prefix))
      return EBGP_NETWORK_DUPLICATE;
  }

  // Create route and make it feasible (i.e. add flags BEST,
  // FEASIBLE and INTERNAL)
  route= route_create(prefix, NULL, router->rid, BGP_ORIGIN_IGP);
  route_flag_set(route, ROUTE_FLAG_BEST, 1);
  route_flag_set(route, ROUTE_FLAG_FEASIBLE, 1);
  route_flag_set(route, ROUTE_FLAG_INTERNAL, 1);
  route_localpref_set(route, _default_options.local_pref);
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  route_flag_set(route, ROUTE_FLAG_EXTERNAL_BEST, 0);
#endif
  return _bgp_router_add_route(router, route);
}

// ----- bgp_router_del_network -------------------------------------
/**
 * This function removes a locally originated network.
 */
int bgp_router_del_network(bgp_router_t * router, ip_pfx_t prefix)
{
  bgp_route_t * route= NULL;
  unsigned int index;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "bgp_router_del_network(");
    bgp_router_dump_id(gdsdebug, router);
    stream_printf(gdsdebug, ", ");
    ip_prefix_dump(gdsdebug, prefix);
    stream_printf(gdsdebug, ")\n");
  }

  // Look for the route and mark it as unfeasible
  for (index= 0; index < bgp_routes_size(router->local_nets);
      index++) {
    route= bgp_routes_at(router->local_nets, index);
    if (!ip_prefix_cmp(&route->prefix, &prefix)) {
      route_flag_set(route, ROUTE_FLAG_FEASIBLE, 0);
      break;
    }
    route= NULL;
  }

  // If the network exists:
  if (route != NULL) {

    // Withdraw this route
    // NOTE: this should be made through a call to
    // as_decision_process, but some details need to be fixed before:
    // pOriginPeer is NULL, and the local networks should be
    // advertised as other routes...
    /// *****

    // Remove the route from the Loc-RIB

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    rib_remove_route(router->loc_rib, prefix, NULL);
#else
    rib_remove_route(router->loc_rib, prefix);
#endif

    // Remove the route from the list of local networks.
    routes_list_remove_at(router->local_nets, index);

    // Free route
    route_destroy(&route);

    //stream_printf(gdsdebug, "RUN DECISION PROCESS...\n");

    bgp_router_decision_process_disseminate(router, prefix, NULL);

    return ESUCCESS;
  }
  return EUNSPECIFIED;
}

// ----- as_add_qos_network -----------------------------------------
/**
 *
 */
#ifdef BGP_QOS
int as_add_qos_network(bgp_router_t * router, ip_pfx_t prefix,
		       net_link_delay_t delay)
{
  bgp_route_t * route= route_create(prefix, NULL,
				    router->rid, BGP_ORIGIN_IGP);
  route_flag_set(route, ROUTE_FLAG_FEASIBLE, 1);
  route_localpref_set(route, _default_options.local_pref);

  qos_route_update_delay(route, delay);
  route->tDelay.uWeight= 1;
  qos_route_update_bandwidth(route, 0);
  route->tBandwidth.uWeight= 1;

  return _bgp_router_add_route(router, route);
}
#endif

// ----- bgp_router_ecomm_process -----------------------------------
/**
 * Apply output extended communities:
 *   - REDISTRIBUTION COMMUNITIES
 *
 * Returns:
 *   0 => Ignore route (destroy)
 *   1 => Redistribute
 */
int bgp_router_ecomm_process(bgp_peer_t * peer, bgp_route_t * route)
{
  unsigned int index;
  uint8_t action_param;
  bgp_ecomm_t * comm;

  if (route->attr->ecomms != NULL) {

    for (index= 0; index < ecomms_length(route->attr->ecomms); index++) {
      comm= ecomms_get_at(route->attr->ecomms, index);
      switch (comm->type_high) {
	case ECOMM_RED:
	  if (ecomm_red_match(comm, peer)) {
	    switch ((comm->type_low >> 3) & 0x07) {
	      case ECOMM_RED_ACTION_PREPEND:
		action_param= (comm->type_low & 0x07);
		route_path_prepend(route, peer->router->asn,
				   action_param);
		break;
	      case ECOMM_RED_ACTION_NO_EXPORT:
		route_comm_append(route, COMM_NO_EXPORT);
		break;
	      case ECOMM_RED_ACTION_IGNORE:
		return 0;
		break;
	      default:
		abort();
	    }
	  }
	  break;
      }
    }
  }
  return 1;
}

// -----[ _bgp_router_advertise_to_peer ]----------------------------
/**
 * Advertise a route to given peer:
 *
 *   + route-reflectors:
 *     - do not redistribute a route from a non-client peer to
 *       non-client peers
 *     - do not redistribute a route from a client peer to the
 *       originator client peer
 *   - do not redistribute a route learned through iBGP to an
 *     iBGP peer
 *   - avoid sending to originator peer
 *   - avoid sending to a peer in AS-Path (SSLD)
 *   - check standard communities (NO_ADVERTISE and NO_EXPORT)
 *   - apply redistribution communities
 *   - strip non-transitive extended communities
 *   - update Next-Hop (next-hop-self/next-hop)
 *   - prepend AS-Path (if redistribution to an external peer)
 */
static inline int _bgp_router_advertise_to_peer(bgp_router_t * router,
						bgp_peer_t * dst_peer,
						bgp_route_t * route)
{
#define INTERNAL 0
#define EXTERNAL 1
#define LOCAL    2
  static char * acLocType[3]= { "INT", "EXT", "LOC" };

  bgp_route_t * new_route= NULL;
  int from;
  int to;
  bgp_peer_t * src_peer= NULL;

  /*STREAM_DEBUG(STREAM_LEVEL_INFO, "ADVERTISE_TO_PEER (");
  bgp_router_dump_id(gdserr, router);
  STREAM_DEBUG(STREAM_LEVEL_INFO, ") (");
  bgp_peer_dump_id(gdserr, dst_peer);
  STREAM_DEBUG(STREAM_LEVEL_INFO, ")\n");
  STREAM_DEBUG(STREAM_LEVEL_INFO, "  route = ");
  route_dump(gdserr, route);
  STREAM_DEBUG(STREAM_LEVEL_INFO, "\n");*/

  // Determine route origin type (internal/external/local)
  if (route->peer != NULL) {
    src_peer= route_peer_get(route);
    if (router->asn == route_peer_get(route)->asn)
      from= INTERNAL;
    else
      from= EXTERNAL;
  } else {
    from= LOCAL;
  }
  
  // Determine route destination type (internal/external)
  if (router->asn == dst_peer->asn)
    to= INTERNAL;
  else
    to= EXTERNAL;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "advertise_to_peer (");
    ip_prefix_dump(gdsdebug, route->prefix);
    stream_printf(gdsdebug, ") from ");
    bgp_router_dump_id(gdsdebug, router);
    stream_printf(gdsdebug, " to ");
    bgp_peer_dump_id(gdsdebug, dst_peer);
    stream_printf(gdsdebug, " (%s --> %s)\n",
	       acLocType[from], acLocType[to]);
  }
  
#ifdef __ROUTER_LIST_ENABLE__
  // Append the router-list with the router-id of this router
  cluster_list_append(route->attr->router_list, router->rid);
#endif

  // Do not redistribute to the originator neighbor peer
  if ((from != LOCAL) &&
      (dst_peer->router_id == src_peer->router_id)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered(next-hop-peer)\n");
    return -1;
  }

  // Do not redistribute to other peers
  if (route_comm_contains(route, COMM_NO_ADVERTISE)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered(comm_no_advertise)\n");
    return -1;
  }

  // Copy the route. This is required since subsequent filters may
  // alter the route's attributes !!
  new_route= route_copy(route);

  // ************** ROUTE-REDISTRIBUTION MATRIX ******************
  //
  // +----------+----------+------+------------------------------+
  // | FROM     | TO       | iBGP | ROUTE-REFLECTOR              |
  // +----------+----------+------+------------------------------+
  // | EXTERNAL | EXTERNAL |  OK  |   OK                         | (1)
  // | INTERNAL | EXTERNAL |  OK  |   OK                         | (2)
  // | LOCAL    | EXTERNAL |  OK  |   OK                         | (3)
  // | EXTERNAL | INTERNAL |  OK  |   OK                         | (4)
  // | INTERNAL | INTERNAL | ---- |                              | (5)
  // | client   | client   |      |   OK (update Originator-ID,  | (5a)
  // |          |          |      |       Cluster-ID-List)       |
  // | n-client | client   |      |   OK (update Originator-ID,  | (5b)
  // |          |          |      |       Cluster-ID-List)       |
  // | n-client | n-client |      |  ----                        | (5c)
  // | LOCAL    | INTERNAL |  OK  |   OK                         | (6)
  // +----------+----------+------+------------------------------+
  //
  // ****************************************************************
  switch (to) {
  case EXTERNAL: // case (1), (2) and (3)
    // Do not redistribute outside confederation (here AS)
    if (route_comm_contains(new_route, COMM_NO_EXPORT)) {
      STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered(comm_no_export)\n");
      route_destroy(&new_route);
      return -1;
    }
    // Avoid loop creation (SSLD, Sender-Side Loop Detection)
    if (route_path_contains(new_route, dst_peer->asn)) {
      STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered(ssld)\n");
      route_destroy(&new_route);
      return -1;
    }
    // Clear Originator and Cluster-ID-List fields
    route_originator_clear(new_route);
    route_cluster_list_clear(new_route);
    break;

  case INTERNAL:
    switch (from) {
    case EXTERNAL: // case (4)
      break;
    case INTERNAL: // case (5)
      if (router->reflector) {

	// Do not redistribute from non-client to non-client (5c)
	if (!route_flag_get(new_route, ROUTE_FLAG_RR_CLIENT) &&
	    !bgp_peer_flag_get(dst_peer, PEER_FLAG_RR_CLIENT)) {
	  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered (RR: non-client --> non-client)\n");
	  route_destroy(&new_route);
	  return -1;
	}

	// Update Originator-ID if missing (< 0 => missing)
	if (route_originator_get(new_route, NULL) < 0)
	  route_originator_set(new_route, src_peer->router_id);
	else {
	  if (originator_equals(new_route->attr->originator,
				&dst_peer->router_id)) {
	    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered (originator-id SSLD)\n");
	    route_destroy(&new_route);
	    return -1;
	  }
	}

	// Create or append Cluster-ID-List
	route_cluster_list_append(new_route, router->cluster_id);

      } else {
	STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered(iBGP-peer --> iBGP-peer)\n");
	route_destroy(&new_route);
	return -1;
      }
      break;

    case LOCAL: // case (6)
      // Set Originator-ID
      if (router->reflector)
	route_originator_set(new_route, router->rid);
      break;
    }
    
    break;
    
  }
  
  // Check output filter and extended communities
  if (!bgp_router_ecomm_process(dst_peer, new_route)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered (ext-community)\n");
    route_destroy(&new_route);
    return -1;
  }

  // Remove non-transitive communities
  route_ecomm_strip_non_transitive(new_route);

  // Discard MED if advertising to an external peer
  // (must be done before applying policy filters)
  if (to == EXTERNAL)
    route_med_clear(new_route);
  
  // Apply policy filters (output)
  if (!filter_apply(dst_peer->filter[FILTER_OUT], router, new_route)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "out-filtered (policy)\n");
    route_destroy(&new_route);
    return -1;
  }

  // Update attributes before redistribution:
  // - next-hop
  // - AS-Path
  // - RR
  //
  // +----------+----------+------------------------+
  // | FROM     | TO       | ACTIONS                |
  // +----------+----------+------------------------+
  // |          | EXTERNAL | next-hop, AS-path      |
  // | INTERNAL | INTERNAL | ---                    |
  // | EXTERNAL | INTERNAL | next-hop               |
  // *----------+----------+------------------------+
  if (to == EXTERNAL) {
	
    // Change the route's next-hop to this router's RID
    // (or optionally to a specified next-hop)
    if (dst_peer->next_hop != NET_ADDR_ANY)
      route_set_nexthop(new_route, dst_peer->next_hop);
    else
      route_set_nexthop(new_route, router->rid);
    
    // Prepend AS-Number
    route_path_prepend(new_route, router->asn, 1);
    
  } else if (to == INTERNAL) {
    
    // Change the route's next-hop to this router (next-hop-self)
    // or to an optionally specified next-hop. RESTRICTION: this
    // is prohibited if the route is being reflected.
    if (from == EXTERNAL) {
      if (bgp_peer_flag_get(route_peer_get(new_route),
			    PEER_FLAG_NEXT_HOP_SELF)) {
	route_set_nexthop(new_route, router->rid);
      } else if (dst_peer->next_hop != NET_ADDR_ANY) {
	route_set_nexthop(new_route, dst_peer->next_hop);
      }
    }
    
  }
  
  bgp_peer_announce_route(dst_peer, new_route);
  return 0;
}

// -----[ _bgp_router_withdraw_to_peer ]-----------------------------
/**
 * Withdraw the given prefix to the given peer router.
 */
static inline void _bgp_router_withdraw_to_peer(bgp_router_t * router,
						bgp_peer_t * peer,
						ip_pfx_t prefix)
{
  STREAM_DEBUG(STREAM_LEVEL_DEBUG,
	    "*** AS%d withdraw to AS%d ***\n", router->asn,
	    peer->asn);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  // WARNING : It is not used anymore, we don't udate then
#else
  bgp_peer_withdraw_prefix(peer, prefix);
#endif

}

// ----- bgp_router_start -------------------------------------------
/**
 * Run the BGP router. The router starts by sending its local prefixes
 * to its peers.
 */
int bgp_router_start(bgp_router_t * router)
{
  unsigned int index;

  // Open all BGP sessions
  for (index= 0; index < bgp_peers_size(router->peers); index++)
    if (bgp_peer_open_session(bgp_peers_at(router->peers, index)) != 0)
      return -1;
  return ESUCCESS;
}

// ----- bgp_router_stop --------------------------------------------
/**
 * Stop the BGP router. Each peering session is brought to IDLE
 * state.
 */
int bgp_router_stop(bgp_router_t * router)
{
  unsigned int index;

  // Close all BGP sessions
  for (index= 0; index < bgp_peers_size(router->peers); index++)
    if (bgp_peer_close_session(bgp_peers_at(router->peers, index)) != 0)
      return -1;
  return ESUCCESS;
}

// ----- bgp_router_reset -------------------------------------------
/**
 * This function shuts down every peering session, then restarts
 * them.
 */
int bgp_router_reset(bgp_router_t * router)
{
  int result;

  // Stop the router
  result= bgp_router_stop(router);
  if (result != ESUCCESS)
    return result;

  // Start the router
  result= bgp_router_start(router);
  if (result != ESUCCESS)
    return result;

  return ESUCCESS;
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
typedef struct {
  bgp_router_t * router;
  bgp_routes_t * pBestEBGPRoutes;
} SSecondBestCtx;

int bgp_router_copy_ebgp_routes(void * pItem, void * pContext)
{
  SSecondBestCtx * pSecondRoutes = (SSecondBestCtx *) pContext;
  bgp_route_t * route = *(bgp_route_t ** )pItem;

  if (route->peer->asn != pSecondRoutes->router->asn) {
    if (pSecondRoutes->pBestEBGPRoutes == NULL)
      pSecondRoutes->pBestEBGPRoutes = routes_list_create(ROUTES_LIST_OPTION_REF);
    routes_list_append(pSecondRoutes->pBestEBGPRoutes, route);
  }
  return 0;
}
#endif

// -----[ bgp_router_peer_rib_out_remove ]---------------------------
/**
 * Check in the peer's Adj-RIB-in if it has received from this router
 * a route towards the given prefix.
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
int bgp_router_peer_rib_out_remove(bgp_router_t * router,
				   bgp_peer_t * peer,
				   ip_pfx_t prefix,
				   net_addr_t * next_hop)
#else
int bgp_router_peer_rib_out_remove(bgp_router_t * router,
				   bgp_peer_t * peer,
				   ip_pfx_t prefix)
#endif
{
  int iWithdrawRequired= 0;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  if (next_hop != NULL) {
    if (rib_find_one_exact(peer->adj_rib[RIB_OUT], prefix, next_hop) != NULL) {
      rib_remove_route(peer->asj_rib[RIB_OUT], prefix, next_hop);
      iWithdrawRequired = 1;
    }
  } else {
    if (rib_find_exact(peer->adj_rib[RIB_OUT], prefix) != NULL) {
      rib_remove_route(peer->adj_rib[RIB_OUT], prefix, next_hop);
      iWithdrawRequired = 1;
    }
  }
#else /* __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__ */
  if (rib_find_exact(peer->adj_rib[RIB_OUT], prefix) != NULL) {
    rib_remove_route(peer->adj_rib[RIB_OUT], prefix);
    iWithdrawRequired= 1;
  }
#endif /* __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__ */

  return iWithdrawRequired;
}

// -----[ _bgp_router_decision_process_run ]-------------------------
/**
 * Select one route with the following rules (see the actual
 * definition of the decision process in the global array DP_RULES[]):
 *
 *   1. prefer highest LOCAL-PREF
 *   2. prefer shortest AS-PATH
 *   3. prefer lowest ORIGIN (IGP < EGP < INCOMPLETE)
 *   4. prefer lowest MED
 *   5. prefer eBGP over iBGP
 *   6. prefer nearest next-hop (IGP)
 *   7. prefer shortest CLUSTER-ID-LIST
 *   8. prefer lowest neighbor router-ID
 *   9. final tie-break (prefer lowest neighbor IP address)
 *
 * Note: Originator-ID should be substituted to router-ID in rule (8)
 * if route has been reflected (i.e. if Originator-ID is present in
 * the route)
 *
 * The function returns the index of the rule that broke the ties
 * incremented by 1. If the returned value is 0, that means that there
 * was a single rule (no choice). Otherwise, if 1 is returned, that
 * means that the Local-Pref rule broke the ties, and so on...
 */
static inline
unsigned int _bgp_router_decision_process_run(bgp_router_t * router,
					      bgp_routes_t * routes)
{
  unsigned int index;

  // Apply the decision process rules in sequence until there is 1 or
  // 0 route remaining or until all the rules were applied.
  for (index= 0; index < DP_NUM_RULES; index++) {
    if (bgp_routes_size(routes) <= 1)
      break;
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "rule: [ %s ]\n", DP_RULES[index].name);
    DP_RULES[index].rule(router, routes);
  }

  // Check that at most a single best route will be returned.
  if (bgp_routes_size(routes) > 1) {
    STREAM_ERR(STREAM_LEVEL_FATAL, "Error: decision process did not return a single best route\n");
    abort();
  }
  
  return index;
}

// -----[ bgp_router_peer_rib_out_replace ]--------------------------
/**
 *
 */
void bgp_router_peer_rib_out_replace(bgp_router_t * router,
				     bgp_peer_t * peer,
				     bgp_route_t * new_route)
{
  rib_replace_route(peer->adj_rib[RIB_OUT], new_route);
}

// ----- bgp_router_decision_process_disseminate_to_peer ------------
/**
 * This function is responsible for the dissemination of a route to a
 * peer. The route is only propagated if the session with the peer is
 * in state ESTABLISHED.
 *
 * If the route is NULL, the function will send an explicit withdraw
 * to the peer if a route for the same prefix was previously sent to
 * this peer.
 *
 * If the route is not NULL, the function will check if it can be
 * advertised to this peer (checking with output filters). If the
 * route is accepted, it is sent to the peer. Otherwise, an explicit
 * withdraw is sent if a route for the same prefix was previously sent
 * to this peer.
 */
void bgp_router_decision_process_disseminate_to_peer(bgp_router_t * router,
						     ip_pfx_t prefix,
						     bgp_route_t * route,
						     bgp_peer_t * peer)
{
  bgp_route_t * new_route;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  net_addr_t next_hop;
#endif

  if (peer->session_state != SESSION_STATE_ESTABLISHED)
    return;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "DISSEMINATE (");
    ip_prefix_dump(gdsdebug, prefix);
    stream_printf(gdsdebug, ") from ");
    bgp_router_dump_id(gdsdebug, router);
    stream_printf(gdsdebug, " to ");
    bgp_peer_dump_id(gdsdebug, peer);
    stream_printf(gdsdebug, "\n");
  }

  if (route == NULL) {
    // A route was advertised to this peer => explicit withdraw
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    if (bgp_router_peer_rib_out_remove(router, peer, prefix, NULL)) {
      bgp_peer_withdraw_prefix(peer, prefix, NULL);
#else
      if (bgp_router_peer_rib_out_remove(router, peer, prefix)) {
	bgp_peer_withdraw_prefix(peer, prefix);
#endif
	STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\texplicit-withdraw\n");
      }
    } else {

      new_route= route_copy(route);
      if (_bgp_router_advertise_to_peer(router, peer, new_route) == 0) {
	STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\treplaced\n");
	bgp_router_peer_rib_out_replace(router, peer, new_route);
      } else {
	route_destroy(&new_route);
	
	STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\tfiltered\n");
	
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	next_hop= route_get_nexthop(route);
	if (bgp_router_peer_rib_out_remove(router, peer, prefix, &next_hop)) {
	  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\texplicit-withdraw\n");
	  //As it may be an eBGP session, the Next-Hop may have changed!
	  //If the next-hop has changed the path identifier as well ... :)
	  if (peer->asn != router->asn)
	    next_hop= router->rid;
	  else
	    next_hop= route_get_nexthop(route);
	  bgp_peer_withdraw_prefix(peer, prefix, &(next_hop));
#else
	  if (bgp_router_peer_rib_out_remove(router, peer, prefix)) {
	    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\texplicit-withdraw\n");
	    bgp_peer_withdraw_prefix(peer, prefix);
#endif
	    
	  }
	}
      }
    }
    
    
// ----- bgp_router_decision_process_disseminate --------------------
/**
 * Disseminate route to Adj-RIB-Outs.
 *
 * If there is no best route, then
 *   - if a route was previously announced, send an explicit withdraw
 *   - otherwize do nothing
 *
 * If there is one best route, then send an update. If a route was
 * previously announced, it will be implicitly withdrawn.
 */
void bgp_router_decision_process_disseminate(bgp_router_t * router,
					     ip_pfx_t prefix,
					     bgp_route_t * route)
{
  unsigned int index;
  bgp_peer_t * peer;

  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    peer= bgp_peers_at(router->peers, index);
    if (bgp_peer_send_enabled(peer)) {
      bgp_router_decision_process_disseminate_to_peer(router, prefix,
						      route, peer);
    }
  }
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_disseminate_external_best(bgp_router_t * router,
							  ip_pfx_t prefix,
							  bgp_route_t * route)
{
  unsigned int index;
  bgp_peer_t * peer;

  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    peer= bgp_peers_at(router->peers, index);
    if (router->asn == peer->asn)
      bgp_router_decision_process_disseminate_to_peer(router, prefix,
						      route, peer);
  }
}
#endif

// ----- bgp_router_best_flag_off -----------------------------------
/**
 * Clear the BEST flag from the given Loc-RIB route. Clear the BEST
 * flag from the corresponding Adj-RIB-In route as well.
 *
 * PRE: the route is in the Loc-RIB (=> the route is local or there
 * exists a corresponding route in an Adj-RIB-In).
 */
void bgp_router_best_flag_off(bgp_route_t * old_route)
{
  bgp_route_t * adj_route;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  net_addr_t next_hop;
#endif

  /* Remove BEST flag from old route in Loc-RIB. */
  route_flag_set(old_route, ROUTE_FLAG_BEST, 0);

  /* If the route is not LOCAL, then there is a reference to the peer
     that announced it. */
  if (old_route->peer != NULL) {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    next_hop= route_get_nexthop(old_route);
    adj_route= rib_find_one_exact(old_route->peer->adj_rib[RIB_IN],
				  old_route->prefix, &next_hop);
#else
    adj_route= rib_find_exact(old_route->peer->adj_rib[RIB_IN],
			      old_route->prefix);
#endif

    /* It is possible that the route does not exist in the Adj-RIB-In
       if the best route has been withdrawn. So check if the route in
       the Adj-RIB-In exists... */
    if (adj_route != NULL) {    
    
      /* Remove BEST flag from route in Adj-RIB-In. */
      route_flag_set(adj_route, ROUTE_FLAG_BEST, 0);
    }
  }
}

// ----- bgp_router_feasible_route ----------------------------------
/**
 * This function updates the ROUTE_FLAG_FEASIBLE flag of the given
 * route. If the next-hop of the route is reachable, the flag is set
 * and the function returns 1. Otherwise, the flag is cleared and the
 * function returns 0.
 */
int bgp_router_feasible_route(bgp_router_t * router, bgp_route_t * route)
{
  const rt_entries_t * rtentries;

  /* Get the route towards the next-hop */
  rtentries= node_rt_lookup(router->node, route->attr->next_hop);

  if (rtentries != NULL) {
    route_flag_set(route, ROUTE_FLAG_FEASIBLE, 1);
    return 1;
  } else {
    route_flag_set(route, ROUTE_FLAG_FEASIBLE, 0);
    return 0;
  }
}

// ----- bgp_router_get_best_routes ---------------------------------
/**
 * This function retrieves from the Loc-RIB the best route(s) towards
 * the given prefix.
 */
 bgp_routes_t * bgp_router_get_best_routes(bgp_router_t * router,
					   ip_pfx_t prefix)
   {
     bgp_routes_t * routes;
     bgp_route_t * route;
     
     routes= routes_list_create(ROUTES_LIST_OPTION_REF);
     
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
     route= rib_find_one_exact(router->loc_rib, prefix, NULL);
#else
     route= rib_find_exact(router->loc_rib, prefix);
#endif
     
  if (route != NULL)
    routes_list_append(routes, route);

  return routes;
 }
 
// ----- bgp_router_get_feasible_routes -----------------------------
/**
 * This function retrieves from the Adj-RIB-ins the feasible routes
 * towards the given prefix.
 *
 * Note: this function will update the 'feasible' flag of the eligible
 * routes.
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
 bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * router,
					       ip_pfx_t prefix,
					       const uint8_t uOnlyEBGP)
#else
   bgp_routes_t * bgp_router_get_feasible_routes(bgp_router_t * router,
						 ip_pfx_t prefix)
#endif
   {
  bgp_routes_t * routes;
  bgp_peer_t * peer;
  bgp_route_t * route;
  unsigned int index;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_routes_t * routesSeek;
  uint16_t uIndex;
#endif

  routes= routes_list_create(ROUTES_LIST_OPTION_REF);

  // Get from the Adj-RIB-ins the list of available routes.
  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    peer= bgp_peers_at(router->peers, index);

    /* Check that the peering session is in ESTABLISHED state */
    if (peer->session_state == SESSION_STATE_ESTABLISHED) {

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
      if (peer->asn != router->asn || uOnlyEBGP == 0) {
#endif
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      routesSeek = rib_find_exact(peer->adj_rib[RIB_IN], prefix);
      if (routesSeek != NULL) {
	for (uIndex = 0; uIndex < bgp_routes_size(routesSeek); uIndex++) {
	  route = bgp_routes_at(routesSeek, uIndex);
#else
      route= rib_find_exact(peer->adj_rib[RIB_IN], prefix);
#endif

      /* Check that a route is present in the Adj-RIB-in of this peer
	 and that it is eligible (according to the in-filters) */
      if ((route != NULL) &&
	  (route_flag_get(route, ROUTE_FLAG_ELIGIBLE))) {
	
	
	/* Check that the route is feasible (next-hop reachable, and
	   so on). Note: this call will actually update the 'feasible'
	   flag of the route. */
	if (bgp_router_feasible_route(router, route)) {
	  routes_list_append(routes, route);
	} 
      }

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	}
      }
#endif
    }
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    }
#endif
  }

  return routes;
}

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
// ----- bgp_router_check_dissemination_external_best ----------------
/**
 *
 */
void bgp_router_check_dissemination_external_best(bgp_router_t * router, 
			  bgp_routes_t * pEBGPRoutes, bgp_route_t * pOldEBGPRoute, 
			  bgp_route_t * pEBGPRoute, ip_pfx_t prefix)
{
  pEBGPRoute = (bgp_route_t *) bgp_routes_at(pEBGPRoutes, 0);
  STREAM_DEBUG("\tnew-external-best: ");
  STREAM_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pEBGPRoute);
  STREAM_DEBUG("\n");

  if ((pOldEBGPRoute == NULL) || 
    !route_equals(pOldEBGPRoute, pEBGPRoute)) {

    if (pOldEBGPRoute != NULL)
      STREAM_DEBUG("\t*** UPDATED EXTERNAL BEST ROUTE ***\n");
    else
      STREAM_DEBUG("\t*** NEW EXTERNAL BEST ROUTE ***\n");

    if (pOldEBGPRoute != NULL)
      route_flag_set(pOldEBGPRoute, ROUTE_FLAG_EXTERNAL_BEST, 0);

    route_flag_set(pEBGPRoute, ROUTE_FLAG_EXTERNAL_BEST, 1);
    route_flag_set(pEBGPRoutes->data[0], ROUTE_FLAG_EXTERNAL_BEST, 1);

    bgp_router_decision_process_disseminate_external_best(router, 
						  prefix, pEBGPRoute);
  } /*else {
    route_destroy(&pEBGPRoute);
  }*/

}

// ----- bgp_router_get_external_best --------------------------------
/**
 *
 */
bgp_route_t * bgp_router_get_external_best(bgp_routes_t * routes)
{
  int iIndex;

  for (iIndex = 0; iIndex < bgp_routes_size(routes); iIndex++) {
    if (route_flag_get(bgp_routes_at(routes, iIndex), ROUTE_FLAG_EXTERNAL_BEST))
      return bgp_routes_at(routes, iIndex);
  }
  return NULL;
}

// ----- bgp_router_get_old_ebgp_route -------------------------------
/**
 *
 */
bgp_route_t * bgp_router_get_old_ebgp_route(bgp_routes_t * routes, 
					  bgp_route_t * pOldRoute)
{
  bgp_route_t * pOldEBGPRoute= NULL;
  //Is pOldRoute route also the best EBGP route?
  if (bgp_options_flag_isset(BGP_OPT_EXT_BEST)) {
    //If the old best route also was the best external route, 
    //it has not been announced as a best external route but 
    //as the main best route!
    if ((pOldRoute != NULL) && 
	route_flag_get(pOldRoute, ROUTE_FLAG_EXTERNAL_BEST))
      pOldEBGPRoute = NULL;
    else
      pOldEBGPRoute = bgp_router_get_external_best(routes);
    STREAM_DEBUG("\told-external-best: ");
    STREAM_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), 
						      pOldEBGPRoute);
    STREAM_DEBUG("\n");
  }
  return pOldEBGPRoute;
}
#endif

// ---- bgp_router_decision_process_no_best_route --------------------
/**
 *
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_no_best_route(bgp_router_t * router, 
				ip_pfx_t prefix, bgp_route_t * old_route, 
				bgp_route_t * old_ebgp_route)
#else
void bgp_router_decision_process_no_best_route(bgp_router_t * router, 
				ip_pfx_t prefix, bgp_route_t * old_route)
#endif
{
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\t*** NO BEST ROUTE ***\n");

  // If a route towards this prefix was previously installed, then
  // withdraw it. Otherwise, do nothing...
  if (old_route != NULL) {
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    if (bgp_options_flag_isset(BGP_OPT_EXT_BEST) &&
	(old_ebgp_route != NULL)) {
      route_flag_set(old_bgp_route, ROUTE_FLAG_EXTERNAL_BEST, 0);
    }
#endif

/*
  stream_printf(gdserr, "OLD-ROUTE[%p]: ", pOldRoute);
  route_dump(gdserr, pOldRoute);
  stream_printf(gdserr, "; PEER[%p]: ", pOldRoute->peer);
  bgp_peer_dump(gdserr, pOldRoute->peer);
  stream_printf(gdserr, "; ADJ-RIB-IN[%p]", pOldRoute->peer->adj_ribIn);
  stream_printf(gdserr, "\n");
*/

    // THIS FUNCTION CALL MUST APPEAR BEFORE THE ROUTE IS REMOVED FROM
    // THE LOC-RIB (AND DESTROYED) !!!
    bgp_router_best_flag_off(old_route);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    rib_remove_route(router->loc_rib, prefix, NULL);
#else
    rib_remove_route(router->loc_rib, prefix);
#endif

/*
  stream_printf(gdserr, "OLD-ROUTE[%p]: ", pOldRoute);
  route_dump(gdserr, pOldRoute);
  stream_printf(gdserr, "; PEER[%p]: ", pOldRoute->peer);
  bgp_peer_dump(gdserr, pOldRoute->peer);
  stream_printf(gdserr, "; ADJ-RIB-IN[%p]", pOldRoute->peer->adj_ribIn);
  stream_printf(gdserr, "\n");
*/

    bgp_router_rt_del_route(router, prefix);
    bgp_router_decision_process_disseminate(router, prefix, NULL);
  }
}

// ----- bgp_router_decision_process_unchanged_best_route ------------
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_unchanged_best_route(bgp_router_t * router, 
					ip_pfx_t prefix, bgp_routes_t * routes, 
					bgp_route_t * pOldRoute, bgp_route_t * route, 
					int iRank, bgp_routes_t * pEBGPRoutes,
					bgp_route_t * pOldEBGPRoute)
#else
void bgp_router_decision_process_unchanged_best_route(bgp_router_t * router, 
					ip_pfx_t prefix, bgp_routes_t * routes, 
					bgp_route_t * pOldRoute, bgp_route_t * route, 
					int iRank)
#endif
{

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_route_t * pEBGPRoute;
#endif
  /**************************************************************
   * In this case, both routes (old and new) are equal from the
   * BGP point of view. There is thus no need to send BGP
   * messages.
   *
   * However, it is possible that the IGP next-hop of the route
   * has changed. If so, the route must be updated in the routing
   * table!
   **************************************************************/

  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\t*** BEST ROUTE UNCHANGED ***\n");

  /* Mark route in Adj-RIB-In as best (since it has probably been
     replaced). */
  route_flag_set(route, ROUTE_FLAG_BEST, 1);
  route_flag_set(bgp_routes_at(routes, 0), ROUTE_FLAG_BEST, 1);

  /* Update ROUTE_FLAG_DP_IGP of old route */
  route_flag_set(pOldRoute, ROUTE_FLAG_DP_IGP,
		 route_flag_get(route, ROUTE_FLAG_DP_IGP));

#ifdef __BGP_ROUTE_INFO_DP__
  pOldRoute->rank= (uint8_t) iRank;
  bgp_routes_at(routes, 0)->rank= (uint8_t) iRank;
#endif

  /* If the IGP next-hop has changed, we need to re-insert the
     route into the routing table with the new next-hop. */
  bgp_router_rt_add_route(router, route);

  /* Route has not changed. */
  route_destroy(&route);
  route= pOldRoute;

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  if (pEBGPRoutes != NULL && bgp_routes_size(pEBGPRoutes) > 0) {
    pEBGPRoute = bgp_routes_at(pEBGPRoutes, 0);
    bgp_router_check_dissemination_external_best(router, pEBGPRoutes, 
				    pOldEBGPRoute, pEBGPRoute, prefix);
  }
#endif
}

// ----- bgp_router_decision_process_update_best_route ---------------
/**
 *
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
void bgp_router_decision_process_update_best_route(bgp_router_t * router, 
				      ip_pfx_t prefix, bgp_routes_t * routes, 
				      bgp_route_t * pOldRoute, bgp_route_t * route, 
				      int iRank, bgp_routes_t * pEBGPRoutes,
				      bgp_route_t * pOldEBGPRoute)
#else
void bgp_router_decision_process_update_best_route(bgp_router_t * router, 
				      ip_pfx_t prefix, bgp_routes_t * routes, 
				      bgp_route_t * pOldRoute, bgp_route_t * route, 
				      int iRank)
#endif
{

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_route_t * pEBGPRoute;
#endif

  /**************************************************************
   * In this case, the route is new or one of its BGP attributes
   * has changed. Thus, we must update the route into the routing
   * table and we must propagate the BGP route to the peers.
   **************************************************************/

  if (pOldRoute != NULL) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\t*** UPDATED BEST ROUTE ***\n");
  } else {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\t*** NEW BEST ROUTE ***\n");
  }

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  if (pOldRoute != NULL) {
    //As the RIBs accept more than one route we have to delete manually 
    //the old routes in the LOC-RIB!
//	if (rib_find_one_exact(router->loc_rib, pOldRoute->prefix, &(pOldRoute->next_hop)) != NULL) 
    rib_remove_route(router->loc_rib, pOldRoute->prefix, NULL);
  }
#endif
  
 if (pOldRoute != NULL) 
    bgp_router_best_flag_off(pOldRoute);

  /* Mark route in Loc-RIB and Adj-RIB-In as best. This must be
     done after the call to 'bgp_router_best_flag_off'. */
  route_flag_set(route, ROUTE_FLAG_BEST, 1);
  route_flag_set(bgp_routes_at(routes, 0), ROUTE_FLAG_BEST, 1);

#ifdef __BGP_ROUTE_INFO_DP__
  route->rank= (uint8_t) iRank;
  bgp_routes_at(routes, 0)->rank= (uint8_t) iRank;
#endif

  /* Insert in Loc-RIB */
  assert(rib_add_route(router->loc_rib, route) == 0);

  /* Insert in the node's routing table */
  bgp_router_rt_add_route(router, route);

#ifndef __EXPERIMENTAL_WALTON__
  //It is obsolete when using walton
  //We disseminate the information in another function 
  //see bgp_router_walton_dp_disseminate in walton.c
  bgp_router_decision_process_disseminate(router, prefix, route);
#endif

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  if (pEBGPRoutes != NULL && bgp_routes_size(pEBGPRoutes) > 0) {
    pEBGPRoute = bgp_routes_at(pEBGPRoutes, 0);
    bgp_router_check_dissemination_external_best(router, pEBGPRoutes, 
				      pOldEBGPRoute, pEBGPRoute, prefix);
  }
#endif
}

// ----- bgp_router_decision_process --------------------------------
/**
 * Phase I - Calculate degree of preference (LOCAL_PREF) for each
 *           single route. Operate on separate Adj-RIB-Ins.
 *           This phase is carried by 'peer_handle_message' (peer.c).
 *
 * Phase II - Selection of best route on the basis of the degree of
 *            preference and then on tie-breaking rules (AS-Path
 *            length, Origin, MED, ...).
 *
 * Phase III - Dissemination of routes.
 *
 * In our implementation, we distinguish two main cases:
 * - a withdraw has been received from a peer for a given prefix. In
 * this case, if the best route towards this prefix was received by
 * the given peer, the complete decision process has to be
 * run. Otherwise, nothing more is done (the route has been removed
 * from the peer's Adj-RIB-In by 'peer_handle_message');
 * - an update has been received. The complete decision process has to
 * be run.
 */
int bgp_router_decision_process(bgp_router_t * router,
				bgp_peer_t * pOriginPeer,
	 			ip_pfx_t prefix)
{
  bgp_routes_t * routes;
  int iIndex;
  bgp_route_t * route, * pOldRoute;
  int iRank;

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  bgp_routes_t * pEBGPRoutes= NULL;
  bgp_route_t * pOldEBGPRoute= NULL;
  int iRankEBGP, iEBGPRoutesCount;
#endif

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pOldRoute= rib_find_one_exact(router->loc_rib, prefix, NULL);
#else
  pOldRoute= rib_find_exact(router->loc_rib, prefix);
#endif

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug,
	       "----------------------------------------"
	       "---------------------------------------\n");
    stream_printf(gdsdebug, "DECISION PROCESS for ");
    ip_prefix_dump(gdsdebug, prefix);
    stream_printf(gdsdebug, " in ");
    bgp_router_dump_id(gdsdebug, router);
    stream_printf(gdsdebug, "\n");
    stream_printf(gdsdebug, "\told-best: ");
    route_dump(gdsdebug, pOldRoute);
    stream_printf(gdsdebug, "\n");
  }

  // Local routes can not be overriden and must be kept in Loc-RIB.
  // Decision process stops here in this case.
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {
    return 0;
  }

  /* Build list of eligible routes */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  routes= bgp_router_get_feasible_routes(router, prefix, 0);

  pOldEBGPRoute = bgp_router_get_old_ebgp_route(routes, pOldRoute);
#else
  routes= bgp_router_get_feasible_routes(router, prefix);
#endif

  /* Reset DP_IGP flag, log eligibles */
  for (iIndex= 0; iIndex < bgp_routes_size(routes); iIndex++) {
    route= (bgp_route_t *) bgp_routes_at(routes, iIndex);

    /* Clear flag that indicates that the route depends on the
       IGP. See 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib'
       for more information. */
    route_flag_set(route, ROUTE_FLAG_DP_IGP, 0);


    /* Log eligible route */
    STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
      stream_printf(gdsdebug, "\teligible: ");
      route_dump(gdsdebug, route);
      stream_printf(gdsdebug, "\n");
    }

    route_flag_set(route, ROUTE_FLAG_BEST, 0);
  }

  // If there is a single eligible & feasible route, it depends on the
  // IGP (see 'dp_rule_nearest_next_hop' and 'bgp_router_scan_rib')
  // for more information.
  if (bgp_routes_size(routes) == 1)
    route_flag_set(bgp_routes_at(routes, 0), ROUTE_FLAG_DP_IGP, 1);

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_router_walton_unsynchronized_all(router);
  // Compare eligible routes
  iRank= 0;
  if (bgp_routes_size(routes) > 1) {
    iRank= bgp_router_walton_decision_process_run(router, routes);
    assert((bgp_routes_size(routes) == 0) ||
	   (bgp_routes_size(routes) == 1));
  } else {
    //TODO : do a loop on each iNextHopCount ...
    if (bgp_routes_size(routes) != 0) {
      //      STREAM_DEBUG(STREAM_LEVEL_DEBUG, "only one route known ... disseminating to all : %d\n", bgp_routes_size(routes));
      bgp_router_walton_disseminate_select_peers(router, routes, 1);
    }
  }
#else
  // Compare eligible routes
  iRank= 0;
  if (bgp_routes_size(routes) > 1) 
    iRank= _bgp_router_decision_process_run(router, routes);
  assert((bgp_routes_size(routes) == 0) ||
	 (bgp_routes_size(routes) == 1));
#endif

  // If one best-route has been selected
  if (bgp_routes_size(routes) > 0) {
    route= route_copy(bgp_routes_at(routes, 0));

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    if (bgp_options_flag_isset(BGP_OPT_EXT_BEST)) {
      //If the Best route selected is not an EBGP one, run the decision process
      //against the set of EBGP routes.
      if (route->peer->asn == router->asn)
	pEBGPRoutes = bgp_router_get_feasible_routes(router, prefix, 1);
      else
	route_flag_set(route, ROUTE_FLAG_EXTERNAL_BEST, 1);
    
      if (pEBGPRoutes != NULL) {
	for (iEBGPRoutesCount = 0; iEBGPRoutesCount < bgp_routes_size(pEBGPRoutes); 
								      iEBGPRoutesCount++){
	  STREAM_DEBUG("\teligible external : ");
	  STREAM_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), 
	      pEBGPRoutes->data[iEBGPRoutesCount]);
	  STREAM_DEBUG("\n");
      }
	if (bgp_routes_size(pEBGPRoutes) > 1) {
	  iRankEBGP = bgp_router_walton_decision_process_run(router, pEBGPRoutes);
	}
	assert((ptr_array_length(pEBGPRoutes) == 0) ||
	   (ptr_array_length(pEBGPRoutes) == 1));
      }
    }
#endif //__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__

    STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
      stream_printf(gdsdebug, "\tnew-best: ");
      route_dump(gdsdebug, route);
      stream_printf(gdsdebug, "\n");
    }

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    // New/updated route: install in Loc-RIB & advertise to peers
    if ((pOldRoute == NULL) ||
	!route_equals(pOldRoute, route)) {
      bgp_router_decision_process_update_best_route(router, prefix, 
			  routes, pOldRoute, route, iRank, pEBGPRoutes,
			  pOldEBGPRoute);
    } else {
      bgp_router_decision_process_unchanged_best_route(router, prefix, 
			  routes, pOldRoute, route, iRank, pEBGPRoutes,
			  pOldEBGPRoute);
    }
#else
    // New/updated route: install in Loc-RIB & advertise to peers
    if ((pOldRoute == NULL) ||
	!route_equals(pOldRoute, route)) {
      bgp_router_decision_process_update_best_route(router, prefix, 
				      routes, pOldRoute, route, iRank);
    } else {
      bgp_router_decision_process_unchanged_best_route(router, prefix, 
				      routes, pOldRoute, route, iRank);
    }
#endif //__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__

  } else {

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    bgp_router_decision_process_no_best_route(router, prefix, pOldRoute, 
							    pOldEBGPRoute);
#else
    bgp_router_decision_process_no_best_route(router, prefix, pOldRoute);
#endif //__EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__

  }

  routes_list_destroy(&routes);

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  routes_list_destroy(&pEBGPRoutes);
#endif

  STREAM_DEBUG(STREAM_LEVEL_DEBUG,
	    "----------------------------------------"
	    "---------------------------------------\n");
  return 0;
}

// ----- bgp_router_handle_message ----------------------------------
/**
 * Handle a BGP message received from the lower layer (network layer
 * at this time). The function looks for the destination peer and
 * delivers it for processing...
 * This function is responsible for freeing the BGP message (message
 * payload). It should however not destroy the encapsulating network
 * message !!!
 */
int bgp_router_handle_message(simulator_t * sim,
			      void * handler,
			      net_msg_t * msg)
{
  bgp_router_t * router= (bgp_router_t *) handler;
  bgp_msg_t * bgp_msg= (bgp_msg_t *) msg->payload;
  bgp_peer_t * peer;
  net_error_t error;

  if ((peer= bgp_router_find_peer(router, msg->src_addr)) != NULL) {
    error= bgp_peer_handle_message(peer, bgp_msg);
    if (error != ESUCCESS)
      return error;
    _bgp_router_msg_listener(msg);
    bgp_msg_destroy(&bgp_msg);
  } else {
    STREAM_ERR_ENABLED(STREAM_LEVEL_WARNING) {
      stream_printf(gdserr, "WARNING: BGP message received from unknown peer !\n");
      stream_printf(gdserr, "WARNING:   destination=");
      bgp_router_dump_id(gdserr, router);
      stream_printf(gdserr, "\nWARNING:   source=");
      ip_address_dump(gdserr, msg->src_addr);
      stream_printf(gdserr, "\n");
    }
    bgp_msg_destroy(&bgp_msg);
    return -1;
  }
  return 0;
}

// ----- bgp_router_dump_id -------------------------------------------------
/**
 *
 */
 void bgp_router_dump_id(gds_stream_t * stream, bgp_router_t * router)
{
  stream_printf(stream, "AS%d:", router->asn);
  node_dump_id(stream, router->node);
}

typedef struct {
  bgp_router_t * router;
  array_t * pPrefixes;
} SScanRIBCtx;

// ----- bgp_router_scan_rib_for_each -------------------------------
/**
 * This function is a helper function for 'bgp_router_scan_rib'. This
 * function checks if the best route used to reach a given prefix must
 * be updated due to an IGP change.
 *
 * The function works in two step:
 * (1) checks if the current best route was selected based on the IGP
 *     (i.e. the route passed the IGP rule) and checks if its next-hop
 *     is still reachable and if not, goes to step (2) otherwise,
 *     the function returns
 *
 * (2) looks in the Adj-RIB-Ins for a prefix whose IGP cost is lower
 *     than these of the current best route. If one is found, run the
 *     decision process
 *
 * The function does not run the BGP decision process itself but add
 * to a list the prefixes for which the decision process must be run.
 */
int bgp_router_scan_rib_for_each(uint32_t key, uint8_t key_len,
				 void * pItem, void * pContext)
{
  SScanRIBCtx * pCtx= (SScanRIBCtx *) pContext;
  bgp_router_t * router= pCtx->router;
  ip_pfx_t prefix;
  bgp_route_t * route;
  unsigned int index;
  bgp_peer_t * peer;
  unsigned int uBestWeight;
  rt_info_t * routeInfo;
  //rt_info_t * pCurRouteInfo;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uIndexRoute;
  bgp_routes_t * routes;
  bgp_router_walton_unsynchronized_all(router);
#endif
  bgp_route_t * pAdjRoute;

  prefix.network= key;
  prefix.mask= key_len;

  /*fprintf(stderr, "SCAN PREFIX ");
  bgp_router_dump_id(gdserr, router);
  fprintf(stderr, " ");
  ip_prefix_dump(gdserr, prefix);
  fprintf(stderr, "\n");*/

  _array_append(pCtx->pPrefixes, &prefix);
  return 0;
  
  /* Looks up for the best BGP route towards this prefix. If the route
     does not exist, schedule the current prefix for the decision
     process */
#if defined __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__
  route= rib_find_one_exact(router->loc_rib, prefix, NULL);
#else
  route= rib_find_exact(router->loc_rib, prefix);
#endif
  if (route == NULL) {
    _array_append(pCtx->pPrefixes, &prefix);
    return 0;
  } else {

    /*fprintf(stderr, "- checking peering session...\n");
      fflush(stderr);*/

    /* Check if the peer that has announced this route is down */
    if ((route->peer != NULL) &&
	(route->peer->session_state != SESSION_STATE_ESTABLISHED)) {
      STREAM_ERR_ENABLED(STREAM_LEVEL_WARNING) {
	stream_printf(gdserr, "*** SESSION NOT ESTABLISHED [");
	ip_prefix_dump(gdserr, route->prefix);
	stream_printf(gdserr, "] ***\n");
      }
      _array_append(pCtx->pPrefixes, &prefix);
      return 0;
    }
    
    routeInfo= rt_find_best(router->node->rt, route->attr->next_hop,
			     NET_ROUTE_ANY);    

    /*** Next-hop no more reachable => trigger decision process ***/
    if (routeInfo == NULL) {
      _array_append(pCtx->pPrefixes, &prefix);
      return 0;	
    }

    /* Check if the IP next-hop of the BGP route has changed. Indeed,
       the BGP next-hop is not always the IP next-hop. If this next-hop
       has changed, the decision process must be run. */
    
    /*fprintf(stderr, "- checking route's next-hop...\n");
      fflush(stderr);*/
    
    /*pCurRouteInfo= rt_find_exact(router->node->rt,
      route->prefix, NET_ROUTE_BGP);
      assert(pCurRouteInfo != NULL);
      if (rt_entry_compare((rt_entry_t *) pCurRouteInfo->entries->data[0],
      (rt_entry_t *) routeInfo->entries->data[0])) {
      _array_append(pCtx->pPrefixes, &prefix);
      fprintf(stderr, "NEXT-HOP HAS CHANGED\n");
      return 0;
      }*/

    /* If the best route was chosen based on the IGP weight, then
       there is a possible impact on BGP */
    if (route_flag_get(route, ROUTE_FLAG_DP_IGP)) {

      uBestWeight= routeInfo->metric;

      /* Lookup in the Adj-RIB-Ins for routes that were also selected
	 based on the IGP, that is routes that were compared to the
	 current best route based on the IGP cost to the next-hop. These
	 routes are marked with the ROUTE_FLAG_DP_IGP flag as well */
      for (index= 0; index < bgp_peers_size(router->peers); index++) {
	peer= bgp_peers_at(router->peers, index);
	
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	routes= rib_find_exact(peer->adj_rib[RIB_IN], route->prefix);
	if (routes != NULL) {
	  for (uIndexRoute = 0; uIndexRoute < bgp_routes_size(routes); uIndexRoute++) {
	    pAdjRoute = bgp_routes_at(routes, uIndexRoute);
#else
	pAdjRoute= rib_find_exact(peer->adj_rib[RIB_IN], route->prefix);
#endif
		
	if (pAdjRoute != NULL) {
	  
	  /* Is there a route (IGP ?) towards the destination ? */
	  routeInfo= rt_find_best(router->node->rt,
				  pAdjRoute->attr->next_hop,
				  NET_ROUTE_ANY);
	  
	  /* Three cases are now possible:
	     (1) route becomes reachable => run DP
	     (2) route no more reachable => do nothing (becoz the route is
	     not the best
	     (3) IGP cost is below cost of the best route => run DP
	  */
	  if ((routeInfo != NULL) &&
	      !route_flag_get(pAdjRoute, ROUTE_FLAG_FEASIBLE)) {
	    /* The next-hop was not reachable (route unfeasible) and is
	       now reachable, thus run the decision process */
	    _array_append(pCtx->pPrefixes, &prefix);
	    return 0;
	    
	  } else if (routeInfo != NULL) {
	    
	    /* IGP cost is below cost of the best route, thus run the
	       decision process */
	    if (routeInfo->metric < uBestWeight) {
	      _array_append(pCtx->pPrefixes, &prefix);
	      return 0;
	      
	    }	
	    
	  }
	  
	}
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
	  }
	}
#endif
	
      }
      
    }
  }
    
  return 0;
}

// -----[ bgp_router_clear_rib ]-------------------------------------
/**
 * This function clears the Loc-RIB and the Adj-RIBs of a router.
 */
int bgp_router_clear_rib(bgp_router_t * router)
{
  unsigned int index;
  bgp_peer_t * peer;

  //---Edited by Pradeep Bangera ---------------
  //remove the local ribs
  rib_destroy(&router->loc_rib);
  router->loc_rib= rib_create(0);
  //--------------------------------------------

  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    peer= bgp_peers_at(router->peers, index);
    rib_destroy(&peer->adj_rib[RIB_IN]);
    rib_destroy(&peer->adj_rib[RIB_OUT]);
    //----------Edited by Pradeep Bangera ---------------------------
    peer->adj_rib[RIB_OUT]= rib_create(0);
    peer->adj_rib[RIB_IN]= rib_create(0);
    //---------------------------------------------------------------
  }
  return 0;
}

// -----[ bgp_router_clear_adj_rib ]----------------------------------
/**
 * This function only clears the Adj-RIBs of a router.
 */
int bgp_router_clear_adj_rib(bgp_router_t * router)
{
  unsigned int index;
  bgp_peer_t * peer;

  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    peer= bgp_peers_at(router->peers, index);
    rib_destroy(&peer->adj_rib[RIB_IN]);
    rib_destroy(&peer->adj_rib[RIB_OUT]);
    //----------Edited by Pradeep Bangera ---------------------------
    peer->adj_rib[RIB_OUT]= rib_create(0);
    peer->adj_rib[RIB_IN]= rib_create(0);
    //---------------------------------------------------------------
  }
  return 0;
}

// -----[ _bgp_router_prefixes_for_each ]----------------------------
/**
 *
 */
static int _bgp_router_prefixes_for_each(uint32_t key, uint8_t key_len,
					 void * pItem, void * pContext)
{
  gds_radix_tree_t * pPrefixes= (gds_radix_tree_t *) pContext;
  bgp_route_t * route= (bgp_route_t *) pItem;

  radix_tree_add(pPrefixes, route->prefix.network,
		 route->prefix.mask, (void *) 1);
  return 0;
}

// -----[ _bgp_router_alloc_prefixes ]-------------------------------
static inline void _bgp_router_alloc_prefixes(gds_radix_tree_t ** ppPrefixes)
{
  /* If list (radix-tree) is not allocated, create it now. The
     radix-tree is created without destroy function and acts thus as a
     list of references. */
  if (*ppPrefixes == NULL) {
    *ppPrefixes= radix_tree_create(32, NULL);
  }
}

// -----[ _bgp_router_free_prefixes ]--------------------------------
static inline void _bgp_router_free_prefixes(gds_radix_tree_t ** ppPrefixes)
{
  if (*ppPrefixes != NULL) {
    radix_tree_destroy(ppPrefixes);
    *ppPrefixes= NULL;
  }
}

// -----[ _bgp_router_get_peer_prefixes ]----------------------------
/**
 * This function builds a list of all prefixes received from this
 * peer (in Adj-RIB-in). The list of prefixes is implemented as a
 * radix-tree in order to guarantee that each prefix will be present
 * at most one time (uniqueness).
 */
static inline int _bgp_router_get_peer_prefixes(bgp_router_t * router,
						bgp_peer_t * peer,
						gds_radix_tree_t ** ppPrefixes)
{
  unsigned int index;
  int iResult= 0;

  _bgp_router_alloc_prefixes(ppPrefixes);

  if (peer != NULL) {
    iResult= rib_for_each(peer->adj_rib[RIB_IN],
			  _bgp_router_prefixes_for_each,
			  *ppPrefixes);
  } else {
    for (index= 0; index < bgp_peers_size(router->peers); index++) {
      peer= bgp_peers_at(router->peers, index);
      iResult= rib_for_each(peer->adj_rib[RIB_IN],
			    _bgp_router_prefixes_for_each,
			    *ppPrefixes);
      if (iResult != 0)
	break;
    }
  }
  return iResult;
}

// -----[ _bgp_router_get_local_prefixes ]---------------------------
static inline int _bgp_router_get_local_prefixes(bgp_router_t * router,
						 gds_radix_tree_t ** ppPrefixes)
{
  _bgp_router_alloc_prefixes(ppPrefixes);
  
  return rib_for_each(router->loc_rib, _bgp_router_prefixes_for_each,
		      *ppPrefixes);
}

// -----[ _bgp_router_prefixes ]-------------------------------------
/**
 * This function builds a list of all the prefixes known in this
 * router (in Adj-RIB-Ins and Loc-RIB).
 */
static inline gds_radix_tree_t * _bgp_router_prefixes(bgp_router_t * router)
{
  gds_radix_tree_t * pPrefixes= NULL;

  /* Get prefixes from all Adj-RIB-Ins */
  _bgp_router_get_peer_prefixes(router, NULL, &pPrefixes);

  /* Get prefixes from the RIB */
  _bgp_router_get_local_prefixes(router, &pPrefixes);

  return pPrefixes;
}

// ----- _bgp_router_refresh_sessions -------------------------------
/*
 * This function scans the peering sessions. For each session, it
 * checks that the peer router is still reachable. If it is not, the
 * session is teared down (see 'bgp_peer_session_refresh' for more
 * details).
 */
static inline void _bgp_router_refresh_sessions(bgp_router_t * router)
{
  unsigned int index;

  for (index= 0; index < bgp_peers_size(router->peers); index++)
    bgp_peer_session_refresh(bgp_peers_at(router->peers, index));
}

// ----- bgp_router_scan_rib ----------------------------------------
/**
 * This function scans the RIB of the BGP router in order to find
 * routes for which the distance to the next-hop has changed. For each
 * route that has changed, the decision process is re-run.
 */
int bgp_router_scan_rib(bgp_router_t * router)
{
  SScanRIBCtx sCtx;
  int iIndex;
  int iResult;
  gds_radix_tree_t * pPrefixes;
  ip_pfx_t prefix;

  /* Scan peering sessions */
  _bgp_router_refresh_sessions(router);

  /* initialize context */
  sCtx.router= router;
  sCtx.pPrefixes= _array_create(sizeof(ip_pfx_t), 0, 0, NULL, NULL, NULL);

  /* Build a list of all available prefixes in this router */
  pPrefixes= _bgp_router_prefixes(router);

  /* Traverses the whole Loc-RIB in order to find prefixes that depend
     on the IGP (links up/down and metric changes) */
  iResult= radix_tree_for_each(pPrefixes,
			       bgp_router_scan_rib_for_each,
			       &sCtx);

  /* For each route in the list, run the BGP decision process */
  if (iResult == 0)
    for (iIndex= 0; iIndex < _array_length(sCtx.pPrefixes); iIndex++) {
      _array_get_at(sCtx.pPrefixes, iIndex, &prefix);
      bgp_router_decision_process(router, NULL, prefix);
    }

  _bgp_router_free_prefixes(&pPrefixes);
    
  _array_destroy(&sCtx.pPrefixes);
  
  return iResult;
}

// -----[ _bgp_router_rerun_for_each ]-------------------------------
static int _bgp_router_rerun_for_each(uint32_t key, uint8_t key_len,
				      void * pItem, void * pContext)
{
  bgp_router_t * router= (bgp_router_t *) pContext;
  ip_pfx_t prefix;

  prefix.network= key;
  prefix.mask= key_len;  

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "decision-process [");
    ip_prefix_dump(gdsdebug, prefix);
    stream_printf(gdsdebug, "]\n");
    stream_flush(gdsdebug);
  }

  return bgp_router_decision_process(router, NULL, prefix);
}

// -----[ bgp_router_rerun ]-----------------------------------------
/**
 * Rerun the decision process for the given prefixes. If the length of
 * the prefix mask is 0, rerun for all known prefixes (from Adj-RIB-In
 * and Loc-RIB). Otherwize, only rerun for the specified prefix.
 */
int bgp_router_rerun(bgp_router_t * router, ip_pfx_t prefix)
{
  int iResult;
  gds_radix_tree_t * pPrefixes= NULL;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "rerun [");
    bgp_router_dump_id(gdsdebug, router);
    stream_printf(gdsdebug, "]\n");
    stream_flush(gdsdebug);
  }

  _bgp_router_alloc_prefixes(&pPrefixes);

  /* Populate with all prefixes */
  if (prefix.mask == 0) {
    assert(!_bgp_router_get_peer_prefixes(router, NULL, &pPrefixes));
    assert(!_bgp_router_get_local_prefixes(router, &pPrefixes));
  } else {
    radix_tree_add(pPrefixes, prefix.network,
		   prefix.mask, (void *) 1);
  }

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "prefix-list ok\n");
    stream_flush(gdsdebug);
  }

  /* For each route in the list, run the BGP decision process */
  iResult= radix_tree_for_each(pPrefixes, _bgp_router_rerun_for_each, router);

  /* Free list of prefixes */
  _bgp_router_free_prefixes(&pPrefixes);

  return iResult;
}

// -----[ bgp_router_peer_readv_prefix ]-----------------------------
/**
 *
 */
int bgp_router_peer_readv_prefix(bgp_router_t * router, bgp_peer_t * peer,
				 ip_pfx_t prefix)
{
  bgp_route_t * route;

  if (prefix.mask == 0) {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: not yet implemented\n");
    return -1;
  } else {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    route= rib_find_one_exact(router->loc_rib, prefix, NULL);
#else
    route= rib_find_exact(router->loc_rib, prefix);
#endif
    if (route != NULL) {
      bgp_router_decision_process_disseminate_to_peer(router, prefix,
						      route, peer);
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////
// DUMP FUNCTIONS
/////////////////////////////////////////////////////////////////////

// ----- bgp_router_dump_networks -----------------------------------
/**
 *
 */
void bgp_router_dump_networks(gds_stream_t * stream, bgp_router_t * router)
{
  unsigned int index;

  for (index= 0; index < bgp_routes_size(router->local_nets);
       index++) {
    route_dump(stream, bgp_routes_at(router->local_nets, index));
    stream_printf(stream, "\n");
  }
  stream_flush(stream);
}

// ----- bgp_router_networks_for_each -------------------------------
int bgp_router_networks_for_each(bgp_router_t * router,
				 gds_array_foreach_f foreach,
				 void * ctx)
{
  return _array_for_each((array_t *) router->local_nets, foreach, ctx);
}

// ----- bgp_router_dump_peers --------------------------------------
/**
 *
 */
void bgp_router_dump_peers(gds_stream_t * stream, bgp_router_t * router)
{
  unsigned int index;

  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    bgp_peer_dump(stream, bgp_peers_at(router->peers, index));
    stream_printf(stream, "\n");
  }
  stream_flush(stream);
}

// -----[ bgp_router_peers_for_each ]--------------------------------
/**
 *
 */
int bgp_router_peers_for_each(bgp_router_t * router,
			      gds_array_foreach_f foreach,
			      void * ctx)
{
  return bgp_peers_for_each(router->peers, foreach, ctx);
}

typedef struct {
  bgp_router_t * router;
  gds_stream_t * stream;
  char * cDump;
} bgp_route_dump_ctx_t;

// ----- _bgp_router_dump_route -------------------------------------
/**
 *
 */
static int _bgp_router_dump_route(uint32_t key, uint8_t key_len,
				  void * item, void * context)
{
  bgp_route_dump_ctx_t * ctx= (bgp_route_dump_ctx_t *) context;

  route_dump(ctx->stream, (bgp_route_t *) item);
  stream_printf(ctx->stream, "\n");
  stream_flush(ctx->stream);
  return 0;
}

// ----- bgp_router_dump_rib ----------------------------------------
/**
 * Dump the content of the Loc-RIB on the provided output stream.
 */
 void bgp_router_dump_rib(gds_stream_t * stream, bgp_router_t * router)
{
  bgp_route_dump_ctx_t ctx= { .router= router, .stream= stream };

  rib_for_each(router->loc_rib, _bgp_router_dump_route, &ctx);
  stream_flush(stream);
}

// ----- bgp_router_dump_adjrib -------------------------------------
/**
 * Dump the content of Adj-RIB-ins/-outs.
 *
 * Arguments:
 * @param dir    selects between Adj-RIB-in and Adj-RIB-out
 * @param peer   selects a specific peer or all peers (NULL)
 * @param prefix selects a specific prefix (prefix length < 32),
 *               the longest prefix matching an address (prefix length >= 32),
 *               or all prefixes (prefix length == 0)
 */
void bgp_router_dump_adjrib(gds_stream_t * stream, bgp_router_t * router,
			    bgp_peer_t * peer, ip_pfx_t prefix,
			    bgp_rib_dir_t dir)
{
  unsigned int i;

  // All peers
  if (peer == NULL) {
    for (i= 0; i < bgp_peers_size(router->peers); i++) {
      bgp_peer_dump_adjrib(stream, bgp_peers_at(router->peers, i),
			   prefix, dir);
    }
  }

  // Single peer
  else {
    bgp_peer_dump_adjrib(stream, peer, prefix, dir);
  }

  stream_flush(stream);
}

// ----- bgp_router_dump_rib_address --------------------------------
/**
 *
 */
void bgp_router_dump_rib_address(gds_stream_t * stream, bgp_router_t * router,
				 net_addr_t addr)
{
  bgp_route_t * route;
  ip_pfx_t prefix;

  prefix.network= addr;
  prefix.mask= 32;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  route= rib_find_one_best(router->loc_rib, prefix);
#else
  route= rib_find_best(router->loc_rib, prefix);
#endif
  if (route != NULL) {
    route_dump(stream, route);
    stream_printf(stream, "\n");
  }

  stream_flush(stream);
}

// ----- bgp_router_dump_rib_prefix ---------------------------------
/**
 *
 */
void bgp_router_dump_rib_prefix(gds_stream_t * stream, bgp_router_t * router,
				ip_pfx_t prefix)
{
  bgp_route_t * route;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  route= rib_find_one_exact(router->loc_rib, prefix, NULL);
#else
  route= rib_find_exact(router->loc_rib, prefix);
#endif
  if (route != NULL) {
    route_dump(stream, route);
    stream_printf(stream, "\n");
  }

  stream_flush(stream);
}

// ----- bgp_router_info --------------------------------------------
void bgp_router_info(gds_stream_t * stream, bgp_router_t * router)
{
  stream_printf(stream, "router-id : ");
  ip_address_dump(stream, router->rid);
  stream_printf(stream, "\n");
  stream_printf(stream, "as-number : %u\n", router->asn);
  stream_printf(stream, "cluster-id: ");
  ip_address_dump(stream, router->cluster_id);
  stream_printf(stream, "\n");
  if (router->node->name != NULL)
    stream_printf(stream, "name      : %s\n", router->node->name);
  stream_flush(stream);
}

// -----[ bgp_router_show_route_info ]-------------------------------
/**
 * Show information about a best BGP route from the Loc-RIB of this
 * router:
 * - decision-process-rule: rule of the decision process that was
 *   used to select this route as best.
 */
int bgp_router_show_route_info(gds_stream_t * stream, bgp_router_t * router,
			       bgp_route_t * route)
{
  bgp_routes_t * routes;

  stream_printf(stream, "prefix: ");
  ip_prefix_dump(stream, route->prefix);
  stream_printf(stream, "\n");

#ifdef __BGP_ROUTE_INFO_DP__
  if (route->rank <= 0) {
    stream_printf(stream, "decision-process-rule: %d [ Single choice ]\n",
	       route->rank);
  } else if (route->rank >= DP_NUM_RULES) {
    stream_printf(stream, "decision-process-rule: %d [ Invalid rule ]\n",
	       route->rank);
  } else {
    stream_printf(stream, "decision-process-rule: %d [ %s ]\n",
	       route->rank, DP_RULES[route->rank-1].name);
  }
#endif
  routes= bgp_router_get_feasible_routes(router, route->prefix);
  if (routes != NULL) {
    stream_printf(stream, "decision-process-feasibles: %d\n",
	       bgp_routes_size(routes));
    routes_list_destroy(&routes);
  }
  stream_flush(stream);
  return 0;
}

// -----[ _bgp_router_show_route_info ]------------------------------
/**
 *
 */
static int _bgp_router_show_route_info(uint32_t key, uint8_t key_len,
				       void * item, void * context)
{
  bgp_route_dump_ctx_t * ctx= (bgp_route_dump_ctx_t *) context;
  bgp_route_t * route= (bgp_route_t *) item;

  bgp_router_show_route_info(ctx->stream, ctx->router, route);

  stream_flush(ctx->stream);

  return 0;
}

// -----[ bgp_router_show_routes_info ]------------------------------
/**
 *
 */
void bgp_router_show_routes_info(gds_stream_t * stream, bgp_router_t * router,
				 ip_dest_t dest)
{
  bgp_route_dump_ctx_t ctx;
  bgp_route_t * route;

  switch (dest.type) {

  case NET_DEST_ANY:
    ctx.router= router;
    ctx.stream= stream;
    rib_for_each(router->loc_rib, _bgp_router_show_route_info, &ctx);
    break;

  case NET_DEST_ADDRESS:
//TODO : implements to support walton
#if ! (defined __EXPERIMENTAL_WALTON__)
    dest.prefix.network= dest.addr;
    dest.prefix.mask= 32;
    route= rib_find_best(router->loc_rib, dest.prefix);
    if (route != NULL)
      bgp_router_show_route_info(stream, router, route);
#endif
    break;

  case NET_DEST_PREFIX:
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    route= rib_find_one_exact(router->loc_rib, dest.prefix, NULL);
#else
    route= rib_find_exact(router->loc_rib, dest.prefix);
#endif
    if (route != NULL)
      bgp_router_show_route_info(stream, router, route);
    break;

  default:
    ;
  }
}


/////////////////////////////////////////////////////////////////////
// LOAD FUNCTIONS
/////////////////////////////////////////////////////////////////////

typedef struct {
  bgp_router_t * router;            // Router where routes are loaded
  uint8_t        options;           // Options
  int            return_code;       // Code to return in case of error
  unsigned int   routes_ok;         // Number of routes loaded without error
  unsigned int   routes_bad_target; //                  with bad target
  unsigned int   routes_bad_peer;   //                  with bad peer
  unsigned int   routes_ignored;    //                  ignored by API (ex: IP6)
} SBGP_LOAD_RIB_CTX;
  
// -----[ _bgp_router_load_rib_handler ]-----------------------------
/**
 * Handle a BGP route destined to a target router.
 *
 * The function performs three actions:
 * 1). Check that the route was collected on the correct router.
 *     To do that, checks that the address of the target router
 *     equals the address of the route's peer. For CISCO dumps, this
 *     information is not available and the check always succeeds.
 * 2). Check that the target router has a peer that corresponds to
 *     the route's next-hop.
 * 3). Inject the route into the target's Adj-RIB-in and runs the
 *     decision process for the route's prefix.
 */
static int _bgp_router_load_rib_handler(int status,
					bgp_route_t * route,
					net_addr_t peer_addr,
					asn_t peer_asn,
					void * ctx)
{
  SBGP_LOAD_RIB_CTX * pCtx= (SBGP_LOAD_RIB_CTX *) ctx;
  bgp_router_t * router= pCtx->router;
  bgp_peer_t * peer= NULL;

  // Check that there is a route to handle (according to API)
  if (status != BGP_INPUT_STATUS_OK) {
    pCtx->routes_ignored++;
    return BGP_INPUT_SUCCESS;
  }

  // 1). Check that the target router (addr/ASN) corresponds to the
  //     route's peer (addr/ASN). Ignored if peer addr is 0 (CISCO).
  if (!(pCtx->options & BGP_ROUTER_LOAD_OPTIONS_FORCE) &&
      ((router->rid != peer_addr) ||
       (router->asn != peer_asn))) {
    
    STREAM_ERR_ENABLED(STREAM_LEVEL_SEVERE) {
      stream_printf(gdserr,
		 "Error: invalid peer (IP address/AS number mismatch)\n"
		 "Error: local router = AS%d:", router->asn);
      bgp_router_dump_id(gdserr, router);
      stream_printf(gdserr,"\nError: MRT peer router = AS%d:", peer_asn);
      ip_address_dump(gdserr, peer_addr);
      stream_printf(gdserr, "\n");
    }
    route_destroy(&route);
    pCtx->routes_bad_target++;
    return pCtx->return_code;
  }

  // 2). Check that the target router has a peer that corresponds
  //     to the route's next-hop.
  if (peer == NULL) {
    
    // Look for the peer in the router...
    peer= bgp_router_find_peer(router, route->attr->next_hop);
    // If the peer does not exist, auto-create it if required or
    // drop an error message.
    if (peer == NULL) {
      if ((pCtx->options & BGP_ROUTER_LOAD_OPTIONS_AUTOCONF) != 0) {
	bgp_auto_config_session(router, route->attr->next_hop,
				path_last_as(route->attr->path_ref), &peer);
      } else {
	STREAM_ERR_ENABLED(STREAM_LEVEL_SEVERE) {
	  stream_printf(gdserr, "Error: peer not found \"");
	  ip_address_dump(gdserr, route->attr->next_hop);
	  stream_printf(gdserr, "\"\n");
	}
	route_destroy(&route);
	pCtx->routes_bad_peer++;
	return pCtx->return_code;
      }
    }
  }

  // 3). Update the target router's Adj-RIB-in and
  //     run the decision process
  route->peer= peer;
  route_flag_set(route, ROUTE_FLAG_FEASIBLE, 1);
  if (route->peer == NULL) {
    route_flag_set(route, ROUTE_FLAG_INTERNAL, 1);
    rib_add_route(router->loc_rib, route);
  } else {
    route_flag_set(route, ROUTE_FLAG_ELIGIBLE,
		   bgp_peer_route_eligible(route->peer, route));
    route_flag_set(route, ROUTE_FLAG_BEST,
		   bgp_peer_route_feasible(route->peer, route));
    rib_replace_route(route->peer->adj_rib[RIB_IN], route);
  }

  // TODO: shouldn't we take into account the soft-restart flag ?
  // If the (virtual) session is down, we should still store the
  // received routes in the Adj-RIB-in, but not run the decision
  // process.

  bgp_router_decision_process(router, route->peer, route->prefix);

  pCtx->routes_ok++;
  return BGP_INPUT_SUCCESS;
}

// -----[ bgp_router_load_rib ]--------------------------------------
/**
 * This function loads an existing BGP routing table into the given
 * bgp router instance. The routes are considered local and will not
 * be replaced by routes received from peers. The routes are marked
 * as best and feasible and are directly installed into the Loc-RIB.
 */
int bgp_router_load_rib(bgp_router_t * router, const char * filename,
			bgp_input_type_t format, uint8_t options)
{
  int result;
  SBGP_LOAD_RIB_CTX sCtx= {
    .router           = router,
    .options          = options,
    .return_code      = 0, // Ignore errors
    .routes_ok        = 0,
    .routes_bad_target= 0,
    .routes_bad_peer  = 0,
    .routes_ignored   = 0,
  };

  // Load routes
  result= bgp_routes_load(filename, format,
			  _bgp_router_load_rib_handler, &sCtx);
  if (result != BGP_INPUT_SUCCESS)
    return result;

  // Show summary
  if (options & BGP_ROUTER_LOAD_OPTIONS_SUMMARY) {
    stream_printf(gdsout, "Source: %s\n", filename);
    stream_printf(gdsout, "Routes loaded         : %u\n", sCtx.routes_ok);
    stream_printf(gdsout, "Routes with bad target: %u\n", sCtx.routes_bad_target);
    stream_printf(gdsout, "Routes with bad peer  : %u\n", sCtx.routes_bad_peer);
    stream_printf(gdsout, "Routes ignored        : %u\n", sCtx.routes_ignored);
  }

  return ESUCCESS;
}

// -----[ _bgp_router_save_route_mrtd ]------------------------------
/**
 *
 */
/*static int _bgp_router_save_route_mrtd(uint32_t key, uint8_t key_len,
				       void * item, void * context)
{
  bgp_route_dump_ctx_t * ctx= (bgp_route_dump_ctx_t *) context;

  stream_printf(ctx->stream, "TABLE_DUMP|%u|", 0);
  route_dump_mrt(ctx->stream, (bgp_route_t *) item);
  stream_printf(ctx->stream, "\n");
  return 0;
  }*/

// -----[ bgp_router_show_stats ]------------------------------------
/**
 * Show statistics about the BGP router:
 * - num-peers: number of peers.
 *
 * - num-prefixes/peer: number of prefixes received from each
 *   peer. For each peer, a line with the number of best routes and
 *   the number of non-selected routes is shown. 
 *
 * - rule-stats: number of best routes selected by each decision
 *   process rule. This is showned as a set of numbers. The first one
 *   gives the number of routes with no choice. The second one, the
 *   number of routes selected based on the LOCAL-PREF, etc.
 */
void bgp_router_show_stats(gds_stream_t * stream, bgp_router_t * router)
{
  unsigned int index;
  bgp_peer_t * peer;
  gds_enum_t * routes;
  bgp_route_t * route;
  unsigned int num_prefixes, num_best;
  int aiNumPerRule[DP_NUM_RULES+1];
  int rule;

  memset(&aiNumPerRule, 0, sizeof(aiNumPerRule));
  stream_printf(stream, "num-peers: %d\n",
		bgp_peers_size(router->peers));
  stream_printf(stream, "num-networks: %d\n",
		bgp_routes_size(router->local_nets));

  // Number of prefixes (best routes)
  num_best= 0;
  routes= trie_get_enum(router->loc_rib);
  while (enum_has_next(routes)) {
    route= *((bgp_route_t **) enum_get_next(routes));
    if (route_flag_get(route, ROUTE_FLAG_BEST)) {
      num_best++;
      if (route->rank <= DP_NUM_RULES)
	aiNumPerRule[route->rank]++;
    }
  }
  enum_destroy(&routes);
  stream_printf(stream, "num-best: %d\n", num_best);

  // Classification of best route selections
  stream_printf(stream, "rule-stats:");
  for (rule= 0; rule <= DP_NUM_RULES; rule++) {
    stream_printf(stream, " %d", aiNumPerRule[rule]);
  }
  stream_printf(stream, "\n");

  // Number of best/non-best routes per peer
  stream_printf(stream, "num-prefixes/peer:\n");
  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    peer= bgp_peers_at(router->peers, index);
    num_prefixes= 0;
    num_best= 0;
    routes= trie_get_enum(peer->adj_rib[RIB_IN]);
    while (enum_has_next(routes)) {
      route= *((bgp_route_t **) enum_get_next(routes));
      num_prefixes++;
      if (route_flag_get(route, ROUTE_FLAG_BEST)) {
	num_best++;
	if (route->rank <= DP_NUM_RULES)
	  aiNumPerRule[route->rank]++;
      }
    }
    enum_destroy(&routes);
    bgp_peer_dump_id(stream, peer);
    stream_printf(stream, ": %d / %d\n",  num_best, num_prefixes);
  }
}


