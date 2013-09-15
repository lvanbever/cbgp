// ==================================================================
// @(#)node.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 08/08/2005
// $Id: node.c,v 1.19 2009-08-31 09:48:28 bqu Exp $
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
#include <libgds/str_util.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/link.h>
#include <net/link-list.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/node.h>
#include <net/spt.h>
#include <net/subnet.h>

// ----- node_create ------------------------------------------------
net_error_t node_create(net_addr_t rid, net_node_t ** node_ref,
			int options)
{
  net_node_t * node;
  net_error_t error;

  if (rid == NET_ADDR_ANY)
    return ENET_NODE_INVALID_ID;

  node=(net_node_t *) MALLOC(sizeof(net_node_t));
  node->rid= rid;
  node->name= NULL;
  node->ifaces= net_links_create();
  node->protocols= protocols_create();
  node->rt= rt_create();
  node->coord.latitude= 0;
  node->coord.longitude= 0;
  node->syslog_enabled= 0;
  node->domains= uint16_array_create2(0,
				      ARRAY_OPTION_UNIQUE|
				      ARRAY_OPTION_SORTED);
  node->spt= NULL;

  // Activate ICMP protocol
  error= node_register_protocol(node, NET_PROTOCOL_ICMP, node);
  if (error != ESUCCESS)
    return error;

  // Create loopback interface with node's RID
  if (options & NODE_OPTIONS_LOOPBACK) {
    error= node_add_iface(node, net_iface_id_addr(rid), NET_IFACE_LOOPBACK);
    if (error != ESUCCESS)
      return error;
  }

#ifdef OSPF_SUPPORT
  node->pOSPFAreas= uint32_array_create2(0,
					 ARRAY_OPTION_SORTED|
					 ARRAY_OPTION_UNIQUE);
  node->pOspfRT= OSPF_rt_create();
#endif

  *node_ref= node;
  return ESUCCESS;
}

// ----- node_destroy -----------------------------------------------
/**
 *
 */
void node_destroy(net_node_t ** node_ref)
{
  if (*node_ref != NULL) {
    rt_destroy(&(*node_ref)->rt);
    protocols_destroy(&(*node_ref)->protocols);
    net_links_destroy(&(*node_ref)->ifaces);

#ifdef OSPF_SUPPORT
    _array_destroy((array_t **)(&(*node_ref)->pOSPFAreas));
    OSPF_rt_destroy(&(*node_ref)->pOspfRT);
#endif

    uint16_array_destroy(&(*node_ref)->domains);
    if ((*node_ref)->name)
      str_destroy(&(*node_ref)->name);
    spt_destroy(&((*node_ref)->spt));
    FREE(*node_ref);
    *node_ref= NULL;
  }
}

// ----- node_get_name ----------------------------------------------
char * node_get_name(net_node_t * node)
{
  return node->name;
}

// ----- node_set_name ----------------------------------------------
void node_set_name(net_node_t * node, const char * name)
{
  if (node->name)
    str_destroy(&node->name);
  if (name != NULL)
    node->name= str_create(name);
  else
    node->name= NULL;
}

// -----[ node_set_coord ]--------------------------------------------
/**
 *
 */

// -----[ node_get_coord ]--------------------------------------------
/**
 *
 */
void node_get_coord(net_node_t * node, float * latitude, float * longitude)
{
  if (latitude != NULL)
    *latitude= node->coord.latitude;
  if (longitude != NULL)
    *longitude= node->coord.longitude;
}

// -----[ node_dump_id ]-------------------------------------------
void node_dump_id(gds_stream_t * stream, net_node_t * node)
{
  ip_address_dump(stream, node->rid);
}


// ----- node_dump ---------------------------------------------------
/**
 *
 */
void node_dump(gds_stream_t * stream, net_node_t * node)
{ 
  node_dump_id(stream, node);
}

// ----- node_info --------------------------------------------------
/**
 *
 */
void node_info(gds_stream_t * stream, net_node_t * node)
{
  unsigned int index;

  stream_printf(stream, "id       : ");
  ip_address_dump(stream, node->rid);
  stream_printf(stream, "\n");
  stream_printf(stream, "domain   :");
  for (index= 0; index < uint16_array_size(node->domains); index++) {
    stream_printf(stream, " %d", node->domains->data[index]);
  }
  stream_printf(stream, "\n");
  if (node->name != NULL)
    stream_printf(stream, "name     : %s\n", node->name);
  stream_printf(stream, "addresses: ");
  node_addresses_dump(stream, node);
  stream_printf(stream, "\n");
  stream_printf(stream, "latitude : %f\n", node->coord.latitude);
  stream_printf(stream, "longitude: %f\n", node->coord.longitude);
}


/////////////////////////////////////////////////////////////////////
//
// NODE INTERFACES FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ node_add_iface ]-------------------------------------------
/**
 * This function adds an interface to the node. The interface
 * identifier is an IP address or an IP prefix. The interface type
 * is one of router-to-router (RTR), point-to-point (PTP),
 * point-to-multipoint (PTMP) or virtual.
 *
 * If the identifier is incorrect or an interface with the same
 * identifier already exists, the function fails with error:
 *   NET_ERROR_MGMT_INVALID_IFACE_ID
 *
 * Otherwise, the function should return
 *   ESUCCESS
 */
int node_add_iface(net_node_t * node, net_iface_id_t tIfaceID,
		   net_iface_type_t tIfaceType)
{
  net_iface_t * pIface;
  int iResult;

  // Create interface
  iResult= net_iface_factory(node, tIfaceID, tIfaceType, &pIface);
  if (iResult != ESUCCESS)
    return iResult;

  return node_add_iface2(node, pIface);
}

// -----[ node_add_iface2 ]------------------------------------------
/**
 * Attach an interface to the node.
 *
 * If an interface with the same identifier already exists, the
 * function fails with error
 *   NET_ERROR_MGMT_DUPLICATE_IFACE
 */
int node_add_iface2(net_node_t * node, net_iface_t * pIface)
{
  net_error_t error= net_links_add(node->ifaces, pIface);
  if (error != ESUCCESS)
    net_iface_destroy(&pIface);
  return error;
}

// -----[ node_find_iface ]------------------------------------------
/**
 * Find an interface link based on its identifier.
 */
net_iface_t * node_find_iface(net_node_t * node, net_iface_id_t tIfaceID)
{
  return net_links_find(node->ifaces, tIfaceID);
}

// -----[ node_find_iface_to ]-------------------------------------
net_iface_t * node_find_iface_to(net_node_t * node, net_elem_t * to)
{
  unsigned int index;
  net_iface_t * iface;

  /* TBR stream_printf(logout, "node_find_iface_to(");
  node_dump_id(logout, node);
  stream_printf(logout, ", ");
  switch (to->type) {
  case LINK:
    stream_printf(logout, "LINK:");
    net_iface_dump_id(logout, to->link);
    break;
  case NODE:
    stream_printf(logout, "NODE:");
    node_dump_id(logout, to->node);
    break;
  case SUBNET:
    stream_printf(logout, "SUBNET:");
    subnet_dump_id(logout, to->subnet);
    break;
  }
  stream_printf(logout, ")\n");*/

  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= (net_iface_t *) node->ifaces->data[index];
    /* TBR stream_printf(logout, "\t");
    net_iface_dump_id(logout, iface);
    stream_printf(logout, " ??\n");*/
    if (!net_iface_is_connected(iface))
      continue;
    switch (to->type) {
    case LINK:
      if (iface->type != NET_IFACE_PTP)
	continue;
      if (iface->dest.iface == to->link)
	return iface;
      break;
    case NODE:
      if (iface->type != NET_IFACE_RTR)
	continue;
      if (iface->dest.iface->owner == to->node)
	return iface;
      break;
    case SUBNET:
      if (iface->type != NET_IFACE_PTMP)
	continue;
      if (iface->dest.subnet == to->subnet)
	return iface;
      break;
    default:
      abort();
    }
  }
  return NULL;
}


// -----[ node_ifaces_dump ]-----------------------------------------
/**
 * This function shows the list of interfaces of a given node's, along
 * with their type.
 */
void node_ifaces_dump(gds_stream_t * stream, net_node_t * node)
{
  unsigned int index;
  net_iface_t * iface;

  // Show network interfaces
  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    net_iface_dump(stream, iface, 0);
    stream_printf(stream, "\n");
  }
}

// -----[ node_ifaces_load_clear ]-----------------------------------
void node_ifaces_load_clear(net_node_t * node)
{
  gds_enum_t * links= net_links_get_enum(node->ifaces);
  net_iface_t * iface;

  while (enum_has_next(links)) {
    iface= *((net_iface_t **) enum_get_next(links));
    net_iface_set_load(iface, 0);
  }
  enum_destroy(&links);
}


/////////////////////////////////////////////////////////////////////
//
// NODE LINKS FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_links_dump --------------------------------------------
/**
 * Dump all the node's links. The function works by scanning through
 * all the node's interfaces. If an interface is connected, it is
 * dumped, by using a "link dump" function (i.e. net_link_dump()
 * defined in src/net/link.h).
 */
void node_links_dump(gds_stream_t * stream, net_node_t * node)
{
  unsigned int index;
  net_iface_t * iface;

  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    if (!net_iface_is_connected(iface))
      continue;
    net_link_dump(stream, iface);
    stream_printf(stream, "\n");
  }
}

// ----- node_links_enum --------------------------------------------
/**
 * THIS SHOULD BE MOVED TO src/cli/enum.c
net_iface_t * node_links_enum(net_node_t * node, int state)
{
  static gds_enum_t * pEnum;
  net_iface_t * pIface;

  if (state == 0)
    pEnum= net_links_get_enum(node->ifaces);
  if (enum_has_next(pEnum)) {
    pIface= (net_iface_t *) enum_get_next(pEnum);
    return pIface;
  }
  enum_destroy(&pEnum);
  return NULL;
}
*/

// ----- node_links_save --------------------------------------------
void node_links_save(gds_stream_t * stream, net_node_t * node)
{
  gds_enum_t * ifaces= net_links_get_enum(node->ifaces);
  net_iface_t * iface;

  while (enum_has_next(ifaces)) {
    iface= *((net_iface_t **) enum_get_next(ifaces));
  
    // Skip tunnels
    if ((iface->type == NET_IFACE_VIRTUAL) ||
	(iface->type == NET_IFACE_LOOPBACK))
      continue;
  
    // This side of the link
    ip_address_dump(stream, node->rid);
    stream_printf(stream, "\t");
    net_iface_dump_id(stream, iface);
    stream_printf(stream, "\t");

    // Other side of the link
    switch (iface->type) {
    case NET_IFACE_RTR:
      ip_address_dump(stream, iface->dest.iface->owner->rid);
      stream_printf(stream, "\t");
      ip_address_dump(stream, node->rid);
      stream_printf(stream, "/32");
      break;
    case NET_IFACE_PTP:
      ip_address_dump(stream, iface->dest.iface->owner->rid);
      stream_printf(stream, "\t");
      ip_address_dump(stream, node->rid);
      stream_printf(stream, "/32");      
      break;
    case NET_IFACE_PTMP:
      ip_prefix_dump(stream, iface->dest.subnet->prefix);
      stream_printf(stream, "\t---");
      break;
    default:
      abort();
    }

    // Link load
    stream_printf(stream, "\t%u", iface->phys.load);
    stream_printf(stream, "\t%u\n", iface->phys.capacity);
  }
    
  enum_destroy(&ifaces);
}

// -----[ node_has_address ]-----------------------------------------
/**
 * This function checks if the node has the given address. The
 * function first checks the loopback address (that identifies the
 * node). Then, the function checks the multi-point links of the node
 * for an 'interface' address.
 *
 * Return:
 * 1 if the node has the given address
 * 0 otherwise
 */
net_iface_t * node_has_address(net_node_t * node, net_addr_t addr)
{
  unsigned int index;
  net_iface_t * iface;

  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    if (net_iface_has_address(iface, addr))
      return iface;
  }
  return NULL;
}

// -----[ node_has_prefix ]------------------------------------------
net_iface_t * node_has_prefix(net_node_t * node, ip_pfx_t pfx)
{
  unsigned int index;
  net_iface_t * iface;
  
  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    if (net_iface_has_prefix(iface, pfx))
      return iface;
  }
  return NULL;
}

// ----- node_addresses_for_each ------------------------------------
/**
 * This function calls the given callback function for each address
 * owned by the node. The first address is the node's loopback address
 * (its identifier). The other addresses are the interface addresses
 * (that are set on multi-point links, for instance).
 */
int node_addresses_for_each(net_node_t * node, gds_array_foreach_f foreach,
			    void * ctx)
{
  int result;
  unsigned int index;
  net_iface_t * iface;

  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    if (iface->type == NET_IFACE_RTR)
      continue;
    if ((result= foreach(&iface->addr, ctx)) != 0)
      return result;
  }
  return 0;
}

// ----- node_addresses_dump ----------------------------------------
/**
 * This function shows the list of addresses owned by the node. The
 * first address is the node's loopback address (its identifier). The
 * other addresses are the interface addresses (that are set on
 * multi-point links, for instance).
 */
void node_addresses_dump(gds_stream_t * stream, net_node_t * node)
{
  unsigned int index;
  net_iface_t * iface;
  int space= 0;

  // Show interface addresses
  for (index= 0; index < net_ifaces_size(node->ifaces); index++) {
    iface= net_ifaces_at(node->ifaces, index);
    if (iface->type == NET_IFACE_RTR)
      continue;
    if (space)
      stream_printf(stream, " ");
    ip_address_dump(stream, iface->addr);
    space= 1;
  }
}


/////////////////////////////////////////////////////////////////////
//
// ROUTING TABLE FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_rt_add_route ------------------------------------------
/**
 * Add a route to the given prefix into the given node. The node must
 * have a direct link towards the given next-hop address. The weight
 * of the route can be specified as it can be different from the
 * outgoing link's weight.
 *
 * \param node    is the node were the route is installed.
 * \param pfx     is the route's destination prefix.
 * \param oif_id  is the outgoing interface identifier. It can be set
 *                to IP_ADDR_ANY.
 * \param gateway is the route's gateway. It can be set to
 *                IP_ADDR_ANY.
 * \param weight  is the route's metric.
 * \param type    is the route's type.
 * \retval ESUCCESS in case of success or a negative error code in
 *         case of error.
 */
int node_rt_add_route(net_node_t * node, ip_pfx_t pfx,
		      net_iface_id_t oif_id, net_addr_t gateway,
		      uint32_t weight, uint8_t type)
{
  net_iface_t * oif= NULL;

  // Lookup the next-hop's interface (no recursive lookup is allowed,
  // it must be a direct link !)
  if (oif_id.network != IP_ADDR_ANY) {
    oif= node_find_iface(node, oif_id);
    if (oif == NULL)
      return ENET_IFACE_UNKNOWN;

    // The interface cannot be a loopback interface
    if (oif->type == NET_IFACE_LOOPBACK)
      return ENET_IFACE_INCOMPATIBLE;
  }

  return node_rt_add_route_link(node, pfx, oif, gateway,
				weight, type);
}

// ----- node_rt_add_route_link -------------------------------------
/**
 * Add a route to the given prefix into the given node. The outgoing
 * interface is a link. The weight of the route can be specified as it
 * can be different from the outgoing link's weight.
 *
 * Pre: the outgoing link (next-hop interface) must exist in the node.
 */
int node_rt_add_route_link(net_node_t * node, ip_pfx_t pfx,
			   net_iface_t * oif, net_addr_t gateway,
			   uint32_t weight, uint8_t type)
{
  rt_info_t * rtinfo;
  net_iface_t * sub_link;
  net_error_t result;

  // If a next-hop has been specified, check that it is reachable
  // through the given interface.
  if ((gateway != IP_ADDR_ANY) && (oif != NULL)) {
    switch (oif->type) {
    case NET_IFACE_RTR:
      if (oif->dest.iface->owner->rid != gateway)
	return ENET_RT_NH_UNREACH;
      break;
    case NET_IFACE_PTP:
      if (!node_has_address(oif->dest.iface->owner, gateway))
	return ENET_RT_NH_UNREACH;
      break;
    case NET_IFACE_PTMP:
      sub_link= net_subnet_find_link(oif->dest.subnet, gateway);
      if ((sub_link == NULL) || (sub_link->addr == oif->addr))
	return ENET_RT_NH_UNREACH;
      break;
    default:
      abort();
    }
  } else if ((gateway == IP_ADDR_ANY) && (oif == NULL)) {
    return ENET_RT_NO_GW_NO_OIF;
  }

  // Check if route info already exists (if we add a parallel path)
  rtinfo= rt_find_exact(node->rt, pfx, type);

  // Build route info
  if (rtinfo == NULL) {
    rtinfo= rt_info_create(pfx, weight, type);
    result= rt_info_add_entry(rtinfo, oif, gateway);
    if (result != ESUCCESS) {
      rt_info_destroy(&rtinfo);
      return result;
    }
    result= rt_add_route(node->rt, pfx, rtinfo);
  } else
    result= rt_info_add_entry(rtinfo, oif, gateway);

  return result;
}

// ----- node_rt_del_route ------------------------------------------
/**
 * This function removes a route from the node's routing table. The
 * route is identified by the prefix, the next-hop address (i.e. the
 * outgoing interface) and the type.
 *
 * If the next-hop is not present (NULL), all the routes matching the
 * other parameters will be removed whatever the next-hop is.
 */
int node_rt_del_route(net_node_t * node, ip_pfx_t * pfx,
		      net_iface_id_t * oif_id, net_addr_t * next_hop,
		      uint8_t type)
{
  net_iface_t * iface= NULL;

  // Lookup the outgoing interface (no recursive lookup is allowed,
  // it must be a direct link !)
  if (oif_id != NULL)
    iface= node_find_iface(node, *oif_id);

  return rt_del_route(node->rt, pfx, iface, next_hop, type);
}

// ----- node_rt_dump -----------------------------------------------
/**
 * Dump the node's routing table.
 */
void node_rt_dump(gds_stream_t * stream, net_node_t * node, ip_dest_t dest)
{
  if (node->rt != NULL)
    rt_dump(stream, node->rt, dest);
  stream_flush(stream);
}

// ----- node_rt_lookup ---------------------------------------------
/**
 * This function looks for the next-hop that must be used to reach a
 * destination address. The function looks up the static/IGP/BGP
 * routing table.
 */
const rt_entries_t * node_rt_lookup(net_node_t * node, net_addr_t dst_addr)
{
  rt_info_t * rtinfo;

  if (node->rt != NULL) {
    rtinfo= rt_find_best(node->rt, dst_addr, NET_ROUTE_ANY);
    if (rtinfo != NULL)
      return rtinfo->entries;
  }
  return NULL;
}

const rt_info_t * node_rt_lookup2(net_node_t * node, net_addr_t dst_addr)
{
  if (node->rt != NULL)
    return rt_find_best(node->rt, dst_addr, NET_ROUTE_ANY);
  return NULL;
}

/////////////////////////////////////////////////////////////////////
//
// PROTOCOL FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// ----- node_register_protocol -------------------------------------
/**
 * Register a new protocol into the given node. The protocol is
 * identified by a number (see protocol_t.h for definitions). The
 * destroy callback function is used to destroy PDU corresponding to
 * this protocol in case the message carrying such PDU is not
 * delivered to the handler (message dropped or incorrectly routed for
 * instance). The protocol handler is composed of the handle_event
 * callback function and the handler context pointer.
 */
int node_register_protocol(net_node_t * node, net_protocol_id_t id,
			   void * handler)
{
  return protocols_add(node->protocols, id, handler);
}

// -----[ node_get_protocol ]----------------------------------------
/**
 * Return the handler for the given protocol. If the protocol is not
 * supported, NULL is returned.
 */
net_protocol_t * node_get_protocol(net_node_t * node,
				   net_protocol_id_t id)
{
  return protocols_get(node->protocols, id);
}


/////////////////////////////////////////////////////////////////////
//
// IGP DOMAIN MEMBERSHIP FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ node_igp_domain_add ]--------------------------------------
int node_igp_domain_add(net_node_t * node, uint16_t id)
{
  return uint16_array_add(node->domains, id);
}

// -----[ node_belongs_to_igp_domain ]-------------------------------
/**
 * Test if a node belongs to a given IGP domain.
 *
 * Return value:
 *   TRUE (1) if node belongs to the given IGP domain
 *   FALSE (0) otherwise.
 */
int node_belongs_to_igp_domain(net_node_t * node, uint16_t id)
{
  unsigned int index;

  if (uint16_array_index_of(node->domains, id, &index) == 0)
    return 1;
  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// TRAFFIC LOAD FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

// -----[ node_load_flow ]-------------------------------------------
int node_load_flow(net_node_t * node, net_addr_t src_addr,
		   net_addr_t dst_addr, unsigned int bytes,
		   flow_stats_t * stats, ip_trace_t ** trace_ref,
		   ip_opt_t * opts)
{
  ip_trace_t * trace;
  net_error_t result= ESUCCESS;
  array_t * traces;

  flow_stats_count(stats, bytes);

  if (opts == NULL)
    opts= ip_options_create();
  ip_options_load(opts, bytes);
  traces= icmp_trace_send(node, dst_addr, 255, opts);
  assert(traces != NULL);
  assert(_array_length(traces) == 1); // ECMP is not enabled

  _array_get_at(traces, 0, &trace);

  // No trace returned. Update statistics.
  if (trace == NULL) {
    flow_stats_failure(stats, bytes);
    return -1;
  }

  if (trace->status != ESUCCESS)
    result= trace->status;

  // If trace not requested by caller, free
  if (trace_ref != NULL) {
    *trace_ref= trace;
    trace= NULL;
    _array_set_at(traces, 0, &trace);
  }
  _array_destroy(&traces);

  // \todo In the future, additional checks could be added here...
  // \li destination reached ?

  if (result != ESUCCESS)
    flow_stats_failure(stats, bytes);
  else
    flow_stats_success(stats, bytes);
  return 0;
}

typedef struct {
  net_node_t   * target_node;
  uint8_t        options;
  flow_stats_t * stats;
} _netflow_ctx_t;

// -----[ _node_netflow_handler ]------------------------------------
static int _node_netflow_handler(flow_t * flow, flow_field_map_t * map,
				 void * context)
{
  _netflow_ctx_t * ctx= (_netflow_ctx_t *) context;
  net_node_t * node= ctx->target_node;
  ip_trace_t * trace;

  if (ctx->options & NET_NODE_NETFLOW_OPTIONS_DETAILS) {
    stream_printf(gdsout, "src:");
    ip_address_dump(gdsout, flow->src_addr);
    stream_printf(gdsout, " dst:");
    ip_address_dump(gdsout, flow->dst_addr);
    if (flow_field_map_isset(map, FLOW_FIELD_DST_MASK))
      stream_printf(gdsout, "/%u", flow->dst_mask);
    stream_printf(gdsout, " octets:%u ", flow->bytes);
  }

  if (node_load_flow(node, flow->src_addr, flow->dst_addr,
		     flow->bytes, ctx->stats, &trace, NULL) < 0) {
    if (ctx->options & NET_NODE_NETFLOW_OPTIONS_DETAILS)
      stream_printf(gdsout, "failed\n");
    return NETFLOW_ERROR;
  }

  if (ctx->options & NET_NODE_NETFLOW_OPTIONS_DETAILS) {
    stream_printf(gdsout, "status: ");
    network_perror(gdsout, trace->status);
    stream_printf(gdsout, "\n");
  }

  ip_trace_destroy(&trace);
  return NETFLOW_SUCCESS;
}

// -----[ node_load_netflow ]----------------------------------------
int node_load_netflow(net_node_t * node, const char * filename,
		      uint8_t options, flow_stats_t * stats)
{
  _netflow_ctx_t ctx= {
    .target_node= node,
    .options    = options,
    .stats      = stats,
  };
  flow_field_map_t map;

  flow_field_map_init(&map);
  flow_field_map_set(&map, FLOW_FIELD_SRC_IP);
  flow_field_map_set(&map, FLOW_FIELD_DST_IP);
  flow_field_map_set(&map, FLOW_FIELD_OCTETS);

  return netflow_load(filename, &map, _node_netflow_handler, &ctx);
}


/////////////////////////////////////////////////////////////////////
//
// SYSLOG
//
/////////////////////////////////////////////////////////////////////

// -----[ node_syslog ]--------------------------------------------
gds_stream_t * node_syslog(net_node_t * self)
{
  if (self->syslog_enabled)
    return gdserr;
  return NULL;
}

// -----[ node_syslog_set_enabled ]--------------------------------
void node_syslog_set_enabled(net_node_t * self, int enabled)
{
  self->syslog_enabled= enabled;
}
