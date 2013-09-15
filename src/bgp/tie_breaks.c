// ==================================================================
// @(#)tie_breaks.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 28/07/2003
// $Id: tie_breaks.c,v 1.5 2009-03-24 15:54:50 bqu Exp $
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
// These tie-break functions MUST satisfy the following constraints:
// * return (1) if ROUTE1 is prefered over ROUTE2
// * return (-1) if ROUTE2 is prefered over ROUTE1
// * return (0) if the function can not decide
//
// The later case SHOULD NOT occur since non-determinism is then
// introduced. The decision process WILL ABORT the simulator in
// case the tie-break function can not decide.
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>

#include <libgds/stream.h>

#include <bgp/attr/path.h>
#include <bgp/attr/path_segment.h>
#include <bgp/tie_breaks.h>

// ----- tie_break_next_hop -----------------------------------------
/**
 * This function compares the router-id of the sender of the
 * routes. The route learned from the sender with the lowest router-id
 * is prefered.
 */
int tie_break_next_hop(bgp_route_t * route1, bgp_route_t * route2)
{
  if (route1->attr->next_hop < route2->attr->next_hop)
    return 1;
  else if (route1->attr->next_hop > route2->attr->next_hop)
    return -1;

  return 0;
}

// ----- tie_break_hash ---------------------------------------------
/**
 * This function computes hashes of the paths contained in both routes
 * and decides on this basis which route is prefered. If the hash
 * based decision is not sufficient then, the function tries to find
 * the first
 *
 * PRE-CONDITION: Paths should have the same length.
 */
/*int tie_break_hash(bgp_route_t * route1, bgp_route_t * route2)
{
  int iHash1= path_hash(route1->pASPathRef);
  int iHash2= path_hash(route2->pASPathRef);
  int iIndex1, iIndex2, iSegIndex1, iSegIndex2;
  uint16_t uASNumber1, uASNumber2;
  SPathSegment * pSegment1= NULL, * pSegment2= NULL;

  // Compare hashes
  if (iHash1 > iHash2)
    return 1;
  else if (iHash1 < iHash2)
    return -1;

  // If hashes are equal, find the first different segment in each path
  // and prefer the longuest onlowest-one (hyp: length of AS-Paths
  // should be equal, but a little assert can avoid bad surprises)
  assert(path_length(route1->pASPathRef) == path_length(route2->pASPathRef));

  // The following code finds the first position in both paths for
  // which the corresponding AS differ. The route for which this AS
  // number is the lowest is prefered.
  // For efficiency reasons, this code directly accesses the internal
  // structure of the AS-Paths.

  iIndex1= 0;
  iSegIndex1= 0;
  iIndex2= 0;
  iSegIndex2= 0;

  while ((iIndex1 < path_num_segments(route1->pASPathRef)) &&
	 (iIndex2 < path_num_segments(route2->pASPathRef))) {
    if (pSegment1 == NULL)
      pSegment1= (SPathSegment *) route1->pASPathRef->data[iIndex1];
    if (pSegment2 == NULL)
      pSegment2= (SPathSegment *) route2->pASPathRef->data[iIndex2];
    if (iSegIndex1 >= pSegment1->uLength) {
      iIndex1++;
      if (iIndex1 >= path_num_segments(route1->pASPathRef))
	break;
      pSegment1= route1->pASPathRef->data[iIndex1];
      iSegIndex1= 0;
    }
    if (iSegIndex2 >= pSegment2->uLength) {
      iIndex2++;
      if (iIndex2 >= path_num_segments(route2->pASPathRef))
	break;
      pSegment2= route2->pASPathRef->data[iIndex2];
      iSegIndex2= 0;
    }
    switch (pSegment1->uType) {
    case AS_PATH_SEGMENT_SEQUENCE:
      uASNumber1= pSegment1->auValue[iSegIndex1];
      iSegIndex1++;
      break;
    case AS_PATH_SEGMENT_SET:
      uASNumber1= 65535;
      while (iSegIndex1 < pSegment1->uLength)
	if (pSegment1->auValue[iSegIndex1] < uASNumber1)
	  uASNumber1= pSegment1->auValue[iSegIndex1];
      break;
    default:
      abort();
    }
    switch (pSegment2->uType) {
    case AS_PATH_SEGMENT_SEQUENCE:
      uASNumber2= pSegment2->auValue[iSegIndex2];
      iSegIndex2++;
      break;
    case AS_PATH_SEGMENT_SET:
      uASNumber2= 65535;
      while (iSegIndex2 < pSegment2->uLength)
	if (pSegment2->auValue[iSegIndex2] < uASNumber2)
	  uASNumber2= pSegment2->auValue[iSegIndex2];
      break;
    default:
      abort();
    }
    if (uASNumber1 < uASNumber2)
      return 1;
    else if (uASNumber1 > uASNumber2)
      return -1;
  }

  // If the function was not able to decide on the basis of the above
  // rules, it relies on a final decision function which is based on
  // the router-id.
  return tie_break_router_id(route1, route2);
  }*/

// ----- tie_break_low_ISP ------------------------------------------
/**
 *
 */
 /*int tie_break_low_ISP(bgp_route_t * route1, bgp_route_t * route2)
{
  if ((path_length(route1->pASPathRef) > 1) &&
      (path_length(route2->pASPathRef) > 1)) {
    
    //if (route1->pASPath->asPath[1] < route2->pASPath->asPath[1])
    //  return 1;
    //else if (route1->pASPath->asPath[1] > route2->pASPath->asPath[1])
    //  return -1;
    
  }
  return tie_break_hash(route1, route2);
}*/

// ----- tie_break_high_ISP -----------------------------------------
/**
 *
 */
  /*int tie_break_high_ISP(bgp_route_t * route1, bgp_route_t * route2)
{
  if ((path_length(route1->pASPathRef) > 1) &&
      (path_length(route2->pASPathRef) > 1)) {
  
  //  if (route1->pASPath->asPath[1] > route2->pASPath->asPath[1])
  //    return 1;
  //  else if (route1->pASPath->asPath[1] < route2->pASPath->asPath[1])
  //    return -1;
  
  }
  return tie_break_hash(route1, route2);
}*/
