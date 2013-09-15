// ==================================================================
// @(#)network.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 4/07/2003
// $Id: network.c,v 1.59 2009-08-31 09:48:28 bqu Exp $
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
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <libgds/fifo.h>
#include <libgds/stream.h>
#include <libgds/memory.h>
#include <libgds/stack.h>
#include <libgds/str_util.h>
#include <libgds/trie.h>

#include <net/error.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/icmp.h>
#include <net/igp_domain.h>
#include <net/ipip.h>
#include <net/link-list.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/node.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/subnet.h>
#include <bgp/message.h>
#include <ui/output.h>
#include <util/str_format.h>

static network_t  * _default_network= NULL;
static simulator_t * _thread_sim= NULL;

//#define NETWORK_DEBUG

#ifdef NETWORK_DEBUG
static int _network_debug_for_each(gds_stream_t * stream, void * context,
				   char format)
{
  va_list * ap= (va_list*) context;
  net_node_t * node;
  net_addr_t addr;
  net_msg_t * msg;
  net_iface_t * iface;
  rt_entry_t * rtentry;
  int error;

  switch (format) {
  case 'a':
    addr= va_arg(*ap, net_addr_t);
    ip_address_dump(stream, addr);
    break;
  case 'e':
    error= va_arg(*ap, int);
    network_perror(stream, error);
    break;
  case 'i':
    iface= va_arg(*ap, net_iface_t *);
    net_iface_dump_id(stream, iface);
    break;
  case 'm':
    msg= va_arg(*ap, net_msg_t *);
    stream_printf(stream, "[");
    message_dump(stream, msg);
    stream_printf(stream, "]");
    break;
  case 'n':
    node= va_arg(*ap, net_node_t *);
    node_dump_id(stream, node);
    break;
  case 'r':
    rtentry= va_arg(*ap, rt_entry_t *);
    rt_entry_dump(stream, rtentry);
    break;
  }
  return 0;
}
#endif /* NETWORK_DEBUG */

static inline void ___network_debug(const char * msg, ...)
{
#ifdef NETWORK_DEBUG
  va_list ap;

  va_start(ap, msg);
  stream_printf(gdsout, "NET_DBG::");
  str_format_for_each(gdsout, _network_debug_for_each, &ap, msg);
  stream_flush(gdsout);
  va_end(ap);
#endif /* NETWORK_DEBUG */
}

/////////////////////////////////////////////////////////////////////
//
// NETWORK PHYSICAL MESSAGE PROPAGATION FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ _thread_set_simulator ]------------------------------------
/**
 * Set the current thread's simulator context. This function
 * currently supports a single thread.
 */
static inline void _thread_set_simulator(simulator_t * sim)
{
  _thread_sim= sim;
}

// -----[ thread_set_simulator ]-------------------------------------
void thread_set_simulator(simulator_t * sim)
{
  _thread_set_simulator(sim);
}

// -----[ _thread_get_simulator ]------------------------------------
/**
 * Return the current thread's simulator context. This function
 * currently supports a single thread.
 */
static inline simulator_t * _thread_get_simulator()
{
  assert(_thread_sim != NULL);
  return _thread_sim;
}

// -----[ _network_send_callback ]-------------------------------------
/**
 * This function is used to receive messages "from the wire". The
 * function handles an event from the simulator. When such an event is
 * received, the event's message is delivered to the event's network
 * interface.
 */
static int _network_send_callback(simulator_t * sim,
				  void * ctx)
{
  net_send_ctx_t * send_ctx= (net_send_ctx_t *) ctx;
  net_error_t error;

  // Deliver message to the destination interface
  _thread_set_simulator(sim);
  assert(send_ctx->dst_iface != NULL);
  error= net_iface_recv(send_ctx->dst_iface, send_ctx->msg);
  _thread_set_simulator(NULL);

  // Free the message context. The message MUST be freed by
  // node_recv_msg() if the message has been delivered or in case
  // the message cannot be forwarded.
  FREE(ctx);

  return error/*(error == ESUCCESS)?0:-1*/;
}

// -----[ _network_send_ctx_dump ]-----------------------------------
/**
 * Callback function used to dump the content of a message event. See
 * also 'simulator_dump_events' (sim/simulator.c).
 */
static void _network_send_ctx_dump(gds_stream_t * stream, void * ctx)
{
  net_send_ctx_t * send_ctx= (net_send_ctx_t *) ctx;

  stream_printf(stream, "net-msg n-h:");
  node_dump_id(stream, send_ctx->dst_iface->owner);
  stream_printf(stream, " [");
  message_dump(stream, send_ctx->msg);
  stream_printf(stream, "]");
}

// -----[ _network_send_ctx_create ]---------------------------------
/**
 * This function creates a message context to be pushed on the
 * simulator's events queue. The context contains the message and the
 * destination interface.
 */
static inline net_send_ctx_t *
_network_send_ctx_create(net_iface_t * dst_iface, net_msg_t * msg)
{
  net_send_ctx_t * send_ctx=
    (net_send_ctx_t *) MALLOC(sizeof(net_send_ctx_t));
  
  send_ctx->dst_iface= dst_iface;
  send_ctx->msg= msg;
  return send_ctx;
}

// -----[ _network_send_ctx_destroy ]--------------------------------
/**
 * This function is used by the simulator to free events which will
 * not be processed.
 */
static void _network_send_ctx_destroy(void * ctx)
{
  net_send_ctx_t * send_ctx= (net_send_ctx_t *) ctx;
  message_destroy(&send_ctx->msg);
  FREE(send_ctx);
}

static sim_event_ops_t _network_send_ops= {
  .callback= _network_send_callback,
  .destroy = _network_send_ctx_destroy,
  .dump    = _network_send_ctx_dump,
};

// -----[ network_drop ]----------------------------------------------
/**
 * This function is used to drop a message. The function is used when
 * a message is sent to an interface that is disabled or not
 * connected.
 *
 * The function can write an optional error message on the standard
 * output stream.
 */
void network_drop(net_msg_t * msg, net_error_t error,
		  const char * reason, ...)
{
  ___network_debug("network_drop\n");
  va_list ap;
  va_start(ap, reason);

  ip_opt_hook_msg_error(msg, error);

#ifdef NETWORK_DEBUG
  stream_printf(gdserr, "*** \033[31;1mMESSAGE DROPPED\033[0m ***\n");
  stream_printf(gdserr, "message: ");
  message_dump(gdserr, msg);
  stream_printf(gdserr, "\nreason : ");
  stream_vprintf(gdserr, reason, ap);
  stream_printf(gdserr, "\n");
#endif /* NETWORK_DEBUG */

  message_destroy(&msg);
  va_end(ap);
}

// -----[ network_send ]---------------------------------------------
/**
 * This function is used to send a message "on the wire" between two
 * interfaces or between an interface and a subnet. The function
 * pushes the message to the simulator's scheduler.
 */
void network_send(net_iface_t * dst_iface, net_msg_t * msg)
{
  assert(!sim_post_event(_thread_get_simulator(),
			 &_network_send_ops,
			 _network_send_ctx_create(dst_iface, msg),
			 dst_iface->phys.delay, SIM_TIME_REL));
}

// -----[ network_get_simulator ]------------------------------------
simulator_t * network_get_simulator(network_t * network)
{
  if (network->sim == NULL)
    network->sim= sim_create(sim_get_default_scheduler());
  return network->sim;
}


/////////////////////////////////////////////////////////////////////
//
// NETWORK TOPOLOGY MANAGEMENT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ node_add_tunnel ]------------------------------------------
/**
 * Add a tunnel interface on this node.
 *
 * Arguments:
 *   node
 *   tunnel remote end-point
 *   tunnel identifier (local interface address)
 *   outgoing interface (optional)
 *   source address (optional)
 */
net_error_t node_add_tunnel(net_node_t * node,
			    net_addr_t dst_point,
			    net_addr_t addr,
			    net_iface_id_t * oif_id,
			    net_addr_t src_addr)
{
  net_iface_t * oif= NULL;
  net_iface_t * tunnel;
  net_error_t error;

  // If an outgoing interface is specified, check that it exists
  if (oif_id != NULL) {
    oif= node_find_iface(node, *oif_id);
    if (oif == NULL)
      return ENET_IFACE_UNKNOWN;
  }

  // Create tunnel
  error= ipip_link_create(node, dst_point, addr, oif, src_addr, &tunnel);
  if (error != ESUCCESS)
    return error;

  // Add interface (interface is destroyed by node_add_iface2()
  // in case of error)
  return node_add_iface2(node, tunnel);
}

// -----[ node_del_tunnel ]------------------------------------------
/**
 *
 */
int node_del_tunnel(net_node_t * node, net_addr_t addr)
{
  return EUNSUPPORTED;
}

// -----[ node_ipip_enable ]-----------------------------------------
/**
 *
 */
int node_ipip_enable(net_node_t * node)
{
  return node_register_protocol(node, NET_PROTOCOL_IPIP, node);
}

// ----- node_links_for_each ----------------------------------------
int node_links_for_each(net_node_t * node, gds_array_foreach_f foreach,
			void * ctx)
{
  return _array_for_each((array_t *) node->ifaces, foreach, ctx);
}


/////////////////////////////////////////////////////////////////////
//
// IP MESSAGE HANDLING
//
/////////////////////////////////////////////////////////////////////

static inline
net_error_t _node_ip_input(net_node_t * node,
			   net_iface_t * iif,
			   net_iface_t * lif,
			   net_msg_t * msg);

// -----[ _node_error_dump ]-----------------------------------------
static inline void _node_error_dump(net_node_t * node, net_error_t error)
{
  gds_stream_t * syslog= node_syslog(node);

  stream_printf(syslog, "@");
  node_dump_id(syslog, node);
  stream_printf(syslog, ": ");
  network_perror(syslog, error);
  stream_printf(syslog, "\n");
}

// -----[ _node_ip_fwd_error ]---------------------------------------
/**
 * This function is responsible for handling errors that occur during
 * the processing of IP messages. An error message is displayed
 * (delegated to _node_error_dump()).
 *
 * If the IP message that caused the error is not ICMP and if an ICMP
 * error message code is provided, an ICMP error message is sent to
 * the source.
 */
static inline net_error_t
_node_ip_fwd_error(net_node_t * node,
		   net_msg_t * msg,
		   net_error_t error,
		   uint8_t icmp_error)
{
  _node_error_dump(node, error);

  ___network_debug("node_ip_fwd_error node=%n error=%e\n", node, error);

  ip_opt_hook_msg_error(msg, error);

  if ((icmp_error != 0) && !is_icmp_error(msg)) {
    icmp_send_error(node, NET_ADDR_ANY, msg->src_addr,
		    icmp_error, _thread_get_simulator());
  }

  network_drop(msg, error, "forwarding error \"%s\"", network_strerror(error));
  return error;
}

// -----[ _node_ip_process_msg ]-------------------------------------
/**
 * This function is responsible for handling messages that are
 * addressed locally.
 *
 * If there is no protocol handler for the message, an ICMP error
 * with type "protocol-unreachable" is sent to the source.
 */
static inline net_error_t
_node_ip_process_msg(net_node_t * node, net_msg_t * msg)
{
  net_protocol_t * proto;
  net_error_t error;

  ___network_debug("node_ip_process_msg node=%n\n", node);

  // Find protocol (based on message field)
  proto= protocols_get(node->protocols, msg->protocol);
  if (proto == NULL)
    return _node_ip_fwd_error(node, msg,
			      ENET_PROTO_UNREACH,
			      ICMP_ERROR_PROTO_UNREACH);

  // Call protocol handler
  error= protocol_recv(proto, msg, _thread_get_simulator());

  // Free only the message. Dealing with the payload was the
  // responsability of the protocol handler.
  msg->payload= NULL;
  message_destroy(&msg);
  return error;
}

// -----[ _node_ip_input ]-------------------------------------------
static inline
net_error_t _node_ip_input(net_node_t * node, net_iface_t * iif,
			   net_iface_t * lif, net_msg_t * msg)
{
  net_error_t error;

  if (iif == lif) {

  ___network_debug("node_ip_input [process] node=%n, lif=%i\n", node, lif);

    // Process ICMP options
    error= ip_opt_hook_msg_rcvd(node, iif, msg);
    if (error != ESUCCESS)
      return error;

    return _node_ip_process_msg(node, msg);

  } else {

    ___network_debug("node_ip_input [loopback] node=%n, lif=%i\n", node, lif);

    return net_iface_recv(lif, msg);

  }
  return ESUCCESS;
}

// -----[ _node_ip_output ]------------------------------------------
/**
 * This function forwards a message through a specific output
 * interface of one node.
 *
 * \param node      is the source node.
 * \param iif       is the interface that initially received the
 *                  message
 *                  (NULL if the message is not being forwarded).
 * \param rtentries is the set of routing entries (and outgoing
 *                  interfaces) used to forward the message.
 * \param msg       is the message to be sent.
 * \retval ESUCCESS in case of success, a negative error code
 *         otherwize.
 *
 * Note: if the message cannot be sent through the outgoing interface,
 *       then it is the responsibility of this function (_node_ip_output)
 *       to destroy the message !!!
 */
static inline net_error_t
_node_ip_output(net_node_t * node, net_iface_t * iif,
		const rt_entries_t * rtentries,
		net_msg_t * msg)
{
  const rt_entry_t * rtentry;
  net_addr_t dst= msg->dst_addr;
  net_addr_t l2_addr;
  net_error_t error;

  ___network_debug("node_ip_output from:%n msg:%m\n", node, msg);

  // Default is to use entry 0
  rtentry= rt_entries_get_at(rtentries, 0);

  // Process RT entries (in case of ECMP)
  error= ip_opt_hook_msg_ecmp(node, msg, &rtentries);
  if (error != ESUCCESS)
    return error;

  // Recursive lookup ? resolving the real outgoing interface
  // is done for protocols such as BGP where the next-hop is
  // not necessarily adjacent and requires an IGP/static route
  if (rtentry->oif == NULL) {
    ___network_debug("recursive lookup\n");
    dst= rtentry->gateway;
    rtentries= node_rt_lookup(node, dst);
    if (rtentries == NULL)
      return _node_ip_fwd_error(node, msg, ENET_HOST_UNREACH, 0);
    // Default is to use entry 0
    rt_entry_t * next_rtentry= rt_entries_get_at(rtentries, 0);
    if (rtentry == next_rtentry)
      return _node_ip_fwd_error(node, msg, ENET_HOST_UNREACH, 0);
    rtentry= next_rtentry;
  }

  // Check that the outgoing interface is different from the
  // incoming interface. A normal router should send an ICMP Redirect
  // if it is the first hop. We don't handle this case in C-BGP.
  // Note: the following code has been disabled as it causes a different
  //       behaviour in record-route when used over a 1-hop loop.
  //       The expected behaviour is to receive a TTL-expired error
  //       unless the --check-loop option is enabled. In the later case
  //       a ENET_FWD_LOOP error is returned.
  /*if (iif == rtentry->oif)
    return _node_ip_fwd_error(node, msg, ENET_FWD_LOOP,
    ICMP_ERROR_NET_UNREACH);*/

  l2_addr= rtentry->gateway;

  // Fix source address (if not already set)
  if (msg->src_addr == NET_ADDR_ANY)
    msg->src_addr= net_iface_src_address(rtentry->oif);

  if (rtentry->oif->type == NET_IFACE_PTMP) {
    // For point-to-multipoint interfaces, need to get identifier of
    // next-hop (role played by ARP in the case of Ethernet)
    // In C-BGP, the layer-2 address is equal to the destination's
    // interface IP address. Note: this will not work with unnumbered
    // interfaces !
    if (l2_addr == NET_ADDR_ANY)
      l2_addr= dst;
    // should be written as
    //   l2_addr= net_iface_map_l2(msg->dst_addr);
  }

  // Process ICMP options
  error= ip_opt_hook_msg_out(node, rtentry->oif, msg);
  if (error != ESUCCESS)
    return error;

  // Forward along this link...
  ___network_debug("node_ip_output oif:%i\n", rtentry->oif);
  error= net_iface_send(rtentry->oif, l2_addr, msg);

  // If the message could not be sent through that interface,
  // i.e. an error code is received, then the message is destroyed.
  if (error != ESUCCESS) {
    if (error == ENET_HOST_UNREACH) {
      network_drop(msg, error, "message could not be output (%s)",
		   network_strerror(error));
      return error;
    } else
      network_drop(msg, error, "message could not be output (%s)",
		   network_strerror(error));
  }

  // Even if the message was sent on the link but the link was broken,
  // a success is returned as the node can't figure that the message
  // was not delivered by the link.
  return ESUCCESS;
}

// -----[ node_recv_msg ]--------------------------------------------
/**
 * This function handles a message received at this node. If the node
 * is the destination, the message is delivered locally. Otherwize,
 * the function looks up if it has a route towards the destination
 * and, if so, the function forwards the message to the next-hop.
 *
 * Important: this function (or any of its delegates) is responsible
 * for freeing the message in case it is delivered or in case it can
 * not be forwarded.
 *
 * Note: the function also decreases the TTL of the message. If the
 * message is not delivered locally and if the TTL is 0, the message
 * is discarded.
 */
net_error_t node_recv_msg(net_node_t * node,
			  net_iface_t * iif,
			  net_msg_t * msg)
{
  const rt_entries_t * rtentries= NULL;
  net_iface_t * lif;
  net_error_t error;

  ___network_debug("node_recv_msg node:%n msg:%m iif:%i\n", node, msg, iif);

  // Incoming interface must be fixed
  assert(iif != NULL);

  // A node should never receive an IP datagram with a TTL of 0
  assert(msg->ttl > 0);

  // Process ICMP options
  error= ip_opt_hook_msg_in(node, iif, msg, &rtentries);
  if (error != ESUCCESS)
    return error;

  /********************
   * Local delivery ? *
   ********************/

  // Check list of interface addresses to see if the packet
  // is for this node.
  if (rtentries == NULL) {
    lif= node_has_address(node, msg->dst_addr);
    if (lif != NULL)
      return _node_ip_input(node, iif, lif, msg);
  }

  /**********************
   * IP Forwarding part *
   **********************/

  // Decrement TTL (if TTL is less than or equal to 1,
  // discard and raise ICMP time exceeded message)
  if (msg->ttl <= 1) {
    /*return*/ _node_ip_fwd_error(node, msg,
				  ENET_TIME_EXCEEDED,
				  ICMP_ERROR_TIME_EXCEEDED);
    return ESUCCESS;
  }
  msg->ttl--;

  // Find route to destination (if no route is found,
  // discard and raise ICMP host unreachable message)
  if (rtentries == NULL) {
    rtentries= node_rt_lookup(node, msg->dst_addr);
    if (rtentries == NULL) {
      /*return*/ _node_ip_fwd_error(node, msg,
				ENET_HOST_UNREACH,
				ICMP_ERROR_HOST_UNREACH);
      return ESUCCESS;
    }
  }

  // DEBUG order BGP
  // Check if a BGP message is forwarded based on a BGP route
  // and display a warning if it is the case...
  /*if (msg->protocol == NET_PROTOCOL_BGP) {
    const rt_info_t * rtinfo= node_rt_lookup2(node, msg->dst_addr);
    if (rtinfo != NULL) {
      if (rtinfo->type == NET_ROUTE_BGP) {
	stream_printf(gdserr, "Warning: BGP message routed through a BGP route\n");
	stream_printf(gdserr, "  node = ");
	node_dump_id(gdserr, node);
	stream_printf(gdserr, "\n");
	stream_printf(gdserr, "  msg  = ");
	message_dump(gdserr, msg);
	stream_printf(gdserr, "\n");
	ip_dest_t dst;
	int pfx_len;
	for (pfx_len= 32; pfx_len >= 0; pfx_len--) {
	  dst= net_dest_prefix(msg->dst_addr, (unsigned char) pfx_len);
	  stream_printf(gdserr, "ROUTES TOWARDS ");
	  ip_prefix_dump(gdserr, dst.prefix);
	  stream_printf(gdserr, "\n");
	  node_rt_dump(gdserr, node, dst);
	}
      }
    }
    }*/
  
  return _node_ip_output(node, iif, rtentries, msg);
}

// -----[ node_send ]------------------------------------------------
net_error_t node_send(net_node_t * node, net_msg_t * msg,
		      const rt_entries_t * rtentries,
		      simulator_t * sim)
{
  ___network_debug("node_send node=%n msg=%m\n", node, msg);

  net_error_t error;
  net_iface_t * lif;

  if (sim != NULL)
    _thread_set_simulator(sim);

  // Process IP options
  error= ip_opt_hook_msg_sent(node, msg, &rtentries);
  if (error != ESUCCESS) {
    network_drop(msg, error, "ip opt hook reported an error (%s)",
		 network_strerror(error));
    return error;
  }

  if (rtentries != NULL)
    ___network_debug("SEND rtentry=%r\n", rtentries->data[0]);

  // Check source address (if specified, must belong to node)
  //
  // Note: the following verification is currently disabled as it is
  //       required to be able to send messages with a non-local
  //       source address in the case of ECMP exploration (through
  //       record-route). Enabling the verification breaks ECMP
  //       record-route. A possible workaround is to optionaly
  //       disable this verification.
  /*if ((msg->src_addr != IP_ADDR_ANY) &&
      (node_has_address(node, msg->src_addr) == NULL))
      return ENET_IFACE_UNKNOWN;*/

  // Local delivery ?
  if (rtentries == NULL) {
    lif= node_has_address(node, msg->dst_addr);
    if (lif != NULL) {
      // Set source address (if unspecified)
      if (msg->src_addr == NET_ADDR_ANY)
	msg->src_addr= lif->addr;
      return _node_ip_input(node, lif, lif, msg);
    }
  }
  
  // Find outgoing interface & next-hop
  if (rtentries == NULL) {
    rtentries= node_rt_lookup(node, msg->dst_addr);
    if (rtentries == NULL)
      return _node_ip_fwd_error(node, msg, ENET_HOST_UNREACH, 0);

    // DEBUG BGP order
    /*if (msg->protocol == NET_PROTOCOL_BGP) {
      const rt_info_t * rtinfo= node_rt_lookup2(node, msg->dst_addr);
      if (rtinfo != NULL) {
	stream_printf(gdserr, "Info: BGP message sent through ");
	net_route_type_dump(gdserr, rtinfo->type);
	stream_printf(gdserr, "\n");
	stream_printf(gdserr, "  node = ");
	node_dump_id(gdserr, node);
	stream_printf(gdserr, "\n");
	stream_printf(gdserr, "  msg  = ");
	message_dump(gdserr, msg);
	stream_printf(gdserr, "\n");
	stream_printf(gdserr, "  route=");
	rt_info_dump(gdserr, rtinfo);
	stream_printf(gdserr, "\n");
      }
      }*/
    // DEBUG

  }

  return _node_ip_output(node, NULL, rtentries, msg);
}

// -----[ node_send_msg ]--------------------------------------------
net_error_t node_send_msg(net_node_t * node,
			  net_addr_t src_addr,
			  net_addr_t dst_addr,
			  net_protocol_id_t proto,
			  uint8_t ttl,
			  void * payload,
			  FPayLoadDestroy f_destroy,
			  ip_opt_t * opts,
			  simulator_t * sim)
{
  net_msg_t * msg;

  ___network_debug("node_send_msg from:%n src:%a dst:%a\n",
		   node, src_addr, dst_addr);

  if (ttl == 0)
    ttl= 255;

  // Build "IP" message
  msg= message_create(src_addr, dst_addr, proto, ttl, payload, f_destroy);
  message_set_options(msg, opts);
  
  return node_send(node, msg, NULL, sim);
}


/////////////////////////////////////////////////////////////////////
//
// NETWORK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- network_nodes_destroy --------------------------------------
void network_nodes_destroy(void ** ppItem)
{
  node_destroy((net_node_t **) ppItem);
}

// ----- network_create ---------------------------------------------
/**
 *
 */
network_t * network_create()
{
  network_t * network= (network_t *) MALLOC(sizeof(network_t));
  
  network->domains= igp_domains_create();
  network->nodes= trie_create(network_nodes_destroy);
  network->subnets= subnets_create();
  network->sim= NULL;
  return network;
}

// ----- network_destroy --------------------------------------------
/**
 *
 */
void network_destroy(network_t ** network_ref)
{
  network_t * network;

  if (*network_ref != NULL) {
    network= *network_ref;
    igp_domains_destroy(&network->domains);
    trie_destroy(&network->nodes);
    subnets_destroy(&network->subnets);
    if (network->sim != NULL)
      sim_destroy(&network->sim);
    FREE(*network_ref);
    *network_ref= NULL;
  }
}

// -----[ network_get_default ]--------------------------------------
/**
 * Get the default network.
 */
network_t * network_get_default()
{
  assert(_default_network != NULL);
  return _default_network;
}

// ----- network_add_node -------------------------------------------
/**
 * Add a node to the given network.
 */
net_error_t network_add_node(network_t * network, net_node_t * node)
{
   // Check that node does not already exist
  if (network_find_node(network, node->rid) != NULL)
    return ENET_NODE_DUPLICATE;

  node->network= network;
  if (trie_insert(network->nodes, node->rid, 32, node, 0) != 0)
    return EUNEXPECTED;
  return ESUCCESS;
}

// ----- network_add_subnet -------------------------------------------
/**
 * Add a subnet to the given network.
 */
net_error_t network_add_subnet(network_t * network, net_subnet_t * subnet)
{
  // Check that subnet does not already exist
  if (network_find_subnet(network, subnet->prefix) != NULL)
    return ENET_SUBNET_DUPLICATE;

  if (subnets_add(network->subnets, subnet) < 0)
    return EUNEXPECTED;
  return ESUCCESS;
}

// -----[ network_add_igp_domain ]-----------------------------------
net_error_t network_add_igp_domain(network_t * network,
				   igp_domain_t * domain)
{
  if (network_find_igp_domain(network, domain->id) != NULL)
    return ENET_IGP_DOMAIN_DUPLICATE;
  
  if (igp_domains_add(network->domains, domain) < 0)
    return EUNEXPECTED;
  return ESUCCESS;
}

// ----- network_find_node ------------------------------------------
/**
 *
 */
net_node_t * network_find_node(network_t * network, net_addr_t addr)
{
  return (net_node_t *) trie_find_exact(network->nodes, addr, 32);
}

// -----[ network_find_subnet ]--------------------------------------
net_subnet_t * network_find_subnet(network_t * network, ip_pfx_t prefix)
{ 
  return subnets_find(network->subnets, prefix);
}

// -----[ network_find_igp_domain ]----------------------------------
igp_domain_t * network_find_igp_domain(network_t * network,
				       uint16_t id)
{
  return igp_domains_find(network->domains, id);
}

// -----[ network_to_file ]------------------------------------------
int network_to_file(gds_stream_t * stream, network_t * network)
{
  gds_enum_t * nodes= trie_get_enum(network->nodes);
  net_node_t * node;

  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    node_dump(stream, node);
    stream_printf(stream, "\n");
  }
  enum_destroy(&nodes);
  return 0;
}

// -----[ network_dump_subnets ]-------------------------------------
void network_dump_subnets(gds_stream_t * stream, network_t * network)
{
  gds_enum_t * subnets= _array_get_enum((array_t*) network->subnets);
  net_subnet_t * subnet;

  while (enum_has_next(subnets)) {
    subnet= *((net_subnet_t **) enum_get_next(subnets));
    ip_prefix_dump(stream, subnet->prefix);
    stream_printf(stream, "\n");
  }
  enum_destroy(&subnets);
}

// -----[ network_ifaces_load_clear ]----------------------------------
/**
 * Clear the load of all links in the topology.
 */
void network_ifaces_load_clear(network_t * network)
{
  gds_enum_t * nodes= trie_get_enum(network->nodes);
  net_node_t * node;
  
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    node_ifaces_load_clear(node);
  }
  enum_destroy(&nodes);
}

// -----[ network_links_save ]---------------------------------------
/**
 * Save the load of all links in the topology.
 */
int network_links_save(gds_stream_t * stream, network_t * network)
{
  gds_enum_t * nodes= trie_get_enum(network->nodes);
  net_node_t * node;

  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    node_links_save(stream, node);
  }
  enum_destroy(&nodes);
  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _network_init ]--------------------------------------------
void _network_init()
{
  _default_network= network_create();
}

// -----[ _network_done ]--------------------------------------------
void _network_done()
{
  if (_default_network != NULL)
    network_destroy(&_default_network);
}

