// ==================================================================
// @(#)record-route.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/05/2007
// $Id: record-route.c,v 1.6 2009-06-25 14:27:58 bqu Exp $
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
# include <config.h>
#endif

#include <libgds/stream.h>

#include <net/node.h>
#include <net/prefix.h>
#include <net/protocol.h>
#include <bgp/attr/path.h>
#include <bgp/record-route.h>
#include <bgp/rib.h>

// -----[ bgp_record_route ]-----------------------------------------
int bgp_record_route(bgp_router_t * router,
		     ip_pfx_t prefix,
		     bgp_path_t ** path_ref,
		     uint8_t options)
{
  bgp_router_t * cur_router= router;
  bgp_router_t * prev_router= NULL;
  bgp_route_t * route;
  bgp_path_t * path= path_create();
  net_node_t * node;
  net_protocol_t * protocol;
  int result= AS_RECORD_ROUTE_UNREACH;
  network_t * network= router->node->network;

  *path_ref= NULL;

  while (cur_router != NULL) {
    
    // Is there, in the current node, a BGP route towards the given
    // prefix ?
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    route= rib_find_one_best(cur_router->loc_rib, prefix);
#else
    if (options & AS_RECORD_ROUTE_OPT_EXACT_MATCH)
      route= rib_find_exact(cur_router->loc_rib, prefix);
    else
      route= rib_find_best(cur_router->loc_rib, prefix);
#endif
    if (route != NULL) {
      
      // Record current node's AS-Num ??
      if ((prev_router == NULL) ||
	  ((options & AS_RECORD_ROUTE_OPT_PRESERVE_DUPS) ||
	   (prev_router->asn != cur_router->asn))) {
	if (path_append(&path, cur_router->asn) < 0) {
	  result= AS_RECORD_ROUTE_TOO_LONG;
	  break;
	}
      }
      
      // If the route's next-hop is this router, then the function
      // terminates.
      if (node_has_address(cur_router->node, route->attr->next_hop)) {
	result= AS_RECORD_ROUTE_SUCCESS;
	break;
      }
      
      // Otherwize, looks for next-hop router
      node= network_find_node(network, route->attr->next_hop);
      if (node == NULL)
	break;
      
      // Get the current node's BGP instance
      protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
      if (protocol == NULL)
	break;
      prev_router= cur_router;
      cur_router= (bgp_router_t *) protocol->handler;
      
    } else
      break;
  }
  *path_ref= path;
  return result;
}

// -----[ bgp_dump_recorded_route ]----------------------------------
/**
 * This function dumps the result of a call to
 * 'bgp_router_record_route'.
 */
void bgp_dump_recorded_route(gds_stream_t * stream, bgp_router_t * router,
			     ip_pfx_t prefix, bgp_path_t * path,
			     int result)
{
  // Display record-route results
  node_dump_id(stream, router->node);
  stream_printf(stream, "\t");
  ip_prefix_dump(stream, prefix);
  stream_printf(stream, "\t");
  switch (result) {
  case AS_RECORD_ROUTE_SUCCESS: stream_printf(stream, "SUCCESS"); break;
  case AS_RECORD_ROUTE_TOO_LONG: stream_printf(stream, "TOO_LONG"); break;
  case AS_RECORD_ROUTE_UNREACH: stream_printf(stream, "UNREACHABLE"); break;
  default:
    stream_printf(stream, "UNKNOWN_ERROR");
  }
  stream_printf(stream, "\t");
  path_dump(stream, path, 0);
  stream_printf(stream, "\n");

  stream_flush(stream);
}
