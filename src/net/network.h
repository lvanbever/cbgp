// ==================================================================
// @(#)network.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 4/07/2003
// $Id: network.h,v 1.33 2009-03-24 16:20:17 bqu Exp $
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

/**
 * \file
 * Provide functions for handling a complete network of nodes and
 * the transmission of messages between them.
 */

#ifndef __NET_NETWORK_H__
#define __NET_NETWORK_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/stream.h>
#include <libgds/trie.h>
#include <libgds/types.h>

#include <net/iface.h>
#include <net/icmp_options.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/net_path.h>
#include <net/link.h>
#include <net/routing.h>
#include <sim/simulator.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ thread_set_simulator ]-----------------------------------
  void thread_set_simulator(simulator_t * sim);

  // -----[ network_drop ]-------------------------------------------
  void network_drop(net_msg_t * msg, net_error_t error,
		    const char * reason, ...);
  // -----[ network_send ]-------------------------------------------
  void network_send(net_iface_t * dst_iface, net_msg_t * msg);

  // -----[ network_get_simulator ]----------------------------------
  simulator_t * network_get_simulator(network_t * network);
  // ----- node_post_event ------------------------------------------
  int node_post_event(net_node_t * node);
  // ----- node_links_for_each --------------------------------------
  int node_links_for_each(net_node_t * node, gds_array_foreach_f foreach,
			  void * ctx);

  ///////////////////////////////////////////////////////////////////
  // IP MESSAGE HANDLING
  ///////////////////////////////////////////////////////////////////
  
  // -----[ node_send ]------------------------------------------------
  /**
   * Send a message from a node to the message's destination address.
   * If optional routing entries are provided, the message will be
   * forwarded according to them (see _node_ip_output for more
   * details about this).
   *
   * \param node      is the source node.
   * \param msg       is the message to be sent.
   * \param rtentries are optional routing entries that must be used
   *                  to forward this packet. The normal behavior is
   *                  to have rtentries == NULL.
   * \param sim       is the simulator queue to be used.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   *
   * \todo What is the intended behaviour if the message's source
   *       address has been specified ? First, we should check that the
   *       node has an interface numbered with that address (NOT DONE).
   *       Second, we should try to forward through that interface.
   *       Shall we check that there is a valid route to the destination
   *       that goes through that interface ? What is the correct
   *       behaviour in real IP implementations ?? (ASK SBARRE)
   */
  net_error_t node_send(net_node_t * node, net_msg_t * msg,
			const rt_entries_t * rtentries,
			simulator_t * sim);

  // ----- node_send_msg --------------------------------------------
  /**
   * Build a message that is then sent from a node to a specified
   * destination address. This is just a wrapper for node_send() that
   * first build the message.
   *
   * \param self is the node from which the message will be sent.
   * \param src_addr  is the message's source address.
   * \param dst_addr  is the message's destination address.
   * \param proto     is the message's protocol field.
   * \param ttl       is the message's initial time-to-live.
   * \param payload   is the message's payload.
   * \param f_destroy is the message's destructor.
   * \param opts      is the message's IP options.
   * \param sim       is the simulator queue to be used.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   *
   * \todo Check that the message's destructor still makes sense. In
   *       the latest changes, the destruction of messages was stored
   *       in protocol specific data structures, not in messages.
   */
  net_error_t node_send_msg(net_node_t * self,
			    net_addr_t src_addr,
			    net_addr_t dst_addr,
			    net_protocol_id_t proto,
			    uint8_t ttl,
			    void * payload,
			    FPayLoadDestroy f_destroy,
			    ip_opt_t * opts,
			    simulator_t * sim);

  // ----- node_recv_msg --------------------------------------------
  net_error_t node_recv_msg(net_node_t * self,
			    net_iface_t * iif,
			    net_msg_t * msg);

  // ----- node_ipip_enable -----------------------------------------
  int node_ipip_enable(net_node_t * node);
  // ----- node_add_tunnel ------------------------------------------
  int node_add_tunnel(net_node_t * node, net_addr_t tDstPoint,
		      net_addr_t addr, net_iface_id_t * ptOutIfaceID,
		      net_addr_t src_addr);
  
  ///////////////////////////////////////////////////////////////////
  // NETWORK METHODS
  ///////////////////////////////////////////////////////////////////
  
  // ----- network_create -------------------------------------------
  /**
   * Create a new network.
   */
  network_t * network_create();

  // ----- network_destroy ------------------------------------------
  /**
   * Destroy an existing network.
   *
   * \param network_ref is a reference to the network to be destroyed.
   */
  void network_destroy(network_t ** network_ref);

  // -----[ network_get_default ]------------------------------------
  network_t * network_get_default();

  // -----[ network_add_node ]---------------------------------------
  /**
   * Add a node to a network.
   *
   * \param network is the target network.
   * \param node    is the new member node.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   */
  int network_add_node(network_t * network, net_node_t * node);

  // -----[ network_add_subnet ]-------------------------------------
  /**
   * Add a subnet to a network.
   *
   * \param network is the target network.
   * \param subnet  is the new member subnet.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   */
  int network_add_subnet(network_t * network, net_subnet_t * subnet);

  // -----[ network_add_igp_domain ]---------------------------------
  /**
   * Add an IGP domain to a network.
   *
   * \param network is the target network.
   * \param domain  is the new member domain.
   * \retval ESUCCESS in case of success, a negative error code
   *         otherwize.
   */
  int network_add_igp_domain(network_t * network, igp_domain_t * domain);

  // -----[ network_find_node ]--------------------------------------
  /**
   * Find a node in the network, based on its RID.
   *
   * \param network is the target network.
   * \param addr    is the searched node's RID.
   * \retval the searched node, or NULL if it was not found.
   */
  net_node_t * network_find_node(network_t * network, net_addr_t addr);

  // -----[ network_find_subnet ]------------------------------------
  /**
   * Find a subnet in the network, based on its prefix.
   *
   * \param network is the target network.
   * \param prefix is the searched subnet's prefix.
   * \retval the searched subnet, or NULL if it was not found.
   */
  net_subnet_t * network_find_subnet(network_t * network, ip_pfx_t prefix);

  // -----[ network_find_igp_domain ]--------------------------------
  /**
   * Find an IGP domain in the network, based on its ID.
   *
   * \param network is the target network.
   * \param id      is the searched IGP domain's ID.
   * \retval the searched IGP domain, or NULL if it was not found.
   */
  igp_domain_t * network_find_igp_domain(network_t * network, uint16_t id);

  // ----- network_to_file ------------------------------------------
  int network_to_file(gds_stream_t * stream, network_t * network);

  
  //---- network_dump_subnets ---------------------------------------
  void network_dump_subnets(gds_stream_t * stream, network_t * network);

  // -----[ _network_init ]------------------------------------------
  /**
   * This function must be called by the library initialization
   * function.
   */
  void _network_init();

  // -----[ _network_done ]------------------------------------------
  /**
   * This function must be called by the library finalization
   * function.
   */
  void _network_done();

  ///////////////////////////////////////////////////////////////////
  // FUNCTIONS FOR GLOBAL TOPOLOGY MANAGEMENT
  ///////////////////////////////////////////////////////////////////

  // -----[ network_ifaces_load_clear ]------------------------------
  void network_ifaces_load_clear(network_t * network);
  // ----- network_links_save ---------------------------------------
  int network_links_save(gds_stream_t * stream, network_t * network);

    
#ifdef __cplusplus
}
#endif

#endif /* __NET_NETWORK_H__ */
