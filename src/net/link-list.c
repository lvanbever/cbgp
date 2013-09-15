// ==================================================================
// @(#)link-list.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/08/2003
// $Id: link-list.c,v 1.12 2009-03-24 16:14:59 bqu Exp $
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

#include <stdio.h>
#include <net/iface.h>
#include <net/link.h>
#include <net/link-list.h>

/**
 * IMPORTANT NODE:
 *   The interfaces are identified by their interface address.
 *   However, using only interface addresses as identifier prevents
 *   the following situation where RTR and PTP/PTMP interfaces are
 *   intermixed:
 *
 *         1.0.0.1 <--> 192.168.0.1    (RTR)
 *         1.0.0.1 <--> 192.168.0.1/24 (PTP/PTMP)
 *       
 *   This will not work as both interfaces have the same address.
 *
 *   It is possible to change this behaviour by uncommenting the
 *   following symbol definition. USE THIS WITH CARE as most of the
 *   validation tests make the assumption that interfaces are solely
 *   identified by their address.
 */
//#define __IFACE_ID_ADDR_MASK


// ----- _net_links_link_compare ------------------------------------
/**
 * Links in this list are ordered based on their IDs. A link identifier
 * depends on the interface id (see net_iface_id()).
 */
static int _net_links_link_compare(const void * item1,
				   const void * item2,
				   unsigned int elt_size)
{
  net_iface_t * iface1= *((net_iface_t **) item1);
  net_iface_t * iface2= *((net_iface_t **) item2);

#ifdef __IFACE_ID_ADDR_MASK

  ip_pfx_t sId1= net_iface_id(iface1);
  ip_pfx_t sId2= net_iface_id(iface2);

  // Lexicographic order based on (addr, mask)
  // Note that we MUST rely on the unmasked address (network)
  // We cannot use the ip_prefixes_compare() function !!!
  if (sId1.network > sId2.network)
    return 1;
  else if (sId1.network < sId2.network)
    return -1;

  if (sId1.mask > sId2.mask)
    return 1;
  else if (sId1.mask < sId2.mask)
    return -1;

#else

  if (iface1->addr > iface2->addr)
    return 1;
  if (iface1->addr < iface2->addr)
    return -1;

#endif /* __IFACE_ID_ADDR_MASK */

  return 0;
}

// ----- _net_links_link_destroy -------------------------------------
static void _net_links_link_destroy(void * item, const void * ctx)
{
  net_iface_t ** link_ref= (net_iface_t **) item;
  net_link_destroy(link_ref);
}

// ----- net_links_create -------------------------------------------
net_ifaces_t * net_links_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED|ARRAY_OPTION_UNIQUE,
			  _net_links_link_compare,
			  _net_links_link_destroy,
			  NULL);
}

// ----- net_links_destroy ------------------------------------------
void net_links_destroy(net_ifaces_t ** links_ref)
{
  ptr_array_destroy(links_ref);
}

// -----[ net_ifaces_add ]-------------------------------------------
/**
 * Add an interface to the list.
 *
 * Returns
 *   ESUCCESS if the interface could be added
 *   < 0 in case of error (duplicate)
 */
net_error_t net_links_add(net_ifaces_t * links, net_iface_t * iface)
{
  if (ptr_array_add(links, &iface) < 0)
    return ENET_IFACE_DUPLICATE;
  return ESUCCESS;
}

// ----- net_links_dump ---------------------------------------------
void net_links_dump(gds_stream_t * stream, net_ifaces_t * ifaces)
{
  unsigned int index;

  for (index= 0; index < net_ifaces_size(ifaces); index++) {
    net_link_dump(stream, net_ifaces_at(ifaces, index));
    stream_printf(stream, "\n");
  }
}

// -----[ net_links_find ]-------------------------------------------
/**
 * Find an interface based on its ID.
 */
net_iface_t * net_links_find(net_ifaces_t * ifaces, net_iface_id_t tIfaceID)
{
  // Identification in this case is only the destination address
  net_iface_t sWrapIface= {
    .addr= tIfaceID.network,
#ifdef __IFACE_ID_ADDR_MASK
    .mask= tIfaceID.mask,
#endif /* __IFACE_ID_ADDR_MASK */
  };
  net_iface_t * pWrapIface= &sWrapIface;
  unsigned int index;

  if (ptr_array_sorted_find_index(ifaces, &pWrapIface, &index) == 0)
    return net_ifaces_at(ifaces, index);
  return NULL;
}

// -----[ net_links_get_enum ]---------------------------------------
gds_enum_t * net_links_get_enum(net_ifaces_t * ifaces)
{
  return _array_get_enum((array_t *) ifaces);
}

// -----[ net_links_find_iface ]-------------------------------------
net_iface_t * net_links_find_iface(net_ifaces_t * ifaces,
				   net_addr_t addr)
{
  net_iface_t * iface;
  unsigned int index;
  ip_pfx_t prefix;

  for (index= 0; index < net_ifaces_size(ifaces); index++) {
    iface= net_ifaces_at(ifaces, index);
    prefix= net_iface_id(iface);
    if (addr == prefix.network)
      return iface;
  }
  return NULL;
}

// -----[ net_ifaces_get_smallest ]----------------------------------
//
// Returns smallest iface addr in links list.
// Used to obtain correct OSPF-ID of a node.
int net_ifaces_get_smallest(net_ifaces_t * ifaces, net_addr_t * addr)
{
  unsigned int index;
  net_iface_t * iface;
  net_addr_t iface_addr;
  int found= 0;
  net_addr_t smallest= IP_ADDR_ANY;

  for (index= 0; index < net_ifaces_size(ifaces); index++) {
    iface= net_ifaces_at(ifaces, index);
    iface_addr= net_iface_src_address(iface);
    if (!found || (iface_addr < smallest)) {
      smallest= iface_addr;
      found= 1;
    }
  }

  if (!found)
    return ENET_IFACE_UNKNOWN;

  *addr= smallest;
  return ESUCCESS;
}

// -----[ net_ifaces_get_highest ]-----------------------------------
int net_ifaces_get_highest(net_ifaces_t * ifaces, net_addr_t * addr)
{
  unsigned int index;
  net_iface_t * iface;
  net_addr_t iface_addr;
  int found= 0;
  net_addr_t highest= IP_ADDR_ANY;

  for (index= 0; index < net_ifaces_size(ifaces); index++) {
    iface= net_ifaces_at(ifaces, index);
    iface_addr= net_iface_src_address(iface);
    if (!found || (iface_addr > highest)) {
      highest= iface_addr;
      found= 1;
    }
  }

  if (!found)
    return ENET_IFACE_UNKNOWN;

  *addr= highest;
  return ESUCCESS;
}
