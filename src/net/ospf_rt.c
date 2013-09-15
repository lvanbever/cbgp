// ==================================================================
// @(#)ospf_rt.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/06/2005
// $Id: ospf_rt.c,v 1.16 2009-03-24 16:22:01 bqu Exp $
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

#ifdef OSPF_SUPPORT

#include <string.h>
#include <net/ospf_rt.h>
#include <net/routing_t.h>
#include <net/prefix.h>
#include <net/node.h>
#include <assert.h>
#include <net/link-list.h>
#include <net/net_types.h>
/*only for test function*/
#include <net/network.h>


//////////////////////////////////////////////////////////////////////////
///////  next hop object & next hop list object
//////////////////////////////////////////////////////////////////////////
// ----- ospf_next_hop_create ---------------------------------------
/* A Next Hop is a couple Link + IpAddress.
   IpAddress is used to distinguish beetween more than one node reachable
   towards the same link (typically when link is toward transit network)
*/
SOSPFNextHop * ospf_next_hop_create(net_iface_t * pLink, net_addr_t tAddr)
{
  SOSPFNextHop * pNH = (SOSPFNextHop *) MALLOC(sizeof(SOSPFNextHop));
  pNH->pLink      = pLink;
  pNH->tAddr      = tAddr;
  pNH->tAdvRouterAddr = 0;
  return pNH;
}

void ospf_next_hop_destroy(SOSPFNextHop ** ppNH)
{
  if (*ppNH != NULL){
    FREE(*ppNH);
    *ppNH = NULL;
  }
}

// ----- ospf_next_hops_compare -----------------------------------------------
int ospf_next_hops_compare(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SOSPFNextHop * pNH1= *((SOSPFNextHop **) pItem1);
  SOSPFNextHop * pNH2= *((SOSPFNextHop **) pItem2);
  
  int iCmpResult =  _net_links_link_compare(&(pNH1->pLink), &(pNH2->pLink), 0);

  if (iCmpResult == 0) {
    if (pNH1->tAddr > pNH2->tAddr)
      iCmpResult = 1;
    else if (pNH1->tAddr < pNH2->tAddr)
      iCmpResult = -1;
    else 
      iCmpResult =  0;
  }
  return iCmpResult;
}

// ----- ospf_next_hop_dst -----------------------------------------
void ospf_next_hop_dst(void * pItem)
{
  SOSPFNextHop * pNH= *((SOSPFNextHop **) pItem);
  
  ospf_next_hop_destroy(&pNH);
}


// ----- OSPF_next_hop_dump --------------------------------------------
void ospf_next_hop_dump(FILE* pStream, SOSPFNextHop * pNH, int iPathType) { 
  char cAddr[20], * pcAddr = cAddr; 
   
  fprintf(pStream, "IF ");
  ip_prefix_to_string(pcAddr, net_iface_id(pNH->pLink));
  fprintf(pStream, "%s\tNH ", pcAddr);
  if (pNH->tAddr != OSPF_NO_IP_NEXT_HOP)
    ip_address_dump(pStream, pNH->tAddr);
  else
    fprintf(pStream, " --  ");
  if (iPathType == OSPF_PATH_TYPE_INTER)
    ip_address_dump(pStream, pNH->tAdvRouterAddr);
}

// ----- OSPF_next_hop_string --------------------------------------------
void ospf_next_hop_to_string(char * pString, SOSPFNextHop * pNH)
{ 
  SPrefix sPrefix;
  char * pcTmp;
  
  sprintf(pString, "IF <");
  link_get_prefix(pNH->pLink, &sPrefix);
  pcTmp = pString + strlen("IF <");
  ip_prefix_to_string(pcTmp, &sPrefix);
  strcat(pString, "> IP <");
  pcTmp = pString + strlen(pString);
  if (pNH->tAddr != OSPF_NO_IP_NEXT_HOP)
    ip_address_to_string(pcTmp, pNH->tAddr);
  else
    strcat(pString, "-DIRECT-");
  strcat(pString, ">");
}

// ----- ospf_nh_list_length --------------------------------------------
int ospf_nh_list_length(next_hops_list_t * pNHList)
{
  return ptr_array_length(pNHList);
}

// ----- ospf_nh_list_get_at --------------------------------------------------
void ospf_nh_list_get_at(next_hops_list_t * pNHList, int iIndex, SOSPFNextHop ** ppNH)
{
  ptr_array_get_at(pNHList, iIndex, ppNH);
} 


// ----- ospf_nh_list_create --------------------------------------------------
next_hops_list_t * ospf_nh_list_create()
{
  return ptr_array_create(ARRAY_OPTION_UNIQUE|ARRAY_OPTION_SORTED, ospf_next_hops_compare, 
                                               ospf_next_hop_dst);
}

// ----- ospf_nh_list_destroy --------------------------------------------------
void ospf_nh_list_destroy(next_hops_list_t ** pNHList)
{
  ptr_array_destroy((SPtrArray **) pNHList);
}

// ----- ospf_nh_list_add --------------------------------------------------
int ospf_nh_list_add(next_hops_list_t * pNHList, SOSPFNextHop * pNH)
{
  return ptr_array_add(pNHList, &pNH);
}

// ----- ospf_nh_list_copy --------------------------------------------------
next_hops_list_t * ospf_nh_list_copy(next_hops_list_t * pNHList)
{
  next_hops_list_t * pNHLCopy = ospf_nh_list_create();
  int iIndex;
  SOSPFNextHop * pCurrentNH, *pNHCopy;
  
  for (iIndex = 0; iIndex < ptr_array_length(pNHList); iIndex++){
    ptr_array_get_at(pNHList, iIndex, &pCurrentNH);
    //TODO sostituire con memcopy
    pNHCopy = ospf_next_hop_create(pCurrentNH->pLink, pCurrentNH->tAddr);
    pNHCopy->tAdvRouterAddr = pCurrentNH->tAdvRouterAddr; 
    ospf_nh_list_add(pNHLCopy, pNHCopy);
  }
  return pNHLCopy;
}

// ----- ospf_nh_list_find --------------------------------------------------
/*next_hops_list_t * ospf_nh_list_find(next_hops_list_t * pNHList, SNetDest sDest)
{
  SOSPFNextHop * pWrapNH, sWrapNH; 
  net_iface_t * pWrapLink, sWrapLink;
  pWrapNH = &sWrapNH;
  pWrapLink = &sWrapLink;
  pWrapNH->pLink = pWrapLink;
  
  if (sDest.tType == NET_DEST_ADDR) //dest is a router
    (pWrapLink->UDestId).tAddr = sDest.uDest.tAddr;
  else if (sDest.tType == NET_DEST_PREFIX) //dest is a subnet
    
}*/


// ----- ospf_nh_list_add_list -------------------------------------------------
/** 
  *  Add next hops in pNHListSource to next hops list pNHListDest.
  * Make a copy of each added next hop.
  * Adding fails if NH is already in pNHListDest. 
*/
void ospf_nh_list_add_list(next_hops_list_t * pNHListDest, next_hops_list_t * pNHListSource)
{
  int iIndex;
  SOSPFNextHop * pCurrentNH, * pNHCopy;
  
  for (iIndex = 0; iIndex < ptr_array_length(pNHListSource); iIndex++){
    ptr_array_get_at(pNHListSource, iIndex, &pCurrentNH);
    pNHCopy = ospf_next_hop_create(pCurrentNH->pLink, pCurrentNH->tAddr);
    pNHCopy->tAdvRouterAddr = pCurrentNH->tAdvRouterAddr;
    //if next hop is already present add function fails
    if (ospf_nh_list_add(pNHListDest, pNHCopy) < 0) 
      ospf_next_hop_destroy(&pNHCopy);
  }
}

// ----- ospf_nh_list_dump --------------------------------------------------
/*
   pcSpace should not be NULL! 
   USAGE pcSapace = "" or 
         pcSpace = "\t"
*/
void ospf_nh_list_dump(FILE * pStream, next_hops_list_t * pNHList, 
		       int iPathType, char * pcSpace)
{
  int iIndex;
  SOSPFNextHop  sNH, * pNH;
  pNH = &sNH;
  assert(pNHList != NULL);
//   LOG_DEBUG("ospf_nh_list_dump\n");
  int iStop =  ospf_nh_list_length(pNHList);
  for (iIndex = 0; iIndex < iStop; iIndex++) {
    ptr_array_get_at(pNHList, iIndex, &pNH);
    ospf_next_hop_dump(pStream, pNH, iPathType);
    if (iIndex != iStop - 1)
      fprintf(pStream, "%s", pcSpace);
  }
}


//////////////////////////////////////////////////////////////////////////
///////  route info list method
//////////////////////////////////////////////////////////////////////////
// ----- OSPF_rt_route_info_destroy --------------------------------------
void OSPF_rt_route_info_destroy(void ** ppItem)
{
  OSPF_route_info_destroy((SOSPFRouteInfo **) ppItem);
}

// ----- OSPF_rt_info_list_cmp -------------------------------------------
/**
 * Compare two routes towards the same destination 
 * [for details see chap. 11 of RFC2328]
 TODO 
 * - prefer the route with the lowest type (STATIC > IGP > BGP)
 * - prefer route with smallest path-thype (EXT1 > EXT2 > INTER > INTRA)
 * - prefer route with smallest cost
 * - prefer route with smallest next-hop
 */
int OSPF_rt_info_list_cmp(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SOSPFRouteInfo * pRI1= *((SOSPFRouteInfo **) pItem1);
  SOSPFRouteInfo * pRI2= *((SOSPFRouteInfo **) pItem2);

  // Prefer lowest routing protocol type STATIC > IGP > BGP
  if (pRI1->tType > pRI2->tType)
    return 1;
  if (pRI1->tType < pRI2->tType)
    return -1;

  // Prefer route with lowest metric
  if (pRI1->uWeight > pRI2->uWeight)
    return 1;
  if (pRI1->uWeight < pRI2->uWeight)
    return -1;

  // Tie-break: prefer route with lowest next-hop address
  /*if (link_get_addr(pRI1->pNextHopIf) > link_get_addr(pRI2->pNextHopIf))
    return 1;
  if (link_get_addr(pRI1->pNextHopIf) < link_get_addr(pRI2->pNextHopIf))
    return- 1;*/
  
  return 0;
}

// ----- OSPF_rt_info_list_dst -------------------------------------------
void OSPF_rt_info_list_dst(void * pItem)
{
  SOSPFRouteInfo * pRI= *((SOSPFRouteInfo **) pItem);
  OSPF_route_info_destroy(&pRI);
}

// ----- OSPF_rt_info_list_create ----------------------------------------
SOSPFRouteInfoList * OSPF_rt_info_list_create()
{
  return (SOSPFRouteInfoList *) ptr_array_create(ARRAY_OPTION_SORTED|
						ARRAY_OPTION_UNIQUE,
						OSPF_rt_info_list_cmp,
						OSPF_rt_info_list_dst);
}

// ----- OSPF_rt_info_list_destroy ---------------------------------------
void OSPF_rt_info_list_destroy(SOSPFRouteInfoList ** ppRouteInfoList)
{
  SPtrArray * pPtrArray = (SPtrArray *)(*ppRouteInfoList);
  ptr_array_destroy(&pPtrArray);
}

// ----- OSPF_rt_info_list_length ----------------------------------------
int OSPF_rt_info_list_length(SOSPFRouteInfoList * pRouteInfoList)
{
  return _array_length((SArray *) pRouteInfoList);
}

// ----- rt_info_list_get -------------------------------------------
SOSPFRouteInfo * OSPF_rt_info_list_get(SOSPFRouteInfoList * pRouteInfoList,
				 int iIndex)
{
  if (iIndex < ptr_array_length((SPtrArray *) pRouteInfoList))
    return pRouteInfoList->data[iIndex];
  return NULL;
}

// ----- OSPF_rt_info_list_add -------------------------------------------
int OSPF_rt_info_list_add(SOSPFRouteInfoList * pRouteInfoList,
			SOSPFRouteInfo * pRouteInfo)
{
  if (ptr_array_add((SPtrArray *) pRouteInfoList,
		    &pRouteInfo) < 0) {
    return OSPF_RT_ERROR_ADD_DUP;
  }
  return OSPF_RT_SUCCESS;
}



//////////////////////////////////////////////////////////////////////////
/////// OSPF methods for routing table
//////////////////////////////////////////////////////////////////////////

// ----- OSPF_route_info_create -----------------------------------------------
/**
 *ospf_dest_type_t  uOSPFDestinationType;
  SPrefix           sPrefix;
  uint32_t          uWeight;
  ospf_area_t       uOSPFArea;
  ospf_path_type_t  uOSPFPathType;
  SPtrArray *       pNextHops;
  net_route_type_t  tType 
 */
SOSPFRouteInfo * OSPF_route_info_create(ospf_dest_type_t  tOSPFDestinationType,
                                       SPrefix            sPrefix,
				       net_link_delay_t   uWeight,
				       ospf_area_t        tOSPFArea,
				       ospf_path_type_t   tOSPFPathType,
				       next_hops_list_t * pNHList)
{
  SOSPFRouteInfo * pRouteInfo=
    (SOSPFRouteInfo *) MALLOC(sizeof(SOSPFRouteInfo));
  
  pRouteInfo->tOSPFDestinationType = tOSPFDestinationType;
  pRouteInfo->sPrefix              = sPrefix;
  pRouteInfo->uWeight              = uWeight;
  pRouteInfo->tOSPFArea            = tOSPFArea;
  pRouteInfo->tOSPFPathType        = tOSPFPathType;
  pRouteInfo->tType                = NET_ROUTE_IGP;
  
  pRouteInfo->aNextHops = pNHList;
  
  return pRouteInfo;
}

// ----- route_info_destroy ----------------------------------------------------
int OSPF_route_info_add_nextHop(SOSPFRouteInfo * pRouteInfo, SOSPFNextHop * pNH) 
{
  return ospf_nh_list_add(pRouteInfo->aNextHops, pNH);
}

// ----- OSPF_route_info_destroy -----------------------------------------------
/**
 *
 */
void OSPF_route_info_destroy(SOSPFRouteInfo ** ppOSPFRouteInfo)
{
  if (*ppOSPFRouteInfo != NULL) {
//     fprintf(stdout, "\nOSPF_route_info_destroy(): ");
//     OSPF_route_info_dump(stdout, (*ppOSPFRouteInfo));
    if ((*ppOSPFRouteInfo)->aNextHops != NULL){     
      
//       ospf_nh_list_dump(stdout, (*ppOSPFRouteInfo)->aNextHops, "\n");
      ospf_nh_list_destroy(&((*ppOSPFRouteInfo)->aNextHops));
    }
    FREE(*ppOSPFRouteInfo);
    *ppOSPFRouteInfo= NULL;
  }
}

// ----- OSPF_dest_type_dump -----------------------------------------
void OSPF_dest_type_dump(FILE * pStream, ospf_dest_type_t tDestType)
{
  switch (tDestType) {
    case OSPF_DESTINATION_TYPE_NETWORK : 
           fprintf(pStream, "N");
	   break;
    case OSPF_DESTINATION_TYPE_ROUTER : 
           fprintf(pStream, "R");
	   break;
    default : 
           fprintf(pStream, "???");
  }
}

// ----- OSPF_area_dump -----------------------------------------
void OSPF_area_dump(FILE * pStream, ospf_area_t tOSPFArea)
{
  if (tOSPFArea == 0)
    fprintf(pStream," B ");
  else
    fprintf(pStream," %d ", tOSPFArea);
}

// ----- OSPF_path_type_dump -----------------------------------------
void OSPF_path_type_dump(FILE * pStream, ospf_path_type_t tOSPFPathType)
{
  switch (tOSPFPathType) {
    case OSPF_PATH_TYPE_INTRA : 
           fprintf(pStream, "INTRA");
	   break;
    case OSPF_PATH_TYPE_INTER : 
           fprintf(pStream, "INTER");
	   break;
    case OSPF_PATH_TYPE_EXTERNAL_1 : 
           fprintf(pStream, "EXT1");
	   break;
    case OSPF_PATH_TYPE_EXTERNAL_2 : 
           fprintf(pStream, "EXT2");
	   break;
    default : 
           fprintf(pStream, "%d", tOSPFPathType);
  }
}

// ----- OSPF_route_type_dump --------------------------------------------
/**
 *
 */
void OSPF_route_type_dump(FILE * pStream, net_route_type_t tType)
{
  switch (tType) {
  case NET_ROUTE_STATIC:
    fprintf(pStream, "STATIC");
    break;
  case NET_ROUTE_IGP:
    fprintf(pStream, "IGP");
    break;
  case NET_ROUTE_BGP:
    fprintf(pStream, "BGP");
    break;
  default:
    fprintf(pStream, "???");
  }
}

typedef struct { 
  FILE *   pStream;
  uint32_t uCount;
} SNextHop_Dump_Context;



// ----- OSPF_rt_find_exact ----------------------------------------------
/**
 * Find the route that exactly matches the given prefix. If a
 * particular route type is given, returns only the route with the
 * requested type.
 *
 * Parameters:
 * - routing table
 * - prefix
 * - route type (can be NET_ROUTE_ANY if any type is ok)
 * - ospf area (can be NO_OSPF_AREA if any area is ok)
 */
SOSPFRouteInfo * OSPF_rt_find_exact(SOSPFRT * pRT, SPrefix sPrefix,
			      net_route_type_t tType, ospf_area_t tArea)
{
  SOSPFRouteInfoList * pRIList;
  int iIndex;
  SOSPFRouteInfo * pRouteInfo;

  /* First, retrieve the list of routes that exactly match the given
     prefix */
  pRIList= (SOSPFRouteInfoList *)
    trie_find_exact((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen);

  /* Then, select the first returned route that matches the given
     route-type (if requested) */
  if (pRIList != NULL) {
    assert(OSPF_rt_info_list_length(pRIList) != 0);

    for (iIndex= 0; iIndex < ptr_array_length(pRIList); iIndex++) {
      pRouteInfo= OSPF_rt_info_list_get(pRIList, iIndex);
      if (pRouteInfo->tType & tType)
	if (tArea == ospf_ri_get_area(pRouteInfo) ||
			tArea == OSPF_NO_AREA)
          return pRouteInfo;
    }
  }

  return NULL;
}

typedef struct {
  SPrefix * pPrefix;
  SOSPFNextHop * pNextHop;
  net_route_type_t tType;
  SPtrArray * pRemovalList;
} SOSPFRouteInfoDel;


// ----- OSPF_info_prefix_dst -----------------------------------------
void OSPF_info_prefix_dst(void * pItem)
{
  ip_prefix_destroy((SPrefix **) pItem);
}

// ----- net_info_schedule_removal -----------------------------------
/**
 * This function adds a prefix whose entry in the routing table must
 * be removed. This is used when a route-info-list has been emptied.
 */
void OSPF_info_schedule_removal(SOSPFRouteInfoDel * pRIDel,
			       SPrefix * pPrefix)
{
  if (pRIDel->pRemovalList == NULL)
    pRIDel->pRemovalList= ptr_array_create(0, NULL, OSPF_info_prefix_dst);

  pPrefix= ip_prefix_copy(pPrefix);
  ptr_array_append(pRIDel->pRemovalList, pPrefix);
}

// ----- net_info_removal_for_each ----------------------------------
/**
 *
 */
int OSPF_info_removal_for_each(void * pItem, void * pContext)
{
  SNetRT * pRT= (SNetRT *) pContext;
  SPrefix * pPrefix= *(SPrefix **) pItem;

  return trie_remove(pRT, pPrefix->tNetwork, pPrefix->uMaskLen);
}

// ----- OSPF_net_info_removal -------------------------------------------
/**
 * Remove the scheduled prefixes from the routing table.
 */
int OSPF_info_removal(SOSPFRouteInfoDel * pRIDel,
			SOSPFRT * pRT)
{
  int iResult= 0;

  if (pRIDel->pRemovalList != NULL) {
    iResult= _array_for_each((SArray *) pRIDel->pRemovalList,
			     OSPF_info_removal_for_each,
			     pRT);
    ptr_array_destroy(&pRIDel->pRemovalList);
  }
  return iResult;
}

// ----- OSPF_rt_info_list_del -------------------------------------------
/**
 * This function removes from the route-list all the routes that match
 * the given attributes: next-hop and route type. A wildcard can be
 * used for the next-hop attribute (by specifying a NULL next-hop
 * link).
 *
 * NOTE: If a route is found and next hop is specified, and the route
 *       has more than one next hop, only the next hop that matches
 *       is removed, while the route no.
 */
int OSPF_rt_info_list_del(SOSPFRouteInfoList * pRouteInfoList,
			  SOSPFRouteInfoDel * pRIDel,
			  SPrefix * pPrefix)
{
  int iIndex;
  unsigned int uNHPos;
  SOSPFRouteInfo * pRI;
  int iRemoved= 0;
  int iResult= NET_RT_ERROR_DEL_UNEXISTING;

  /* Lookup the whole list of routes... */
  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
       iIndex++) {
    pRI= (SOSPFRouteInfo *) pRouteInfoList->data[iIndex];

    /* If the type matches and the next-hop is found or we do not care
       about the next-hop, then remove the route */
    if (((pRIDel->pNextHop == NULL) ||
	 ptr_array_sorted_find_index((SPtrArray *)(pRI->aNextHops),  &(pRIDel->pNextHop), &uNHPos) == 0) &&
	(pRI->tType == pRIDel->tType)) {
      
	//if we care about next hop and there are more than one next hops...
	if ((pRIDel->pNextHop != NULL) && (ptr_array_length((SPtrArray *)(pRI->aNextHops)) > 1))
	  ptr_array_remove_at((SPtrArray *)(pRI->aNextHops), uNHPos);//... remove only next hop 
	else
	  ptr_array_remove_at((SPtrArray *) pRouteInfoList, iIndex);//... remove the route
      iRemoved++;

      /* If we don't use wildcards for the next-hop, we can stop here
	 since there can not be multiple routes in the list with the
	 same next-hop */
//       if (pRIDel->pNextHopIf == NULL) {
// 	iResult= NET_RT_SUCCESS;
// 	break;
//       }

    }
    
  }
  
  /* If the list of routes does not contain any route, it should be
     destroyed. Schedule the prefix for removal... */
   if (OSPF_rt_info_list_length(pRouteInfoList) == 0)
     OSPF_info_schedule_removal(pRIDel, pPrefix);

  if (iRemoved > 0)
    iResult= NET_RT_SUCCESS;

  return iResult;
}

// ----- OSPF_rt_for_each ------------------------------------------------------
int OSPF_rt_for_each(SNetRT * pRT, FRadixTreeForEach fForEach, void * pContext){  
  return trie_for_each(pRT, fForEach, pContext);
}


// ----- OSPF_rt_del_for_each --------------------------------------------
/**
 * Helper function for the 'OSPF_rt_del_route' function. Handles the
 * deletion of a single prefix (can be called by
 * 'radix_tree_for_each')
 */
int OSPF_rt_del_for_each(uint32_t uKey, uint8_t uKeyLen,
		    void * pItem, void * pContext)
{
  SOSPFRouteInfoList * pRIList = (SOSPFRouteInfoList *) pItem;
  SOSPFRouteInfoDel  * pRIDel  = (SOSPFRouteInfoDel *)  pContext;
  SPrefix sPrefix;
  int iResult;

  if (pRIList == NULL)
    return -1;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  /* Remove from the list all the routes that match the given
     attributes */
  iResult= OSPF_rt_info_list_del(pRIList, pRIDel, &sPrefix);

  /* If we use widlcards, the call never fails, otherwise, it depends
     on the 'rt_info_list_del' call */
  return ((pRIDel->pPrefix != NULL) &&
	  (pRIDel->pNextHop != NULL)) ? iResult : 0;
}

// ----- OSPF_rt_del_route -----------------------------------------------
/**
 * Remove route(s) from the given routing table. The route(s) to be
 * removed must match the given attributes: prefix, next-hop and
 * type. However, wildcards can be used for the prefix and next-hop
 * attributes. The NULL value corresponds to the wildcard.
 *
 * NOTE: if a next hop is passed and a route with the same next hop 
 *       is found, and the route has more than one next hop, 
 *       only the next hop is destroyed
 */
int OSPF_rt_del_route(SOSPFRT * pRT, SPrefix * pPrefix, SOSPFNextHop * pNextHop,
		 net_route_type_t tType)
{
  SOSPFRouteInfoList * pRIList;
  SOSPFRouteInfoDel sRIDel;
  int iResult;

  /* Prepare the attributes of the routes to be removed (context) */
  sRIDel.pPrefix= pPrefix;
  sRIDel.pNextHop= pNextHop;
  sRIDel.tType= tType;
  sRIDel.pRemovalList= NULL;

  /* Prefix specified ? or wildcard ? */
  if (pPrefix != NULL) {

    /* Get the list of routes towards the given prefix */
    pRIList=
      (SOSPFRouteInfoList *) trie_find_exact((STrie *) pRT,
					    pPrefix->tNetwork,
					    pPrefix->uMaskLen);

    iResult= OSPF_rt_del_for_each(pPrefix->tNetwork, pPrefix->uMaskLen,
			     pRIList, &sRIDel);

  } else {

    /* Remove all the routes that match the given attributes, whatever
       the prefix is */
    iResult= trie_for_each((STrie *) pRT, OSPF_rt_del_for_each, &sRIDel);

  }

  OSPF_info_removal(&sRIDel, pRT);

  return iResult;
}

// ----- OSPF_route_info_dump --------------------------------------------
/**
 * Output format:
 * <dst-prefix> <link/if> <weight> <type> [ <state> ]
 */
void OSPF_route_info_dump(FILE * pStream, SOSPFRouteInfo * pRouteInfo)
{
  fprintf(pStream, " ");
  OSPF_dest_type_dump(pStream, pRouteInfo->tOSPFDestinationType);
  fprintf(pStream, "   ");
  ip_prefix_dump(pStream, pRouteInfo->sPrefix);
  fprintf(pStream, "\t"); 
  OSPF_path_type_dump(pStream, pRouteInfo->tOSPFPathType);
  fprintf(pStream, "   ");
  OSPF_area_dump(pStream, pRouteInfo->tOSPFArea);
  fprintf(pStream, "   ");
  
  //OSPF_route_type_dump(pStream, pRouteInfo->tType);
  //fprintf(pStream, "\t%u\t", pRouteInfo->uWeight);
  ospf_nh_list_dump(stdout, pRouteInfo->aNextHops, 
		    pRouteInfo->tType, "\n\t\t\t\t\t\t\t\t");
}


// ----- OSPF_rt_il_dst --------------------------------------------------
void OSPF_rt_il_dst(void ** ppItem)
{
  OSPF_rt_info_list_destroy((SOSPFRouteInfoList **) ppItem);
}

// ----- OSPF_rt_create --------------------------------------------------
/**
 *
 */
SOSPFRT * OSPF_rt_create()
{
  return (SOSPFRT *) trie_create(OSPF_rt_il_dst);
}

// ----- OSPF_rt_destroy -------------------------------------------------
/**
 *
 */
void OSPF_rt_destroy(SOSPFRT ** ppRT)
{
  trie_destroy((STrie **) ppRT);
}


// ----- OSPF_rt_add_route -----------------------------------------------
/**
 *
 */
int OSPF_rt_add_route(SOSPFRT * pRT, SPrefix sPrefix,
		 SOSPFRouteInfo * pRouteInfo)
{
  SOSPFRouteInfoList * pRIList;
  // LOG_DEBUG("OSPF_rt_add_route...enter\n");
  assert(pRT != NULL);
  pRIList=
    (SOSPFRouteInfoList *) trie_find_exact((STrie *) pRT,
					  sPrefix.tNetwork,
					  sPrefix.uMaskLen);
  //  LOG_DEBUG("OSPF_rt_add_route...\n");
  if (pRIList == NULL) {
    pRIList= OSPF_rt_info_list_create();
    trie_insert((STrie *) pRT, sPrefix.tNetwork, sPrefix.uMaskLen, pRIList);
  }

  return OSPF_rt_info_list_add(pRIList, pRouteInfo);
}



// ----- OSPF_rt_perror -------------------------------------------------------
/**
 *
 */
void OSPF_rt_perror(FILE * pStream, int iErrorCode)
{
  switch (iErrorCode) {
  case OSPF_RT_SUCCESS:
    fprintf(pStream, "Success"); break;
  case OSPF_RT_ERROR_NH_UNREACH:
    fprintf(pStream, "Next-Hop is unreachable"); break;
  case OSPF_RT_ERROR_IF_UNKNOWN:
    fprintf(pStream, "Interface is unknown"); break;
  case OSPF_RT_ERROR_ADD_DUP:
    fprintf(pStream, "Route already exists"); break;
  case OSPF_RT_ERROR_DEL_UNEXISTING:
    fprintf(pStream, "Route does not exist"); break;
  default:
    fprintf(pStream, "Unknown error");
  }
}

typedef struct {
  FILE * pStream;
  ospf_area_t tArea;
  ospf_path_type_t tPathType;
  int    iOption;
  int *  piPreintedRoutes;
} SOspfRtDumpContext;

// ----- OSPF_rt_info_list_dump ------------------------------------------
int OSPF_rt_info_list_dump(FILE * pStream, SOSPFRouteInfoList * pRouteInfoList, SOspfRtDumpContext * pContext)
{
  int iIndex;
  
    for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRouteInfoList);
	 iIndex++) {
	 
      switch (pContext->iOption){
        case 0                              : 
	       OSPF_route_info_dump(pStream, (SOSPFRouteInfo *) pRouteInfoList->data[iIndex]);
	       fprintf(pStream, "\n"); 
	       (*(pContext->piPreintedRoutes))++;
	       break;
	       
        case OSPF_RT_OPTION_SORT_AREA       : {
	       if (((SOSPFRouteInfo *)(pRouteInfoList->data[iIndex]))->tOSPFArea == pContext->tArea){
	         OSPF_route_info_dump(pStream, (SOSPFRouteInfo *) pRouteInfoList->data[iIndex]);
		 fprintf(pStream, "\n");
		 (*(pContext->piPreintedRoutes))++;
	       }
	     }
	     break;
	     
	case OSPF_RT_OPTION_SORT_PATH_TYPE  : {
	       if (((SOSPFRouteInfo *)(pRouteInfoList->data[iIndex]))->tOSPFPathType == pContext->tPathType){
	         OSPF_route_info_dump(pStream, (SOSPFRouteInfo *) pRouteInfoList->data[iIndex]);
		 fprintf(pStream, "\n");
		 (*(pContext->piPreintedRoutes))++;
	       }
	     }
	     break;
	     
	case OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE  : 
//	       fprintf(stdout, "Path type to print: %d/%d Area: %d/%d\n", 
//			       pContext->tPathType,((SOSPFRouteInfo *)(pRouteInfoList->data[iIndex]))->tOSPFPathType, 
//			       pContext->tArea, ((SOSPFRouteInfo *)(pRouteInfoList->data[iIndex]))->tOSPFArea);
	       if (((SOSPFRouteInfo *)(pRouteInfoList->data[iIndex]))->tOSPFArea == pContext->tArea && 
	           ((SOSPFRouteInfo *)(pRouteInfoList->data[iIndex]))->tOSPFPathType == pContext->tPathType){
	         OSPF_route_info_dump(pStream, (SOSPFRouteInfo *) pRouteInfoList->data[iIndex]);
		 fprintf(pStream, "\n");
		 (*(pContext->piPreintedRoutes))++;
	       }
	     break;
      }
    }
  return 0;
}

// ----- OSPF_rt_dump_for_each -------------------------------------------
int OSPF_rt_dump_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem, void * pContext)
{
  FILE * pStream= ((SOspfRtDumpContext *)(pContext))->pStream;
//   ospf_area_t tArea= ((SOspfRtDumpContext *)(pContext))->tArea;
//   ospf_area_t tPathType= ((SOspfRtDumpContext *)(pContext))->tPathType;
//   int iOption= ((SOspfRtDumpContext *)(pContext))->iOption;
  
  SOSPFRouteInfoList * pRIList= (SOSPFRouteInfoList *) pItem;
  SPrefix sPrefix;

  sPrefix.tNetwork= uKey;
  sPrefix.uMaskLen= uKeyLen;

  return OSPF_rt_info_list_dump(pStream, pRIList, pContext);
}

// ----- OSPF_rt_dump ----------------------------------------------------
/**
 * Dump the routing table for the given destination. The destination
 * can be of type NET_DEST_ANY, NET_DEST_ADDRESS and NET_DEST_PREFIX.
 */
int OSPF_rt_dump(FILE * pStream, SOSPFRT * pRT, int iOption, ospf_path_type_t tPathType, 
                 ospf_area_t tArea, int * piPrintedRoutes)
{
  SOspfRtDumpContext sDumpContext;
  sDumpContext.pStream         = pStream;
  sDumpContext.tPathType       = tPathType;
  sDumpContext.tArea           = tArea;
  sDumpContext.iOption         = iOption;
  sDumpContext.piPreintedRoutes = piPrintedRoutes;
  
  return trie_for_each((STrie *) pRT, OSPF_rt_dump_for_each, &sDumpContext);
}

// ----- ospf_rt_test() -----------------------------------------------
int ospf_rt_test(){
  char * pcEndPtr;
  SPrefix  sSubnetPfxTx;
  SPrefix  sSubnetPfxTx1;
  net_addr_t tAddrA, tAddrB, tAddrC;
  
  LOG_DEBUG("ospf_rt_test(): START\n");
  assert(!ip_string_to_address("192.168.0.2", &pcEndPtr, &tAddrA));
  assert(!ip_string_to_address("192.168.0.3", &pcEndPtr, &tAddrB));
  assert(!ip_string_to_address("192.168.0.4", &pcEndPtr, &tAddrC));
  
  net_node_t * pNodeA= node_create(tAddrA);
  net_node_t * pNodeB= node_create(tAddrB);
  net_node_t * pNodeC= node_create(tAddrC);
  
  assert(!ip_string_to_prefix("192.168.0.0/24", &pcEndPtr, &sSubnetPfxTx));
  assert(!ip_string_to_prefix("192.120.0.0/24", &pcEndPtr, &sSubnetPfxTx1));
  
  net_subnet_t * pSubTx = subnet_create(sSubnetPfxTx.tNetwork,
                                              sSubnetPfxTx.uMaskLen, NET_SUBNET_TYPE_TRANSIT);
//   net_subnet_t * pSubTx1 = subnet_create(sSubnetPfxTx1.tNetwork,
//                                                sSubnetPfxTx.uMaskLen,
// 					       NET_SUBNET_TYPE_TRANSIT);
   
  LOG_DEBUG("ospf_rt_test(): BUILDS small network for test: 3 router A,B,C on a subnet Tx... ");
  assert(node_add_link_to_subnet(pNodeA, pSubTx, 1, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB, pSubTx, 2, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeC, pSubTx, 3, 300, 1) >= 0);
  assert(node_add_link_to_router(pNodeA, pNodeC, 100, 1) >= 0);
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_rt_test(): CHECK node_find_link_to_subnet...");
  net_iface_t * pLinkAS = node_find_link(pNodeA, ip_address_to_dest(1));
  assert(pLinkAS != NULL);
  net_iface_t * pLinkAC = node_find_link(pNodeA, ip_address_to_dest(tAddrC));
  assert(pLinkAC != NULL);
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_rt_test(): CHECK next hop features...");
  SOSPFNextHop * pNHB = ospf_next_hop_create(pLinkAS, tAddrB);
  assert(pNHB != NULL);
  assert(pNHB->pLink == pLinkAS && pNHB->tAddr == tAddrB);
  SOSPFNextHop * pNHC1 = ospf_next_hop_create(pLinkAS, tAddrC);
  assert(pNHC1 != NULL);
  SOSPFNextHop * pNHC2 = ospf_next_hop_create(pLinkAC, 0);
  assert(pNHC2 != NULL);
  SOSPFNextHop * pNHTx = ospf_next_hop_create(pLinkAS, 0);
  assert(pNHTx != NULL);
  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_rt_test(): CHECK next hop list features...");
  next_hops_list_t * pNHListB = ospf_nh_list_create();
  assert(pNHListB != NULL);
  next_hops_list_t * pNHListC = ospf_nh_list_create();
  next_hops_list_t * pNHListTx = ospf_nh_list_create();
  
  assert(ospf_nh_list_add(pNHListB, pNHB) >= 0);
  assert(ospf_nh_list_add(pNHListC, pNHC1) >= 0);
  assert(ospf_nh_list_add(pNHListTx, pNHTx) >= 0);

  LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_rt_test(): CHECK route info functions...");
  SPrefix sPrefixB, sPrefixC;
  sPrefixB.tNetwork = tAddrB;
  sPrefixB.uMaskLen = 32;
  sPrefixC.tNetwork = tAddrC;
  sPrefixC.uMaskLen = 32;
  SOSPFRouteInfo * pRiB = NULL; 
  pRiB = OSPF_route_info_create(OSPF_DESTINATION_TYPE_ROUTER,
                                       sPrefixB,
				       100,
				       BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pNHListB);
  assert(pRiB != NULL);      
  SOSPFRouteInfo * pRiC = NULL; 
  pRiC = OSPF_route_info_create(OSPF_DESTINATION_TYPE_ROUTER,
                                       sPrefixC,
				       100,
				       BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pNHListC);	       
       
  SOSPFRouteInfo * pRiTx = NULL; 
  pRiTx = OSPF_route_info_create(OSPF_DESTINATION_TYPE_NETWORK,
                                       sSubnetPfxTx,
				       100,
BACKBONE_AREA,
				       OSPF_PATH_TYPE_INTRA ,
				       pNHListTx);
  assert(pRiTx != NULL);  
 
  assert(OSPF_route_info_add_nextHop(pRiC, pNHC2) >= 0);

  LOG_DEBUG(" ok!\n");
  LOG_DEBUG("ospf_rt_test(): CHECK routing table function... ");
  SOSPFRT * pRT = OSPF_rt_create();
  assert(pRT != NULL);
  
  assert(OSPF_rt_add_route(pRT, sPrefixB, pRiB) >= 0);
  assert(OSPF_rt_add_route(pRT, sPrefixC, pRiC) >= 0);
  assert(OSPF_rt_add_route(pRT, sSubnetPfxTx, pRiTx) >= 0);
  LOG_DEBUG("... ok!\n");
  //int totRoutes;
  //OSPF_rt_dump(stdout, pRT, 0, 0, 0, &totRoutes);
  
  
  //TEST 1 - delete all IGP route for a prefix
  LOG_DEBUG("ospf_rt_test(): Deleting all IGP route for a prefix... ");
  assert(OSPF_rt_del_route(pRT, &sPrefixB, NULL, NET_ROUTE_IGP) == 0);
  //OSPF_rt_dump(stdout, pRT, 0, 0, 0, &totRoutes);
  LOG_DEBUG("ok!\n");
  
  //TEST 2 - delete all IGP route that has next hop same as pNH;
  LOG_DEBUG("ospf_rt_test(): Deleting all IGP route that has next hop same as pNH... ");
  SOSPFNextHop * pNH1 = ospf_next_hop_create(pNHTx->pLink, pNHTx->tAddr);
  assert(OSPF_rt_del_route(pRT, NULL, pNH1, NET_ROUTE_IGP) == 0);
  ospf_next_hop_destroy(&pNH1);
  //OSPF_rt_dump(stdout, pRT, 0, 0, 0, &totRoutes);
  LOG_DEBUG("ok!\n");
  
  //TEST 3 - delete only the next hop when the route has more than one
  LOG_DEBUG("ospf_rt_test(): Deleting only the next hop when the route has more than one... ");
  SOSPFNextHop * pNH2 = ospf_next_hop_create(pNHC2->pLink, pNHC2->tAddr);
  assert(OSPF_rt_del_route(pRT, NULL, pNH2, NET_ROUTE_IGP) == 0);
  ospf_next_hop_destroy(&pNH2);
  //OSPF_rt_dump(stdout, pRT, 0, 0, 0, &totRoutes);
	  
  LOG_DEBUG("... ok!\n");
  
  
  //TODO add advertising router... for inter area...
  LOG_DEBUG("ospf_rt_test(): Destroying routing table... ");
  OSPF_rt_destroy(&pRT);
  assert(pRT == NULL);
  LOG_DEBUG(" ok!\n");
  
  subnet_destroy(&pSubTx);
  node_destroy(&pNodeA);
  node_destroy(&pNodeB);
  node_destroy(&pNodeC);
  LOG_DEBUG("ospf_rt_test(): END\n");
  
  return 1;
}

#endif

