// ==================================================================
// @(#)node.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 08/08/2005
// $Id: node.h,v 1.14 2009-08-31 09:48:28 bqu Exp $
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
 * Provide functions for handling network nodes.
 *
 * The following code demonstrates how a node is created.
 * \code
 * net_node_t * node;
 * int result;
 * result= node_create(IPV4(10,0,0,1), &node, NODE_OPTIONS_LOOPBACK);
 * if (result != ESUCCESS)
 *   exit(EXIT_FAILURE);
 * // Use node...
 * node_destroy(&node);
 * \endcode
 */

#ifndef __NET_NODE_H__
#define __NET_NODE_H__

#include <net/icmp.h>
#include <net/iface.h>
#include <net/net_types.h>
#include <net/protocol.h>
#include <net/traffic/stats.h>

// ----- Netflow load options -----
#define NET_NODE_NETFLOW_OPTIONS_SUMMARY 0x01
#define NET_NODE_NETFLOW_OPTIONS_DETAILS 0x02

// ----- Node creation options -----
#define NODE_OPTIONS_LOOPBACK 0x01

#ifdef __cplusplus
extern "C" {
#endif

  // ----- node_create ----------------------------------------------
  /**
   * Create a network node.
   * \param addr is  the node's identifier (an IP address).
   * \param node_ref is a pointer to the new node.
   * \param options  is a set of creation options.
   * \retval an error code.
   */
  net_error_t node_create(net_addr_t addr, net_node_t ** node_ref,
			  int options);

  // ----- node_destroy ---------------------------------------------
  /**
   * Destroy a network node.
   *
   * \param node_ref is a pointer to the node to be destroyed.
   */
  void node_destroy(net_node_t ** node_ref);
  
  // ----- node_set_name --------------------------------------------
  /**
   * Set the name of a node.
   *
   * \param node is the node.
   * \param name is the new name of the node. If \p name is NULL, the
   *   previous name is removed.
   */
  void node_set_name(net_node_t * node, const char * name);

  // ----- node_get_name --------------------------------------------
  /**
   * Get the name of a node.
   *
   * \param node is the node.
   * \retval the name of the node if is exists,
   *   or NULL otherwise.
   */
  char * node_get_name(net_node_t * node);

  // -----[ node_dump_id ]-------------------------------------------
  void node_dump_id(gds_stream_t * stream, net_node_t * node);
  // ----- node_dump ------------------------------------------------
  void node_dump(gds_stream_t * stream, net_node_t * node);
  // ----- node_info ------------------------------------------------
  void node_info(gds_stream_t * stream, net_node_t * node);
  
  
  ///////////////////////////////////////////////////////////////////
  // NODE INTERFACES FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ node_add_iface ]-----------------------------------------
  int node_add_iface(net_node_t * node, net_iface_id_t tIfaceID,
		     net_iface_type_t tIfaceType);
  // -----[ node_add_iface2 ]----------------------------------------
  int node_add_iface2(net_node_t * node, net_iface_t * iface);
  // -----[ node_find_iface ]----------------------------------------
  net_iface_t * node_find_iface(net_node_t * node, net_iface_id_t tIfaceID);
  // -----[ node_find_iface_to ]-------------------------------------
  net_iface_t * node_find_iface_to(net_node_t * node, net_elem_t * to);
  // -----[ node_ifaces_load_clear ]---------------------------------
  void node_ifaces_load_clear(net_node_t * node);


  ///////////////////////////////////////////////////////////////////
  // NODE LINKS FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- node_links_dump ------------------------------------------
  void node_links_dump(gds_stream_t * stream, net_node_t * node);
  // ----- node_links_save ------------------------------------------
  void node_links_save(gds_stream_t * stream, net_node_t * node);

  // -----[ node_has_address ]---------------------------------------
  net_iface_t * node_has_address(net_node_t * node, net_addr_t addr);
  // -----[ node_has_prefix ]----------------------------------------
  net_iface_t * node_has_prefix(net_node_t * node, ip_pfx_t pfx);
  // ----- node_addresses_for_each ----------------------------------
  int node_addresses_for_each(net_node_t * node,
			      gds_array_foreach_f foreach,
			      void * ctx);
  // ----- node_addresses_dump --------------------------------------
  void node_addresses_dump(gds_stream_t * stream, net_node_t * node);
  // -----[ node_ifaces_dump ]---------------------------------------
  void node_ifaces_dump(gds_stream_t * stream, net_node_t * node);
  
  
  ///////////////////////////////////////////////////////////////////
  // ROUTING TABLE FUNCTIONS
  ///////////////////////////////////////////////////////////////////
  
  // ----- node_rt_add_route ----------------------------------------
  int node_rt_add_route(net_node_t * node, ip_pfx_t pfx,
			net_iface_id_t oif_id, net_addr_t gateway,
			uint32_t weight, uint8_t type);
  // ----- node_rt_add_route_link -----------------------------------
  int node_rt_add_route_link(net_node_t * node, ip_pfx_t pfx,
			     net_iface_t * oif, net_addr_t gateway,
			     uint32_t weight, uint8_t type);
  // ----- node_rt_del_route ----------------------------------------
  int node_rt_del_route(net_node_t * node, ip_pfx_t * pfx,
			net_iface_id_t * oif_id, net_addr_t * next_hop,
			uint8_t type);
  // ----- node_rt_dump ---------------------------------------------
  void node_rt_dump(gds_stream_t * stream, net_node_t * node,
		    ip_dest_t dest);
  // -----[ node_rt_lookup ]-----------------------------------------
  const rt_entries_t * node_rt_lookup(net_node_t * self,
				      net_addr_t dst_addr);
  const rt_info_t * node_rt_lookup2(net_node_t * node,
				    net_addr_t dst_addr);

  
  ///////////////////////////////////////////////////////////////////
  // PROTOCOL FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // ----- node_register_protocol -----------------------------------
  int node_register_protocol(net_node_t * node, net_protocol_id_t id,
			     void * handler);
  // -----[ node_get_protocol ]--------------------------------------
  net_protocol_t * node_get_protocol(net_node_t * node,
				     net_protocol_id_t id);


  ///////////////////////////////////////////////////////////////////
  // IGP DOMAIN MEMBERSHIP FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ node_igp_domain_add ]------------------------------------
  int node_igp_domain_add(net_node_t * node, uint16_t id);

  // -----[ node_belongs_to_igp_domain ]-----------------------------
  int node_belongs_to_igp_domain(net_node_t * node, uint16_t id);


  ///////////////////////////////////////////////////////////////////
  // TRAFFIC LOAD FUNCTIONS
  ///////////////////////////////////////////////////////////////////

  // -----[ node_load_flow ]-------------------------------------------
  /**
   * Load a flow of traffic from a node.
   *
   * \param node
   *    is the node where the flow is to be loaded.
   * \param src_addr
   *   is the flow source address. This is seldom used. It only makes
   *   sense if the node uses different forwarding tables depending
   *   on the flow source address.
   * \param dst_addr
   *   is the flow destination. It is used to find the forwarding
   *   path.
   * \param bytes
   *   is the the number of bytes carried in this flow.
   * \param stats
   *   is an optional statistics object (can be NULL).
   * \param trace_ref
   *   is an optional pointer to the resulting IP trace (can be NULL).
   * \retval an error code.
   */
  int node_load_flow(net_node_t * node, net_addr_t src_addr,
		     net_addr_t dst_addr, unsigned int bytes,
		     flow_stats_t * stats, ip_trace_t ** trace_ref,
		     ip_opt_t * opts);

  // -----[ node_load_netflow ]--------------------------------------
  int node_load_netflow(net_node_t * node, const char * file_name,
			uint8_t options, flow_stats_t * stats);


  ///////////////////////////////////////////////////////////////////
  // SYSLOG
  ///////////////////////////////////////////////////////////////////

  // -----[ node_syslog ]--------------------------------------------
  gds_stream_t * node_syslog(net_node_t * self);
  // -----[ node_syslog_set_enabled ]--------------------------------
  void node_syslog_set_enabled(net_node_t * self, int enabled);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_NODE_H__ */
