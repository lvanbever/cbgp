// ==================================================================
// @(#)igp.c
//
// Simple model of an intradomain routing protocol (IGP).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: igp.c,v 1.17 2009-08-31 09:48:28 bqu Exp $
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
// TODO:
// - handle the PTP case the same way as the SUBNET case
// - register routes through directly connected interfaces
//   as NET_ROUTE_DIRECT routes and not NET_ROUTE_IGP
// ==================================================================

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>

#include <libgds/array.h>
#include <libgds/stack.h>

#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/link-list.h>
#include <net/network.h>
#include <net/node.h>
#include <net/routing.h>
#include <net/subnet.h>
#include <util/str_format.h>

//#define IGP_DEBUG

typedef struct spt_comp_t {
  spt_t        * spt;
  gds_fifo_t   * fifo;
  igp_domain_t * domain;
} spt_comp_t;

// -----[ _net_elem_dump ]-------------------------------------------
static inline void _net_elem_dump(gds_stream_t * stream, net_elem_t * elem)
{
  switch (elem->type) {
  case LINK:
    stream_printf(stream, "LINK:");
    net_iface_dump_id(stream, elem->link);
    break;
  case NODE:
    stream_printf(stream, "NODE:");
    ip_address_dump(stream, elem->node->rid);
    break;
  case SUBNET:
    stream_printf(stream, "SUBNET:");
    ip_prefix_dump(stream, elem->subnet->prefix);
    break;
  default:
    abort();
  }
}

// -----[ _debug_for_each ]------------------------------------------
#ifdef IGP_DEBUG
static int _debug_for_each(gds_stream_t * stream, void * context,
			   char format)
{
  va_list * ap= (va_list*) context;
  net_addr_t addr;
  net_elem_t * elem;
  net_iface_t * link;
  igp_weight_t weight;
  rt_entry_t * rtentry;
  ip_pfx_t prefix;
  spt_vertex_t * vertex;

  switch (format) {
  case 'a':
    addr= va_arg(*ap, net_addr_t);
    ip_address_dump(stream, addr);
    break;
  case 'e':
    elem= va_arg(*ap, net_elem_t *);
    if (elem == NULL)
      stream_printf(stream, "null");
    else
      _net_elem_dump(stream, elem);
    break;
  case 'i':
  case 'l':
    link= va_arg(*ap, net_iface_t *);
    if (link == NULL) {
      stream_printf(stream, "null");
    } else {
      net_iface_dump_id(stream, link);
      stream_printf(stream, " (%d)", net_iface_get_metric(link, 0));
    }
    break;
  case 'p':
    prefix= va_arg(*ap, ip_pfx_t);
    ip_prefix_dump(stream, prefix);
    break;
  case 'r':
    rtentry= va_arg(*ap, rt_entry_t *);
    if (rtentry != NULL)
      rt_entry_dump(stream, rtentry);
    break;
  case 'w':
    weight= va_arg(*ap, igp_weight_t);
    if (weight == IGP_MAX_WEIGHT)
      stream_printf(stream, "max-metric");
    else
      stream_printf(stream, "%u", weight);
    break;
  case 'v':
    vertex= va_arg(*ap, spt_vertex_t *);
    spt_vertex_dump(stream, vertex);
    break;
  }
  return 0;
}
#endif /* IGP_DEBUG */

// -----[ ___igp_debug ]---------------------------------------------
static inline void
___igp_debug(const char * format, ...)
{
#ifdef IGP_DEBUG
  va_list ap;

  va_start(ap, format);
  stream_printf(gdsout, "IGP_DBG::");
  str_format_for_each(gdsout, _debug_for_each, &ap, format);
  va_end(ap);
#endif /* IGP_DEBUG */
}

// -----[ spt_context_t ]--------------------------------------------
/**
 * Structure used to push information on the FIFO queue during
 * Breadth-First Search. The following informations are stored:
 * - addr (address of the node to be visited)
 * - outgoing interface (on root node)
 * - gateway
 * - weight
 * - incoming interface (used to reach that node)
 */
typedef struct {
  net_iface_t  * iif;     // incoming interface (reaching the node)
  spt_vertex_t * vertex;  // vertex in SPT
} spt_context_t;

// -----[ _spt_update_node ]-----------------------------------------
/**
 * Create/update SPT info for visited node. Three different cases are
 * possible:
 *   1). no vertex exists for this node => create
 *   2). vertex exists
 *     2.1). new_weight < weight => update weight and entries
 *     2.2). new_weight == weight (ECMP) => add entry 
 *
 * When a vertex is created or when its weight is changed, it must be
 * visited again (i.e. pushed onto the FIFO queue).
 */
static inline spt_vertex_t * _spt_update_node(spt_comp_t * spt_comp,
					      net_elem_t elem,
					      spt_vertex_t * prev_vertex,
					      igp_weight_t weight)
{
  ip_pfx_t prefix= net_elem_prefix(&elem);
  spt_vertex_t * vertex= NULL;

  ___igp_debug("  * update weight:%w ", weight);
  
  vertex= spt_get_vertex(spt_comp->spt, prefix);
  if (vertex == NULL) {
    // ------------------------------------------
    // CASE 1: Not yet visited: create SPT vertex
    // ------------------------------------------
    vertex= spt_vertex_create(elem, weight);
    spt_set_vertex(spt_comp->spt, vertex);
    if (prev_vertex != NULL)
      spt_vertex_add_pred(vertex, prev_vertex);
    ___igp_debug("NEW\n");
    return vertex;
  } else {
    if (vertex->weight > weight) {
      // -----------------------------------------------------
      // CASE 2.1: Update with new weight, interface & gateway
      // -----------------------------------------------------
      vertex->weight= weight;

      spt_vertex_clear_preds(vertex);
      spt_vertex_add_pred(vertex, prev_vertex);
      ___igp_debug("BETTER\n");
      return vertex;
    } else if (vertex->weight == weight) {
      // ------------------------------------
      // CASE 2.2: Add equal-cost path (ECMP)
      // ------------------------------------
      spt_vertex_add_pred(vertex, prev_vertex);
      ___igp_debug("ECMP\n");
    } else {
      ___igp_debug("KEEP\n");
    }
  }

  return NULL; // do not continue traversal through this node
}

// -----[ _push ]----------------------------------------------------
/**
 * Push additional nodes on the stack. These nodes will be processed
 * later.
 */
static inline void _push(spt_comp_t * spt_comp,
			 net_iface_t * iif,
			 spt_vertex_t * vertex)
{
  spt_context_t * ctx;

  ___igp_debug("  * push dst:%e\n", &vertex->elem);

  ctx= (spt_context_t *) MALLOC(sizeof(spt_context_t));
  ctx->iif= iif;
  ctx->vertex= vertex;
  assert(fifo_push(spt_comp->fifo, ctx) == 0);
}

// -----[ _link_get_next_elem ]--------------------------------------
/**
 * Determine what is the end side of the link, depending on the
 * source.
 */
static inline int _link_get_next_elem(net_elem_t * from,
				      net_iface_t * link,
				      net_elem_t * to)
{
  // Depending on source node, compute next element
  switch (from->type) {
  case NODE:
    switch (link->type) {
    case NET_IFACE_LOOPBACK:
    case NET_IFACE_VIRTUAL:
      ___igp_debug("  skip link:%l [if = loopback/virtual]\n", link);
      return 0;
      break;
    case NET_IFACE_PTMP:
      to->type= SUBNET;
      to->subnet= link->dest.subnet;
      break;
    case NET_IFACE_RTR:
      to->type= NODE;
      to->node= link->dest.iface->owner;
      break;
    case NET_IFACE_PTP:
      to->type= LINK;
      to->link= link->dest.iface;
      break;
    default:
      abort();
    }
    break;
  case SUBNET:
    to->type= NODE;
    to->node= link->owner;
    break;
  case LINK:
    to->type= NODE;
    to->node= link->dest.iface->owner;
    break;
  default:
    abort();
  }
  return 1;
}

// -----[ _link_traverse ]-------------------------------------------
/**
 * Filter unacceptable links. Consider only the links that have the
 * following properties:
 * - link must be enabled (UP)
 * - link must be connected
 * - the IGP weight must be greater than 0
 *   and lower than IGP_MAX_WEIGHT
 */
//static inline
void _link_traverse(spt_comp_t * spt_comp,
		    spt_context_t * context,
		    net_iface_t * link)
{
  igp_weight_t weight= 0;
  net_elem_t next_elem= { .type=NODE };
  spt_vertex_t * vertex= context->vertex;
  spt_vertex_t * next_vertex;

  // Get the end-side of the link
  if (!_link_get_next_elem(&vertex->elem, link, &next_elem))
    return;

  // Filter: stop if tail-end is outside of domain
  if ((next_elem.type == NODE) &&
      !igp_domain_contains_router(spt_comp->domain, next_elem.node)) {
    ___igp_debug("  skip link:%l [dst node outside domain]\n", link);
    return;
  }

  // Filter: cannot go back through incoming interface
  // TODO: should be changed to work with PTMP links !!!
  if ((context->iif != NULL) && (context->iif->dest.iface == link)) {
    ___igp_debug("  skip link:%l [oif == iif]\n", link);
    return;
  }

  // Filter: cannot traverse a link that is disabled or disconnected
  if (!net_iface_is_enabled(link) ||
      !net_iface_is_connected(link)) {
    ___igp_debug("  skip link:%l [link disabled or disconnected]\n", link);
    return;
  }

  // Filter: cannot traverse a link with a weight equal to 0 or max-metric
  if (vertex->elem.type != LINK) {
    weight= net_iface_get_metric(link, 0);
    if ((weight == 0) || (weight == IGP_MAX_WEIGHT)) {
      ___igp_debug("  skip link:%l [weight is 0 or max-metric]\n", link);
      return;
    }
  }

  // Compute weight to reach destination through this link
  // (no update if link is used to leave a subnet)
  if (vertex->elem.type == SUBNET) {
    weight= vertex->weight;
  } else {
    weight= net_igp_add_weights(vertex->weight, weight);
    if (weight == IGP_MAX_WEIGHT) {
      ___igp_debug("  skip link:%l [ path weight is max-metric]\n", link);
      return;
    }
  }

  ___igp_debug("  traverse link:%l (%w)\n", link, weight);
  
  // Update SPT
  next_vertex= _spt_update_node(spt_comp, next_elem, vertex, weight);
  if (next_vertex != NULL)
    _push(spt_comp, link, next_vertex);
}

// -----[ spt_bfs ]--------------------------------------------------
net_error_t spt_bfs(net_node_t * root, igp_domain_t * domain,
		    spt_t ** spt_ref)
{
  net_elem_t elem;               // current element: node or subnet
  net_iface_t * link;
  spt_context_t * context;
  unsigned int index;
  net_ifaces_t * ifaces= NULL;
  spt_vertex_t * prev_vertex= NULL;
  spt_comp_t spt_comp;

  // Create data structures (FIFO queue and list of visited nodes)
  spt_comp.fifo= fifo_create(100000, NULL);
  spt_comp.spt= spt_create(root);
  spt_comp.domain= domain;

  // Start with root node (src node)
  elem.type= NODE;
  elem.node= root;
  ___igp_debug("START root:%e\n", &elem);
  _push(&spt_comp, NULL, spt_comp.spt->root);

  // Breadth-First Search
  while (fifo_depth(spt_comp.fifo) > 0) {

    context= (spt_context_t *) fifo_pop(spt_comp.fifo);
    prev_vertex= context->vertex;
    elem= prev_vertex->elem;

    ___igp_debug("VISIT src:%e (%w)\n", &elem, prev_vertex->weight);

    ifaces= NULL;
    link= NULL;
    switch (elem.type) {
    case NODE:
      ifaces= elem.node->ifaces;
      break;
    case SUBNET:
      if (!subnet_is_transit(elem.subnet))
	break;
      ifaces= elem.subnet->ifaces;
      break;
    case LINK:
      link= elem.link->dest.iface;
      break;
    default: abort();
    }

    if (link != NULL) {
      _link_traverse(&spt_comp, context, link);
    } else if (ifaces != NULL) {
      // Traverse all the outbound links of the current element
      for (index= 0; index < net_ifaces_size(ifaces); index++) {
	link= net_ifaces_at(ifaces, index);
	_link_traverse(&spt_comp, context, link);
      }
    }
    FREE(context);
  }
  fifo_destroy(&spt_comp.fifo);

  *spt_ref= spt_comp.spt;
  return ESUCCESS;
}

typedef gds_radix_tree_t fib_t;

typedef struct _fib_comp_t {
  spt_vertex_t * vertex;
  rt_entry_t   * rtentry;
} _fib_comp_t;

static inline void _fib_comp_push(gds_stack_t * stack,
				  spt_vertex_t * vertex,
				  rt_entry_t * rtentry)
{
  _fib_comp_t * ctx= MALLOC(sizeof(_fib_comp_t));
  ctx->vertex= vertex;
  ctx->rtentry= rtentry;
  assert(stack_push(stack, ctx) >= 0);
}

static inline void _fib_comp_pop(gds_stack_t * stack,
				 spt_vertex_t ** vertex,
				 rt_entry_t ** rtentry)
{
  _fib_comp_t * ctx= (_fib_comp_t *) stack_pop(stack);
  assert(ctx != NULL);
  *vertex= ctx->vertex;
  *rtentry= ctx->rtentry;
  FREE(ctx);
}

// -----[ _spt_install_fib_entry ]-----------------------------------
static inline void _spt_install_fib_entry(fib_t * fib, ip_pfx_t prefix,
					  igp_weight_t weight,
					  rt_entry_t * rtentry)
{
  rt_info_t * rtinfo;

  ip_prefix_mask(&prefix);
  rtinfo= radix_tree_get_exact(fib, prefix.network, prefix.mask);
  if (rtinfo == NULL) {
    rtinfo= rt_info_create(prefix, weight, NET_ROUTE_IGP);
    assert(radix_tree_add(fib, prefix.network, prefix.mask,
			  rtinfo) >= 0);
  }

  if (rt_entries_add(rtinfo->entries, rtentry) < 0)
    rt_entry_destroy(&rtentry);
}

// -----[ _spt_install_fib ]-----------------------------------------
static inline
net_error_t _spt_install_fib(fib_t * fib,
			     spt_vertex_t * vertex,
			     rt_entry_t * rtentry)
{
  unsigned int index;
  net_iface_t * iface;
  net_ifaces_t * ifaces;
  igp_weight_t weight;

  switch (vertex->elem.type) {
  case NODE:
    ifaces= vertex->elem.node->ifaces;
    for (index= 0; index < net_ifaces_size(ifaces); index++) {
      iface= net_ifaces_at(vertex->elem.node->ifaces, index);
      if ((iface->type != NET_IFACE_LOOPBACK) &&
	  (iface->type != NET_IFACE_VIRTUAL))
	continue;
      weight= vertex->weight;
      weight= net_igp_add_weights(weight, net_iface_get_metric(iface, 0));
      _spt_install_fib_entry(fib, net_iface_dst_prefix(iface),
			     weight, rt_entry_add_ref(rtentry));
    }
    break;
  case LINK:
  case SUBNET:
    _spt_install_fib_entry(fib, vertex->id, vertex->weight,
			   rt_entry_add_ref(rtentry));
    break;
  default:
    abort();
  }
  return ESUCCESS;
}

// -----[ _spt_compute_fib ]-----------------------------------------
static inline
net_error_t _spt_compute_fib(net_node_t * node,
			     spt_t * spt, fib_t ** fib_ref)
{
  spt_vertex_t * vertex, * succ;
  gds_stack_t * stack= stack_create(10000);
  rt_entry_t * rtentry= NULL, * new_rtentry;
  fib_t * fib= radix_tree_create(32, NULL);
  unsigned int index;
  net_iface_t * iface;

  _fib_comp_push(stack, spt->root, NULL);
  
  // Traverse the DAG and compute the RT entry (oif + gw) for each vertex
  while (stack_depth(stack) > 0) {
    _fib_comp_pop(stack, &vertex, &rtentry);

    for (index= 0; index < spt_vertices_size(vertex->succs); index++) {
      succ= vertex->succs->data[index];
      
      // Compute RT entry for this node
      //   1). first RT entry on the path
      //   2.a). first (previous) RT entry was to a SUBNET, need to fix gateway
      //   2.b). copy previous RT entry
      if (rtentry == NULL) {
	assert(vertex->elem.type == NODE);
	iface= node_find_iface_to(vertex->elem.node, &succ->elem);
	assert(iface != NULL);
	new_rtentry= rt_entry_create(iface, IP_ADDR_ANY);
      } else {
	if ((rtentry->oif->type == NET_IFACE_PTMP) &&
	    (rtentry->gateway == IP_ADDR_ANY)) {
	  assert(vertex->elem.type == SUBNET);
	  iface= subnet_find_iface_to(vertex->elem.subnet, &succ->elem);
	  new_rtentry= rt_entry_create(rtentry->oif, iface->addr);
	} else
	  new_rtentry= rt_entry_add_ref(rtentry);
      }

      ___igp_debug("spt_compute_fib %v %r\n", succ, new_rtentry);

      _spt_install_fib(fib, succ, new_rtentry);
      
      _fib_comp_push(stack, succ, new_rtentry);
    }

    rt_entry_destroy(&rtentry);

  }

  stack_destroy(&stack);
  *fib_ref= fib;
  return ESUCCESS;
}

// ----- _igp_compute_prefix_for_each -------------------------------
static int _igp_compute_prefix_for_each(uint32_t key, uint8_t key_len,
					void * item, void * ctx)
{
  net_node_t * node= (net_node_t *) ctx;
  rt_info_t * rtinfo= (rt_info_t *) item;
  ip_pfx_t prefix= { .network= key, .mask= key_len };

  return rt_add_route(node->rt, prefix, rtinfo);
}

// ----- igp_compute_domain -----------------------------------------
int igp_compute_domain(igp_domain_t * domain, int keep_spt)
{
  gds_enum_t * routers= trie_get_enum(domain->routers);
  net_node_t * node;
  fib_t * fib= NULL;
  int result= ESUCCESS;

  while (enum_has_next(routers) && (result == ESUCCESS)) {
    node= *((net_node_t **) enum_get_next(routers));

    if (node->spt != NULL)
      spt_destroy(&node->spt);
    
    // Remove all IGP routes from node
    node_rt_del_route(node, NULL, NULL, NULL, NET_ROUTE_IGP);
    
    // Compute shortest-path tree (SPT)
    result= spt_bfs(node, domain, &node->spt);
    if (result != ESUCCESS)
      continue;

    // Compute the FIB based on the SPT
    result= _spt_compute_fib(node, node->spt, &fib);
    if (result != ESUCCESS)
      continue;
    
    // Add FIB content to node's FIB
    result= radix_tree_for_each(fib, _igp_compute_prefix_for_each, node);

    // Destroy the temporary FIB
    radix_tree_destroy(&fib);

    if (!keep_spt)
      spt_destroy(&node->spt);
  }
  enum_destroy(&routers);
  return result;
}
