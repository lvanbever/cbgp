// ==================================================================
// @(#)dp_rt.c
//
// This file contains the routines used to install/remove entries in
// the node's routing table when BGP routes are installed, removed or
// updated.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/01/2007
// $Id: dp_rt.c,v 1.7 2009-03-24 14:13:05 bqu Exp $
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
#include <stdlib.h>

#include <libgds/stream.h>
#include <net/link.h>
#include <net/node.h>
#include <bgp/as.h>
#include <bgp/dp_rt.h>
#include <bgp/route.h>

// ----- _bgp_router_rt_add_route_error -----------------------------
static void _bgp_router_rt_add_route_error(bgp_router_t * router,
					   bgp_route_t * route,
					   net_error_t error)
{
  if (stream_enabled(gdserr, STREAM_LEVEL_FATAL)) {
    stream_printf(gdserr, "Error: could not install BGP route in RT of ");
    bgp_router_dump_id(gdserr, router);
    stream_printf(gdserr, "\n  BGP route: ");
    route_dump(gdserr, route);
    stream_printf(gdserr, "\n  reason   : ");
    network_perror(gdserr, error);
    stream_printf(gdserr, "\n");
  }
  abort();
}

// ----- bgp_router_rt_add_route ------------------------------------
/**
 * This function inserts a BGP route into the node's routing
 * table. The route's next-hop is resolved before insertion.
 */
void bgp_router_rt_add_route(bgp_router_t * router, bgp_route_t * route)
{
  rt_info_t * old_rtinfo;
  int result;
  rt_info_t * rtinfo;

  // Get the previous route if it exists.
  old_rtinfo= rt_find_exact(router->node->rt, route->prefix,
			    NET_ROUTE_BGP);

  if (old_rtinfo != NULL) {
    // Remove the previous route.
    node_rt_del_route(router->node, &route->prefix,
		      NULL, NULL, NET_ROUTE_BGP);
  }

  // Add a route with BGP nexthop as gateway and no outgoing
  // interface. Upon forwarding, a recursive lookup will be
  // performed.
  rtinfo= rt_info_create(route->prefix, 0, NET_ROUTE_BGP);
  rt_entries_add(rtinfo->entries,
		 rt_entry_create(NULL, route->attr->next_hop));
  result= rt_add_route(router->node->rt, route->prefix, rtinfo);

  if (result)
    _bgp_router_rt_add_route_error(router, route, result);
}

// ----- bgp_router_rt_del_route ------------------------------------
/**
 * This function removes from the node's routing table the BGP route
 * towards the given prefix.
 */
void bgp_router_rt_del_route(bgp_router_t * router, ip_pfx_t prefix)
{
  /*rt_info_t * pRouteInfo;

  fprintf(stderr, "DEL ROUTE towards ");
  ip_prefix_dump(stderr, prefix);
  fprintf(stderr, " ");
  pRouteInfo= rt_find_exact(router->node->rt, prefix, NET_ROUTE_ANY);
  if (pRouteInfo != NULL) {
    net_route_info_dump(stderr, pRouteInfo);
    fprintf(stderr, "\n");
  } else {
    fprintf(stderr, "*** NONE ***\n");
    }*/

  assert(!node_rt_del_route(router->node, &prefix,
			    NULL, NULL, NET_ROUTE_BGP));
}
