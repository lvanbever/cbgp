// ==================================================================
// @(#)bgp_debug.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 09/04/2004
// $Id: bgp_debug.c,v 1.15 2009-03-24 14:10:35 bqu Exp $
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

#include <stdio.h>

#include <bgp/as.h>
#include <bgp/bgp_debug.h>
#include <bgp/dp_rules.h>
#include <bgp/peer.h>
#include <bgp/peer-list.h>
#include <bgp/rib.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>

// ----- bgp_debug_dp -----------------------------------------------
/**
 *
 */
void bgp_debug_dp(gds_stream_t * stream, bgp_router_t * router,
		  ip_pfx_t prefix)
{
  bgp_route_t * pOldRoute;
  bgp_routes_t * routes;
  bgp_peer_t * peer;
  bgp_route_t * route;
  unsigned int index;
  int iNumRoutes, iOldNumRoutes;
  int iRule;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  uint16_t uIndex;
  bgp_routes_t * routesRIBIn;
#endif

  stream_printf(stream, "Debug Decision Process\n");
  stream_printf(stream, "----------------------\n");
  stream_printf(stream, "AS%u, ", router->asn);
  bgp_router_dump_id(stream, router);
  stream_printf(stream, ", ");
  ip_prefix_dump(stream, prefix);
  stream_printf(stream, "\n\n");

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  pOldRoute= rib_find_one_exact(router->loc_rib, prefix, NULL);
#else
  pOldRoute= rib_find_exact(router->loc_rib, prefix);
#endif

  /* Display current best route: */
  stream_printf(stream, "[ Current Best route: ]\n");
  if (pOldRoute != NULL) {
    route_dump(stream, pOldRoute);
    stream_printf(stream, "\n");
  } else
    stream_printf(stream, "<none>\n");

  stream_printf(stream, "\n");

  stream_printf(stream, "[ Eligible routes: ]\n");
  /* If there is a LOCAL route: */
  if ((pOldRoute != NULL) &&
      route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {
    route_dump(stream, pOldRoute);
    stream_printf(stream, "\n");
  }

  // Build list of eligible routes received from peers
  routes= routes_list_create(ROUTES_LIST_OPTION_REF);
  for (index= 0; index < bgp_peers_size(router->peers); index++) {
    peer= bgp_peers_at(router->peers, index);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    routesRIBIn = rib_find_exact(peer->adj_rib[RIB_IN], prefix);
    if (routesRIBIn != NULL) {
      for (uIndex = 0; uIndex < routes_list_get_num(routesRIBIn); uIndex++) {
	route = routes_list_get_at(routesRIBIn, uIndex);
#else
    route= rib_find_exact(peer->adj_rib[RIB_IN], prefix);
#endif
    if ((route != NULL) &&
	(route_flag_get(route, ROUTE_FLAG_FEASIBLE)))
      routes_list_append(routes, route);
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
      }
    }
#endif
  }

  routes_list_dump(stream, routes);
  iNumRoutes= ptr_array_length(routes);
  iOldNumRoutes= iNumRoutes;

  stream_printf(stream, "\n");

  // Start decision process
  if ((pOldRoute == NULL) ||
      !route_flag_get(pOldRoute, ROUTE_FLAG_INTERNAL)) {

    for (iRule= 0; iRule < DP_NUM_RULES; iRule++) {
      if (iNumRoutes <= 1)
	break;
      iOldNumRoutes= iNumRoutes;
      stream_printf(stream, "[ %s ]\n", DP_RULES[iRule].name);

      DP_RULES[iRule].rule(router, routes);

      iNumRoutes= ptr_array_length(routes);
      if (iNumRoutes < iOldNumRoutes)
	routes_list_dump(stream, routes);
    }

  } else
    stream_printf(stream, "*** local route is preferred ***\n");

  stream_printf(stream, "\n");

  stream_printf(stream, "[ Best route ]\n");
  if (iNumRoutes > 0)
    routes_list_dump(stream, routes);
  else
    stream_printf(stream, "<none>\n");
  
  routes_list_destroy(&routes);

}

