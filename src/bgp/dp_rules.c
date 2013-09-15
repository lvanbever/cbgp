// ==================================================================
// @(#)dp_rules.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/11/2002
// $Id: dp_rules.c,v 1.23 2009-08-04 06:06:10 bqu Exp $
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
#include <string.h>
#include <libgds/stream.h>

#include <bgp/as.h>
#include <bgp/dp_rules.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/network.h>
#include <net/routing.h>

// -----[ decision process ]-----------------------------------------
struct dp_rule_t DP_RULES[DP_NUM_RULES]= {
  { "Highest LOCAL-PREF", dp_rule_highest_pref },
  { "Shortest AS-PATH", dp_rule_shortest_path },
  { "Lowest ORIGIN", dp_rule_lowest_origin },
  { "Lowest MED", dp_rule_lowest_med },
  { "eBGP over iBGP", dp_rule_ebgp_over_ibgp },
  { "Nearest NEXT-HOP", dp_rule_nearest_next_hop },
  { "Lowest ROUTER-ID", dp_rule_lowest_router_id },
  { "Shortest CLUSTER-ID-LIST", dp_rule_shortest_cluster_list },
  { "Lowest neighbor address", dp_rule_lowest_neighbor_address },
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  { "Lowest Nexthop", dp_rule_lowest_nh },
  { "Lowest Path", dp_rule_lowest_path },
#endif
};

typedef struct _options_t {
  bgp_med_type_t med_type;
} _options_t;
static _options_t _default_options= {
  .med_type= BGP_MED_TYPE_DETERMINISTIC,
};

// -----[ dp_rules_options_set_med_type ]----------------------------
void dp_rules_options_set_med_type(bgp_med_type_t med_type)
{
  _default_options.med_type= med_type;
}

// -----[ dp_rules_str2med_type ]------------------------------------
net_error_t dp_rules_str2med_type(const char * str, bgp_med_type_t * med_type)
{
  if (!strcmp(str, "deterministic")) {
    *med_type= BGP_MED_TYPE_DETERMINISTIC;
    return ESUCCESS;
  } else if (!strcmp(str, "always-compare")) {
    *med_type= BGP_MED_TYPE_ALWAYS_COMPARE;
    return ESUCCESS;
  }
  return EUNSPECIFIED;
}

// -----[ function prototypes ]--------------------------------------
static uint32_t _dp_rule_igp_cost(bgp_router_t * router,
				  net_addr_t next_hop);

// ----- bgp_dp_rule_generic ----------------------------------------
/**
 * Optimized generic decision process rule. This function takes a
 * route comparison function as argument, process the best route and
 * eliminates all the routes that are not as best as the best route.
 *
 * PRE: fCompare must not be NULL
 */
static inline void bgp_dp_rule_generic(bgp_router_t * router,
				       bgp_routes_t * routes,
				       FDPRouteCompare compare)
{
  unsigned int index;
  int result;
  bgp_route_t * best_route= NULL;
  bgp_route_t * route;

  // Determine the best route. This loop already eliminates all the
  // routes that are not as best as the current best route.
  index= 0;
  while (index < bgp_routes_size(routes)) {
    route= bgp_routes_at(routes, index);
    if (best_route != NULL) {
      result= compare(router, best_route, route);
      if (result < 0) {
	best_route= route;
	index++;
      } else if (result > 0) {
	routes_list_remove_at(routes, index);
      } else
	index++;
    } else {
      best_route= route;
      index++;
    }
  }

  // Eliminates remaining routes that are not as best as the best
  // route.
  index= 0;
  while (index < bgp_routes_size(routes)) {
    if (compare(router, bgp_routes_at(routes, index), best_route) < 0)
      routes_list_remove_at(routes, index);
    else
      index++;
  }
}

// ----- bgp_dp_rule_local_route_cmp --------------------------------
static int bgp_dp_rule_local_route_cmp(bgp_router_t * router,
				       bgp_route_t * route1,
				       bgp_route_t * route2)
{
  // If both routes are INTERNAL, returns 0
  // If route1 is INTERNAL and route2 is not, returns 1
  // Otherwize, returns -1
  return (route_flag_get(route1, ROUTE_FLAG_INTERNAL)-
	  route_flag_get(route2, ROUTE_FLAG_INTERNAL));
}

// ----- bgp_dp_rule_local ------------------------------------------
/**
 * This function prefers routes that are locally originated,
 * i.e. inserted with the bgp_router_add_network() function.
 */
void bgp_dp_rule_local(bgp_router_t * router,
		       bgp_routes_t * routes)
{
  if (router->asn == 1) {
    stream_printf(gdsout, "before:\n");
    routes_list_dump(gdsout, routes);
  }

  bgp_dp_rule_generic(router, routes,
		      bgp_dp_rule_local_route_cmp);

  if (router->asn == 1) {
    stream_printf(gdsout, "after:\n");
    routes_list_dump(gdsout, routes);
  }
}

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
SIntArray * dp_rule_nexthop_counter_create(){
  return int_array_create(ARRAY_OPTION_UNIQUE | ARRAY_OPTION_SORTED);
}

int dp_rule_nexthop_add(SIntArray * piNextHop, net_addr_t tNextHop)
{
  return int_array_add(piNextHop, &tNextHop);
}

int dp_rule_nexthop_get_count(SIntArray * piNextHop)
{
  return int_array_length(piNextHop);
}

void dp_rule_nexthop_destroy(SIntArray ** ppiNextHop)
{
  int_array_destroy(ppiNextHop);
}

int dp_rule_no_selection(bgp_router_t * router, bgp_routes_t * routes)
{
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  unsigned int index;
  int iNextHopCount;

  for (index = 0; index < bgp_routes_size(routes); index++) {
    dp_rule_nexthop_add(piNextHopCounter,
			route_get_nexthop(bgp_routes_at(routes, index)));
  }
    
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;

}

int dp_rule_lowest_nh(bgp_router_t * router, bgp_routes_t * routes)
{
  net_addr_t tLowestNextHop= MAX_ADDR;
  net_addr_t tNH;
  bgp_route_t * route;
  unsigned int index;
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;

  // Calculate lowest NextHop
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index); 
    tNH= route_get_nexthop(route); 
    if (tNH < tLowestNextHop)
      tLowestNextHop= tNH;
  }

  // Discard routes from neighbors with an higher NextHop
  index= 0;
  while (index < bgp_routes_size(routes)) {
    route= bgp_routes_at(routes, index);
    tNH= route_get_nexthop(route); 
    if (tNH > tLowestNextHop)
      ptr_array_remove_at(routes, index);
    else
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
  }

  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
}

int dp_rule_lowest_path(bgp_router_t * router, bgp_routes_t * routes)
{ 
  unsigned int index;
  bgp_route_t * route;
  bgp_path_t * tPath;
  bgp_path_t * tLowestPath;
  bgp_path_t * tPathMax;
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
  bgp_route_t * pSavedRoute= NULL;
    
  tPathMax = path_max_value();
  tLowestPath = tPathMax;
  
  // Calculate lowest AS-PATH
  for (index = 0; index < bgp_routes_size(routes); index++) {
    route = bgp_routes_at(routes, index);
    tPath = route_get_path(route); 
    if (path_comparison(tPath, tLowestPath) <= 0) {
      tLowestPath = tPath;
      pSavedRoute = route;
    }
  } 

  assert(pSavedRoute != NULL);
    
  dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(pSavedRoute));
  path_destroy(&tPathMax);
  
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
}

#endif
// ----- dp_rule_highest_pref ---------------------------------------
/**
 * Remove routes that do not have the highest LOCAL-PREF.
 */
int dp_rule_highest_pref(bgp_router_t * router, bgp_routes_t * routes)
{
  unsigned int index;
  bgp_route_t * route;
  uint32_t uHighestPref= 0;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif
  
  // Calculate highest degree of preference
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);
    if (route_localpref_get(route) > uHighestPref)
      uHighestPref= route_localpref_get(route);
  }
  // Exclude routes with a lower preference
  index= 0;
  while (index < bgp_routes_size(routes)) {
    if (route_localpref_get(bgp_routes_at(routes, index))
	< uHighestPref)
      ptr_array_remove_at(routes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
  }

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_shortest_path --------------------------------------
/**
 * Remove routes that do not have the shortest AS-PATH.
 */
int dp_rule_shortest_path(bgp_router_t * router, bgp_routes_t * routes)
{
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  unsigned int index;
  bgp_route_t * route;
  uint8_t uMinLen= 255;

  // Calculate shortest AS-Path length
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);
    if (route_path_length(route) < uMinLen)
      uMinLen= route_path_length(route);
  }
  // Exclude routes with a longer AS-Path
  index= 0;
  while (index < bgp_routes_size(routes)) {
    if (route_path_length(bgp_routes_at(routes, index)) > uMinLen)
      ptr_array_remove_at(routes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_origin --------------------------------------
/**
 * Remove routes that do not have the lowest ORIGIN (IGP < EGP <
 * INCOMPLETE).
 */
int dp_rule_lowest_origin(bgp_router_t * router, bgp_routes_t * routes)
{
  unsigned int index;
  bgp_route_t * route;
  bgp_origin_t tLowestOrigin= BGP_ORIGIN_INCOMPLETE;

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest Origin
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);
    if (route_get_origin(route) < tLowestOrigin)
      tLowestOrigin= route_get_origin(route);
  }
  // Exclude routes with a greater Origin
  index= 0;
  while (index < bgp_routes_size(routes)) {
    if (route_get_origin(bgp_routes_at(routes, index))	> tLowestOrigin)
      ptr_array_remove_at(routes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      iidex++;
    }
#else
      index++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_med -----------------------------------------
/**
 * Remove routes that do not have the lowest MED.
 *
 * Note: MED comparison can be done in two different ways. With the
 * deterministic way, MED values are only compared between routes
 * received from teh same neighbor AS. With the always-compare way,
 * MED can always be compared.
 */
int dp_rule_lowest_med(bgp_router_t * router, bgp_routes_t * routes)
{
  unsigned int index, index2;
  int iLastAS;
  bgp_route_t * route, * route2;
  uint32_t uMinMED= UINT_MAX;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // WARNING: MED can only be compared between routes from the same AS
  // !!!
  // This constraint may be changed by two options :
  //   1) with the option always-compare-med. 
  //     In this case, the med is always compared between two routes even if
  //     they're not announced by the sames AS.
  //   2) the deterministic MED Cisco option.
  //     The router has to group each route by AS. Secondly, it has to
  //     elect the best route of each group and then continue to apply
  //     the remaining rules of the decision process to these best
  //     routes.
  //
  // WARNING : if the two options are set, the MED of the best routes of each
  // group has to be compared too.

  switch (_default_options.med_type) {
  case BGP_MED_TYPE_ALWAYS_COMPARE:
   
    // Calculate lowest MED
    for (index= 0; index < bgp_routes_size(routes); index++) {
      route= bgp_routes_at(routes, index);
      if (route_med_get(route) < uMinMED)
	uMinMED= route_med_get(route);
    }
    // Discard routes with an higher MED
    index= 0;
    while (index < bgp_routes_size(routes)) {
      if (route_med_get(bgp_routes_at(routes, index)) > uMinMED)
	ptr_array_remove_at(routes, index);
      else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
    }
    break;

  case BGP_MED_TYPE_DETERMINISTIC:

    index= 0;
    while (index < bgp_routes_size(routes)) {
      route= bgp_routes_at(routes, index);
      
      if (route != NULL) {
      
	iLastAS= route_path_last_as(route);
	
	index2= index+1;
	while (index2 < bgp_routes_size(routes)) {
	  route2= bgp_routes_at(routes, index2);
	  
	  if (route2 != NULL) {
	    if (iLastAS == route_path_last_as(route2)) {
	      if (route->attr->med < route2->attr->med) {
		routes->data[index2]= NULL;
	      } else if (route->attr->med > route2->attr->med) {
		routes->data[index]= NULL;
		break;
	      }
	    }
	  }
	  
	  index2++;
	}
      }
      index++;
    }
    index= 0;
    while (index < bgp_routes_size(routes)) {
      if (routes->data[index] == NULL) {
	ptr_array_remove_at(routes, index);
      } else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
    }
    break;
  default:
    abort();
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_ebgp_over_ibgp -------------------------------------
/**
 * If there is an eBGP route, remove all the iBGP routes.
 */
int dp_rule_ebgp_over_ibgp(bgp_router_t * router, bgp_routes_t * routes)
{
  unsigned int index;
  int iEBGP= 0;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Detect if there is at least one route learned through an eBGP
  // session:
  for (index= 0; index < bgp_routes_size(routes); index++) {
    if (bgp_routes_at(routes, index)->peer->asn != router->asn) {
      iEBGP= 1;
      break;
    }
  }
  // If there is at least one route learned over eBGP, discard all the
  // routes learned over an iBGP session:
  if (iEBGP) {
    index= 0;
    while (index < bgp_routes_size(routes)) {
      if (bgp_routes_at(routes, index)->peer->asn == router->asn)
	ptr_array_remove_at(routes, index);
      else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
    }
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- _dp_rule_igp_cost ------------------------------------------
/**
 * Helper function which retrieves the IGP cost to the given next-hop.
 */
static uint32_t _dp_rule_igp_cost(bgp_router_t * router, net_addr_t next_hop)
{
  rt_info_t * rtinfo;

  /* Is there a route towards the destination ? */
  rtinfo= rt_find_best(router->node->rt, next_hop, NET_ROUTE_ANY);
  if (rtinfo != NULL)
    return rtinfo->metric;

  STREAM_ERR_ENABLED(STREAM_LEVEL_FATAL) {
    stream_printf(gdserr, "Error: unable to compute IGP cost to next-hop (");
    ip_address_dump(gdserr, next_hop);
    stream_printf(gdserr, ")\nError: ");
    bgp_router_dump_id(gdserr, router);
    stream_printf(gdserr, "\n");
  }
  abort();
  return -1;
}

// ----- dp_rule_nearest_next_hop -----------------------------------
/**
 * Remove the routes that do not have the lowest IGP cost to the BGP
 * next-hop.
 */
int dp_rule_nearest_next_hop(bgp_router_t * router, bgp_routes_t * routes)
{
  unsigned int index;
  bgp_route_t * route;
  uint32_t uLowestCost= UINT_MAX;
  uint32_t uIGPcost;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest IGP-cost
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);

    uIGPcost= _dp_rule_igp_cost(router, route->attr->next_hop);
    if (uIGPcost < uLowestCost)
      uLowestCost= uIGPcost;

    /* Mark route as depending on the IGP weight. This is done in
       order to more efficiently react to IGP changes. See
       'bgp_router_scan_rib' for more information. */
    route_flag_set(route, ROUTE_FLAG_DP_IGP, 1);

  }

  // Discard routes with an higher IGP-cost 
  index= 0;
  while (index < bgp_routes_size(routes)) {
    route= bgp_routes_at(routes, index);
    uIGPcost= _dp_rule_igp_cost(router, route->attr->next_hop);
    if (uIGPcost > uLowestCost) {
      ptr_array_remove_at(routes, index);
    } else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_router_id -----------------------------------
/**
 * Remove routes that do not have the lowest ROUTER-ID (or
 * ORIGINATOR-ID if the route is reflected).
 */
int dp_rule_lowest_router_id(bgp_router_t * router, bgp_routes_t * routes)
{
  net_addr_t lowest_router_id= MAX_ADDR;
  net_addr_t tID;
  /*net_addr_t tOriginatorID;*/
  bgp_route_t * route;
  unsigned int index;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest ROUTER-ID (or ORIGINATOR-ID)
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);

    /* DEBUG-BEGIN */
    /*
    printf("router-ID: ");
    ip_address_dump(gdsout, route_peer_get(route)->router_id);
    printf("\noriginator-ID-ptr: %p\n", route->attr->pOriginator);
    if (route_originator_get(route, &tOriginatorID) == 0) {
      printf("originator-ID: ");
      ip_address_dump(gdsout, tOriginatorID);
      printf("\n");
      }
    */
    /* DEBUG-END */

    /* Originator-ID or Router-ID ? (< 0 => use router-ID) */
    if (route_originator_get(route, &tID) < 0)
      tID= route_peer_get(route)->router_id;
    if (tID < lowest_router_id)
      lowest_router_id= tID;
  }

  /* DEBUG-BEGIN */
  /*
  printf("max-addr: ");
  ip_address_dump(gdsout, tLowestRouterID);
  printf("\n");
  */
  /* DEBUG-END */

  // Discard routes from neighbors with an higher ROUTER-ID (or
  // ORIGINATOR-ID)
  index= 0;
  while (index < bgp_routes_size(routes)) {
    route= bgp_routes_at(routes, index);
    /* Originator-ID or Router-ID ? (< 0 => use router-ID) */
    if (route_originator_get(route, &tID) < 0)
      tID= route_peer_get(route)->router_id;
    if (tID > lowest_router_id)
      ptr_array_remove_at(routes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_shortest_cluster_list ------------------------------
/**
 * Remove the routes that do not have the shortest CLUSTER-ID-LIST.
 */
int dp_rule_shortest_cluster_list(bgp_router_t * router, bgp_routes_t * routes)
{
  bgp_route_t * route;
  unsigned int index;
  uint8_t len;
  uint8_t min_len= 255;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate shortest cluster-ID-list
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);
    if (route->attr->cluster_list == NULL)
      len= 0;
    else
      len= cluster_list_length(route->attr->cluster_list);
    if (len < min_len)
      min_len= len;
  }
  // Discard routes with a longer cluster-ID-list
  index= 0;
  while (index < bgp_routes_size(routes)) {
    route= bgp_routes_at(routes, index);
    if ((route->attr->cluster_list != NULL) &&
	(cluster_list_length(route->attr->cluster_list) > min_len))
      ptr_array_remove_at(routes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_lowest_neighbor_address ----------------------------
/**
 * Remove routes that are not learned through the peer with the lowest
 * IP address.
 */
int dp_rule_lowest_neighbor_address(bgp_router_t * router,
				    bgp_routes_t * routes)
{
  net_addr_t lowest_addr= MAX_ADDR;
  bgp_route_t * route;
  unsigned int index;
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  SIntArray * piNextHopCounter = dp_rule_nexthop_counter_create();
  int iNextHopCount;
#endif

  // Calculate lowest neighbor address
  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);
    if (route_peer_get(route)->addr < lowest_addr)
      lowest_addr= route_peer_get(route)->addr;
  }
  // Discard routes from neighbors with an higher address
  index= 0;
  while (index < bgp_routes_size(routes)) {
    route= bgp_routes_at(routes, index);
    if (route_peer_get(route)->addr > lowest_addr)
      ptr_array_remove_at(routes, index);
    else
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    {
      dp_rule_nexthop_add(piNextHopCounter, route_get_nexthop(bgp_routes_at(routes, index)));
      index++;
    }
#else
      index++;
#endif
  }
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  iNextHopCount = dp_rule_nexthop_get_count(piNextHopCounter);
  dp_rule_nexthop_destroy(&piNextHopCounter);
  return iNextHopCount;
#else
  return 0;
#endif
}

// ----- dp_rule_final ----------------------------------------------
/**
 *
 */
/*
int dp_rule_final(bgp_router_t * router, bgp_routes_t * routes)
{
  int iResult;

  // *** final tie-break ***
  while (bgp_routes_size(routes) > 1) {
    iResult= router->fTieBreak(bgp_routes_at(routes, 0),
				bgp_routes_at(routes, 1));
    if (iResult == 1) // Prefer ROUTE1
      ptr_array_remove_at(routes, 1);
    else if (iResult == -1) // Prefer ROUTE2
      ptr_array_remove_at(routes, 0);
    else {
      STREAM_ERR(STREAM_LEVEL_FATAL, "Error: final tie-break can not decide\n");
      abort(); // Can not decide !!!
    }
  }
  return 0;
}
*/
