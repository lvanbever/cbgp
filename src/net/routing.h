// ==================================================================
// @(#)routing.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/02/2004
// $Id: routing.h,v 1.20 2009-03-24 16:24:35 bqu Exp $
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
 * Provide data structures and functions to handle a node routing and
 * forwarding table.
 */

#ifndef __NET_ROUTING_H__
#define __NET_ROUTING_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/stream.h>
#include <libgds/types.h>

#include <net/prefix.h>
#include <net/link.h>
#include <net/routing_t.h>


// -----[ rt_entry_t ]-----------------------------------------------
/**
 * Definition of a routing table entry.
 *
 * A routing table entry is composed of an outgoing network interface
 * and an optionnal gateway (IP_ADDR_ANY if unspecified). A routing
 * table entry tells how to forward an IP packet (network message) to
 * its destination.
 *
 * If the outgoing link (iface) is a point-to-point link, the gateway
 * address needs not be specified. To the contrary, the gateway
 * address is mandatory for a multi-point link such as a subnet.
 */
typedef struct {
  /** Outgoing network interface. */
  net_iface_t  * oif;
  /** Optionnal gateway (IP_ADDR_ANY if unspecified). */
  net_addr_t     gateway;
  /** Reference count (for memory management purposes). */
  unsigned int   ref_cnt;
} rt_entry_t;


// -----[ rt_info_t ]------------------------------------------------
/**
 * Definition of routing table information data structure.
 *
 * A routing table information tells how to reach a destination
 * specified as an IP prefix. The IP prefix is associated to a list
 * of routing table entries that tell exactly where to send a packet
 * to get it closer (hopefully) to this prefix.
 */
typedef struct {
  /** Destination IP prefix. */
  ip_pfx_t         prefix;
  /** Metric. */
  uint32_t         metric;
  /** Array of routing table entries. */
  rt_entries_t   * entries;
  /** How the route was learned (routing protocol). */
  net_route_type_t type;
} rt_info_t;


// -----[ rt_infos_t ]-----------------------------------------------
/** Definition of an array of routing table informations. */
typedef ptr_array_t rt_infos_t;
typedef rt_infos_t rt_info_list_t;


#include <net/rt_filter.h>

#ifdef __cplusplus
extern "C" {
#endif

  ///////////////////////////////////////////////////////////////////
  // RT ENTRY (rt_entry_t)
  ///////////////////////////////////////////////////////////////////

  rt_entry_t * rt_entry_create(net_iface_t * oif, net_addr_t gateway);
  void rt_entry_destroy(rt_entry_t ** entry_ref);
  rt_entry_t * rt_entry_add_ref(rt_entry_t * entry);
  rt_entry_t * rt_entry_copy(const rt_entry_t * entry);
  void rt_entry_dump(gds_stream_t * stream, const rt_entry_t * entry);
  int rt_entry_compare(const rt_entry_t * entry1,
		       const rt_entry_t * entry2);

  ///////////////////////////////////////////////////////////////////
  // RT ENTRIES (rt_entries_t)
  ///////////////////////////////////////////////////////////////////
  
  rt_entries_t * rt_entries_create();
  void rt_entries_destroy(rt_entries_t ** entries);
  int rt_entries_contains(const rt_entries_t * entries, rt_entry_t * entry);
  int rt_entries_add(const rt_entries_t * entries, rt_entry_t * entry);
  int rt_entries_del(const rt_entries_t * entries, rt_entry_t * entry);
  unsigned int rt_entries_size(const rt_entries_t * entries);
  rt_entry_t * rt_entries_get_at(const rt_entries_t * entries,
				 unsigned int index);
  int rt_entries_remove_at(const rt_entries_t * entries, unsigned int index);
  rt_entries_t * rt_entries_copy(const rt_entries_t * entries);
  void rt_entries_dump(gds_stream_t * stream, const rt_entries_t * entries);


  ///////////////////////////////////////////////////////////////////
  // ROUTE (rt_info_t)
  ///////////////////////////////////////////////////////////////////

  // -----[ rt_info_create ]-----------------------------------------
  rt_info_t * rt_info_create(ip_pfx_t prefix, uint32_t metric,
			     net_route_type_t type);
  // -----[ rt_info_destroy ]----------------------------------------
  void rt_info_destroy(rt_info_t ** rtinfo_ref);
  // -----[ rt_info_add_entry ]----------------------------------------
  int rt_info_add_entry(rt_info_t * rtinfo,
			net_iface_t * oif, net_addr_t gateway);
  // -----[ rt_info_set_entries ]--------------------------------------
  int rt_info_set_entries(rt_info_t * rtinfo, rt_entries_t * entries);
  // -----[ rt_info_dump ]-------------------------------------------
  void rt_info_dump(gds_stream_t * stream, const rt_info_t * rtinfo);


  ///////////////////////////////////////////////////////////////////
  // ROUTING TABLE (net_rt_t)
  ///////////////////////////////////////////////////////////////////
  
  // ----- rt_create ------------------------------------------------
  net_rt_t * rt_create();
  // ----- rt_destroy -----------------------------------------------
  void rt_destroy(net_rt_t ** rt_ref);
  // ----- rt_find_best ---------------------------------------------
  rt_info_t * rt_find_best(net_rt_t * rt, net_addr_t addr,
			   net_route_type_t type);
  // ----- rt_find_exact --------------------------------------------
  rt_info_t * rt_find_exact(net_rt_t * rt, ip_pfx_t prefix,
			    net_route_type_t type);
  // ----- rt_add_route ---------------------------------------------
  int rt_add_route(net_rt_t * rt, ip_pfx_t prefix,
		   rt_info_t * rtinfo);
  // ----- rt_del_routes --------------------------------------------
  int rt_del_routes(net_rt_t * rt, rt_filter_t * filter);
  // ----- rt_del_route ---------------------------------------------
  int rt_del_route(net_rt_t * rt, ip_pfx_t * prefix,
		   net_iface_t * oif, net_addr_t * next_hop,
		   net_route_type_t type);
  // ----- net_route_type_dump --------------------------------------
  void net_route_type_dump(gds_stream_t * stream, net_route_type_t type);
  // ----- rt_dump --------------------------------------------------
  void rt_dump(gds_stream_t * stream, net_rt_t * rt, ip_dest_t dest);
  
  // ----- rt_for_each ----------------------------------------------
  int rt_for_each(net_rt_t * rt, FRadixTreeForEach for_each,
		  void * ctx);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_ROUTING_H__ */
