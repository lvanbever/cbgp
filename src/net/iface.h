// ==================================================================
// @(#)iface.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 02/08/2003
// $Id: iface.h,v 1.6 2009-03-24 16:10:34 bqu Exp $
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
 * Provide functions for handling network interfaces.
 */

#ifndef __NET_IFACE_H__
#define __NET_IFACE_H__

#include <net/error.h>
#include <net/prefix.h>
#include <net/net_types.h>

typedef ip_pfx_t net_iface_id_t;
typedef enum {
  UNIDIR, BIDIR
} net_iface_dir_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ net_iface_new ]------------------------------------------
  /**
   * Create a new network interface.
   */
  net_iface_t * net_iface_new(net_node_t * node, net_iface_type_t type);

  // -----[ net_iface_destroy ]--------------------------------------
  /**
   * Destroy a network interface.
   */
  void net_iface_destroy(net_iface_t ** iface_ref);

  // -----[ net_iface_connect_iface ]--------------------------------
  /**
   * Connect two network interfaces together.
   */
  int net_iface_connect_iface(net_iface_t * iface, net_iface_t * dst);

  // -----[ net_iface_connect_subnet ]-------------------------------
  /**
   * Connect a network interface to a subnet.
   */
  int net_iface_connect_subnet(net_iface_t * iface, net_subnet_t * dst);

  // -----[ net_iface_disconnect ]-----------------------------------
  /**
   * Disconnect a network interface.
   */
  int net_iface_disconnect(net_iface_t * iface);

  // -----[ net_iface_dest_node ]------------------------------------
  net_node_t * net_iface_dest_node(net_iface_t * iface);
  // -----[ net_iface_dest_subnet ]----------------------------------
  net_subnet_t * net_iface_dest_subnet(net_iface_t * iface);
  // -----[ net_iface_dest_endpoint ]--------------------------------
  net_addr_t net_iface_dest_endpoint(net_iface_t * iface);

  // -----[ net_iface_has_address ]----------------------------------
  /**
   * Test if a network interface has a given IP address.
   */
  int net_iface_has_address(net_iface_t * iface, net_addr_t addr);

  // -----[ net_iface_has_prefix ]-----------------------------------
  /**
   * Test if a network interface has a given IP prefix.
   */
  int net_iface_has_prefix(net_iface_t * iface, ip_pfx_t prefix);

  // -----[ net_iface_src_address ]----------------------------------
  /**
   * Get the address of a network interface.
   */
  net_addr_t net_iface_src_address(net_iface_t * iface);

  // -----[ net_iface_dst_prefix ]-----------------------------------
  ip_pfx_t net_iface_dst_prefix(net_iface_t * iface);
  // -----[ net_iface_dst_elem ]-------------------------------------
  int net_iface_dst_elem(net_iface_t * iface, net_elem_t * elem);

  // -----[ net_iface_id ]-------------------------------------------
  net_iface_id_t net_iface_id(net_iface_t * iface);

  // -----[ net_iface_is_connected ]---------------------------------
  /**
   * Test if a network interface is connected.
   */
  int net_iface_is_connected(net_iface_t * iface);

  // -----[ net_iface_factory ]--------------------------------------
  int net_iface_factory(net_node_t * node, net_iface_id_t id,
			net_iface_type_t type, net_iface_t ** iface_ref);

  // -----[ net_iface_recv ]-----------------------------------------
  int net_iface_recv(net_iface_t * iface, net_msg_t * msg);
  // -----[ net_iface_send ]-----------------------------------------
  int net_iface_send(net_iface_t * iface, net_addr_t next_hop,
		     net_msg_t * msg);

  ///////////////////////////////////////////////////////////////////
  // ATTRIBUTES
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_get_metric ]-----------------------------------
  /**
   * Get the IGP weight of a network interface.
   */
  igp_weight_t net_iface_get_metric(net_iface_t * iface, net_tos_t tos);
  
  // -----[ net_iface_set_metric ]-----------------------------------
  /**
   * Set the IGP weight of a network interface.
   */
  int net_iface_set_metric(net_iface_t * iface, net_tos_t tos,
			   igp_weight_t weight, net_iface_dir_t eDir);
  
  // -----[ net_iface_is_enabled ]-----------------------------------
  /**
   * Test if a network interface is enabled.
   */
  int net_iface_is_enabled(net_iface_t * iface);

  // -----[ net_iface_set_enabled ]----------------------------------
  /**
   * Enable a network interface.
   */
  void net_iface_set_enabled(net_iface_t * iface, int enabled);

  // -----[ net_iface_get_delay ]------------------------------------
  /**
   * Get the delay of a network interface.
   */
  net_link_delay_t net_iface_get_delay(net_iface_t * iface);

  // -----[ net_iface_set_delay ]------------------------------------
  /**
   * Set the delay of a network interface.
   */
  int net_iface_set_delay(net_iface_t * iface, net_link_delay_t delay,
			  net_iface_dir_t dir);
  
  // -----[ net_iface_set_capacity ]-----------------------------------
  /**
   * Set the bandwidth of a network interface.
   */
  int net_iface_set_capacity(net_iface_t * iface, net_link_load_t capacity,
			     net_iface_dir_t dir);

  // -----[ net_iface_get_capacity ]---------------------------------
  /**
   * Get the bandwidth of a network interface.
   */
  net_link_load_t net_iface_get_capacity(net_iface_t * iface);

  // -----[ net_iface_set_load ]-------------------------------------
  /**
   * Set the load of a network interface.
   */
  void net_iface_set_load(net_iface_t * iface, net_link_load_t load);

  // -----[ net_iface_get_load ]-------------------------------------
  /**
   * Get the load of a network interface.
   */
  net_link_load_t net_iface_get_load(net_iface_t * iface);

  // -----[ net_iface_add_load ]-------------------------------------
  /**
   * Add load to a network interface.
   */
  void net_iface_add_load(net_iface_t * iface, net_link_load_t inc_load);


  ///////////////////////////////////////////////////////////////////
  // DUMP
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_dump_type ]------------------------------------
  void net_iface_dump_type(gds_stream_t * stream, net_iface_t * iface);
  // -----[ net_iface_dump_id ]--------------------------------------
  void net_iface_dump_id(gds_stream_t * stream, net_iface_t * iface);
  // -----[ net_iface_dump_dest ]------------------------------------
  void net_iface_dump_dest(gds_stream_t * stream, net_iface_t * iface);
  // -----[ net_iface_dump ]-----------------------------------------
  void net_iface_dump(gds_stream_t * stream, net_iface_t * iface,
		      int withDest);
  // -----[ net_iface_dumpf ]----------------------------------------
  void net_iface_dumpf(gds_stream_t * stream, net_iface_t * iface,
		       const char * format);


  ///////////////////////////////////////////////////////////////////
  // STRING TO TYPE / IDENTIFIER CONVERSION
  ///////////////////////////////////////////////////////////////////

  // -----[ net_iface_str2type ]-------------------------------------
  net_error_t net_iface_str2type(const char * str,
				 net_iface_type_t * ptr_type);
  // -----[ net_iface_type2str ]-------------------------------------
  const char * net_iface_type2str(net_iface_type_t type);
  // -----[ net_iface_str2id ]---------------------------------------
  net_error_t net_iface_str2id(const char * str,
			       net_iface_id_t * ptr_id);


  ///////////////////////////////////////////////////////////////////
  // INTERFACE IDENTIFIER UTILITIES
  ///////////////////////////////////////////////////////////////////

  static inline net_iface_id_t net_iface_id_addr(net_addr_t addr) {
    net_iface_id_t tID=
      { .network= addr,
	.mask= 32 };
    return tID;
  }

  static inline net_iface_id_t net_iface_id_pfx(net_addr_t network,
						uint8_t mask) {
    net_iface_id_t tID=
      { .network= network,
	.mask= mask };
    return tID;
  }

#ifdef __cplusplus
}
#endif

#endif /* __NET_IFACE_H__ */
