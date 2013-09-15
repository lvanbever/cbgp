// ==================================================================
// @(#)ez_topo.h
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

/**
 * \file
 * Provide data structures and functions to help quickly build small
 * network topologies.
 *
 * The following code demonstrates how a simple topology is created.
 * First, nodes are described in an array of ez_node_t structures.
 * This array can be created on the stack. In the example, two nodes
 * are added, both must be member of IGP domain 1 and they must not
 * be assigned a loopback interface.
 * Second, links are described in an array of ez_edge_t structures.
 * This array can also be created on the stack. In the example, a
 * single link is created between node 0 and node 1 (references to
 * the above array of nodes). The link's IGP weight is 1, the address
 * of node 0 on the link is 192.168.0.1 and the address of node 1 on
 * the link is 192.168.0.2.
 * \code
 * ez_node_t nodes[]= {
 *   { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
 *   { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
 * };
 * ez_edge_t edges[]= {
 *   { .src=0, .dst=1, .weight=1,
 *     .src_addr=IPV4(192,168,0,1),
 *     .dst_addr=IPV4(192,168,0,2) },
 * };
 * ez_topo_t * topo= ez_topo_builder(2, nodes, 1, edges);
 * \endcode
 *
 * The following code demonstrates how to build a triangle topology
 * where nodes are connected using PTP links.
 * \code
 * ez_node_t nodes[]= {
 *   { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
 *   { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
 *   { .type=NODE, .domain=1, .options=EZ_NODE_OPT_NO_LOOPBACK },
 * };
 * ez_edge_t edges[]= {
 *   { .src=0, .dst=1, .weight=10,
 *     .src_addr=IPV4(192,168,0,1), .dst_addr=IPV4(192,168,0,2) },
 *   { .src=0, .dst=2, .weight=1,
 *     .src_addr=IPV4(192,168,1,1), .dst_addr=IPV4(192,168,1,2) },
 *   { .src=1, .dst=2, .weight=1,
 *     .src_addr=IPV4(192,168,2,1), .dst_addr=IPV4(192,168,2,2) },
 * };
 * ez_topo_t * topo= ez_topo_builder(3, nodes, 3, edges);
 * \endcode
 */

#ifndef __NET_EZ_TOPO_H__
#define __NET_EZ_TOPO_H__

#include <net/iface.h>
#include <net/igp_domain.h>
#include <net/ip.h>
#include <net/net_types.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>

#define EZ_NODE_OPT_NO_LOOPBACK 0x01
#define EZ_NODE_OPT_STUB        0x02

// -----[ ez_node_t ]------------------------------------------------
/** Description of a single node for the ez topology builder. */
typedef struct ez_node_t {
  /** Type: NODE or SUBNET. */
  net_elem_type_t   type;    
  union {
    /** Node's identifier (lo).
     * If the value is IP_ADDR_ANY, a unique IP address is generated. */
    net_addr_t      addr;
    /** Subnet's prefix. */
    ip_pfx_t        pfx;
  } id;
  /** Domain identifier (assigned only if id > 0). */
  unsigned int      domain;  
  union {
    /** Node's pointer (filled by factory). */
    net_node_t    * node;
    /** Subnet's pointer (filled by factory). */
    net_subnet_t  * subnet;
  };
  /** Options. */
  int               options; 
} ez_node_t;

// -----[ ez_edge_t ]------------------------------------------------
/** Description of a single node for the ez topology builder. */
typedef struct ez_edge_t {
  /** Index of source node. */
  unsigned int   src;
  /** Index of destination node. */
  unsigned int   dst;
  /** Source address. */
  net_addr_t     src_addr;
  /** Destination address. */
  net_addr_t     dst_addr;
  /** IGP weight. */
  unsigned int   weight;
  /** network interface's pointer (filled by factory). */
  net_iface_t  * ref;
  /** Delay. */
  unsigned int   delay;
  /** Capacity. */
  unsigned int   capacity;
} ez_edge_t;

// -----[ ez_topo_t ]------------------------------------------------
/** EZ topology. */
typedef struct ez_topo_t {
  /** network's pointer (filled by factory). */
  network_t     * network;
  /** current IP address (used by auto generation). */
  net_addr_t      addr;
  /** Number of nodes. */
  unsigned int    num_nodes;
  /** Array of nodes. */
  ez_node_t    * nodes;
  /** Number of edges.*/
  unsigned int    num_edges;
  /** Array of edges. */
  ez_edge_t    * edges;
} ez_topo_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ ez_topo_builder ]----------------------------------------
  /**
   * Generate an EZ topology based on a simplified description
   * of nodes and edges.
   *
   * \param num_nodes is the number of nodes to create.
   * \param nodes     is an array of nodes to be created (length of
   *                  array equals num_nodes).
   * \param num_edges is the number of edges to create.
   * \param edges     is an array of edges to be created (length of
   *                  array equals num_edges).
   * \retval an EZ topology, or NULL if an error occurred.
   */
  ez_topo_t * ez_topo_builder(unsigned int num_nodes,
			      ez_node_t nodes[],
			      unsigned int num_edges,
			      ez_edge_t edges[]);

  // -----[ ez_topo_destroy ]------------------------------------------
  /**
   * Destroy an EZ topology.
   *
   * \param eztopo_ref is a reference to the EZ topology to destroy.
   */
  void ez_topo_destroy(ez_topo_t ** eztopo_ref);

#ifdef __cplusplus
}
#endif

// -----[ ez_topo_get_node ]-------------------------------------------
static inline
net_node_t * ez_topo_get_node(ez_topo_t * topo, unsigned int index) {
  assert(index < topo->num_nodes);
  assert(topo->nodes[index].type == NODE);
  return topo->nodes[index].node;
}

// -----[ ez_topo_get_link ]-------------------------------------------
static inline
net_iface_t * ez_topo_get_link(ez_topo_t * topo, unsigned int index) {
  assert(index < topo->num_edges);
  return topo->edges[index].ref;
}

// -----[ ez_topo_igp_compute ]--------------------------------------
static inline
int ez_topo_igp_compute(ez_topo_t * topo, unsigned int id) {
  igp_domain_t * domain= network_find_igp_domain(topo->network, id);
  if (domain == NULL)
    return ENET_IGP_DOMAIN_UNKNOWN;
  return igp_domain_compute(domain, 0);
}

// -----[ ez_topo_sim_run ]------------------------------------------
static inline
int ez_topo_sim_run(ez_topo_t * topo) {
  return sim_run(network_get_simulator(topo->network));
}

#endif /* __NET_EZ_TOPO_H__ */
