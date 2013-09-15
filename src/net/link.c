// ==================================================================
// @(#)link.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 24/02/2004
// $Id: link.c,v 1.27 2009-03-24 16:15:26 bqu Exp $
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

#include <libgds/stream.h>
#include <libgds/memory.h>

#include <net/error.h>
#include <net/iface.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/subnet.h>
#include <net/link.h>
#include <net/link_attr.h>
#include <net/ospf.h>
#include <net/node.h>


/////////////////////////////////////////////////////////////////////
//
// LINK METHODS
//
/////////////////////////////////////////////////////////////////////

// ----- net_link_destroy -------------------------------------------
/**
 *
 */
void net_link_destroy(net_iface_t ** link_ref)
{
  if (*link_ref != NULL) {

    // Destroy context if callback provided
    //if ((*ppLink)->fDestroy != NULL)
    //  (*ppLink)->fDestroy((*ppLink)->pContext);

    // Destroy IGP weights
    //net_igp_weights_destroy(&(*ppLink)->weights);

    net_iface_destroy(link_ref);
  }
}

// -----[ _net_link_create_iface ]-----------------------------------
/**
 * Get the given network interface on the given node. If the
 * interface already exists, check that its type corresponds to the
 * requested interface type.
 *
 * If the interface does not exist, create it.
 */
static inline net_error_t
_net_link_create_iface(net_node_t * node,
		       net_iface_id_t tID,
		       net_iface_type_t type,
		       net_iface_t ** iface_ref)
{
  net_error_t error;

  // Does the interface already exist ?
  *iface_ref= node_find_iface(node, tID);

  if (*iface_ref == NULL) {

    // If the interface does not exist, create it.
    error= net_iface_factory(node, tID, type, iface_ref);
    if (error != ESUCCESS)
      return error;
    error= node_add_iface2(node, *iface_ref);
    if (error != ESUCCESS)
      return error;

  } else {

    // If the interface exists, check that its type corresponds to
    // the requested type.
    if ((*iface_ref)->type != type)
      return ENET_IFACE_INCOMPATIBLE;

  }

  return ESUCCESS;
}

// -----[ net_link_create_rtr ]--------------------------------------
/**
 * Create a single router-to-router (RTR) link.
 */
net_error_t net_link_create_rtr(net_node_t * src_node,
				net_node_t * dst_node,
				net_iface_dir_t dir,
				net_iface_t ** iface_ref)
{
  net_iface_t * src_iface;
  net_iface_t * dst_iface;
  net_error_t error;

  if (src_node == dst_node)
    return ENET_LINK_LOOP;

  // Create source network interface
  error= _net_link_create_iface(src_node, net_iface_id_addr(dst_node->rid),
				NET_IFACE_RTR, &src_iface);
  if (error != ESUCCESS)
    return error;

  // Create destination network interface
  error= _net_link_create_iface(dst_node, net_iface_id_addr(src_node->rid),
				NET_IFACE_RTR, &dst_iface);
  if (error != ESUCCESS)
    return error;
  
  // Connect interfaces in both directions
  error= net_iface_connect_iface(src_iface, dst_iface);
  if (error != ESUCCESS)
    return error;
  if (dir == BIDIR) {
    error= net_iface_connect_iface(dst_iface, src_iface);
    if (error != ESUCCESS)
      return error;
  }

  if (iface_ref != NULL)
    *iface_ref= src_iface;
  return ESUCCESS;
}

// -----[ net_link_create_ptp ]--------------------------------------
net_error_t net_link_create_ptp(net_node_t * src_node,
				net_iface_id_t tSrcIfaceID,
				net_node_t * dst_node,
				net_iface_id_t tDstIfaceID,
				net_iface_dir_t dir,
				net_iface_t ** iface_ref)
{
  net_iface_t * src_iface;
  net_iface_t * dst_iface;
  net_error_t error;

  // Check that endpoints are different
  if (src_node == dst_node)
    return ENET_LINK_LOOP;

  // Check that both prefixes have the same length
  if (tSrcIfaceID.mask != tDstIfaceID.mask)
    return EUNEXPECTED;

  // Check that both masked prefixes are equal
  if (ip_prefix_cmp(&tSrcIfaceID, &tDstIfaceID) != 0)
    return EUNEXPECTED;

  // Check that both interfaces are different
  if (tSrcIfaceID.network == tDstIfaceID.network)
    return EUNEXPECTED;

  // Create source network interface (if it does not exist)
  error= _net_link_create_iface(src_node, tSrcIfaceID,
				NET_IFACE_PTP, &src_iface);
  if (error != ESUCCESS)
    return error;

  // Create destination network interface
  error= _net_link_create_iface(dst_node, tDstIfaceID,
				NET_IFACE_PTP, &dst_iface);
  if (error != ESUCCESS)
    return error;

  // Connect interfaces in both directions
  error= net_iface_connect_iface(src_iface, dst_iface);
  if (error != ESUCCESS)
    return error;
  if (dir == BIDIR) {
    error= net_iface_connect_iface(dst_iface, src_iface);
    if (error != ESUCCESS)
      return error;
  }

  if (iface_ref != NULL)
    *iface_ref= src_iface;
  return ESUCCESS;
}

// -----[ net_link_create_ptmp ]-------------------------------------
/**
 * Create a link to a subnet (point-to-multi-point link).
 */
net_error_t net_link_create_ptmp(net_node_t * src_node,
				 net_subnet_t * subnet,
				 net_addr_t iface_addr,
				 net_iface_t ** iface_ref)
{
  ip_pfx_t tIfaceID= net_iface_id_pfx(iface_addr, subnet->prefix.mask);
  net_iface_t * src_iface;
  net_error_t error;

  error= net_iface_factory(src_node, tIfaceID, NET_IFACE_PTMP, &src_iface);
  if (error != ESUCCESS)
    return error;
  error= node_add_iface2(src_node, src_iface);
  if (error != ESUCCESS)
    return error;

  // Connect interface to subnet
  error= net_iface_connect_subnet(src_iface, subnet);
  if (error != ESUCCESS)
    return error;

  // Connect subnet to interface
  error= subnet_add_link(subnet, src_iface);
  if (error != ESUCCESS)
    return error;

  if (iface_ref != NULL)
    *iface_ref= src_iface;
  return ESUCCESS;
}

// -----[ net_link_set_phys_attr ]-----------------------------------
net_error_t net_link_set_phys_attr(net_iface_t * iface,
				   net_link_delay_t delay,
				   net_link_load_t capacity,
				   net_iface_dir_t dir)
{
  net_error_t error= net_iface_set_delay(iface, delay, dir);
  if (error != ESUCCESS)
    return error;
  return net_iface_set_capacity(iface, capacity, dir);
}


/////////////////////////////////////////////////////////////////////
//
// LINK DUMP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- net_link_dump ----------------------------------------------
/**
 * Show the attributes of a link.
 *
 * Syntax:
 *   <type> <destination> <weight> <delay> <state>
 *     [<tunnel>] [<igp-adv>] [<ospf-area>]
 * where
 *   <type> is one of 'ROUTER', 'TRANSIT' or 'STUB'
 *   <destination> is an IP address (router) or an IP prefix (transit/stub)
 *   <weight> is the link weight
 *   <delay> is the link delay
 *   <state> is either UP or DOWN
 *   <tunnel> is one of 'DIRECT' or 'TUNNEL'
 *   <igp-adv> is either YES or NO
 *   <ospf-area> is an integer
 */
void net_link_dump(gds_stream_t * stream, net_iface_t * link)
{
  net_iface_dump(stream, link, 1);

  /* Link delay */
  stream_printf(stream, "\t%u", link->phys.delay);

  /* Link weight */
  if (link->weights != NULL)
    stream_printf(stream, "\t%u", link->weights->data[0]);
  else
    stream_printf(stream, "\t---");

  /* Link state (up/down) */
  if (net_iface_is_enabled(link))
    stream_printf(stream, "\tUP");
  else
    stream_printf(stream, "\tDOWN");

#ifdef OSPF_SUPPORT
  if (link->tArea != OSPF_NO_AREA)
    stream_printf(stream, "\tarea:%u", link->tArea);
#endif
}

// ----- net_link_dump_load -----------------------------------------
/**
 * Dump the link's load and capacity.
 *
 * Format: <load> <capacity>
 */
void net_link_dump_load(gds_stream_t * stream, net_iface_t * link)
{
  stream_printf(stream, "%u\t%u", link->phys.load, link->phys.capacity);
}

// ----- net_link_dump_info -----------------------------------------
/**
 * 
 */
void net_link_dump_info(gds_stream_t * stream, net_iface_t * link)
{
  unsigned int index;

  stream_printf(stream, "iface    : ");
  net_iface_dump_id(stream, link);
  stream_printf(stream, "\ntype     : ");
  net_iface_dump_type(stream, link);
  stream_printf(stream, "\ndest     : ");
  net_iface_dump_dest(stream, link);
  stream_printf(stream, "\ncapacity : %u", link->phys.capacity);
  stream_printf(stream, "\ndelay    : %u", link->phys.delay);
  stream_printf(stream, "\nload     : %u\n", link->phys.load);
  if (link->weights != NULL) {
    stream_printf(stream, "igp-depth: %u\n",
	       net_igp_weights_depth(link->weights));
    for (index= 0; index < net_igp_weights_depth(link->weights); index++)
      stream_printf(stream, "tos%.2u    : %u\n", index,
		 link->weights->data[index]);
  }
}
