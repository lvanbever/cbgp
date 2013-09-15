// ==================================================================
// @(#)walton.c
//
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 15/02/2006
// $Id: walton.c,v 1.8 2009-03-24 15:55:46 bqu Exp $
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

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
#include <bgp/walton.h>

#include <assert.h>
//#include <string.h>
#include <stdio.h>

#include <libgds/stream.h>
#include <libgds/memory.h>

#include <bgp/dp_rules.h>
#include <bgp/routes_list.h>
#include <ui/output.h>

typedef struct {
  uint16_t uWaltonLimit;
  uint8_t uSynchronized;  //used to evaluate if the new routes has 
  //already been sent!
  SPtrArray * pPeers;
}SWaltonLimitPeers;

// ----- _bgp_router_walton_limit_compare ----------------------------
/**
 *
 */
int _bgp_router_walton_limit_compare(void *pItem1, void * pItem2,
    unsigned int uEltSize)
{
  SWaltonLimitPeers * pWaltonLimitPeers1;
  SWaltonLimitPeers * pWaltonLimitPeers2;

  pWaltonLimitPeers1 = *(SWaltonLimitPeers **)pItem1;
  pWaltonLimitPeers2 = *(SWaltonLimitPeers **)pItem2;

  if (pWaltonLimitPeers1->uWaltonLimit < pWaltonLimitPeers2->uWaltonLimit)
    return -1;
  else if (pWaltonLimitPeers1->uWaltonLimit > pWaltonLimitPeers2->uWaltonLimit)
    return 1;
  else
    return 0;
}


// ----- _bgp_router_walton_limit_destroy ----------------------------
/**
 *
 */
void _bgp_router_walton_limit_destroy(void * pItem)
{
  SWaltonLimitPeers * pWaltonLimitPeers = *(SWaltonLimitPeers **)pItem;

  if (pWaltonLimitPeers != NULL) {
    if (pWaltonLimitPeers->pPeers != NULL) 
      ptr_array_destroy(&(pWaltonLimitPeers->pPeers));
    FREE(pWaltonLimitPeers);
    pWaltonLimitPeers = NULL;
  }
}

// ----- bgp_router_walton_init --------------------------------------
/**
 *
 */
void bgp_router_walton_init(bgp_router_t * pRouter)
{
  pRouter->pWaltonLimitPeers = 
    ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
	_bgp_router_walton_limit_compare,
	_bgp_router_walton_limit_destroy);
}

// ----- bgp_router_walton_finalize ----------------------------------
/**
 *
 */
void bgp_router_walton_finalize(bgp_router_t * pRouter)
{
  ptr_array_destroy(&(pRouter->pWaltonLimitPeers));
}


// ----- bgp_router_walton_unsynchronized_all ------------------------
void bgp_router_walton_unsynchronized_all(bgp_router_t * pRouter)
{
  uint16_t uIndex;
  SPtrArray * pWaltonLimit = pRouter->pWaltonLimitPeers;
  SWaltonLimitPeers * pWaltonLimitPeers = NULL;

  for (uIndex = 0; uIndex < ptr_array_length(pWaltonLimit);
      uIndex++) {
    pWaltonLimitPeers = pWaltonLimit->data[uIndex];
    pWaltonLimitPeers->uSynchronized = 0;
  }

}
// ----- bgp_router_walton_peer_remove -------------------------------
/**
 *
 */
void bgp_router_walton_peer_remove(bgp_router_t * pRouter, bgp_peer_t * pPeer)
{
  unsigned int uIndexWaltonLimit;
  unsigned int uIndexPeers;
  SPtrArray * pWaltonLimitPeers = pRouter->pWaltonLimitPeers;
  bgp_peers_t * pPeers = NULL;
  bgp_peer_t * pPeerFound;

  for (uIndexWaltonLimit = 0; uIndexWaltonLimit < ptr_array_length(pWaltonLimitPeers);
      uIndexWaltonLimit++) {
    pPeers = ((SWaltonLimitPeers *)pWaltonLimitPeers->data[uIndexWaltonLimit])->pPeers;
    for (uIndexPeers = 0; uIndexPeers < ptr_array_length(pPeers); uIndexPeers++) {
      pPeerFound = pPeers->data[uIndexPeers];
      if (pPeerFound == pPeer) {
	ptr_array_remove_at(pPeers, uIndexPeers);
	return;
      }
    }
  }
}

// ----- bgp_router_walton_peer_set ----------------------------------
/**
 *
 */
int bgp_router_walton_peer_set(bgp_peer_t * pPeer, unsigned int uWaltonLimit)
{
  unsigned int uIndex;
  bgp_router_t * pRouter = pPeer->pLocalRouter;
  SWaltonLimitPeers * pWaltonLimitPeers = MALLOC(sizeof(SWaltonLimitPeers));

  bgp_router_walton_peer_remove(pRouter, pPeer);

  pWaltonLimitPeers->uWaltonLimit = uWaltonLimit;

  if (ptr_array_sorted_find_index(pRouter->pWaltonLimitPeers, &pWaltonLimitPeers, &uIndex)) {
    pWaltonLimitPeers->pPeers = ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE, 
	_bgp_router_peers_compare, NULL);
    pWaltonLimitPeers->uSynchronized = 0;
    ptr_array_add(pRouter->pWaltonLimitPeers, &pWaltonLimitPeers);
    /*    LOG_DEBUG("new walton limit peer : %d\n", uWaltonLimit);
	  LOG_DEBUG("router : ");
	  LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), pRouter->tRouterID);
	  LOG_DEBUG(" peer : ");
	  LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), pPeer->tAddr);
	  LOG_DEBUG("\n");*/
  } else {
    FREE(pWaltonLimitPeers);
    /*    LOG_DEBUG("update walton limit peer : %d\n", uWaltonLimit);
	  LOG_DEBUG("router : ");
	  LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), pRouter->tRouterID);
	  LOG_DEBUG(" peer : ");
	  LOG_ENABLED_DEBUG() ip_address_dump(log_get_stream(pMainLog), pPeer->tAddr);
	  LOG_DEBUG("\n");*/
    pWaltonLimitPeers = (SWaltonLimitPeers *)pRouter->pWaltonLimitPeers->data[uIndex];
  }

  return ptr_array_add(pWaltonLimitPeers->pPeers, &pPeer);
}

// ----- bgp_router_walton_get_peers ---------------------------------
/**
 *
 */
SPtrArray * bgp_router_walton_get_peers(bgp_peer_t * pPeer, unsigned int uWaltonLimit)
{
  return NULL;
}

// ----- bgp_router_walton_dump_peers --------------------------------
/**
 *
 */
void bgp_router_walton_dump_peers(bgp_peers_t * pPeers)
{
  int iIndex;

  for (iIndex= 0; iIndex < ptr_array_length(pPeers); iIndex++) {
    LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
      bgp_peer_dump(pLogDebug, bgp_peers_at(pPeers, iIndex));
      log_printf(pLogDebug, "\n");
    }
  }

//  flushir(log_get_stream(pMainLog));
}

void bgp_router_walton_withdraw_old_routes(bgp_router_t * pRouter,
					   bgp_peer_t * pPeer,
					   SPrefix sPrefix,
					   bgp_routes_t * pRoutes)
{
  bgp_routes_t * pRoutesOut;
  bgp_route_t * pRouteOut;
  bgp_route_t * pNewRoute;
  net_addr_t tNextHop;
  uint16_t uIndex;
  uint16_t uIndexNewRoute;
  uint8_t uNHFound = 0;

  pRoutesOut = rib_find_exact(pPeer->pAdjRIBOut, sPrefix);
  if (pRoutesOut != NULL) {
    for (uIndex = 0; uIndex < routes_list_get_num(pRoutesOut); uIndex++) {
      pRouteOut = routes_list_get_at(pRoutesOut, uIndex);
      //If there are some new routes, check if there is one with the same path identifier ...
      if (pRoutes != NULL) {
	uNHFound = 0;
	for (uIndexNewRoute = 0; uIndexNewRoute < routes_list_get_num(pRoutes); uIndexNewRoute++) {
	  pNewRoute = routes_list_get_at(pRoutes, uIndexNewRoute);
	  if (route_get_nexthop(pRouteOut) == route_get_nexthop(pNewRoute)) {
	    uNHFound = 1;
	    break;
	  }
	}
      }
      //There is no more path with the old path identifier ... then withdraw it!
      if (uNHFound == 0) {
	tNextHop = route_get_nexthop(pRouteOut);
	if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix, &tNextHop)) {
	  //As it may be an eBGP session, the Next-Hop may have changed!
	  //If the next-hop is changed the path identifier as well ... :)
	  if (pPeer->uRemoteAS != pRouter->uNumber)
	    tNextHop = pRouter->pNode->tAddr;
	  else
	    tNextHop = route_get_nexthop(pRouteOut);
	  LOG_DEBUG(LOG_LEVEL_DEBUG, "\texplicit-withdraw\n");
	  bgp_peer_withdraw_prefix(pPeer, sPrefix, &tNextHop);
	}
      }
    }
  }


}

typedef struct {
  SPtrArray * pWaltonIDProcessed;
  SPtrArray * pWaltonIDUpdate;
}bgp_routes_tUpdate;

// ----- _bgp_router_walton_nh_compare ----------------------------
/**
 *
 */
int _bgp_router_walton_nh_compare(void *pItem1, void * pItem2,
    unsigned int uEltSize)
{
  net_addr_t tNextHop1;
  net_addr_t tNextHop2;

  tNextHop1 = *(net_addr_t *)pItem1;
  tNextHop2 = *(net_addr_t *)pItem2;

  if (tNextHop1 < tNextHop2)
    return -1;
  else if (tNextHop1 > tNextHop2)
    return 1;
  else
    return 0;
}

bgp_routes_tUpdate * _bgp_router_walton_routes_update_create()
{
  bgp_routes_tUpdate * pRoutesUpdate = (bgp_routes_tUpdate *)MALLOC(sizeof(bgp_routes_tUpdate));

  pRoutesUpdate->pWaltonIDUpdate = NULL;
  pRoutesUpdate->pWaltonIDProcessed = NULL;
				

  return pRoutesUpdate;
}

void _bgp_router_walton_routes_update_destroy(bgp_routes_tUpdate ** pRoutesUpdate)
{
  if (*(pRoutesUpdate)) {
    if ((*pRoutesUpdate)->pWaltonIDProcessed)
      ptr_array_destroy(&((*pRoutesUpdate)->pWaltonIDProcessed));
    if ((*pRoutesUpdate)->pWaltonIDUpdate)
      ptr_array_destroy(&((*pRoutesUpdate)->pWaltonIDUpdate));
    FREE(*pRoutesUpdate);
    *pRoutesUpdate = NULL;
  }
}

// ----- bgp_router_walton_build_updates -----------------------------
/**
 *
 */
bgp_routes_tUpdate * bgp_router_walton_build_updates(bgp_router_t * pRouter,
						bgp_peer_t * pPeer,
						SPrefix sPrefix,
						bgp_routes_t * pRoutes)
{
  bgp_routes_tUpdate * pRoutesUpdate = NULL;
  uint8_t uNHFound = 0;
  uint16_t uIndexRoute;
  uint16_t uIndexOldRoute;
  bgp_routes_t * pOldRoutes;
  bgp_route_t * pOldRoute;
  bgp_route_t * pRoute= NULL;
  net_addr_t tNextHop;
  
  
  pOldRoutes = rib_find_exact(pPeer->pAdjRIBOut, sPrefix);
  if (pOldRoutes != NULL) {
    for (uIndexOldRoute = 0; uIndexOldRoute < routes_list_get_num(pOldRoutes); uIndexOldRoute++) {
      pOldRoute = routes_list_get_at(pOldRoutes, uIndexOldRoute);
/*      LOG_DEBUG("try to find old NH : ");
      LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldRoute);
      LOG_DEBUG("\n"); */
      if (pRoutes != NULL) {
	uNHFound = 0;
	for (uIndexRoute = 0; uIndexRoute < routes_list_get_num(pRoutes); uIndexRoute++) {
	  pRoute = routes_list_get_at(pRoutes, uIndexRoute);
/*	    LOG_DEBUG("\ttry to find old NH : ");
	    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldRoute);
	    LOG_DEBUG("\n");*/


	  //We do not check if it's an eBGP session as it restricts our
	  //implementation to send only one path to an eBGP peer.
	  if (route_get_nexthop(pRoute) == route_get_nexthop(pOldRoute)) {
	    uNHFound = 1;
	    break;
	  }
	}
      }
      if (uNHFound == 0) {
	tNextHop = route_get_nexthop(pOldRoute);
	if (bgp_router_peer_rib_out_remove(pRouter, pPeer, sPrefix, &tNextHop)) {
	  //As it may be an eBGP session, the Next-Hop may have changed!
	  //If the next-hop is changed the path identifier as well ... :)
	  if (pPeer->uRemoteAS != pRouter->uNumber)
	    tNextHop = pRouter->pNode->tAddr;
	  else
	    tNextHop = route_get_nexthop(pOldRoute);
	  LOG_DEBUG(LOG_LEVEL_DEBUG, "\texplicit-withdraw\n");
	  bgp_peer_withdraw_prefix(pPeer, sPrefix, &tNextHop);
	}
	//LOG_DEBUG(LOG_LEVEL_DEBUG, "route found but not old\n");
      } else {
	//LOG_DEBUG(LOG_LEVEL_DEBUG, "route found\n");
	if (pRoutesUpdate == NULL) {
	  pRoutesUpdate = _bgp_router_walton_routes_update_create();
	  pRoutesUpdate->pWaltonIDProcessed = ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
							    _bgp_router_walton_nh_compare, NULL);
	}
	//No update to send if old route and new route are the same!
	tNextHop = route_get_nexthop(pRoute);
	if (!route_equals(pOldRoute, pRoute)) {
	  //LOG_DEBUG(LOG_LEVEL_DEBUG, "routes not equals\n");
	  if (pRoutesUpdate->pWaltonIDUpdate == NULL) 
	    pRoutesUpdate->pWaltonIDUpdate = ptr_array_create(ARRAY_OPTION_SORTED | ARRAY_OPTION_UNIQUE,
							    _bgp_router_walton_nh_compare, NULL);
	  
	  ptr_array_add(pRoutesUpdate->pWaltonIDUpdate, &tNextHop);
	} else {
	  //LOG_DEBUG(LOG_LEVEL_DEBUG, "routes equals\n");
	  ptr_array_add(pRoutesUpdate->pWaltonIDProcessed, &tNextHop);
	}
      }
    }
  } else {
//    LOG_DEBUG(LOG_LEVEL_DEBUG, "no old routes\n");
    pRoutesUpdate = _bgp_router_walton_routes_update_create();
  }
  
  return pRoutesUpdate;
}

// ----- bgp_router_walton_dp_disseminate ----------------------------
/**
 * Disseminate route to Adj-RIB-Outs.
 *
 */
void bgp_router_walton_dp_disseminate(bgp_router_t * pRouter, 
    bgp_peers_t * pPeers,
    SPrefix sPrefix,
    bgp_routes_t * pRoutes)
{
  unsigned int uIndexPeer;
  unsigned int uIndexRoute;
  unsigned int uIndex;
  bgp_peer_t * pPeer;
  bgp_route_t * pRoute;
  bgp_routes_tUpdate * pRoutesUpdate=NULL;
  net_addr_t tNextHop;

  
  for (uIndexPeer= 0; uIndexPeer < ptr_array_length(pPeers); uIndexPeer++) {
    pPeer= (bgp_peer_t *) pPeers->data[uIndexPeer];
    if (!bgp_peer_flag_get(pPeer, PEER_FLAG_VIRTUAL)) {
      
      //Withdraw all the routes present in the RIB-OUT which are not part of the set of best routes!
      ///We must do this check because replacing a route in a RIB now adds it
      //as a RIB is no more restricted to a database of one route per prefix.
      pRoutesUpdate = bgp_router_walton_build_updates(pRouter, pPeer, sPrefix, pRoutes);

      //If pRoutesUpdate == NULL : all routes have been withdrawn
      //else if pRoutesUpdate->pWaltonIDUpdate == NULL : routes have changed since the last advertisement
      //if not found in pRoutesUpdate->pWaltonIDProcessed : new route
      if (pRoutesUpdate) {
	if (pPeer->uSessionState == SESSION_STATE_ESTABLISHED) {
	  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
	  log_printf(pLogDebug, "DISSEMINATE (");
	  ip_prefix_dump(pLogDebug, sPrefix);
	  log_printf(pLogDebug, ") from ");
	  bgp_router_dump_id(pLogDebug, pRouter);
	  log_printf(pLogDebug, " to ");
	  bgp_peer_dump_id(pLogDebug, pPeer);
	  log_printf(pLogDebug, "\n");
	  }
	}

	//bgp_router_walton_withdraw_old_routes(pRouter, pPeer, sPrefix, pRoutes);
	for (uIndexRoute = 0; uIndexRoute < routes_list_get_num(pRoutes); uIndexRoute++) { 
	  pRoute = routes_list_get_at(pRoutes, uIndexRoute);
	  if (pRoutesUpdate->pWaltonIDProcessed == NULL) {
	    LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
	      log_printf(pLogDebug, "propagates changed route (I) : ");
	      route_dump(pLogDebug,  pRoute);
	      log_printf(pLogDebug, "\n");
	    }
	    bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
	      pRoute, pPeer);
	  } else {
	    tNextHop = route_get_nexthop(pRoute);
	    if (pRoutesUpdate->pWaltonIDUpdate && !ptr_array_sorted_find_index(pRoutesUpdate->pWaltonIDUpdate, &tNextHop, &uIndex)) {

	      LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
		log_printf(pLogDebug, "propagates changed route (II) : ");
		route_dump(pLogDebug,  pRoute);
		log_printf(pLogDebug, "\n");
	      }
	    } else {
	      if (ptr_array_sorted_find_index(pRoutesUpdate->pWaltonIDProcessed, &tNextHop, &uIndex)) {

		LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) {
		  log_printf(pLogDebug, "propagates route : ");
		  route_dump(pLogDebug,  pRoute);
		  log_printf(pLogDebug, "\n");
		}
		bgp_router_decision_process_disseminate_to_peer(pRouter, sPrefix,
		  pRoute, pPeer);
	      }
	    }
	  }
	}
	_bgp_router_walton_routes_update_destroy(&pRoutesUpdate);
      }
    }
  }
}

typedef struct {
  uint16_t uWaltonLimit;
  bgp_routes_t * pRoutes;
  bgp_router_t * pRouter;
}SWaltonCtx;
// ----- bgp_router_walton_for_each_peer_sync ------------------------
/**
 * This function search for each non sync peers and each of these have a Walton
 * Limit above uWaltonLimit given in pContext
 */
int bgp_router_walton_for_each_peer_sync(void * pItem, void * pContext)
{
  SWaltonCtx * pWaltonCtx = (SWaltonCtx *)pContext;
  SWaltonLimitPeers * pWaltonPeers = *(SWaltonLimitPeers **)pItem;
  SPrefix sPrefix;
  bgp_route_t * pRoute = NULL;
  bgp_routes_t * pRoutes = NULL;
  SPtrArray * pPeers = NULL;

  /*LOG_DEBUG("walton limit : %d\n", pWaltonPeers->uWaltonLimit);*/
  if (pWaltonPeers->uWaltonLimit >= pWaltonCtx->uWaltonLimit && pWaltonPeers->uSynchronized != 1) {
    pRoutes = pWaltonCtx->pRoutes;
    assert(routes_list_get_num(pRoutes) > 0);
    pRoute = routes_list_get_at(pRoutes, 0);
    sPrefix = pRoute->sPrefix;
    pPeers = (SPtrArray *)(pWaltonPeers->pPeers);
//        LOG_ENABLED_DEBUG() bgp_router_walton_dump_peers(pPeers);
    bgp_router_walton_dp_disseminate(pWaltonCtx->pRouter, pPeers, sPrefix, pRoutes);
    pWaltonPeers->uSynchronized = 1;
  }
  return 0;
}


// ----- bgp_router_walton_disseminate_to_peers ----------------------
/**
 *
 */
void bgp_router_walton_disseminate_select_peers(bgp_router_t * pRouter, 
					    bgp_routes_t * pRoutes, 
					    uint16_t iNextHopCount)
{
/*  unsigned int uIndex = 2;
  SPtrArray * pPeers;
  SWaltonLimitPeers * pWaltonLimitPeers = (SWaltonLimitPeers*)MALLOC(sizeof(SWaltonLimitPeers));
  SWaltonLimitPeers * pWaltonLimitPeersFound = NULL;
  bgp_route_t * pRoute = NULL;
  SPrefix sPrefix;*/

  SWaltonCtx * pWaltonCtx = NULL;

  if (iNextHopCount != 0) {
    pWaltonCtx = (SWaltonCtx *)MALLOC(sizeof(SWaltonCtx));
    pWaltonCtx->uWaltonLimit = iNextHopCount;
    pWaltonCtx->pRoutes = pRoutes;
    pWaltonCtx->pRouter = pRouter;
  
    _array_for_each((SArray *)pRouter->pWaltonLimitPeers, bgp_router_walton_for_each_peer_sync, pWaltonCtx);
    FREE(pWaltonCtx);
  }
}

// ----- bgp_router_walton_decision_process_run ---------------------
/**
 * Select one route with the following rules (see the actual
 * definition of the decision process in the global array DP_RULES[]):
 *
 *   1. prefer highest LOCAL-PREF
 *   2. prefer shortest AS-PATH
 *   3. prefer lowest ORIGIN (IGP < EGP < INCOMPLETE)
 *   4. prefer lowest MED
 *   5. prefer eBGP over iBGP
 *   6. prefer nearest next-hop (IGP)
 *   7. prefer shortest CLUSTER-ID-LIST
 *   8. prefer lowest neighbor router-ID
 *   9. final tie-break (prefer lowest neighbor IP address)
 *
 * Note: Originator-ID should be substituted to router-ID in rule (8)
 * if route has been reflected (i.e. if Originator-ID is present in
 * the route)
 *
 * The function returns the index of the rule that broke the ties
 * incremented by 1. If the returned value is 0, that means that there
 * was a single rule (no choice). Otherwise, if 1 is returned, that
 * means that the Local-Pref rule broke the ties, and so on...
 */
int bgp_router_walton_decision_process_run(bgp_router_t * pRouter,
					   bgp_routes_t * pRoutes)
{
  int iRule;
  int iNextHopCount;
  iNextHopCount = dp_rule_no_selection(pRouter, pRoutes);

  LOG_DEBUG_ENABLED(LOG_LEVEL_DEBUG) log_printf(pLogDebug, "different routes : %d\n", iNextHopCount);
  bgp_router_walton_disseminate_select_peers(pRouter, pRoutes, iNextHopCount);

  // Apply the decision process rules in sequence until there is 1 or
  // 0 route remaining or until all the rules were applied.
  for (iRule= 0; iRule < DP_NUM_RULES; iRule++) {

    if (routes_list_get_num(pRoutes) <= 1)
      break;

    iNextHopCount=DP_RULES[iRule](pRouter, pRoutes);
    LOG_DEBUG(LOG_LEVEL_DEBUG, "rule: [ %s ] remains : %d\n",
              DP_RULE_NAME[iRule], iNextHopCount);
    bgp_router_walton_disseminate_select_peers(pRouter, pRoutes,
                                               iNextHopCount);
    
  }
  
  // Check that at most a single best route will be returned.
  if (routes_list_get_num(pRoutes) > 1) {
    LOG_ERR(LOG_LEVEL_FATAL, "Error: decision process did not return a single best route\n");
    abort();
  }
  
  return iRule;
}
#endif

