// ==================================================================
// @(#)ez_topo.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/08/08
// $Id$
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
#include <net/ez_topo.h>

// -----[ _ez_node_builder ]-----------------------------------------
static inline int _ez_node_builder(ez_topo_t * eztopo,
				   ez_node_t * eznode)
{
  net_error_t error;
  int options= 0;

  if (!(eznode->options & EZ_NODE_OPT_NO_LOOPBACK))
    options|= NODE_OPTIONS_LOOPBACK;

  // Generate address ?
  if (eznode->id.addr == NET_ADDR_ANY)
    eznode->id.addr= ++(eztopo->addr);
  
  // Create and add node
  error= node_create(eznode->id.addr, &eznode->node, options);
  if (error != ESUCCESS)
    return error;
  error= network_add_node(eztopo->network, eznode->node);
  if (error != ESUCCESS) {
    node_destroy(&eznode->node);
    return error;
  }

  // Place in IGP domain ?
  if (eznode->domain > 0) {
    igp_domain_t * domain= network_find_igp_domain(eztopo->network,
						   eznode->domain);
    if (domain == NULL) {
      domain= igp_domain_create(eznode->domain, IGP_DOMAIN_IGP);
      assert(network_add_igp_domain(eztopo->network, domain) == ESUCCESS);
    }
    error= igp_domain_add_router(domain, eznode->node);
    if (error != ESUCCESS)
      return error;
  }

  return error;
}

// -----[ _ez_subnet_builder ]---------------------------------------
static inline int _ez_subnet_builder(ez_topo_t * eztopo,
				     ez_node_t * eznode)
{
  net_error_t error;
  
  eznode->subnet= subnet_create(eznode->id.pfx.network,
				eznode->id.pfx.mask,
				(eznode->options & EZ_NODE_OPT_STUB)?
				NET_SUBNET_TYPE_STUB:
				NET_SUBNET_TYPE_TRANSIT);
  error= network_add_subnet(eztopo->network, eznode->subnet);
  if (error != ESUCCESS)
    return error;
  
  return error;
}

// -----[ _ez_edge_builder ]-----------------------------------------
static inline int _ez_edge_builder(ez_topo_t * eztopo,
				   ez_edge_t * ezedge)
{
  net_error_t error;
  ez_node_t * eznode_src;
  ez_node_t * eznode_dst;
  int to_subnet= 0;

  // Both nodes must exist
  if ((ezedge->src >= eztopo->num_nodes) ||
      (ezedge->dst >= eztopo->num_nodes)) {
    stream_printf(gdserr, "EZ-TOPO: src/dst index is invalid");
    return EUNEXPECTED;
  }

  eznode_src= &eztopo->nodes[ezedge->src];
  eznode_dst= &eztopo->nodes[ezedge->dst];

  // Either node/node or node/subnet
  if (eznode_src->type != NODE) {
    stream_printf(gdserr, "EZ-TOPO: src must be a node\n");
    return EUNEXPECTED;
  }

  to_subnet= (eznode_dst->type == SUBNET);

  // Create link
  if (to_subnet) {

    if (ezedge->src_addr == IP_ADDR_ANY) {
      stream_printf(gdserr, "EZ-TOPO: src address must be defined\n");
      return EUNEXPECTED;
    }

    error= net_link_create_ptmp(eznode_src->node,
				eznode_dst->subnet,
				ezedge->src_addr,
				&ezedge->ref);
  } else {

    if ((ezedge->src_addr != IP_ADDR_ANY) &&
	(ezedge->dst_addr != IP_ADDR_ANY)) {
      error= net_link_create_ptp(eznode_src->node,
				 net_iface_id_pfx(ezedge->src_addr, 30),
				 eznode_dst->node,
				 net_iface_id_pfx(ezedge->dst_addr, 30),
				 BIDIR,
				 &ezedge->ref);
    } else {
      error= net_link_create_rtr(eznode_src->node,
				 eznode_dst->node,
				 BIDIR,
				 &ezedge->ref);
    }
  }

  if (error != ESUCCESS)
    return error;
  
  // Set IGP weight
  if (ezedge->weight > 0)
    net_iface_set_metric(ezedge->ref, 0, ezedge->weight,
			 (to_subnet?0:BIDIR));

  net_link_set_phys_attr(ezedge->ref, ezedge->delay, ezedge->capacity,
			 (to_subnet?0:BIDIR));

  return error;
}

// -----[ _ez_topo_create ]------------------------------------------
static inline
ez_topo_t * _ez_topo_create(unsigned int num_nodes,
			    ez_node_t nodes[],
			    unsigned int num_edges,
			    ez_edge_t edges[])
{
  ez_topo_t * eztopo= (ez_topo_t *) MALLOC(sizeof(ez_topo_t));
  eztopo->network= network_create();
  eztopo->addr= IP_ADDR_ANY;
  eztopo->num_nodes= num_nodes;
  eztopo->nodes= (ez_node_t *) MALLOC(sizeof(ez_node_t)*num_nodes);
  eztopo->num_edges= num_edges;
  eztopo->edges= (ez_edge_t *) MALLOC(sizeof(ez_edge_t)*num_edges);
  return eztopo;
}

// -----[ ez_topo_builder ]------------------------------------------
ez_topo_t * ez_topo_builder(unsigned int num_nodes,
			    ez_node_t nodes[],
			    unsigned int num_edges,
			    ez_edge_t edges[])
{
  unsigned int index;
  ez_topo_t * eztopo;
  ez_node_t * eznode;
  ez_edge_t * ezedge;
  net_error_t error;

  eztopo= _ez_topo_create(num_nodes, nodes, num_edges, edges);

  // Create nodes
  for (index= 0; index < num_nodes; index++) {
    memcpy(&eztopo->nodes[index], &nodes[index], sizeof(ez_node_t));
    eznode= &eztopo->nodes[index];

    switch (eznode->type) {
    case NODE:
      error= _ez_node_builder(eztopo, eznode);
      break;
    case SUBNET:
      error= _ez_subnet_builder(eztopo, eznode);
      break;
    default:
      abort();
    }

    if (error != ESUCCESS)
      goto error_msg;
  }

  // Create edges
  for (index= 0; index < num_edges; index++) {
    memcpy(&eztopo->edges[index], &edges[index], sizeof(ez_edge_t));
    ezedge= &eztopo->edges[index];

    error= _ez_edge_builder(eztopo, ezedge);
    if (error != ESUCCESS)
      goto error_msg;
  }

  return eztopo;

 error_msg:
  abort();
  ez_topo_destroy(&eztopo);
  network_perror(gdserr, error);
  return NULL;
}

// -----[ ez_topo_destroy ]------------------------------------------
void ez_topo_destroy(ez_topo_t ** eztopo_ref)
{
  ez_topo_t * eztopo;

  if (*eztopo_ref != NULL) {
    eztopo= *eztopo_ref;
    if (eztopo->network != NULL)
      network_destroy(&eztopo->network);
    FREE(eztopo->nodes);
    FREE(eztopo->edges);
    FREE(*eztopo_ref);
  }
}

