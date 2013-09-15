// ==================================================================
// @(#)subnet.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/06/2005
// $Id: subnet.c,v 1.20 2009-03-24 16:27:50 bqu Exp $
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

#include <libgds/memory.h>

#include <net/error.h>
#include <net/link-list.h>
#include <net/node.h>
#include <net/subnet.h>
#include <net/ospf.h>
#include <net/network.h>


/////////////////////////////////////////////////////////////////////
//
// net_subnet_t Methods
//
/////////////////////////////////////////////////////////////////////

// ----- _net_subnet_link_compare -----------------------------------
static int _net_subnet_link_compare(const void * item1,
				    const void * item2,
				    unsigned int elt_size)
{
  net_iface_t * link1= *((net_iface_t **) item1);
  net_iface_t * link2= *((net_iface_t **) item2);

  if (link1->addr > link2->addr)
    return 1;
  if (link1->addr < link2->addr)
    return -1;
  return 0;
}

// ----- subnet_create ----------------------------------------------
net_subnet_t * subnet_create(net_addr_t network, uint8_t mask,
			     net_subnet_type_t type)
{
  net_subnet_t * subnet= (net_subnet_t *) MALLOC(sizeof(net_subnet_t));
  assert(mask < 32); // subnets with a mask of /32 are not allowed
  subnet->prefix.network= network;
  subnet->prefix.mask= mask;
  ip_prefix_mask(&subnet->prefix);
  subnet->type = type;
  
#ifdef OSPF_SUPPORT
  subnet->uOSPFArea = OSPF_NO_AREA;
#endif

  // Create an array of references to links
  subnet->ifaces= ptr_array_create(ARRAY_OPTION_SORTED |
				   ARRAY_OPTION_UNIQUE,
				   _net_subnet_link_compare,
				   NULL, NULL);
  return subnet; 
}

// ----- subnet_destroy ---------------------------------------------
void subnet_destroy(net_subnet_t ** subnet_ref)
{
  if (*subnet_ref != NULL) {
    net_links_destroy(&(*subnet_ref)->ifaces);
    FREE(*subnet_ref);
    *subnet_ref= NULL;
  }
}

// -----[ _subnet_type2str ]-----------------------------------------
static inline char * _subnet_type2str(net_subnet_type_t type)
{
  switch (type) {
  case NET_SUBNET_TYPE_TRANSIT : 
    return "transit";
  case NET_SUBNET_TYPE_STUB:
    return "stub";
  default:
    abort();
  }
}

// -----[ _subnet_length ]-------------------------------------------
static inline unsigned int _subnet_length(net_subnet_t * subnet)
{
  return ptr_array_length(subnet->ifaces);
}

// ----- subnet_dump ------------------------------------------------
void subnet_dump(gds_stream_t * stream, net_subnet_t * subnet)
{
  net_iface_t * link = NULL;
  unsigned int index;
  
  stream_printf(stream, "SUBNET PREFIX <");
  ip_prefix_dump(stream, subnet->prefix);
#ifdef OSPF_SUPPORT
  stream_printf(stream, ">\tOSPF AREA  <%u>\t",
	     (unsigned int) subnet->uOSPFArea);
#endif
  stream_printf(stream, "%s\t",_subnet_type2str(subnet->type));
  stream_printf(stream, "LINKED TO: \t");
  for (index = 0; index < _subnet_length(subnet); index++) {
    ptr_array_get_at(subnet->ifaces, index, &link);
    assert(link != NULL);
    ip_address_dump(stream, link->addr);
    stream_printf(stream, "\n---\t\t\t\t\t\t");
  }
  stream_printf(stream,"\n");
}

// -----[ subnet_dump_id ]-------------------------------------------
void subnet_dump_id(gds_stream_t * stream, net_subnet_t * subnet)
{
  ip_prefix_dump(stream, subnet->prefix);
}

// -----[ subnet_info ]----------------------------------------------
void subnet_info(gds_stream_t * stream, net_subnet_t * subnet)
{
  stream_printf(stream, "id       : ");
  subnet_dump_id(stream, subnet);
  stream_printf(stream, "\n");
  stream_printf(stream, "num-ports: %d\n", _subnet_length(subnet));
  stream_printf(stream, "type     : %s\n", _subnet_type2str(subnet->type));
}

// -----[ subnet_links_dump ]----------------------------------------
void subnet_links_dump(gds_stream_t * stream, net_subnet_t * subnet)
{
  int index;
  net_iface_t * iface;

  for (index= 0; index < _subnet_length(subnet); index++) {
    ptr_array_get_at(subnet->ifaces, index, &iface);
    net_iface_dump(stream, iface, 0);
    stream_printf(stream,"\t");
    node_dump_id(stream, iface->owner);
    stream_printf(stream,"\n");
  }
  stream_printf(stream,"\n");
}

// -----[ subnet_add_link ]------------------------------------------
int subnet_add_link(net_subnet_t * subnet, net_iface_t * link)
{
  if (ptr_array_add(subnet->ifaces, &link) < 0)
    return ENET_LINK_DUPLICATE;
  return ESUCCESS;
}

// ----- subnet_getAddr ---------------------------------------------
ip_pfx_t * subnet_get_prefix(net_subnet_t * subnet) {
  return &(subnet->prefix);
}

// ----- subnet_is_transit ------------------------------------------
int subnet_is_transit(net_subnet_t * subnet) {
  return (subnet->type == NET_SUBNET_TYPE_TRANSIT);
}

// ----- subnet_is_stub ---------------------------------------------
int subnet_is_stub(net_subnet_t * subnet) {
  return (subnet->type == NET_SUBNET_TYPE_STUB);
}

// ----- subnet_find_link -------------------------------------------
/**
 * Find a link based on its interface address.
 */
net_iface_t * net_subnet_find_link(net_subnet_t * subnet, net_addr_t dst_addr)
{
  net_iface_t sWrapLink= { .addr= dst_addr };
  net_iface_t * link= NULL;
  unsigned int index;
  net_iface_t * pWrapLink= &sWrapLink;

  if (ptr_array_sorted_find_index(subnet->ifaces, &pWrapLink, &index) == 0)
    link= net_ifaces_at(subnet->ifaces, index);
  return link;
}

// -----[ subnet_find_iface_to ]-----------------------------------
net_iface_t * subnet_find_iface_to(net_subnet_t * subnet,
				   net_elem_t * to)
{
  unsigned int index;
  net_iface_t * iface;
  
  if (to->type != NODE)
    return NULL;

  for (index= 0; index < net_ifaces_size(subnet->ifaces); index++) {
    iface= (net_iface_t *) subnet->ifaces->data[index];
    if (iface->owner == to->node)
      return iface;
  }
  return NULL;
}


/////////////////////////////////////////////////////////////////////
//
// SUBNETS LIST FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- _subnets_compare -------------------------------------------
/**
 *
 */
static int _subnets_compare(const void * item1,
			    const void * item2,
			    unsigned int elt_size){
  net_subnet_t * subnet1= *((net_subnet_t **) item1);
  net_subnet_t * subnet2= *((net_subnet_t **) item2);

  return ip_prefix_cmp(&subnet1->prefix, &subnet2->prefix);
}

// ----- _subnets_destroy -------------------------------------------
static void _subnets_destroy(void * item, const void * ctx)
{
  subnet_destroy(((net_subnet_t **) item));
}

// ----- subnets_create ---------------------------------------------
net_subnets_t * subnets_create()
{
  return ptr_array_create(ARRAY_OPTION_SORTED|
			  ARRAY_OPTION_UNIQUE,
			  _subnets_compare,
			  _subnets_destroy,
			  NULL);
}

// ----- subnets_destroy --------------------------------------------
void subnets_destroy(net_subnets_t ** subnets_ref)
{
  ptr_array_destroy((ptr_array_t **) subnets_ref);
}

// ----- subnets_add ------------------------------------------------
int subnets_add(net_subnets_t * subnets, net_subnet_t * subnet)
{
  return ptr_array_add((ptr_array_t *) subnets, &subnet);
}

// ----- subnets_find -----------------------------------------------
net_subnet_t * subnets_find(net_subnets_t * subnets, ip_pfx_t prefix)
{
  unsigned int index;
  net_subnet_t sWrasubnet, * pWrasubnet= &sWrasubnet;

  sWrasubnet.prefix= prefix;
  ip_prefix_mask(&sWrasubnet.prefix);

  if (ptr_array_sorted_find_index(subnets, &pWrasubnet, &index) == 0)
    return (net_subnet_t *) subnets->data[index];
  
  return NULL;
}
