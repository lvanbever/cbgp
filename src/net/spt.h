// ==================================================================
// @(#)spt.h
//
// Shortest-Path Tree (SPT) data structure
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/08/2008
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

#ifndef __NET_SPT_H__
#define __NET_SPT_H__

#include <assert.h>

#include <libgds/array.h>
#include <libgds/stream.h>

#include <net/net_types.h>
#include <net/routing.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ spt_vertex_dump ]----------------------------------------
  void spt_vertex_dump(gds_stream_t * stream, spt_vertex_t * vertex);
  // -----[ spt_dump ]-----------------------------------------------
  void spt_dump(gds_stream_t * stream, spt_t * spt);
  // -----[ spt_to_graphviz ]----------------------------------------
  void spt_to_graphviz(gds_stream_t * stream, spt_t * spt);
  
#ifdef _cplusplus
}
#endif

// -----[ net_elem_prefix ]------------------------------------------
static inline ip_pfx_t net_elem_prefix(net_elem_t * elem) {
  switch (elem->type) {
  case LINK:  return net_iface_dst_prefix(elem->link);
  case NODE:   return net_iface_id_addr(elem->node->rid);
  case SUBNET: return elem->subnet->prefix;
  default: abort();
  }
}

// -----[ spt_vertex_create ]----------------------------------------
static inline spt_vertex_t * spt_vertex_create(net_elem_t elem,
					       igp_weight_t weight)
{
  spt_vertex_t * vertex=
    (spt_vertex_t *) MALLOC(sizeof(spt_vertex_t));
  vertex->weight= weight;
  vertex->elem= elem;
  vertex->preds= spt_vertices_create(0);
  vertex->succs= spt_vertices_create(0);
  vertex->id= net_elem_prefix(&elem);
  ip_prefix_mask(&vertex->id);
  vertex->rtentries= NULL;
  return vertex;
}

// -----[ spt_vertex_destroy ]---------------------------------------
static inline void _spt_vertex_destroy(void ** item)
{
  spt_vertex_t * vertex= *((spt_vertex_t **) item);
  spt_vertices_destroy(&vertex->preds);
  spt_vertices_destroy(&vertex->succs);
  FREE(vertex);
}

// -----[ spt_vertex_add_pred ]--------------------------------------
static inline void spt_vertex_add_pred(spt_vertex_t * vertex,
				       spt_vertex_t * pred)
{
  // If first fails, don't try second as it should fail
  if (spt_vertices_add(vertex->preds, pred) < 0)
    return;
  // If first succeeds, second must also succeed
  assert(spt_vertices_add(pred->succs, vertex) >= 0);
}

// -----[ spt_vertex_clear_preds ]-----------------------------------
static inline void spt_vertex_clear_preds(spt_vertex_t * vertex)
{
  unsigned int index, index2;
  spt_vertex_t * pred;

  // Need to remove myself as successor in all my predecessors
  for (index= 0; index < spt_vertices_size(vertex->preds); index++) {
    pred= vertex->preds->data[index];
    // Fail if I'm not a successor of my predecessor
    assert(spt_vertices_index_of(pred->succs, vertex, &index2) >= 0);
    assert(spt_vertices_remove_at(pred->succs, index2) >= 0);
  }
  spt_vertices_set_size(vertex->preds, 0);
}

// -----[ spt_create ]-----------------------------------------------
static inline spt_t * spt_create(net_node_t * root) {
  net_elem_t elem;
  ip_pfx_t prefix= { .network=root->rid, .mask=32 };
  spt_t * spt= MALLOC(sizeof(spt_t));

  elem.type= NODE;
  elem.node= root;

  spt->tree= radix_tree_create(32, _spt_vertex_destroy);
  spt->root= spt_vertex_create(elem, 0);
  radix_tree_add(spt->tree, prefix.network, prefix.mask, spt->root);
  return spt;
}

// -----[ spt_destroy ]----------------------------------------------
static inline void spt_destroy(spt_t ** spt_ref) {
  spt_t * spt= *spt_ref;

  if (spt != NULL) {
    radix_tree_destroy(&spt->tree);
    FREE(spt);
    *spt_ref= NULL;
  }
}

// -----[ spt_get_vertex ]-------------------------------------------
static inline spt_vertex_t * spt_get_vertex(spt_t * spt, ip_pfx_t prefix) {
  return (spt_vertex_t *) radix_tree_get_exact(spt->tree,
					       prefix.network,
					       prefix.mask);
}

// -----[ spt_set_vertex ]---------------------------------------------
static inline int spt_set_vertex(spt_t * spt, spt_vertex_t * vertex) {
  return radix_tree_add(spt->tree, vertex->id.network, vertex->id.mask,
			vertex);
}

#endif /* __NET_SPT_H__ */
