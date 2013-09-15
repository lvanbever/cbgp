// ==================================================================
// @(#)qos.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// 
// @date 13/11/2002
// $Id: qos.c,v 1.10 2009-03-24 15:49:01 bqu Exp $
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
#include <libgds/memory.h>

#include <bgp/as.h>
#include <bgp/dp_rules.h>
#include <bgp/attr/path.h>
#include <bgp/attr/path_segment.h>
#include <bgp/peer.h>
#include <bgp/qos.h>

#ifdef BGP_QOS

unsigned int BGP_OPTIONS_QOS_AGGR_LIMIT= 2;

#define _qos_delay_interval(D) (D.tMean-D.tDelay)

void bgp_route_qos_init(bgp_route_t * pRoute)
{
  qos_route_init_delay(pRoute);
  qos_route_init_bandwidth(pRoute);
  pRoute->qos.pAggrRoutes= NULL;
  pRoute->qos.pAggrRoute= NULL;
  pRoute->qos.pEBGPRoute= NULL;
}

void bgp_route_qos_destroy(bgp_route_t * pRoute)
{
  ptr_array_destroy(&pRoute->qos.pAggrRoutes);
}

int bgp_route_qos_equals(bgp_route_t * pRoute1, bgp_route_t * pRoute2)
{
  if (!qos_route_delay_equals(pRoute1, pRoute2) ||
      !qos_route_bandwidth_equals(pRoute1, pRoute2)) {
    return 0;
  }
  return 1;
}

void bgp_route_qos_copy(bgp_route_t * pNewRoute, bgp_route_t * pRoute)
{
  pNewRoute->qos.tDelay= pRoute->qos.tDelay;
  pNewRoute->qos.tBandwidth= pRoute->qos.tBandwidth;
  // Copy also list of aggregatable routes ??
  // Normalement non: alloué par le decision process et références
  // dans Adj-RIB-Outs. A vérifier !!!
  if (pRoute->qos.pAggrRoute != NULL) {
    pNewRoute->qos.pAggrRoutes=
      (SPtrArray *) _array_copy((SArray *) pRoute->qos.pAggrRoutes);
    pNewRoute->qos.pAggrRoute= route_copy(pRoute->qos.pAggrRoute);
  }
}

// ----- qos_route_aggregate ----------------------------------------
/**
 * Aggregate N routes. The announced delay is the best route's delay.
 * The aggregated route has all the attribute (Next-Hop,
 * Communities, ...) of the best routes except the AS-Path which is an
 * AS-SET containing the AS-Num of all the ASes traversed by one of
 * the aggregated routes.
 */
SRoute * qos_route_aggregate(SRoutes * pRoutes, SRoute * pBestRoute)
{
  SRoute * pAggrRoute;
  int iIndex;
  SRoute * pRoute;
  SPath ** ppPaths= (SPath **) MALLOC(sizeof(SPath *)*
				      ptr_array_length(pRoutes));
  qos_delay_t tDelay;
  
  assert(routes_list_get_num(pRoutes) >= 1);

  // Aggregate Delay
  tDelay.tMean= 0;
  tDelay.uWeight= 0;
  for (iIndex= 0; iIndex < routes_list_get_num(pRoutes); iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex];
    tDelay.tMean+= pRoute->tDelay.uWeight*pRoute->tDelay.tMean;
    tDelay.uWeight+= pRoute->tDelay.uWeight;
    ppPaths[iIndex]= pRoute->pASPath;
  }
  tDelay.tMean= (int) (tDelay.tMean/tDelay.uWeight);

  // Aggregate Bandwidth: NOT YET IMPLEMENTED

  // Aggregate Paths
  pAggrRoute= route_copy(pBestRoute);
  pAggrRoute->tDelay= tDelay;
  pAggrRoute->tDelay.tDelay= pBestRoute->tDelay.tDelay;
  path_destroy(&pAggrRoute->pASPath);
  pAggrRoute->pASPath= path_aggregate(ppPaths, routes_list_get_num(pRoutes));
  FREE(ppPaths);

  return pAggrRoute;
}

// ----- qos_route_is_ibgp ------------------------------------------
/**
 * Check if the given route is
 *   - learned through an iBGP session
 * or
 *   - has an aggregate which contains a route learned through an iBGP
 *     session
 */
int qos_route_is_ibgp(SBGPRouter * pRouter, SRoute * pRoute)
{
  int iIndex;

  // Best route learned from iBGP ?
  if (route_peer_get(pRoute)->uRemoteAS == pRouter->uNumber)
    return 1;

  // Route in the aggregate learned from iBGP ?
  if (pRoute->pAggrRoute != NULL) {
    for (iIndex= 0; iIndex < ptr_array_length(pRoute->pAggrRoutes); iIndex++)
      if (route_peer_get((SRoute *) pRoute->pAggrRoutes->data[iIndex])->uRemoteAS
	  == pRouter->uNumber)
	return 1;
  }

  return 0;
}

// ----- qos_route_init_delay ---------------------------------------
/**
 *
 */
void qos_route_init_delay(SRoute * pRoute)
{
  pRoute->tDelay.tDelay= 0;
  pRoute->tDelay.tMean= 0;
  pRoute->tDelay.uWeight= 0;
}

// ----- qos_route_init_bandwidth -----------------------------------
/**
 *
 */
void qos_route_init_bandwidth(SRoute * pRoute)
{
  pRoute->tBandwidth.uBandwidth= 0;
  pRoute->tBandwidth.uMean= 0;
  pRoute->tBandwidth.uWeight= 0;
}

// ----- qos_route_update_delay -------------------------------------
/**
 * Update the route's delay information:
 *
 *   new_delay := old_delay + edge_delay
 *   new_mean  := old_mean + edge_delay
 */
int qos_route_update_delay(SRoute * pRoute, net_link_delay_t tDelay)
{
  pRoute->tDelay.tDelay+= tDelay;
  pRoute->tDelay.tMean+= tDelay;
  return 0;
}

// ----- qos_route_update_bandwidth ---------------------------------
/**
 * Update the route's maximum reservable bandwidth:
 *
 *   new_bw   := min(old_bw, edge_bw)
 *   new_mean := new_bw
 */
int qos_route_update_bandwidth(SRoute * pRoute, uint32_t uBandwidth)
{
  if (pRoute->tBandwidth.uBandwidth > uBandwidth) {
    pRoute->tBandwidth.uBandwidth= uBandwidth;
    pRoute->tBandwidth.uMean= uBandwidth;
  }
  return 0;
}

// ----- qos_route_delay_equals -------------------------------------
/**
 *
 */
int qos_route_delay_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  return (memcmp(&pRoute1->tDelay, &pRoute2->tDelay,
		 sizeof(qos_delay_t)) == 0);
}

// ----- qos_route_bandwidth_equals ---------------------------------
/**
 *
 */
int qos_route_bandwidth_equals(SRoute * pRoute1, SRoute * pRoute2)
{
  return (memcmp(&pRoute1->tBandwidth, &pRoute2->tBandwidth,
		 sizeof(qos_bandwidth_t)) == 0);
}

// ----- qos_route_compare_delay ------------------------------------
/**
 * Ordering must be:
 *
 *   - lowest delay first
 *   - then, largest interval first
 */
int qos_route_compare_delay(void * pItem1, void * pItem2,
			    unsigned int uEltSize)
{
  SRoute * pRoute1= *((SRoute **) pItem1);
  SRoute * pRoute2= *((SRoute **) pItem2);

  net_link_delay_t tDelay1= pRoute1->tDelay.tDelay;
  net_link_delay_t tDelay2= pRoute2->tDelay.tDelay;
  net_link_delay_t tInterval1= _qos_delay_interval(pRoute1->tDelay);
  net_link_delay_t tInterval2= _qos_delay_interval(pRoute2->tDelay);

  // Compare delays
  if (tDelay1 > tDelay2)
    return 1;
  else if (tDelay1 < tDelay2)
    return -1;

  // Compare intervals
  if (tInterval1 < tInterval2)
    return 1;
  else if (tInterval1 > tInterval2)
    return -1;

  return 0; // Delay and interval are equal
}

// ----- qos_decision_process_delay ---------------------------------
/**
 * Let M be the number of routes which we want to keep for
 * aggregation (M = BGP_OPTIONS_QOS_AGGR_LIMIT).
 *
 * Order the routes lexicographically:
 *   - first, lowest delay
 *   - then longest interval
 *
 * Find the highest P < M such that
 *   delay(R[P]) != delay(R[P+1])
 * or such that
 *   (delay(R[P]) == delay(R[P+1])) and (interval(R[P]) < interval(R[P+1]))
 *   
 * Select the P first routes as candidates for aggregation
 *
 * Use the remaining tie-break rules to select at most M-P additional
 * routes as candidates for aggregation
 *
 * Run the decision process on the P first rules to find which one is
 * the best one.
 */
int qos_decision_process_delay(SBGPRouter * pRouter, SRoutes * pRoutes,
			       unsigned int uNumRoutes)
{
  int iIndex, iIndexLast;
  SRoute * pRoute, * pCurrentRoute;
  SRoutes * pAggrRoutes/*, * pOthRoutes*/;

  AS_LOG_DEBUG(pRouter, " > qos_decision_process_delay\n");

  assert(uNumRoutes >= 1);

  // Sort routes based on (delay, delay_interval)
  _array_sort((SArray *) pRoutes, qos_route_compare_delay);

  // If there are more routes than can be aggregated, select a subset
  if (routes_list_get_num(pRoutes) > uNumRoutes) {

    // Find the higher P < M...
    iIndexLast= 0;
    pCurrentRoute= (SRoute *) pRoutes->data[0];
    for (iIndex= 0; (iIndex < ptr_array_length(pRoutes)-1) &&
	   (iIndex < uNumRoutes); iIndex++) {
      pRoute= (SRoute *) pRoutes->data[iIndex+1];
      if ((pCurrentRoute->tDelay.tDelay != pRoute->tDelay.tDelay) ||
	  (_qos_delay_interval(pCurrentRoute->tDelay) !=
	   _qos_delay_interval(pRoute->tDelay)))
	iIndexLast= iIndex;
      pCurrentRoute= pRoute;
    }

    _array_trim((SArray *) pRoutes, iIndexLast+1);

    // Tie-break for the N-P last routes in order to find additional
    // aggregatable routes (optional)
    /*
    pOthRoutes= (SPtrArray *) _array_sub((SArray *) pRoutes,
					 iIndexLast+1,
					 ptr_array_length(pRoutes)-1);
    qos_decision_process_tie_break(pAS, pOthRoutes,
				   uNumRoutes-iIndexLast-1);

    _array_add_all((SArray *) pAggrRoutes, (SArray  *) pOthRoutes);
    _array_destroy((SArray **) &pOthRoutes);
    */

  }

  pAggrRoutes= (SRoutes *) _array_copy((SArray *) pRoutes);

  // Find best route
  qos_decision_process_tie_break(pRouter, pRoutes, 1);

  assert(ptr_array_length(pRoutes) <= 1);

  // Attach the list of aggregatable routes to the best route
  if (ptr_array_length(pRoutes) == 1) {
    ((SRoute *) pRoutes->data[0])->pAggrRoutes= pAggrRoutes;
    ((SRoute *) pRoutes->data[0])->pAggrRoute=
      qos_route_aggregate(pAggrRoutes, (SRoute *) pRoutes->data[0]);
  } else
    routes_list_destroy(&pAggrRoutes);

  AS_LOG_DEBUG(pRouter, " < qos_decision_process_delay\n");
    
  return 0;
}

// ----- qos_advertise_to_peer --------------------------------------
/**
 * Advertise a route to a given peer:
 *
 *   - do not redistribute a route learned through iBGP to an
 *     iBGP peer BUT in this case, redistribute the best eBGP route
 *
 *   !! THE ABOVE RULE SHOULD BE MOVED BEFORE INSERTION IN ADJ-RIB-OUT !!
 *
 *   - avoid sending to originator peer
 *   - avoid sending to a peer in AS-Path (SSLD)
 *   - check standard communities (NO_ADVERTISE and NO_EXPORT)
 *   - apply redistribution communities
 *   - strip non-transitive extended communities
 *   - update Next-Hop (next-hop-self)
 *   - prepend AS-Path (if redistribution to an external peer)
 */
int qos_advertise_to_peer(SBGPRouter * pRouter, SPeer * pPeer, SRoute * pRoute)
{
  SRoute * pNewRoute= NULL;
  int iExternalSession= (pRouter->uNumber != pPeer->uRemoteAS);

  AS_LOG_DEBUG(pRouter, " > qos_advertise_peer\n");

  // If this route was learned through an iBGP session, do not
  // redistribute it to an internal peer
  if ((route_peer_get(pRoute)->uRemoteAS == pRouter->uNumber) &&
      (!iExternalSession))
    return -1;

  // Do not redistribute to peer that has announced this route
  if (pPeer->tAddr == pRoute->tNextHop)
     return -1;

  // Avoid loop creation (SSLD, Sender-Side Loop Detection)
  if ((iExternalSession) &&
      (route_path_contains(pRoute, pPeer->uRemoteAS)))
    return -1;

  // Do not redistribute to other peers
  if (route_comm_contains(pRoute, COMM_NO_ADVERTISE))
    return -1;

  // Do not redistribute outside confederation (here AS)
  if ((iExternalSession) &&
      (route_comm_contains(pRoute, COMM_NO_EXPORT)))
    return -1;

  // Copy the route. This is required since the call to
  // as_ecomm_red_process can alter the route's attribute !!
  route_copy_count++;
  pNewRoute= route_copy(pRoute);

  // Check output filter and redistribution communities
  if (as_ecomm_red_process(pPeer, pNewRoute)) {

    route_ecomm_strip_non_transitive(pNewRoute);

    if (filter_apply(pPeer->pOutFilter, pRouter, pNewRoute)) {

      // The route's next-hop is this router (next-hop-self)
      route_nexthop_set(pNewRoute, pRouter->pNode->tAddr);

      // Append AS-Number if exteral peer (eBGP session)
      if (iExternalSession)
	route_path_append(pNewRoute, pRouter->uNumber);
      
      LOG_DEBUG("*** AS%d advertise to AS%d ***\n",
		pRouter->uNumber, pPeer->uRemoteAS);

      peer_announce_route(pPeer, pNewRoute);
      return 0;
    }
  }

  route_destroy_count++;
  route_destroy(&pNewRoute);

  return -1;
}

// ----- qos_decision_process_disseminate_to_peer -------------------
/**
 *
 */
void qos_decision_process_disseminate_to_peer(SBGPRouter * pRouter,
					      SPrefix sPrefix,
					      SRoute * pRoute,
					      SPeer * pPeer)
{
#ifndef __EXPERIMENTAL_WALTON__
  SRoute * pNewRoute;

  if (pPeer->uSessionState == SESSION_STATE_ESTABLISHED) {
    LOG_DEBUG("\t->peer: AS%d\n", pPeer->uRemoteAS);

    if (pRoute == NULL) {
      // A route was advertised to this peer => explicit withdraw
      if (rib_find_exact(pPeer->pAdjRIBOut, sPrefix) != NULL) {
	rib_remove_route(pPeer->pAdjRIBOut, sPrefix);
	peer_withdraw_prefix(pPeer, sPrefix);
	LOG_DEBUG("\texplicit-withdraw\n");
      }
    } else {
      route_copy_count++;
      pNewRoute= route_copy(pRoute);
      if (qos_advertise_to_peer(pRouter, pPeer, pNewRoute) == 0) {
	LOG_DEBUG("\treplaced\n");
	rib_replace_route(pPeer->pAdjRIBOut, pNewRoute);
      } else {
	route_destroy_count++;
	route_destroy(&pNewRoute);
	LOG_DEBUG("\tfiltered\n");
	if (rib_find_exact(pPeer->pAdjRIBOut, sPrefix) != NULL) {
	  LOG_DEBUG("\texplicit-withdraw\n");
	  rib_remove_route(pPeer->pAdjRIBOut, sPrefix);
	  peer_withdraw_prefix(pPeer, sPrefix);
	}
      }
    }
  }
#endif
}

// ----- qos_decision_process_tie_break -----------------------------
/**
 * Select one route with the following rules:
 *
 *   - prefer largest Delay-interval
 *   - prefer shortest AS-PATH
 *   - final tie-break
 *
 * Must go to the end of the function to select a unique best route,
 * but keep N good routes to aggregate
 */
void qos_decision_process_tie_break(SBGPRouter * pRouter, SRoutes * pRoutes,
				    unsigned int uNumRoutes)
{
  int iIndex;
  SRoute * pCurrentRoute, * pRoute;

  // *** lowest delay, then largest interval ***
  if (ptr_array_length(pRoutes) < 1)
    return;
  pCurrentRoute= (SRoute *) pRoutes->data[0];
  for (iIndex= 0; iIndex < ptr_array_length(pRoutes)-1; iIndex++) {
    pRoute= (SRoute *) pRoutes->data[iIndex+1];
    if ((pCurrentRoute->tDelay.tDelay != pRoute->tDelay.tDelay) ||
	(_qos_delay_interval(pCurrentRoute->tDelay) !=
	 _qos_delay_interval(pRoute->tDelay))) {
      iIndex++;
      while (ptr_array_length(pRoutes) > iIndex) {
	ptr_array_remove_at(pRoutes, iIndex);
      }
    }
    pCurrentRoute= pRoute;
  }

  // *** shortest AS-PATH ***
  dp_rule_shortest_path(pRoutes);
  if (ptr_array_length(pRoutes) <= uNumRoutes)
    return;

  // *** eBGP over iBGP ***
  dp_rule_ebgp_over_ibgp(pRouter, pRoutes);
  if (ptr_array_length(pRoutes) <= uNumRoutes)
    return;

  // *** nearest NEXT-HOP (lowest IGP-cost) ***
  dp_rule_nearest_next_hop(pRouter, pRoutes);
  if (ptr_array_length(pRoutes) <= uNumRoutes)
    return;

  dp_rule_final(pRouter, pRoutes);
}

// ----- qos_decision_process_disseminate ---------------------------
/**
 * Disseminate route to Adj-RIB-Outs.
 *
 * If there is no best route, then
 *   - if a route was previously announced, send an explicit withdraw
 *   - otherwise do nothing
 *
 * If there is one best route, then send an update. If a route was
 * previously announced, it will be implicitly withdrawn.
 */
void qos_decision_process_disseminate(SBGPRouter * pRouter, SPrefix sPrefix,
				      SRoute * pRoute)
{
#define QOS_REDISTRIBUTE_NOT  0
#define QOS_REDISTRIBUTE_BEST 1
#define QOS_REDISTRIBUTE_EBGP 2

  int iIndex, iIndex2;
  SPeer * pPeer;
  SRoutes * pAggrRoutes= NULL;
  SRoute * pEBGPRoute= NULL;
  int iRedistributionType;
  int iAggregateIBGP;

  AS_LOG_DEBUG(pRouter, " > qos_decision_process_disseminate.begin\n");

  iAggregateIBGP= qos_route_is_ibgp(pRouter, pRoute);

  if (pRoute->pAggrRoute != NULL) {
    pAggrRoutes= pRoute->pAggrRoutes;
    pEBGPRoute= pRoute->pEBGPRoute;
    pRoute= pRoute->pAggrRoute;
  }

  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer *) pRouter->pPeers->data[iIndex];

    iRedistributionType= QOS_REDISTRIBUTE_BEST;

    // If one route of the aggregate was learned through the iBGP,
    // prevent the redistribution to all the iBGP peer.
    // Announce the eBGP route if it exists
    if ((pPeer->uRemoteAS == pRouter->uNumber) &&
	(iAggregateIBGP)) {
      if (pEBGPRoute != NULL)
	iRedistributionType= QOS_REDISTRIBUTE_EBGP;
      else
	iRedistributionType= QOS_REDISTRIBUTE_NOT;
    }

    // If the route is an aggregate, check every peer which has
    // announced a route in the aggregate and prevent the
    // redistribution
    if ((iRedistributionType != QOS_REDISTRIBUTE_NOT) &&
	(pAggrRoutes != NULL)) {
      for (iIndex2= 0; iIndex2 < ptr_array_length(pAggrRoutes); iIndex2++)
	if (((SRoute *) pAggrRoutes->data[iIndex2])->tNextHop ==
	    pPeer->tAddr) {
	  iRedistributionType= QOS_REDISTRIBUTE_NOT;
	  break;
	}
    }

    switch (iRedistributionType) {
    case QOS_REDISTRIBUTE_BEST:
      LOG_DEBUG("\tredistribution allowed to %d:", pPeer->uRemoteAS);
      LOG_ENABLED_DEBUG()
	ip_address_dump(log_get_stream(pMainLog), pPeer->tAddr);
      LOG_DEBUG("\n");
      qos_decision_process_disseminate_to_peer(pRouter, sPrefix, pRoute,
					       pPeer);
      break;
    case QOS_REDISTRIBUTE_EBGP:
      LOG_DEBUG("\tredistribution of eBGP allowed to %d:", pPeer->uRemoteAS);
      LOG_ENABLED_DEBUG()
	ip_address_dump(log_get_stream(pMainLog), pPeer->tAddr);
      LOG_DEBUG("\n");
      qos_decision_process_disseminate_to_peer(pRouter, sPrefix, pEBGPRoute,
					       pPeer);      
      break;
    default:
      LOG_DEBUG("\tredistribution refused to %d:", pPeer->uRemoteAS);
      LOG_ENABLED_DEBUG()
	ip_address_dump(log_get_stream(pMainLog), pPeer->tAddr);
      LOG_DEBUG("\n");
      qos_decision_process_disseminate_to_peer(pRouter, sPrefix, NULL,
					       pPeer);
    }
  }

  AS_LOG_DEBUG(pRouter, " < qos_decision_process_disseminate.end\n");

}

// ----- qos_decision_process ----------------------------------------
/**
 * Phase I - Calculate degree of preference (LOCAL_PREF) for each
 *           single route. Operate on separate Adj-RIB-Ins.
 *           This phase is carried by 'peer_handle_message' (peer.c).
 *
 * Phase II - Selection of best route on the basis of the degree of
 *            preference and then on tie-breaking rules (AS-Path
 *            length, Origin, MED, ...).
 *
 * Phase III - Dissemination of routes.
 *
 * In our implementation, we distinguish two main cases:
 * - a withdraw has been received from a peer for a given prefix. In
 * this case, if the best route towards this prefix was received by
 * the given peer, the complete decision process has to be
 * run. Otherwise, nothing more is done (the route has been removed
 * from the peer's Adj-RIB-In by 'peer_handle_message');
 * - an update has been received. The complete decision process has to
 * be run.
 */
int qos_decision_process(SBGPRouter * pRouter, SPeer * pOriginPeer,
			 SPrefix sPrefix)
{
#ifndef __EXPERIMENTAL_WALTON__
  SRoutes * pRoutes= routes_list_create();
  SRoutes * pEBGPRoutes= routes_list_create();
  int iIndex;
  SPeer * pPeer;
  SRoute * pRoute, * pOldRoute;

  AS_LOG_DEBUG(pRouter, " > qos_decision_process.begin\n");

  LOG_DEBUG("\t<-peer: AS%d\n", pOriginPeer->uRemoteAS);

  pOldRoute= rib_find_exact(pRouter->pLocRIB, sPrefix);
  LOG_DEBUG("\tbest: ");
  LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pOldRoute);
  LOG_DEBUG("\n");

  // *** lock all Adj-RIB-Ins ***

  // Build list of eligible routes and list of eligible eBGP routes
  for (iIndex= 0; iIndex < ptr_array_length(pRouter->pPeers); iIndex++) {
    pPeer= (SPeer*) pRouter->pPeers->data[iIndex];
    pRoute= rib_find_exact(pPeer->pAdjRIBIn, sPrefix);
    if ((pRoute != NULL) &&
	(route_flag_get(pRoute, ROUTE_FLAG_FEASIBLE))) {
      assert(ptr_array_append(pRoutes, pRoute) >= 0);
      LOG_DEBUG("\teligible: ");
      LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
      LOG_DEBUG("\n");
      if (route_peer_get(pRoute)->uRemoteAS != pRouter->uNumber)
	assert(ptr_array_append(pEBGPRoutes, pRoute) >= 0);
    }
  }

  // Keep routes with highest degree of preference
  if (ptr_array_length(pRoutes) > 1)
    as_decision_process_dop(pRouter, pRoutes);

  // Tie-break
  if (ptr_array_length(pRoutes) > 1)
    qos_decision_process_delay(pRouter, pRoutes, BGP_OPTIONS_QOS_AGGR_LIMIT);

  assert(ptr_array_length(pRoutes) <= 1);

  if (ptr_array_length(pRoutes) == 1) {

    LOG_DEBUG("\tselected: ");
    LOG_ENABLED_DEBUG()
      route_dump(log_get_stream(pMainLog), (SRoute *) pRoutes->data[0]);
    LOG_DEBUG("\n");

    // Carry aggregated route ?
    if (((SRoute *) pRoutes->data[0])->pAggrRoute != NULL) {
      LOG_DEBUG("\taggregated: ");
      LOG_ENABLED_DEBUG()
	route_dump(log_get_stream(pMainLog),
		    ((SRoute *) pRoutes->data[0])->pAggrRoute);

      LOG_DEBUG("\n");
    }

    // If best route is iBGP or aggregate contains an iBGP route, then
    // find the best eBGP route (and aggregate: optional)
    if (qos_route_is_ibgp(pRouter, (SRoute *) pRoutes->data[0])) {

      // Keep eBGP routes with highest degree of preference
      if (ptr_array_length(pEBGPRoutes) > 1)
	as_decision_process_dop(pRouter, pEBGPRoutes);

      // Tie-break for eBGP routes
      if (ptr_array_length(pEBGPRoutes) > 1)
	qos_decision_process_delay(pRouter, pEBGPRoutes,
				   BGP_OPTIONS_QOS_AGGR_LIMIT);

      assert(ptr_array_length(pRoutes) <= 1);

      if (ptr_array_length(pEBGPRoutes) == 1)
	((SRoute *) pRoutes->data[0])->pEBGPRoute=
	  (SRoute *) pEBGPRoutes->data[0];

    }

    route_copy_count++;
    pRoute= route_copy((SRoute *) pRoutes->data[0]);
    route_flag_set(pRoute, ROUTE_FLAG_BEST, 1);
    
    LOG_DEBUG("\tnew best: ");
    LOG_ENABLED_DEBUG() route_dump(log_get_stream(pMainLog), pRoute);
    LOG_DEBUG("\n");
    
    // New/updated route
    // => install in Loc-RIB
    // => advertise to peers
    if ((pOldRoute == NULL) || !route_equals(pOldRoute, pRoute)) {
      if (pOldRoute != NULL)
	route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);
      assert(rib_add_route(pRouter->pLocRIB, pRoute) == 0);
      qos_decision_process_disseminate(pRouter, sPrefix, pRoute);
    } else {
      route_destroy(&pRoute);
      pRoute= pOldRoute;
    }
  } else if (ptr_array_length(pRoutes) == 0) {
    LOG_DEBUG("no best\n");
    // If a route towards this prefix was previously installed, then
    // withdraw it. Otherwise, do nothing...
    if (pOldRoute != NULL) {
      //LOG_DEBUG("there was a previous best-route\n");
      rib_remove_route(pRouter->pLocRIB, sPrefix);
      //LOG_DEBUG("previous best-route removed\n");
      route_flag_set(pOldRoute, ROUTE_FLAG_BEST, 0);
      // Withdraw should ideally only be sent to peers that already
      // have received this route. Since we do not maintain
      // Adj-RIB-Outs, the 'withdraw' message is sent to all peers...
      qos_decision_process_disseminate(pRouter, sPrefix, NULL);
    }
  }

  // *** unlock all Adj-RIB-Ins ***
  
  routes_list_destroy(&pRoutes);
  routes_list_destroy(&pEBGPRoutes);
  
  AS_LOG_DEBUG(pRouter, " < qos_decision_process.end\n");
#endif
  return 0;
}

// ----- qos_path_aggregate -----------------------------------------
/**
 * PRECONDITIONS:
 * - no loop in paths (should be verified due to SSLD);
 * - AS-Path starts with an AS-SEQUENCE segment
 */
/*
SPath * qos_path_aggregate(uint16_t uNumPaths; SPath * apPaths[])
{
  int iIndex;
  int * piIndexes= MALLOC(uNumPaths*sizeof(int));
  int * piSegIndex= MALLOC(uNumPaths*sizeof(int));
  SPathSegment * ppSegment= MALLOC(uNumPaths*sizeof(SPathSegment *));

  assert(uNumPaths > 0);

  // If only one AS-Path to aggregate, return a copy of it.
  if (uNumPaths == 1)
    return path_copy(apPaths[0]);

  // Initialize indexes and current segments.
  // We start from the end of the paths/segments
  for (iIndex= 0; iIndex < uNumPaths; iIndex++) {
    piIndexes[iIndex]= path_num_segments(apPaths[iIndex])-1;
    ppSegments[iIndex]=
      (SPathSegment *) apPaths[iIndex]->data[piIndexes[iIndex]];
    assert(ppSegments[iIndex]->uType == AS_PATH_SEGMENT_SEQUENCE);
    piSegIndex[iIndex]= ppSegments[iIndex]->uLength-1;
  }

  // We use path[0] as a reference:
  // * First, go over all the current segments (of type AS-SEQUENCE)
  //   to find a common AS number
  while (piIndexes[0] > 0) {
    // Compare current AS number of path[0] to every other paths...
    for (iIndex= 1; iIndex < uPathNums; iIndex++) {
      
    }
  }

  return NULL;
}
*/

/////////////////////////////////////////////////////////////////////
// TEST & VALIDATION SECTION
/////////////////////////////////////////////////////////////////////

void _qos_test()
{
  SRoute * pRoute1;
  SRoute * pRoute2;
  SRoute * pRoute3;
  SRoutes * pRoutes;
  SRoute * pAggrRoute;
  SPrefix sPrefix;
  int iIndex;

  sPrefix.tNetwork= 256;
  sPrefix.uMaskLen= 24;

  pRoute1= route_create(sPrefix, NULL, 1, ROUTE_ORIGIN_IGP);
  pRoute2= route_create(sPrefix, NULL, 2, ROUTE_ORIGIN_IGP);
  pRoute3= route_create(sPrefix, NULL, 3, ROUTE_ORIGIN_IGP);

  pRoute1->tDelay.tDelay= 5;
  pRoute1->tDelay.tMean= 5;
  pRoute1->tDelay.uWeight= 1;

  pRoute2->tDelay.tDelay= 5;
  pRoute2->tDelay.tMean= 7.5;
  pRoute2->tDelay.uWeight= 2;

  pRoute3->tDelay.tDelay= 10;
  pRoute3->tDelay.tMean= 10;
  pRoute3->tDelay.uWeight= 1;

  fprintf(stdout, "route-delay-compare: %d\n",
	  qos_route_compare_delay(&pRoute1, &pRoute2, 0));

  route_path_append(pRoute1, 1);
  route_path_append(pRoute2, 2);
  route_path_append(pRoute3, 3);

  pRoutes= ptr_array_create_ref(0);
  ptr_array_append(pRoutes, pRoute1);
  ptr_array_append(pRoutes, pRoute2);
  ptr_array_append(pRoutes, pRoute3);
  _array_sort((SArray *) pRoutes, qos_route_compare_delay);

  for (iIndex= 0; iIndex < ptr_array_length(pRoutes); iIndex++) {
    fprintf(stdout, "*** ");
    route_dump(stdout, (SRoute *) pRoutes->data[iIndex]);
    fprintf(stdout, "\n");
  }

  pAggrRoute= qos_route_aggregate(pRoutes, pRoute1);

  route_dump(stdout, pAggrRoute); fprintf(stdout, "\n");

  routes_list_destroy(&pRoutes);
  route_destroy(&pRoute1);
  route_destroy(&pRoute2);
  route_destroy(&pRoute3);
  route_destroy(&pAggrRoute);
}

#endif
