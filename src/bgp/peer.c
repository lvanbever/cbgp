// ==================================================================
// @(#)peer.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
//
// @date 24/11/2002
// $Id: peer.c,v 1.45 2009-03-26 13:27:28 bqu Exp $
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

#include <assert.h>
#include <stdlib.h>

#include <libgds/stream.h>
#include <libgds/memory.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/network.h>
#include <net/node.h>

#include <bgp/as.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/ecomm.h>
#include <bgp/filter/filter.h>
#include <bgp/message.h>
#include <bgp/peer.h>
#include <bgp/qos.h>
#include <bgp/rib.h>
#include <bgp/route.h>

char * SESSION_STATES[SESSION_STATE_MAX]= {
  "IDLE",
  "OPENWAIT",
  "ESTABLISHED",
  "ACTIVE",
};

// -----[ function prototypes ] -------------------------------------
static inline void _bgp_peer_rescan_adjribin(bgp_peer_t * peer,
					     int clear);
static inline void _bgp_peer_process_update(bgp_peer_t * peer,
					    bgp_msg_update_t * msg);
static inline void _bgp_peer_process_withdraw(bgp_peer_t * peer,
					      bgp_msg_withdraw_t * msg);
static inline int _bgp_peer_send(bgp_peer_t * peer, bgp_msg_t * msg);


// -----[ bgp_peer_create ]------------------------------------------
/**
 * Create a new peer structure and initialize the following
 * structures:
 *   - default input/output filters (accept everything)
 *   - input/output adjacent RIBs
 */
bgp_peer_t * bgp_peer_create(uint16_t asn, net_addr_t addr,
			     bgp_router_t * router)
{
  bgp_peer_t * peer= (bgp_peer_t *) MALLOC(sizeof(bgp_peer_t));
  peer->asn= asn;
  peer->addr= addr;
  peer->router= router;
  peer->router_id= NET_ADDR_ANY;
  peer->filter[FILTER_IN]= NULL; // Default = ACCEPT ANY
  peer->filter[FILTER_OUT]= NULL; // Default = ACCEPT ANY
  peer->adj_rib[RIB_IN]= rib_create(0);
  peer->adj_rib[RIB_OUT]= rib_create(0);
  peer->session_state= SESSION_STATE_IDLE;
  peer->flags= 0;

  // Options
  peer->next_hop= NET_ADDR_ANY;
  peer->src_addr= NET_ADDR_ANY;

  peer->pRecordStream= NULL;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  peer->uWaltonLimit = 1;
  bgp_router_walton_peer_set(peer, 1);
#endif
  peer->last_error= ESUCCESS;
  peer->send_seq_num= 0;
  peer->recv_seq_num= 0;
  return peer;
}

// -----[ bgp_peer_destroy ]-----------------------------------------
/**
 * Destroy the given peer structure and free the related memory.
 */
void bgp_peer_destroy(bgp_peer_t ** ppeer)
{
  if (*ppeer != NULL) {

    /* Free input and output filters */
    filter_destroy(&(*ppeer)->filter[FILTER_IN]);
    filter_destroy(&(*ppeer)->filter[FILTER_OUT]);

    /* Free input and output adjacent RIBs */
    rib_destroy(&(*ppeer)->adj_rib[RIB_IN]);
    rib_destroy(&(*ppeer)->adj_rib[RIB_OUT]);

    FREE(*ppeer);
    *ppeer= NULL;
  }
}

   
// ----- bgp_peer_flag_set ------------------------------------------
/**
 * Change the state of a flag of this peer.
 */
void bgp_peer_flag_set(bgp_peer_t * peer, uint8_t flag, int state)
{
  if (state)
    peer->flags|= flag;
  else
    peer->flags&= ~flag;
}

// ----- bgp_peer_flag_get ------------------------------------------
/**
 * Return the state of a flag of this peer.
 */
int bgp_peer_flag_get(bgp_peer_t * peer, uint8_t flag)
{
  return (peer->flags & flag) != 0;
}

// ----- bgp_peer_set_nexthop ---------------------------------------
/**
 * Set the next-hop to be sent for routes advertised to this peer.
 */
int bgp_peer_set_nexthop(bgp_peer_t * peer, net_addr_t next_hop)
{
  // Check that the local router has this address
  if (!node_has_address(peer->router->node, next_hop))
    return -1;

  peer->next_hop= next_hop;

  return 0;
}

// -----[ bgp_peer_set_source ]--------------------------------------
int bgp_peer_set_source(bgp_peer_t * peer, net_addr_t src_addr)
{
  peer->src_addr= src_addr;

  return 0;
}

// ----- _bgp_peer_prefix_disseminate -------------------------------
/**
 * Internal helper function: send the given route to this prefix.
 */
static int _bgp_peer_prefix_disseminate(uint32_t uKey, uint8_t uKeyLen,
					void * pItem, void * pContext)
{
  bgp_peer_t * peer= (bgp_peer_t *) pContext;
  bgp_route_t * route= (bgp_route_t *) pItem;

  bgp_router_decision_process_disseminate_to_peer(peer->router,
						  route->prefix,
						  route, peer);
  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// BGP SESSION MANAGEMENT FUNCTIONS
//
// One peering session may be in one of 4 states:
//   IDLE       : the session is not established (and administratively
//                down)
//   ACTIVE     : the session is not established due to a network
//                problem 
//   OPENWAIT   : the router has sent an OPEN message to the peer
//                router and awaits an OPEN message reply, or an
//                UPDATE.
//   ESTABLISHED: the router has received an OPEN message or an UPDATE
//                message while being in OPENWAIT state.
/////////////////////////////////////////////////////////////////////

// -----[ bgp_peer_session_ok ]--------------------------------------
/**
 * Checks if the session with the peer is operational. First, the
 * function checks if there is a route towards the peer router. Then,
 * the function checks if the resulting link is up. If both conditions
 * are met, the BGP session is considered OK.
 */
int bgp_peer_session_ok(bgp_peer_t * peer)
{
  int result;
  ip_dest_t dest;
  ip_trace_t * trace= NULL;
  array_t * traces;

  if (bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    dest.type= NET_DEST_ADDRESS;
    dest.addr= peer->addr;

    // This must be updated: the router is not necessarily
    // adjacent... need to perform a kind of traceroute...
    traces= icmp_trace_send(peer->router->node,
			     peer->addr, 255, NULL);
    assert(traces != NULL);
    assert(_array_length(traces) == 1); // ECMP is not enabled
    _array_get_at(traces, 0, &trace);
    result= (trace->status == ESUCCESS);
    _array_destroy(&traces);
  } else {
    result= icmp_ping_send_recv(peer->router->node,
				 0, peer->addr, 0);
    result= (result == ESUCCESS);
  }
  return result;
}

// ----- bgp_peer_session_refresh -----------------------------------
/**
 * Refresh the state of the BGP session. If the session is currently
 * in ESTABLISHED or OPENWAIT state, test if it is still
 * operational. If the session is in ACTIVE state, test if it must be
 * restarted.
 */
void bgp_peer_session_refresh(bgp_peer_t * peer)
{
  if ((peer->session_state == SESSION_STATE_ESTABLISHED) ||
      (peer->session_state == SESSION_STATE_OPENWAIT)) {

    /* Check that the peer is reachable (that is, there is a route
       towards the peer). If not, shutdown the peering. */
    if (!bgp_peer_session_ok(peer)) {
      assert(!bgp_peer_close_session(peer));
      peer->session_state= SESSION_STATE_ACTIVE;
    }

  } else if (peer->session_state == SESSION_STATE_ACTIVE) {
    
    /* Check that the peer is reachable (that is, there is a route
       towards the peer). If yes, open the session. */
    if (bgp_peer_session_ok(peer))
      assert(!bgp_peer_open_session(peer));
    
  }
}

// ----- bgp_peer_open_session --------------------------------------
/**
 * Open the session with the peer. Opening the session includes
 * sending a BGP OPEN message to the peer and switching to OPENWAIT
 * state. Virtual peers are treated in a special way: since the peer
 * does not really exist, no message is sent and the state directly
 * becomes ESTABLISHED if the peer node is reachable.
 *
 * There is also an option for virtual peers. Their routes are learned
 * by the way of the 'recv' command. If the session becomes down, the
 * Adj-RIB-in is normally cleared. However, for virtual peers, when
 * the session is restarted, the routes must be re-sent. Using the
 * soft-restart option prevent the Adj-RIB-in to be cleared. When the
 * session goes up, the decision process is re-run for the routes
 * present in the Adj-RIB-in.
 *
 * Precondition:
 * - the session must be in IDLE or ACTIVE state or an error will be
 *   issued.
 */
int bgp_peer_open_session(bgp_peer_t * peer)
{
  bgp_msg_t * msg;
  int error;

  // Check that operation is permitted in this state
  if ((peer->session_state != SESSION_STATE_IDLE) &
      (peer->session_state != SESSION_STATE_ACTIVE)) {
    STREAM_ERR(STREAM_LEVEL_WARNING, "Warning: session already opened\n");
    STREAM_ERR(STREAM_LEVEL_WARNING, "         router = ");
    bgp_router_dump_id(gdserr, peer->router);
    STREAM_ERR(STREAM_LEVEL_WARNING, "\n");
    STREAM_ERR(STREAM_LEVEL_WARNING, "         peer   = ");
    bgp_peer_dump_id(gdserr, peer);
    STREAM_ERR(STREAM_LEVEL_WARNING, "\n");
    return EBGP_PEER_INVALID_STATE;
  }

  // Initialize "TCP" sequence numbers and router-id
  peer->router_id= NET_ADDR_ANY;
  peer->send_seq_num= 0;
  peer->recv_seq_num= 0;

  /* Send an OPEN message to the peer (except for virtual peers) */
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    msg= bgp_msg_open_create(peer->router->asn,
			     peer->router->rid);
    error= _bgp_peer_send(peer, msg);
    if (error == ESUCCESS) {
      peer->session_state= SESSION_STATE_OPENWAIT;
    } else {
      peer->session_state= SESSION_STATE_ACTIVE;
      peer->router_id= NET_ADDR_ANY;
      peer->send_seq_num= 0;
      peer->recv_seq_num= 0;
    }
    peer->last_error= error;
  } else {
    
    /* For virtual peers, we check that the peer is reachable
     * through the IGP. If so, the state directly goes to
     * ESTABLISHED. Otherwise, the state goes to ACTIVE. */
    if (bgp_peer_session_ok(peer)) {
      peer->session_state= SESSION_STATE_ESTABLISHED;
      peer->router_id= peer->addr;
      
      /* If the virtual peer is configured with the soft-restart
	 option, scan the Adj-RIB-in and run the decision process
	 for each route. */
      if (bgp_peer_flag_get(peer, PEER_FLAG_SOFT_RESTART))
	_bgp_peer_rescan_adjribin(peer, 0);
      
      peer->last_error= ESUCCESS;
    } else {
      peer->session_state= SESSION_STATE_ACTIVE;
      peer->last_error= EBGP_PEER_UNREACHABLE;
    }
  }
  
  return ESUCCESS;
}

// ----- bgp_peer_close_session -------------------------------------
/**
 * Close the BGP session with the peer. Closing the BGP session
 * includes sending a BGP CLOSE message, withdrawing the routes
 * learned through this peer and clearing the peer's Adj-RIB-in.
 *
 * Precondition:
 * - the peer must not be in IDLE state.
 */
int bgp_peer_close_session(bgp_peer_t * peer)
{
  bgp_msg_t * msg;
  int iClear;

  // Check that operation is permitted in this state
  if (peer->session_state == SESSION_STATE_IDLE) {
    STREAM_ERR(STREAM_LEVEL_WARNING, "Warning: session not opened\n");
    return EBGP_PEER_INVALID_STATE;
  }

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "> AS%d.peer_close_session.begin\n",
		  peer->router->asn);
    stream_printf(gdsdebug, "\tpeer: AS%d\n", peer->asn);
  }

  /* If the session is in OPENWAIT or ESTABLISHED state, send a
     CLOSE message to the peer (except for virtual peers). */
  if ((peer->session_state == SESSION_STATE_OPENWAIT) ||
      (peer->session_state == SESSION_STATE_ESTABLISHED)) {
    
    if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
      msg= bgp_msg_close_create(peer->router->asn);
      _bgp_peer_send(peer, msg);
    }
    
  }    
  peer->session_state= SESSION_STATE_IDLE;
  peer->router_id= NET_ADDR_ANY;
  peer->send_seq_num= 0;
  peer->recv_seq_num= 0;
  
  /* For virtual peers configured with the soft-restart option, do
     not clear the Adj-RIB-in. */
  iClear= !(bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL) &&
	    bgp_peer_flag_get(peer, PEER_FLAG_SOFT_RESTART));
  _bgp_peer_rescan_adjribin(peer, iClear);
  
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "< AS%d.peer_close_session.end\n",
	    peer->router->asn);
  
  return ESUCCESS;
}

// ----- bgp_peer_session_error -------------------------------------
/**
 * This function is used to dump information on the peering session
 * (the AS number and the IP address of the local router and the peer
 * router).
 */
void bgp_peer_session_error(bgp_peer_t * peer)
{
  STREAM_ERR_ENABLED(STREAM_LEVEL_FATAL) {
    stream_printf(gdserr, "Error: peer=");
    bgp_peer_dump_id(gdserr, peer);
    stream_printf(gdserr, "\nError: router=");
    bgp_router_dump_id(gdserr, peer->router);
    stream_printf(gdserr, "\n");
  }
}

// ----- _bgp_peer_session_open_rcvd --------------------------------
static inline void _bgp_peer_session_open_rcvd(bgp_peer_t * peer,
					       bgp_msg_open_t * msg)
{
  /* Check that the message does not come from a virtual peer */
  if (bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    STREAM_ERR(STREAM_LEVEL_FATAL,
	    "Error: OPEN message received from virtual peer\n");
    bgp_peer_session_error(peer);
    abort();
  }

  STREAM_DEBUG(STREAM_LEVEL_INFO, "BGP_MSG_RCVD: OPEN\n");
  switch (peer->session_state) {
  case SESSION_STATE_ACTIVE:
    peer->session_state= SESSION_STATE_ESTABLISHED;
    _bgp_peer_send(peer,
		   bgp_msg_open_create(peer->router->asn,
				       peer->router->rid));
    rib_for_each(peer->router->loc_rib,
		 _bgp_peer_prefix_disseminate, peer);
    break;
  case SESSION_STATE_OPENWAIT:
    peer->session_state= SESSION_STATE_ESTABLISHED;
    peer->router_id= msg->router_id;
    rib_for_each(peer->router->loc_rib,
		 _bgp_peer_prefix_disseminate, peer);
    break;
  default:
    STREAM_ERR(STREAM_LEVEL_FATAL, "Error: OPEN received while in %s state\n",
	    SESSION_STATES[peer->session_state]);
    bgp_peer_session_error(peer);
    abort();
  }
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[peer->session_state]);
}

// ----- _bgp_peer_session_close_rcvd -------------------------------
static inline void _bgp_peer_session_close_rcvd(bgp_peer_t * peer)
{
  /* Check that the message does not come from a virtual peer */
  if (bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    STREAM_ERR(STREAM_LEVEL_FATAL,
	    "Error: CLOSE message received from virtual peer\n");
    bgp_peer_session_error(peer);
    abort();
  }

  STREAM_DEBUG(STREAM_LEVEL_INFO, "BGP_MSG_RCVD: CLOSE\n");
  switch (peer->session_state) {
  case SESSION_STATE_ESTABLISHED:
  case SESSION_STATE_OPENWAIT:
    peer->session_state= SESSION_STATE_ACTIVE;
    peer->router_id= NET_ADDR_ANY;
    peer->send_seq_num= 0;
    peer->recv_seq_num= 0;
    _bgp_peer_rescan_adjribin(peer, !bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL));
    break;
  case SESSION_STATE_ACTIVE:
  case SESSION_STATE_IDLE:
    break;
  default:
    STREAM_ERR(STREAM_LEVEL_FATAL, "Error: CLOSE received while in %s state\n",
	      SESSION_STATES[peer->session_state]);
    bgp_peer_session_error(peer);
    abort();    
  }
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[peer->session_state]);
}

// ----- _bgp_peer_session_update_rcvd ------------------------------
static inline void _bgp_peer_session_update_rcvd(bgp_peer_t * peer,
						 bgp_msg_update_t * msg)
{
  STREAM_DEBUG(STREAM_LEVEL_INFO, "BGP_MSG_RCVD: UPDATE\n");
  switch (peer->session_state) {
  case SESSION_STATE_OPENWAIT:
    peer->session_state= SESSION_STATE_ESTABLISHED;
    break;
  case SESSION_STATE_ESTABLISHED:
    break;
  default:
    STREAM_ERR(STREAM_LEVEL_WARNING,
	    "Warning: UPDATE received while in %s state\n",
	    SESSION_STATES[peer->session_state]);
    bgp_peer_session_error(peer);
    abort();
  }
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[peer->session_state]);

  /* Process UPDATE message */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  _bgp_peer_process_update_walton(peer, msg);
#else
  _bgp_peer_process_update(peer, msg);
#endif

}

// ----- _bgp_peer_session_withdraw_rcvd ----------------------------
static inline void _bgp_peer_session_withdraw_rcvd(bgp_peer_t * peer,
						   bgp_msg_withdraw_t * msg)
{
  STREAM_DEBUG(STREAM_LEVEL_INFO, "BGP_MSG_RCVD: WITHDRAW\n");
  switch (peer->session_state) {
  case SESSION_STATE_OPENWAIT:
    peer->session_state= SESSION_STATE_ESTABLISHED;
    break;
  case SESSION_STATE_ESTABLISHED:
    break;
  default:
    STREAM_ERR(STREAM_LEVEL_FATAL, "Error: WITHDRAW received while in %s state\n",
	    SESSION_STATES[peer->session_state]);
    bgp_peer_session_error(peer);
    abort();    
  }
  STREAM_DEBUG(STREAM_LEVEL_DEBUG, "BGP_FSM_STATE: %s\n",
	    SESSION_STATES[peer->session_state]);

  /* Process WITHDRAW message */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  _bgp_peer_process_withdraw_walton(peer,  msg);
#else
  _bgp_peer_process_withdraw(peer, msg);
#endif

}

// ----- _bgp_peer_disable_adjribin ---------------------------------
/**
 * TODO: write documentation...
 */
static int _bgp_peer_disable_adjribin(uint32_t uKey, uint8_t uKeyLen,
				      void * pItem, void * pContext)
{
  bgp_route_t * route= (bgp_route_t *) pItem;
  bgp_peer_t * peer= (bgp_peer_t *) pContext;

  //route_flag_set(route, ROUTE_FLAG_ELIGIBLE, 0);

  /* Since the ROUTE_FLAG_BEST is handled in the Adj-RIB-In, we only
     need to run the decision process if the route is installed in the
     Loc-RIB (i.e. marked as best) */
  if (route_flag_get(route, ROUTE_FLAG_BEST)) {

    STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
      stream_printf(gdsdebug, "\trescan: ", peer->router->asn);
      route_dump(gdsdebug, route);
      stream_printf(gdsdebug, "\n");
    }
    bgp_router_decision_process(peer->router, peer, route->prefix);

  }

  return 0;
}

// ----- _bgp_peer_enable_adjribin ----------------------------------
/**
 * TODO: write documentation...
 */
static int _bgp_peer_enable_adjribin(uint32_t uKey, uint8_t uKeyLen,
				     void * pItem, void * pContext)
{
  bgp_route_t * route= (bgp_route_t *) pItem;
  bgp_peer_t * peer= (bgp_peer_t *) pContext;

  //route_flag_set(route, ROUTE_FLAG_ELIGIBLE, 0);

  bgp_router_decision_process(peer->router, peer, route->prefix);

  return 0;
}

// ----- _bgp_peer_adjrib_clear -------------------------------------
/**
 * TODO: write documentation...
 */
static void _bgp_peer_adjrib_clear(bgp_peer_t * peer, bgp_rib_dir_t dir)
{
  rib_destroy(&peer->adj_rib[dir]);
  peer->adj_rib[dir]= rib_create(0);
}

// ----- _bgp_peer_rescan_adjribin ----------------------------------
/**
 * TODO: write documentation...
 */
static inline void _bgp_peer_rescan_adjribin(bgp_peer_t * peer, int iClear)
{
  if (peer->session_state == SESSION_STATE_ESTABLISHED) {

    rib_for_each(peer->adj_rib[RIB_IN], _bgp_peer_enable_adjribin,
		 peer);

  } else {

    // For each route in Adj-RIB-In, mark as unfeasible
    // and run decision process for each route marked as best
    rib_for_each(peer->adj_rib[RIB_IN], _bgp_peer_disable_adjribin,
		 peer);
    
    // Clear Adj-RIB-In ?
    if (iClear)
      _bgp_peer_adjrib_clear(peer, 1);

  }

}

/////////////////////////////////////////////////////////////////////
//
// BGP FILTERS
//
/////////////////////////////////////////////////////////////////////

// -----[ bgp_peer_set_filter ]----------------------------------------
/**
 * Change a filter of this peer. The previous filter is destroyed.
 */
void bgp_peer_set_filter(bgp_peer_t * peer, bgp_filter_dir_t dir,
			 bgp_filter_t * filter)
{
  if (peer->filter[dir] != NULL)
    filter_destroy(&peer->filter[dir]);
  peer->filter[dir]= filter;
}

// -----[ bgp_peer_filter_get ]--------------------------------------
/**
 * Return a filter of this peer.
 */
bgp_filter_t * bgp_peer_filter_get(bgp_peer_t * peer,
				   bgp_filter_dir_t dir)
{
  return peer->filter[dir];
}


/////////////////////////////////////////////////////////////////////
//
// BGP MESSAGE HANDLING
//
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_announce_route ------------------------------------
/**
 * Announce the given route to the given peer.
 *
 * PRE: the route must be a copy that can live its own life,
 * i.e. there must not be references to the route.
 *
 * Note: if the route is not delivered (for instance in the case of
 * a virtual peer), the given route is freed.
 */
void bgp_peer_announce_route(bgp_peer_t * peer, bgp_route_t * route)
{
  bgp_msg_t * msg;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "announce_route to ");
    bgp_peer_dump_id(gdsdebug, peer);
    stream_printf(gdsdebug, "\n");
  }

  route_peer_set(route, peer);
  msg= bgp_msg_update_create(peer->router->asn, route);

  /* Send the message to the peer */
  if (_bgp_peer_send(peer, msg) < 0)
    route_destroy(&route);
}

// ----- bgp_peer_withdraw_prefix -----------------------------------
/**
 * Withdraw the given prefix to the given peer.
 *
 * Note: withdraws are not sent to virtual peers.
 */
void bgp_peer_withdraw_prefix(bgp_peer_t * peer, ip_pfx_t prefix)
{
  // Send the message to the peer (except if this is a virtual peer)
  _bgp_peer_send(peer,
		 bgp_msg_withdraw_create(peer->router->asn,
					 prefix));
}

// ----- peer_route_delay_update ------------------------------------
/**
 * Add the delay between this router and the next-hop. At this moment,
 * the delay is taken from an existing direct link between this router
 * and the next-hop but in the future such a direct link may not exist
 * and the information should be taken from the IGP.
 */
void peer_route_delay_update(bgp_peer_t * peer, bgp_route_t * route)
{
  net_iface_t * iface;

  STREAM_ERR(STREAM_LEVEL_FATAL,
	  "Error: peer_route_delay_update MUST be modified\n"
	  "Error: to support route-reflection !!!");
  abort();

  /*pLink= node_find_link_to_router(peer->router->node,
    route->next_hop);*/
  assert(iface != NULL);

#ifdef BGP_QOS
  qos_route_update_delay(route, iface->tDelay);
#endif
}

// ----- peer_route_rr_client_update --------------------------------
/**
 * Store in the route a flag indicating whether the route has been
 * learned from an RR client or not. This is used to improve the speed
 * of the redistribution step of the BGP decision process [as.c,
 * bgp_router_advertise_to_peer]
 */
void peer_route_rr_client_update(bgp_peer_t * peer, bgp_route_t * route)
{
  route_flag_set(route, ROUTE_FLAG_RR_CLIENT,
		 bgp_peer_flag_get(peer, PEER_FLAG_RR_CLIENT));
}

// ----- peer_comm_process ------------------------------------------
/**
 * Apply input communities.
 *   - DEPREF community [ EXPERIMENTAL ]
 *   - PREF ext-community [ EXPERIMENTAL ]
 *
 * Returns:
 *   0 => Ignore route (destroy)
 *   1 => Redistribute
 */
int peer_comm_process(bgp_route_t * route)
{
  unsigned int index;
  bgp_ecomm_t * ecomm;

  /* Classical communities */
  if (route->attr->comms != NULL) {
#ifdef __EXPERIMENTAL__
    if (route_comm_contains(route, COMM_DEPREF)) {
      route_localpref_set(route, 0);
    }
#endif
  }

  /* Extended communities */
  if (route->attr->ecomms != NULL) {

    for (index= 0; index < ecomms_length(route->attr->ecomms); index++) {
      ecomm= ecomms_get_at(route->attr->ecomms, index);
      switch (ecomm->type_high) {

#ifdef __EXPERIMENTAL__
      case ECOMM_PREF:
	route_localpref_set(route, ecomm_pref_get(ecomm));
	break;
#endif

      }
    }
  }

  return 1;
}

// ----- bgp_peer_route_eligible ------------------------------------
/**
 * The route is eligible if it passes the input filters. Standard
 * filters are applied first. Then, extended communities actions are
 * taken if any (see 'peer_ecomm_process').
 */
int bgp_peer_route_eligible(bgp_peer_t * peer, bgp_route_t * route)
{
  // Check that the route's AS-path does not contain the local AS
  // number.
  if (route_path_contains(route, peer->router->asn)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "in-filtered(as-path loop)\n");
    return 0;
  }

  // Route-Reflection: deny routes with an originator-ID equal to local
  // router-ID [RFC2796, section7].
  if ((route->attr->originator != NULL) &&
      (*route->attr->originator == peer->router->rid)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "in-filtered(RR: originator-id)\n");
    return 0;
  }

  // Route-Reflection: Avoid cluster-loop creation (MUST be done
  // before local cluster-ID is appended) [RFC2796, section7].
  if ((route->attr->cluster_list != NULL) &&
      (route_cluster_list_contains(route, peer->router->cluster_id))) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "in-filtered(RR: cluster-loop)\n");
    return 0;
  }
  
  // Apply the input filters.
  if (!filter_apply(peer->filter[FILTER_IN], peer->router, route)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "in-filtered(filter)\n");
    return 0;
  }

  // Process communities.
  if (!peer_comm_process(route)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "in-filtered(community)\n");
    return 0;
  }

  return 1;
}

// ----- bgp_peer_route_feasible ------------------------------------
/**
 * The route is feasible if and only if the next-hop is reachable
 * (through a STATIC, IGP or BGP route).
 */
int bgp_peer_route_feasible(bgp_peer_t * self, bgp_route_t * route)
{
  const rt_entries_t * rtentries=
    node_rt_lookup(self->router->node, route->attr->next_hop);
  return (rtentries != NULL);
}

// -----[ _bgp_peer_process_update ]----------------------------------
/**
 * Process a BGP UPDATE message.
 *
 * The operation is as follows:
 * 1). If a route towards the same prefix already exists it is
 *     replaced by the new one. Otherwise the new route is added.
 * 2). Tag the route as received from this peer.
 * 3). Clear the LOCAL-PREF is the session is external (eBGP)
 * 4). Test if the route is feasible (next-hop reachable) and flag
 *     it accordingly.
 * 5). The decision process is run.
 */
static inline void _bgp_peer_process_update(bgp_peer_t * peer,
					    bgp_msg_update_t * msg)
{
  bgp_route_t * route= msg->route;
  bgp_route_t * pOldRoute= NULL;
  ip_pfx_t prefix;
  int need_DP_run;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "\tupdate: ");
    route_dump(gdsdebug, route);
    stream_printf(gdsdebug, "\n");
  }

  // Tag the route as received from this peer
  route_peer_set(route, peer);

  // If route learned over eBGP, clear LOCAL-PREF
  if (peer->asn != peer->router->asn)
    route_localpref_set(route, bgp_options_get_local_pref());

  // Check route against import filter
  route_flag_set(route, ROUTE_FLAG_BEST, 0);
  route_flag_set(route, ROUTE_FLAG_INTERNAL, 0);
  route_flag_set(route, ROUTE_FLAG_ELIGIBLE,
		 bgp_peer_route_eligible(peer, route));
  route_flag_set(route, ROUTE_FLAG_FEASIBLE,
		 bgp_peer_route_feasible(peer, route));
  
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    route_flag_set(route, ROUTE_FLAG_EXTERNAL_BEST, 0);
#endif

  // Update route delay attribute (if BGP-QoS)
  //peer_route_delay_update(peer, route);

  // Update route RR-client flag
  peer_route_rr_client_update(peer, route);

  // The decision process need only be run in the following cases:
  // - the old route was the best
  // - the new route is eligible
  need_DP_run= 0;
  pOldRoute= rib_find_exact(peer->adj_rib[RIB_IN], route->prefix);
  if (((pOldRoute != NULL) &&
       route_flag_get(pOldRoute, ROUTE_FLAG_BEST)) ||
      route_flag_get(route, ROUTE_FLAG_ELIGIBLE)) 
    need_DP_run= 1;

  prefix= route->prefix;
  
  // Replace former route in Adj-RIB-In
  if (route_flag_get(route, ROUTE_FLAG_ELIGIBLE)) {
    assert(rib_replace_route(peer->adj_rib[RIB_IN], route) == 0);
  } else {
    if (pOldRoute != NULL)
      assert(rib_remove_route(peer->adj_rib[RIB_IN], route->prefix) == 0);
    route_destroy(&route);
  }
  
  // Run decision process for this route
  if (need_DP_run)
    bgp_router_decision_process(peer->router, peer, prefix);
}

// -----[ _bgp_peer_process_withdraw ]--------------------------------
/**
 * Process a BGP WITHDRAW message.
 *
 * The operation is as follows:
 * 1). If a route towards the withdrawn prefix already exists, it is
 *     set as 'uneligible'
 * 2). The decision process is ran for the destination prefix.
 * 3). The old route is removed from the Adj-RIB-In.
 */
static inline void _bgp_peer_process_withdraw(bgp_peer_t * peer,
					      bgp_msg_withdraw_t * msg)
{
  bgp_route_t * route;
  
  // Identifiy route to be removed based on destination prefix
  route= rib_find_exact(peer->adj_rib[RIB_IN], msg->prefix);
  
  // If there was no previous route, do nothing
  // Note: we should probably trigger an error/warning message in this case
  if (route == NULL)
    return;

  // Flag the route as un-eligible
  route_flag_set(route, ROUTE_FLAG_ELIGIBLE, 0);

  // Run decision process in case this route is the best route
  // towards this prefix
  bgp_router_decision_process(peer->router, peer,
			      msg->prefix);
  
  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "\tremove: ");
    route_dump(gdsdebug, route);
    stream_printf(gdsdebug, "\n");
  }
  
  assert(rib_remove_route(peer->adj_rib[RIB_IN], msg->prefix) == 0);
}


// -----[ _bgp_peer_seqnum_check ]-----------------------------------
/**
 * Check BGP message sequence number. The sequence number of the
 * received message must be equal to the recv-sequence-number stored
 * in the peering state. If this condition is not true, the program
 * is aborted.
 *
 * There are two exceptions:
 *   - this check is not performed for messages received from a
 *     virtual peering
 *   
 */
static inline int _bgp_peer_seqnum_check(bgp_peer_t * peer,
					  bgp_msg_t * msg)
{
  if ((bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL) == 0) &&
      (peer->recv_seq_num != msg->seq_num)) {
    if (msg->type != BGP_MSG_TYPE_CLOSE) {
      stream_printf(gdserr, "BGP session sequence number check failed\n");
      stream_printf(gdserr, "  router       = ");
      bgp_router_dump_id(gdserr, peer->router);
      stream_printf(gdserr, "\n");
      stream_printf(gdserr, "  peer         = ");
      bgp_peer_dump_id(gdserr, peer);
      stream_printf(gdserr, "\n");
      stream_printf(gdserr, "  rcvd seq-num = %d\n", msg->seq_num);
      stream_printf(gdserr, "  exp. seq-num = %d\n", peer->recv_seq_num);
      stream_printf(gdserr, "  state        = %s\n",
		    SESSION_STATES[peer->session_state]);
      stream_printf(gdserr, "  message      = \n    ");
      bgp_msg_dump(gdserr, peer->router->node, msg);
      stream_printf(gdserr, "\n");
      stream_printf(gdserr, "\n");
      stream_printf(gdserr, "  Note: This error can occur for several reasons. \n");
      stream_printf(gdserr, "        The most common case for this error is when the underlying route of a\n");
      stream_printf(gdserr, "        BGP session changes during the simulation convergence (sim run).\n");
      stream_printf(gdserr, "\n");
      stream_flush(gdserr);
      return -1;
    }
  }
  return 0;
}

// ----- bgp_peer_handle_message ------------------------------------
/**
 * Handle received BGP messages (OPEN, CLOSE, UPDATE, WITHDRAW).
 * Check the sequence number of incoming messages.
 * The caller is responsible for freeing the message. Don't destroy
 * it here !!!
 */
int bgp_peer_handle_message(bgp_peer_t * peer, bgp_msg_t * msg)
{
  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "HANDLE_MESSAGE from ");
    bgp_peer_dump_id(gdsdebug, peer);
    stream_printf(gdsdebug, " in ");
    bgp_router_dump_id(gdsdebug, peer->router);
    stream_printf(gdsdebug, "\n");
  }

  if (_bgp_peer_seqnum_check(peer, msg) < 0)
    return EBGP_PEER_OUT_OF_SEQ;
  peer->recv_seq_num++;

  switch (msg->type) {
  case BGP_MSG_TYPE_UPDATE:
    _bgp_peer_session_update_rcvd(peer, (bgp_msg_update_t *) msg);
    break;

  case BGP_MSG_TYPE_WITHDRAW:
    _bgp_peer_session_withdraw_rcvd(peer, (bgp_msg_withdraw_t *) msg);
    break;

  case BGP_MSG_TYPE_CLOSE:
    _bgp_peer_session_close_rcvd(peer); break;

  case BGP_MSG_TYPE_OPEN:
    _bgp_peer_session_open_rcvd(peer, (bgp_msg_open_t *) msg); break;

  default:
    bgp_peer_session_error(peer);
    STREAM_ERR(STREAM_LEVEL_FATAL, "Unknown BGP message type received (%d)\n",
	    msg->type);
    abort();
  }
  return 0;
}

// -----[ _bgp_peer_send ]-------------------------------------------
/**
 * Send a BGP message to the given peer. If activated, this function
 * will tap BGP messages and record them to a file (see
 * 'pRecordStream').
 *
 * Note: if the peer is virtual, the message will be discarded and the
 * function will return an error.
 */
static inline int _bgp_peer_send(bgp_peer_t * peer, bgp_msg_t * msg)
{
  msg->seq_num= peer->send_seq_num++;
  
  // Record BGP messages (optional)
  if (peer->pRecordStream != NULL) {
    bgp_msg_dump(peer->pRecordStream, NULL, msg);
    stream_printf(peer->pRecordStream, "\n");
    stream_flush(peer->pRecordStream);
  }

  // Send the message
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    return bgp_msg_send(peer->router->node,
			peer->src_addr,
			peer->addr, msg);
  } else {
    bgp_msg_destroy(&msg);
    return EBGP_PEER_INCOMPATIBLE;
  }
}

// -----[ bgp_peer_send_enabled ]------------------------------------
/**
 * Tell if this peer can send BGP messages. Basically, all non-virtual
 * peers are able to send BGP messages. However, if the record
 * functionnality is activated on a virtual peer, this peer will
 * appear as if it was able to send messages. This is needed in order
 * to tap BGP messages sent to virtual peers.
 *
 * Return values:
 *      0 if the peer cannot send messages
 *   != 0 otherwise
 */
int bgp_peer_send_enabled(bgp_peer_t * peer)
{
  return (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL) ||
	  (peer->pRecordStream != NULL));
}

/////////////////////////////////////////////////////////////////////
//
// INFORMATION RETRIEVAL
//
/////////////////////////////////////////////////////////////////////

// ----- bgp_peer_dump_id -------------------------------------------
/**
 *
 */
void bgp_peer_dump_id(gds_stream_t * stream, bgp_peer_t * peer)
{
  stream_printf(stream, "AS%d:", peer->asn);
  ip_address_dump(stream, peer->addr);
}

// ----- bgp_peer_dump ----------------------------------------------
/**
 *
 */
void bgp_peer_dump(gds_stream_t * stream, bgp_peer_t * peer)
{
  int iOptions= 0;

  ip_address_dump(stream, peer->addr);
  stream_printf(stream, "\tAS%d\t%s\t", peer->asn,
	  SESSION_STATES[peer->session_state]);
  ip_address_dump(stream, peer->router_id);
  if (bgp_peer_flag_get(peer, PEER_FLAG_RR_CLIENT)) {
    stream_printf(stream, (iOptions++)?",":"\t(");
    stream_printf(stream, "rr-client");
  }
  if (bgp_peer_flag_get(peer, PEER_FLAG_NEXT_HOP_SELF)) {
    stream_printf(stream, (iOptions++)?",":"\t(");
    stream_printf(stream, "next-hop-self");
  }
  if (bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    stream_printf(stream, (iOptions++)?",":"\t(");
    stream_printf(stream, "virtual");
  }
  if (bgp_peer_flag_get(peer, PEER_FLAG_SOFT_RESTART)) {
    stream_printf(stream, (iOptions++)?",":"\t(");
    stream_printf(stream, "soft-restart");
  }
  stream_printf(stream, (iOptions)?")":"");

  // Session state
  if (peer->session_state == SESSION_STATE_ESTABLISHED) {
    if (!bgp_peer_session_ok(peer)) {
      stream_printf(stream, "\t[DOWN]");
    }
  }

  stream_printf(stream, "\tsnd-seq:%d\trcv-seq:%d", peer->send_seq_num,
	     peer->recv_seq_num);
}

// -----[ bgp_peer_show_info ]---------------------------------------
void bgp_peer_show_info(gds_stream_t * stream, bgp_peer_t * peer)
{
  stream_printf(stream, "address    : ");
  ip_address_dump(stream, peer->addr);
  stream_printf(stream, "\n");
  stream_printf(stream, "asn        : %u\n", peer->asn);
  stream_printf(stream, "router-id  : ");
  ip_address_dump(stream, peer->router_id);
  stream_printf(stream, "\n");
  stream_printf(stream, "state      : %s\n",
		SESSION_STATES[peer->session_state]);
  if (peer->flags & PEER_FLAG_NEXT_HOP_SELF)
    stream_printf(stream, "flag       : next-hop-self\n");
  if (peer->flags & PEER_FLAG_RR_CLIENT)
    stream_printf(stream, "flag       : rr-client\n");
  if (peer->flags & PEER_FLAG_SOFT_RESTART)
    stream_printf(stream, "flag       : soft-restart\n");
  if (peer->flags & PEER_FLAG_VIRTUAL)
    stream_printf(stream, "flag       : virtual\n");
  stream_printf(stream, "snd-seq    : %u\n", peer->send_seq_num);
  stream_printf(stream, "rcv-seq    : %u\n", peer->recv_seq_num);
  if (peer->next_hop != NET_ADDR_ANY) {
    stream_printf(stream, "next-hop   : ");
    ip_address_dump(stream, peer->next_hop);
    stream_printf(stream, "\n");
  }
  if (peer->src_addr != NET_ADDR_ANY) {
    stream_printf(stream, "source-addr: ");
    ip_address_dump(stream, peer->src_addr);
    stream_printf(stream, "\n");
  }
  stream_printf(stream, "last-error : ");
  network_perror(stream, peer->last_error);
  stream_printf(stream, "\n");
}

typedef struct {
  bgp_peer_t * peer;
  gds_stream_t * stream;
} bgp_route_dump_ctx_t;

// ----- bgp_peer_dump_route ----------------------------------------
static int bgp_peer_dump_route(uint32_t key, uint8_t key_len,
			       void * item, void * context)
{
  bgp_route_dump_ctx_t * ctx= (bgp_route_dump_ctx_t *) context;

  route_dump(ctx->stream, (bgp_route_t *) item);
  stream_printf(ctx->stream, "\n");
  return 0;
}

// ----- bgp_peer_dump_adjrib ---------------------------------------
/**
 * Dump Adj-RIB-In/Out.
 *
 * Parameters:
 * @param dir    selects between Adj-RIB-in and Adj-RIB-out
 * @param prefix selects a specific prefix (prefix length < 32),
 *               the longest prefix matching an address (prefix length >= 32),
 *               or all prefixes (prefix length == 0)
 */
void bgp_peer_dump_adjrib(gds_stream_t * stream, bgp_peer_t * peer,
			  ip_pfx_t prefix, bgp_rib_dir_t dir)
{
  bgp_route_dump_ctx_t ctx= { .stream=stream, .peer=peer };
  bgp_route_t * route;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_route_ts * routes;
  uint16_t i;
#endif

  // All prefixes
  if (prefix.mask == 0) {
    rib_for_each(peer->adj_rib[dir], bgp_peer_dump_route, &ctx);
    return;
  }

  // Single address (best match) or single prefix (exact match)
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  if (prefix.mask >= 32) // Single address (best match)
    routes = rib_find_best(peer->adj_rib[dir], prefix);
  else // single prefix (exact match)
    routes = rib_find_exact(peer->adj_rib[dir], prefix);
  if (routes != NULL) {
    for (i= 0; i < routes_list_get_num(routes); i++) {
      route = routes_list_get_at(routes, i);
      if (route != NULL) {
	route_dump(stream, route);
	stream_printf(stream, "\n");
      }
    }
  }
#else
  if (prefix.mask >= 32) // Single address (best match)
    route= rib_find_best(peer->adj_rib[dir], prefix);
  else // single prefix (exact match)
    route= rib_find_exact(peer->adj_rib[dir], prefix);
  if (route != NULL) {
    route_dump(stream, route);
    stream_printf(stream, "\n");
  }
#endif
}

// ----- bgp_peer_dump_filters -----------------------------------
/**
 *
 */
void bgp_peer_dump_filters(gds_stream_t * stream, bgp_peer_t * peer,
			   bgp_filter_dir_t dir)
{
  filter_dump(stream, peer->filter[dir]);
}

// -----[ bgp_peer_set_record_stream ]-------------------------------
/**
 * Set a stream for recording the messages sent to this neighbor. If
 * the given stream is NULL, the recording will be stopped.
 */
int bgp_peer_set_record_stream(bgp_peer_t * peer, gds_stream_t * stream)
{
  if (peer->pRecordStream != NULL) {
    stream_destroy(&(peer->pRecordStream));
    peer->pRecordStream= NULL;
  }
  if (stream != NULL)
    peer->pRecordStream= stream;
  return 0;
}

/////////////////////////////////////////////////////////////////////
//
// WALTON DRAFT (EXPERIMENTAL)
//
/////////////////////////////////////////////////////////////////////

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__

// -----[ _bgp_peer_process_update_walton ]----------------------------------
/**
 * Process a BGP UPDATE message.
 *
 * The operation is as follows:
 * 1). If a route towards the same prefix already exists it is
 *     replaced by the new one. Otherwise the new route is added.
 * 2). Tag the route as received from this peer.
 * 3). Clear the LOCAL-PREF is the session is external (eBGP)
 * 4). Test if the route is feasible (next-hop reachable) and flag
 *     it accordingly.
 * 5). The decision process is run.
 */
static void _bgp_peer_process_update_walton(bgp_peer_t * peer,
					    bgp_msg_update_t * msg)
{
  bgp_route_t * route= msg->route;
  bgp_route_t * pOldRoute= NULL;
  ip_pfx_t prefix;
  int iNeedDecisionProcess;
  bgp_route_ts * pOldRoutes;
  uint16_t uIndexRoute;
  net_addr_t next_hop;

  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "\tupdate: ");
    route_dump(gdsdebug, route);
    stream_printf(gdsdebug, "\n");
  }

  // Tag the route as received from this peer
  route_peer_set(route, peer);

  // If route learned over eBGP, clear LOCAL-PREF
  if (peer->asn != peer->router->asn)
    route_localpref_set(route, bgp_options_get_local_pref());

  // Check route against import filter
  route_flag_set(route, ROUTE_FLAG_BEST, 0);
  route_flag_set(route, ROUTE_FLAG_INTERNAL, 0);
  route_flag_set(route, ROUTE_FLAG_ELIGIBLE,
		 bgp_peer_route_eligible(peer, route));
  route_flag_set(route, ROUTE_FLAG_FEASIBLE,
		 bgp_peer_route_feasible(peer, route));
  
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    route_flag_set(route, ROUTE_FLAG_EXTERNAL_BEST, 0);
#endif

  // Update route RR-client flag
  peer_route_rr_client_update(peer, route);

  // The decision process need only be run in the following cases:
  // - the old route was the best
  // - the new route is eligible
  iNeedDecisionProcess= 0;
  route_flag_set(route, ROUTE_FLAG_BEST, 0);
  pOldRoutes = rib_find_exact(peer->adj_rib[RIB_IN], route->prefix);
  if (pOldRoutes != NULL) {
    for (uIndexRoute = 0; uIndexRoute < routes_list_get_num(pOldRoutes);
	 uIndexRoute++) {
      pOldRoute = routes_list_get_at(pOldRoutes, uIndexRoute);
      STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
	stream_printf(gdsdebug, "\tupdate: ");
	route_dump(gdsdebug, pOldRoute);
	stream_printf(gdsdebug, "\n");
      }
      if ((pOldRoute != NULL) &&
	  route_flag_get(pOldRoute, ROUTE_FLAG_BEST)) {
	route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);
	iNeedDecisionProcess= 1;
	break;
      }
    }
  } else if (route_flag_get(route, ROUTE_FLAG_ELIGIBLE)) {
    iNeedDecisionProcess = 1;
  }

  prefix= route->prefix;
  
  // Replace former route in Adj-RIB-In
  if (route_flag_get(route, ROUTE_FLAG_ELIGIBLE)) {
    assert(rib_replace_route(peer->adj_rib[RIB_IN], route) == 0);
  } else {
    if (pOldRoute != NULL) {
      next_hop = route_get_nexthop(route);
      assert(rib_remove_route(peer->adj_rib[RIB_IN], route->prefix, &next_hop)
	     == 0);
    }
    route_destroy(&route);
  }
  
  // Run decision process for this route
  if (iNeedDecisionProcess)
    bgp_router_decision_process(peer->router, peer, prefix);
}

// -----[ _bgp_peer_process_withdraw_walton ]------------------------
/**
 * Process a BGP WITHDRAW message (Walton draft).
 *
 * The operation is as follows:
 * 1). If a route towards the withdrawn prefix and with the specified
 *     next-hop already exists, it is set as 'uneligible'
 * 2). The decision process is ran for the destination prefix.
 * 3). The old route is removed from the Adj-RIB-In.
 */
static void _bgp_peer_process_walton_withdraw(bgp_peer_t * peer,
					      bgp_msg_withdraw_t * msg)
{
  bgp_route_t * route;
  net_addr_t next_hop;

  //STREAM_DEBUG("walton-withdraw: next-hop=");
  //STREAM_ENABLED_DEBUG() ip_address_dump(stream_get_stream(pMainLog),
  //      			      pMsgWithdraw->next_hop);
  //STREAM_DEBUG("\n");


  // Identifiy route to be removed based on destination prefix and next-hop
  route= rib_find_one_exact(peer->adj_rib[RIB_IN], msg->prefix,
			     msg->next_hop);

  // If there was no previous route, do nothing
  // Note: we should probably trigger an error/warning message in this case
  if (route == NULL)
    return;
  
  // Flag the route as un-eligible
  route_flag_set(route, ROUTE_FLAG_ELIGIBLE, 0);

  // Run decision process in case this route is the best route
  // towards this prefix
  bgp_router_decision_process(peer->router, peer,
			      msg->prefix);
  
  STREAM_DEBUG_ENABLED(STREAM_LEVEL_DEBUG) {
    stream_printf(gdsdebug, "\tremove: ");
    route_dump(gdsdebug, route);
    stream_printf(gdsdebug, "\n");
  }

  assert(rib_remove_route(peer->adj_rib[RIB_IN], msg->prefix,
			  msg->next_hop) == 0);
}

void bgp_peer_withdraw_prefix_walton(bgp_peer_t * peer,
				     ip_pfx_t prefix,
				     net_addr_t * next_hop)
{
  // Send the message to the peer (except if this is a virtual peer)
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL))
    _bgp_peer_send(peer,
		   bgp_msg_withdraw_create(peer->router->asn,
					   prefix, next_hop));
}

// -----[ bgp_peer_set_walton_limit ]--------------------------------
void bgp_peer_set_walton_limit(bgp_peer_t * peer, unsigned int uWaltonLimit)
{
  //TODO : check the MAX_VALUE !
  peer->uWaltonLimit = uWaltonLimit;
  bgp_router_walton_peer_set(peer, uWaltonLimit);
}

// -----[ bgp_peer_get_walton_limit ]--------------------------------
uint16_t bgp_peer_get_walton_limit(bgp_peer_t * peer)
{
  return peer->uWaltonLimit;
}

#endif

