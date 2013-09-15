// ==================================================================
// @(#)rib.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
//
// @date 04/12/2002
// $Id: rib.c,v 1.12 2009-03-24 15:50:17 bqu Exp $
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

#include <assert.h>

#include <libgds/stream.h>

#include <bgp/as.h>
#include <bgp/rib.h>
#include <bgp/route.h>
#include <bgp/routes_list.h>

// -----[ _rib_route_destroy ]---------------------------------------
static void _rib_route_destroy(void ** item_ref)
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  bgp_routes_t ** routes_ref= (bgp_routes_t **) item_ref;
  routes_list_destroy(routes_ref);
#else
  bgp_route_t ** route_ref= (bgp_route_t **) item_ref;
  route_destroy(route_ref);
#endif
}

typedef struct {
  FRadixTreeForEach for_each;
  void * ctx;
} bgp_rib_ctx_t;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
//TODO : propagate the result!!!
static int _rib_trie_for_each(trie_key_t key, trie_key_len_t key_len,
			      void * item, void * ctx)
{
  unsigned int index;
  bgp_routes_t * routes = (bgp_routes_t *) item;
  bgp_rib_ctx_t * rib_ctx= (bgp_rib_ctx_t *) ctx;

  for (index = 0; index < routes_list_get_num(routes); index++) {
    rib_ctx->for_each(key, key_len, 
		      routes_list_get_at(routes, index), 
		      rib_ctx->ctx);
  }
  return 0;
}
#endif

// ----- rib_create -------------------------------------------------
bgp_rib_t * rib_create(uint8_t options)
{
  gds_trie_destroy_f destroy= _rib_route_destroy;

  if (options & RIB_OPTION_ALIAS)
    destroy = NULL;

  return (bgp_rib_t *) trie_create(destroy);
}

// ----- rib_destroy ------------------------------------------------
void rib_destroy(bgp_rib_t ** rib_ref)
{
  trie_destroy(rib_ref);
}

// ----- rib_find_best ----------------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
bgp_routes_t * rib_find_best(bgp_rib_t * rib, ip_pfx_t prefix)
#else
bgp_route_t * rib_find_best(bgp_rib_t * rib, ip_pfx_t prefix)
#endif
{
#if defined __EXPERIMENTAL_WALTON__
  return (bgp_routes_t *) trie_find_best(rib,
					 prefix.network,
					 prefix.mask);
#else
  return (bgp_route_t *) trie_find_best(rib,
					prefix.network,
					prefix.mask);
#endif
}


// ----- rib_find_exact ---------------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
bgp_routes_t * rib_find_exact(bgp_rib_t * rib, ip_pfx_t prefix)
#else
bgp_route_t * rib_find_exact(bgp_rib_t * rib, ip_pfx_t prefix)
#endif
{
#if defined __EXPERIMENTAL_WALTON__
  return (bgp_routes_t *) trie_find_exact(rib,
					  prefix.network,
					  prefix.mask);
#else
  return (bgp_route_t *) trie_find_exact(rib,
					 prefix.network,
					 prefix.mask);
#endif
}

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
// ----- rib_find_one_best -------------------------------------------
/**
 *
 */
bgp_route_t * rib_find_one_best(bgp_rib_t * rib, ip_pfx_t prefix)
{
  bgp_routes_t * routes = rib_find_best(rib, prefix);

  if (routes != NULL) {
    assert(routes_list_get_num(routes) == 0 
	|| routes_list_get_num(routes) == 1);
    if (routes_list_get_num(routes) == 1)
      return routes_list_get_at(routes, 0);
  }
  return NULL;
}

// ----- rib_find_one_exact -----------------------------------------
/**
 *
 */
bgp_route_t * rib_find_one_exact(bgp_rib_t * rib, ip_pfx_t prefix, 
				 net_addr_t * next_hop)
{
  unsigned int index;
  bgp_routes_t * routes = rib_find_exact(rib, prefix);
  bgp_route_t * pRoute;

  if (routes != NULL) {
    if (next_hop != NULL) {
  //LOG_DEBUG("removal : \n");
  //LOG_ENABLED_DEBUG() ip_prefix_dump(log_get_stream(pMainLog), sPrefix);
  //LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), *tNextHop);
  //LOG_DEBUG("\n");
      for (index= 0; index < routes_list_get_num(routes); index++) {
	route= routes_list_get_at(routes, index);
	if (route_get_nexthop(route) == *next_hop)
	  return route;
      }
    //the next hop is not known ... we must be sure there is only one route in
    //the RIB
    } else {
      assert(routes_list_get_num(routes) == 0 
	  || routes_list_get_num(routes) == 1);
      if (routes_list_get_num(routes) == 1)
	return routes_list_get_at(routes, 0);
    }
  }
  return NULL;
}

// ----- _rib_insert_new_route ---------------------------------------
/**
 *
 */
int _rib_insert_new_route(bgp_rib_t * rib, bgp_route_t * route)
{
  bgp_routes_t * routes;
  //TODO: Verify that the option is good!
  routes= routes_list_create(0);
  routes_list_append(routes, route);
  return trie_insert(rib, route->prefix.network,
		     route->prefix.mask, routes, 0);
}

// ----- _rib_replace_route ------------------------------------------
/**
 *
 */
int _rib_replace_route(bgp_rib_t * rib, bgp_routes_t * routes,
		       bgp_route_t * route)
{
  unsigned int index;
  bgp_route_t * route_seek;

  for (index= 0; index < routes_list_get_num(pRoutes); index++) {
    route_seek= routes_list_get_at(routes, index);
    //It's the same route ... OK ... update it!
    if (route_get_nexthop(route_seek) == route_get_nexthop(route)) {
      routes_list_remove_at(routes, index);
      routes_list_append(routes, route);
      return 0;
    }
  }
  //The route has not been found ... insert it
  routes_list_append(routes, route);
  return 0;
}

// ----- _rib_remove_route -------------------------------------------
/**
 *
 */
int _rib_remove_route(bgp_routes_t * routes, net_addr_t next_hop)
{
  unsigned int index;
  bgp_route_t * route_seek;

  for (index= 0; index < routes_list_get_num(pRoutes); index++) {
    route_seek= routes_list_get_at(routes, index);
    if (route_get_nexthop(route_seek) == next_hop) {
      //LOG_DEBUG("remove route ");
      //LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), tNextHop);
      //LOG_DEBUG("\n");
      routes_list_remove_at(routes, index);
      return 0;
    } 
   // else {
   //   LOG_DEBUG("not removed ");
   //   LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), tNextHop);
   //   LOG_DEBUG("\n");
   // }

      
  }
  return 0;
}
#endif

// ----- rib_add_route ----------------------------------------------
/**
 * Add a new route to the RIB
 *
 * Returns
 *   0 in case of success
 *  <0 in case of failure
 */
int rib_add_route(bgp_rib_t * rib, bgp_route_t * route)
{
#if defined __EXPERIMENTAL_WALTON__
  bgp_routes_t * routes= trie_find_exact(rib,
					 route->prefix.network,
					 route->prefix.mask);
  if (routes == NULL) {
    return _rib_insert_new_route(rib, route);
  } else {
    return _rib_replace_route(rib, routes, route);
  }
#else
  int result= trie_insert(rib, route->prefix.network,
			  route->prefix.mask, route,
			  TRIE_INSERT_OR_REPLACE);
  return (result==TRIE_SUCCESS?0:-1);
#endif
}

// ----- rib_replace_route ------------------------------------------
/**
 *
 */
int rib_replace_route(bgp_rib_t * rib, bgp_route_t * route)
{
#if defined __EXPERIMENTAL_WALTON__
  bgp_routes_t * routes= trie_find_exact(rib,
					 route->prefix.network,
					 route->prefix.mask);
  if (routes == NULL) {
    return _rib_insert_new_route(rib, route);
  } else {
    return _rib_replace_route(rib, routes, route);
  }
#else
  return trie_insert(rib, route->prefix.network,
		     route->prefix.mask, route,
		     TRIE_INSERT_OR_REPLACE);
#endif
}

// ----- rib_remove_route -------------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
int rib_remove_route(bgp_rib_t * rib, ip_pfx_t prefix, net_addr_t * next_hop)
#else
int rib_remove_route(bgp_rib_t * rib, ip_pfx_t prefix)
#endif
{
#if defined __EXPERIMENTAL_WALTON__
  bgp_routes_t * routes = trie_find_exact(rib,
				      prefix.network,
				      prefix.mask);
  if (routes != NULL) {
    // Next-Hop == NULL => corresponds to an explicit withdraw!
    // we may destroy the list directly.
    if (next_hop != NULL) {
      _rib_remove_route(routes, *next_hop);
      if (routes_list_get_num(routes) == 0) {
        return trie_remove(rib, prefix.network,
			   prefix.mask);
      }
    } else {
      return trie_remove(rib, prefix.network,
			 prefix.mask);
    }
  }
  return 0;
#else
  return trie_remove(rib,
		     prefix.network,
		     prefix.mask);
#endif
}

// ----- rib_for_each -----------------------------------------------
/**
 *
 */
int rib_for_each(bgp_rib_t * rib, FRadixTreeForEach for_each,
		 void * ctx)
{

#if defined __EXPERIMENTAL_WALTON__
  bgp_rib_ctx_t rib_ctx= {
    .for_each= for_each,
    .ctx=ctx
  };
  return trie_for_each(rib, _rib_trie_for_each, &rib_ctx);
#else
  return trie_for_each(rib, for_each, ctx);
#endif
}
