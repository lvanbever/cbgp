// ==================================================================
// @(#)ip_trace.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 01/03/2008
// $Id: ip_trace.c,v 1.7 2009-08-31 09:48:28 bqu Exp $
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
#include <libgds/array.h>

#include <net/iface.h>
#include <net/ip_trace.h>
#include <net/node.h>

// -----[ _ip_trace_item_node ]---------------------------------------
static inline ip_trace_item_t *
_ip_trace_item_node(net_node_t * node,
		   net_iface_t * iif,
		   net_iface_t * oif)
{
  ip_trace_item_t * item= (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  item->elt.type= NODE;
  item->elt.node= node;
  item->iif= iif;
  item->oif= oif;
  return item;
}

// -----[ _ip_trace_item_subnet ]-------------------------------------
static inline ip_trace_item_t *
_ip_trace_item_subnet(net_subnet_t * subnet)
{
  ip_trace_item_t * item= (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  item->elt.type= SUBNET;
  item->elt.subnet= subnet;
  item->iif= NULL;
  item->oif= NULL;
  return item;
}

// -----[ _ip_trace_item_trace ]-------------------------------------
static inline ip_trace_item_t *
_ip_trace_item_trace(ip_trace_t * trace)
{
  ip_trace_item_t * item= (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  item->elt.type= TRACE;
  item->elt.trace= trace;
  item->iif= NULL;
  item->oif= NULL;
  return item;
}

// -----[ _ip_trace_item_copy ]--------------------------------------
static inline ip_trace_item_t *
_ip_trace_item_copy(ip_trace_item_t * item)
{
  ip_trace_item_t * new_item=
    (ip_trace_item_t *) MALLOC(sizeof(ip_trace_item_t));
  new_item->iif= item->iif;
  new_item->oif= item->oif;
  switch (item->elt.type) {
  case TRACE:
    new_item->elt.type= item->elt.type;
    new_item->elt.trace= ip_trace_copy(item->elt.trace);
    break;
  default:
    new_item->elt.type= item->elt.type;
    new_item->elt= item->elt;
  }
  return new_item;
}

// -----[ _ip_trace_item_destroy ]-----------------------------------
static void _ip_trace_item_destroy(void * item, const void * ctx)
{
  ip_trace_item_t * trace_item= *((ip_trace_item_t **) item);
  if (trace_item->elt.type == TRACE) {
    ip_trace_t * trace= (ip_trace_t *) trace_item->elt.trace;
    ip_trace_destroy(&trace);
  }
  FREE(trace_item);
}

// -----[ ip_trace_create ]------------------------------------------
ip_trace_t * ip_trace_create()
{
  ip_trace_t * trace= (ip_trace_t *) MALLOC(sizeof(ip_trace_t));
  trace->items= ptr_array_create(0, NULL, _ip_trace_item_destroy, NULL);
  trace->delay= 0;
  trace->capacity= NET_LINK_MAX_CAPACITY;
  trace->weight= 0;
  trace->status= ENET_NO_REPLY;
  return trace;
}

// -----[ ip_trace_destroy ]-----------------------------------------
void ip_trace_destroy(ip_trace_t ** trace_ref)
{
  if (*trace_ref != NULL) {
    ptr_array_destroy(&(*trace_ref)->items);
    FREE(*trace_ref);
    *trace_ref= NULL;
  }
}

// -----[ _ip_trace_add ]--------------------------------------------
static inline ip_trace_item_t * _ip_trace_add(ip_trace_t * trace,
					      ip_trace_item_t * item)
{
  assert(ptr_array_add(trace->items, &item) >= 0);
  return item;
}

// -----[ ip_trace_add_node ]----------------------------------------
ip_trace_item_t * ip_trace_add_node(ip_trace_t * trace,
				    net_node_t * node,
				    net_iface_t * iif,
				    net_iface_t * oif)
{
  return _ip_trace_add(trace, _ip_trace_item_node(node, iif, oif));
}

// -----[ ip_trace_add_subnet ]--------------------------------------
ip_trace_item_t * ip_trace_add_subnet(ip_trace_t * trace,
				      net_subnet_t * subnet)
{
  return _ip_trace_add(trace, _ip_trace_item_subnet(subnet));
}

// -----[ ip_trace_add_trace ]---------------------------------------
ip_trace_item_t * ip_trace_add_trace(ip_trace_t * trace,
				     ip_trace_t * trace_item)
{
  return _ip_trace_add(trace, _ip_trace_item_trace(trace_item));
}


// -----[ ip_trace_search ]------------------------------------------
int ip_trace_search(ip_trace_t * trace, net_node_t * node)
{
  unsigned int index;
  ip_trace_item_t * item;
  for (index= 0; index < ip_trace_length(trace); index++) {
    item= ip_trace_item_at(trace, index);
    if ((item->elt.type == NODE) && (item->elt.node == node))
      return 1;
  }
  return 0;
}

// -----[ ip_trace_dump ]--------------------------------------------
void ip_trace_dump(gds_stream_t * stream, ip_trace_t * trace,
		   uint8_t options)
{
  ip_trace_item_t * item;
  int trace_length= 0;
  unsigned int index;
  int space;

  if (options & IP_TRACE_DUMP_LENGTH) {
    // Compute trace length (only nodes)
    for (index= 0; index < ip_trace_length(trace); index++) {
      switch (ip_trace_item_at(trace, index)->elt.type) {
      case NODE:
	trace_length++;
	break;
      case SUBNET:
	if (options & IP_TRACE_DUMP_SUBNETS)
	  trace_length++;
	break;
      case TRACE:
	trace_length++;
	break;
      default:
	abort();
      }
    }

    // Dump trace length to output
    stream_printf(stream, "\t%u\t", trace_length);
  }

  // Dump each (node) hop to output
  space= 0;
  for (index= 0; index < ip_trace_length(trace); index++) {
    if (space)
	stream_printf(stream, " ");
    item= ip_trace_item_at(trace, index);
    space= 0;
    switch (item->elt.type) {
    case NODE:
      //if (item->iif != NULL) {
      //  stream_printf(stream, "[");
      //  net_iface_dump_id(stream, item->iif);
      //  stream_printf(stream, "]");
      //}

      node_dump_id(stream, item->elt.node);

      //if (item->oif != NULL) {
      //  stream_printf(stream, "[");
      //  net_iface_dump_id(stream, item->oif);
      //  stream_printf(stream, "]");
      //}

      space= 1;
      break;
    case SUBNET:
      if (options & IP_TRACE_DUMP_SUBNETS) {
	ip_prefix_dump(stream, item->elt.subnet->prefix);
	space= 1;
      }
      break;
    case TRACE:
      stream_printf(stream, "[");
      if (item->elt.trace != NULL)
	ip_trace_dump(stream, (ip_trace_t *) item->elt.trace, 0);
      else
	stream_printf(stream, "null");
      stream_printf(stream, "]");
      space= 1;
      break;
    default:
      cbgp_fatal("invalid network-element type (%d)\n", item->elt.type);
    }
  }
}

// -----[ ip_trace_copy ]--------------------------------------------
ip_trace_t * ip_trace_copy(ip_trace_t * trace)
{
  unsigned int index;
  ip_trace_t * new_trace= ip_trace_create();
  ip_trace_item_t *item, * new_item;

  // Copy options
  new_trace->delay= trace->delay;
  new_trace->capacity= trace->capacity;
  new_trace->weight= trace->weight;

  // Copy items
  for (index= 0; index < ip_trace_length(trace); index++) {
    item= ip_trace_item_at(trace, index);
    new_item= _ip_trace_item_copy(item);
    _ip_trace_add(new_trace, new_item);
  }

  return new_trace;
}

