// ==================================================================
// @(#)ospf.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/06/2005
// $Id: ospf.c,v 1.28 2009-03-24 16:21:28 bqu Exp $
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

#include <assert.h>
#include <net/net_types.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/network.h>
#include <net/node.h>
#include <net/subnet.h>
#include <net/link-list.h>
#include <net/link.h>
#include <net/prefix.h>
#include <net/spt_vertex.h>
#include <net/igp_domain.h>
#include <net/ospf_deflection.h>
#include <string.h>

#define X_AREA 1
#define Y_AREA 2
#define K_AREA 3


// ----- subnet_belongs_to_area ------------------------------------------
int subnet_belongs_to_area(net_subnet_t * pSubnet, uint32_t tArea);

/////////////////////////////////////////////////////////////////////
///////
/////// OSPF methods for node object
///////
/////////////////////////////////////////////////////////////////////
// ----- node_add_OSPFArea ------------------------------------------
int node_add_OSPFArea(net_node_t * pNode, ospf_area_t OSPFArea)
{
  return int_array_add(pNode->pOSPFAreas, &OSPFArea);
}

// ----- node_belongs_to_area ------------------------------------------
int node_belongs_to_area(net_node_t * pNode, uint32_t tArea)
{
  unsigned int uIndex;
  
  return !_array_sorted_find_index((SArray *)(pNode->pOSPFAreas), &tArea, &uIndex);
}


// ----- node_is_BorderRouter ------------------------------------------
int node_is_BorderRouter(net_node_t * pNode)
{
//  ospf_area_t B = BACKBONE_AREA;
  //unsigned int uPos;

  return ((int_array_length(pNode->pOSPFAreas) > 1) /*&& 
          (int_array_sorted_find_index(pNode->pOSPFAreas, &B, &uPos) == 0)*/);
}

// ----- node_is_InternalRouter ------------------------------------------
int node_is_InternalRouter(net_node_t * pNode) 
{
  return (int_array_length(pNode->pOSPFAreas) == 1);
}

// ----- ospf_node_get_area ------------------------------------------
// Return "area" a node belongs to. 
// For a border router, which belongs to several areas, this return
// BACKBONE_AREA value only if it belongs to BACKBONE. 
// TODO needs of modification when virtual link will be implemented
ospf_area_t ospf_node_get_area(net_node_t * pNode){
  ospf_area_t area = OSPF_NO_AREA;
//  if (node_is_BorderRouter(pNode))
//    area = BACKBONE_AREA;
//  else
  if (_array_length((SArray *) (pNode->pOSPFAreas)) > 0)
    _array_get_at((SArray *)(pNode->pOSPFAreas), 0, &area);
  
  return area;
}

// ----- ospf_node_compute_id ------------------------------------------
net_addr_t ospf_node_compute_id(net_node_t * pNode) {
  net_addr_t ospf_id = net_links_get_smaller_iface(pNode->pLinks);   
  if (ospf_id == 0)
    ospf_id = pNode->tAddr;
  return ospf_id;
}

// ----- ospf_node_get_id ------------------------------------------
net_addr_t ospf_node_get_id(net_node_t * pNode) {
  return pNode->tAddr;//ospf_id;
}

// ----- node_ospf_rt_add_route ------------------------------------------
extern int node_ospf_rt_add_route(net_node_t     * pNode,     ospf_dest_type_t  tOSPFDestinationType,
                       SPrefix        sPrefix,   uint32_t          uWeight,
		       ospf_area_t    tOSPFArea, ospf_path_type_t  tOSPFPathType,
		       next_hops_list_t * pNHList)
{
  SOSPFRouteInfo * pRouteInfo;
  //we should be do it?
  /*pLink= node_links_lookup(pNode, tNextHop);
  if (pLink == NULL) {
    return NET_RT_ERROR_NH_UNREACH;
  }*/

  pRouteInfo = OSPF_route_info_create(tOSPFDestinationType,  sPrefix,
				       uWeight, tOSPFArea, tOSPFPathType, pNHList);
//  LOG_DEBUG("OSPF_route_info_create pass...\n");  
  return OSPF_rt_add_route(pNode->pOspfRT, sPrefix, pRouteInfo);
}

// ----- node_ospf_rt_add_route ------------------------------------------
/**
 * This function removes a route from the node's routing table. The
 * route is identified by the prefix, the next-hop address (i.e. the
 * outgoing interface) and the type.
 *
 * If the next-hop is not present (NULL), all the routes matching the
 * other parameters will be removed whatever the next-hop is.
 */
 
extern int node_ospf_rt_del_route(net_node_t * pNode,   SPrefix * pPrefix, 
                                  SOSPFNextHop * pNextHop, net_route_type_t tType){

  net_iface_t * pLink= NULL;

  // Lookup the next-hop's interface (no recursive lookup is allowed,
  // it must be a direct link !)
  if (pNextHop != NULL) {
    pLink= node_links_lookup(pNode, link_get_address(pNextHop->pLink)); //TODO improved lookup to manage
    if (pLink == NULL) {                                                //ospf next hop
      return NET_RT_ERROR_IF_UNKNOWN;
    }
  }

  return OSPF_rt_del_route(pNode->pRT, pPrefix, pNextHop, tType);
}

/////////////////////////////////////////////////////////////////////
///////
/////// OSPF METHODS FOR LINK OBJECT
///////
/////////////////////////////////////////////////////////////////////

// ----- link_set_ospf_area ------------------------------------------
/* 
 * Set a link to be in an given OSPF area. Perform some check to grant
 * area coerence.
*/ 
int link_set_ospf_area(net_iface_t * pLinkToPeer, ospf_area_t tArea)
{
  int iReturn = OSPF_SUCCESS;
  net_node_t * pSrcNode = NULL;
  
  if (pLinkToPeer == NULL)
    return OSPF_SOURCE_NODE_LINK_MISSING; 

  pSrcNode = pLinkToPeer->pSrcNode;
 
  if (!node_belongs_to_area(pSrcNode, tArea))
    return OSPF_SOURCE_NODE_NOT_IN_AREA; 
 
  
  if (link_is_to_node(pLinkToPeer)) {
    net_node_t * pPeer;
    net_iface_t * pLinkToMyself;
    
    pPeer = network_find_node(link_get_address(pLinkToPeer));
    assert(pPeer != NULL);
    
    //this is a configuration error
    if (!node_belongs_to_area(pPeer, tArea))
      return OSPF_DEST_NODE_NOT_IN_AREA;

//    SNetDest sDest;
//    sDest.tType = NET_DEST_ADDRESS;
//    sDest.uDest.tAddr = pNode->tAddr;
    
    pLinkToMyself = node_find_link(pPeer, ip_address_to_dest(pSrcNode->tAddr));
    assert(pLinkToMyself != NULL);
    
    //this is only a warning... linkToMySelf's area attribute will be changed
    if (pLinkToMyself->tArea != tArea && pLinkToMyself->tArea != OSPF_NO_AREA)
      iReturn = OSPF_LINK_TO_MYSELF_NOT_IN_AREA; 
        
    pLinkToPeer->tArea = pLinkToMyself->tArea = tArea;
  }
  else 
  {
    net_subnet_t * pPeer;
    pPeer= pLinkToPeer->tDest.pSubnet;
    assert(pPeer != NULL);
    
    if (!subnet_belongs_to_area(pPeer, tArea))
      return OSPF_DEST_SUBNET_NOT_IN_AREA; 
    //area tag of link from subnet is setted when area tag of subnet is setted
    pLinkToPeer->tArea =  tArea;
  }
  
  return iReturn;
}
  

/////////////////////////////////////////////////////////////////////
///////
/////// OSPF methods for subnet object
///////
/////////////////////////////////////////////////////////////////////
// ----- subnet_OSPFArea -------------------------------------------------
int subnet_set_OSPFArea(net_subnet_t * pSubnet, uint32_t uOSPFArea)
{
  int iIndex, iReturn = 0;
  net_iface_t * pCurrentLink = NULL;
  if (pSubnet->uOSPFArea != OSPF_NO_AREA && pSubnet->uOSPFArea != uOSPFArea)
    iReturn = -1;
    
  pSubnet->uOSPFArea = uOSPFArea;
  for (iIndex = 0; iIndex < ptr_array_length(pSubnet->pLinks); iIndex++){
    ptr_array_get_at(pSubnet->pLinks, iIndex, &pCurrentLink);
    pCurrentLink->tArea = uOSPFArea;
  }
  
  return iReturn;
}

// ----- subnet_get_OSPFArea ----------------------------------------------
uint32_t subnet_get_OSPFArea(net_subnet_t * pSubnet)
{
  return pSubnet->uOSPFArea;
}

// ----- subnet_belongs_to_area ------------------------------------------
int subnet_belongs_to_area(net_subnet_t * pSubnet, uint32_t tArea)
{
  return pSubnet->uOSPFArea == tArea;
}


/////////////////////////////////////////////////////////////////////
///////
/////// OSPF methods for link object
///////
/////////////////////////////////////////////////////////////////////
// ----- link_ospf_set_area ---------------------------------------------
void link_ospf_set_area(net_iface_t * pLink, ospf_area_t tArea)
{
  pLink->tArea = tArea;
}

/////////////////////////////////////////////////////////////////////
///////
/////// COMPUTATION OF INTRA-AREA ROUTES 
///////
/////////////////////////////////////////////////////////////////////

typedef struct {
  net_node_t    * pNode;
  ospf_area_t   tArea;
} SOspfIntraRouteContext;

// ----- adding_route_towards_subnet -------------------------------------------
void adding_route_towards_subnet(SSptVertex * pVertex, 
		                SOspfIntraRouteContext * pMyContext) {
   int iIndex, iTotSubnet = ptr_array_length(pVertex->aSubnets);
   net_iface_t * pLinkToSub;
   next_hops_list_t * pNHListToSubnet = NULL;
   SPrefix sPrefix;
   SOSPFRouteInfo * pOldRoute = NULL;

   for (iIndex = 0; iIndex < iTotSubnet; iIndex++){
     ptr_array_get_at(pVertex->aSubnets, iIndex, &pLinkToSub); 
       
     if (spt_vertex_is_router(pVertex) && 
         (pMyContext->pNode->tAddr == spt_vertex_to_router(pVertex)->tAddr)) {
	   //In this case subnet is directly connected to root router so
	   //next hops could not be inherited from vertex but must be builded
	   SOSPFNextHop * pNH = ospf_next_hop_create(pLinkToSub, 
			                             OSPF_NO_IP_NEXT_HOP);
           pNHListToSubnet = ospf_nh_list_create();
	   ospf_nh_list_add(pNHListToSubnet, pNH);
       }
       else  //pVertex != root so Next Hops are in Vertex and can be inherited
         pNHListToSubnet = ospf_nh_list_copy(pVertex->pNextHops);
       
       /* If there is only an interface configured on the link towards 
	* router we must simulate the presence of a STUB-NETWORK
	* linked to the current vertex.
	* STUB-NETWORK must have a prefix /32 where address is 
	* the interface on the other side of the link.
	*/
       uint32_t tLinkToSubCost = pLinkToSub->uIGPweight;
       if (pLinkToSub->uDestinationType == NET_LINK_TYPE_ROUTER){
	  if (link_to_router_has_only_iface(pLinkToSub)) {
	    pLinkToSub = link_find_backword(pLinkToSub);
/*		  net_node_t * pPeer = network_find_node((pLinkToSub->UDestId).tAddr);
	    pLinkToSub = node_find_link_to_router(pPeer,
			                         pLinkToSub->pSrcNode->tAddr);*/
	    assert(pLinkToSub != NULL);
	  }
       }
       sPrefix = link_get_ip_prefix(pLinkToSub); 
       pOldRoute = OSPF_rt_find_exact(pMyContext->pNode->pOspfRT, 
		                          sPrefix, NET_ROUTE_IGP, OSPF_NO_AREA);
       
       /* Router can have already a route in the same area 
	* only if subnet is a subnet on a point-to-point link.
	*/
       if (pOldRoute == NULL)
         node_ospf_rt_add_route(pMyContext->pNode,  
			      OSPF_DESTINATION_TYPE_NETWORK,
                              link_get_ip_prefix(pLinkToSub),
			      pVertex->uIGPweight + tLinkToSubCost,
			      pMyContext->tArea, 
			      OSPF_PATH_TYPE_INTRA,
			      pNHListToSubnet);
       else {
         if (ospf_ri_get_cost(pOldRoute) >= pVertex->uIGPweight + tLinkToSubCost) {
/*	   node_ospf_rt_del_route(pMyContext->pNode, &sPrefix, NULL, NET_ROUTE_IGP);
	   node_ospf_rt_add_route(pMyContext->pNode,  OSPF_DESTINATION_TYPE_NETWORK,
                              link_get_ip_prefix(pLinkToSub),
			      pVertex->uIGPweight + tLinkToSubCost,
			      pMyContext->tArea, 
			      OSPF_PATH_TYPE_INTRA,
			      pNHListToSubnet); */
	   pOldRoute->uWeight = pVertex->uIGPweight + tLinkToSubCost;
	   ospf_nh_list_destroy(&(pOldRoute->aNextHops));
	   pOldRoute->aNextHops = pNHListToSubnet;
         }
       }
     }
   
} 

// ----- nh_list_set_adc_router ------------------------------------------------
void nh_list_set_adv_router(next_hops_list_t * pNHList, 
		            net_addr_t tAdvRouterAddr) {
  int iIndex;
  SOSPFNextHop * pNH;
  for (iIndex = 0; iIndex < ptr_array_length(pNHList); iIndex++) {
    ptr_array_get_at(pNHList, iIndex, &pNH);
    pNH->tAdvRouterAddr = tAdvRouterAddr;
  } 
} 

// ----- ospf_build_route_for_each --------------------------------
int ospf_intra_route_for_each(uint32_t uKey, uint8_t uKeyLen,
				void * pItem, void * pContext)
{
  SOspfIntraRouteContext * pMyContext= (SOspfIntraRouteContext *) pContext;
  SSptVertex * pVertex= (SSptVertex *) pItem;
  SPrefix sPrefix;
  next_hops_list_t * pNHList;
  ospf_dest_type_t   tDestType;
// LOG_DEBUG("ospf_build_route_for_each\n"); 
   /* removes the previous route for this node/prefix if it already
     exists */     
   node_ospf_rt_del_route(pMyContext->pNode, &sPrefix, NULL, NET_ROUTE_IGP);
   adding_route_towards_subnet(pVertex, pMyContext);
     
   // Skip route to itself
   if (pMyContext->pNode->tAddr == (net_addr_t) uKey)
    return 0;
     
   //adding OSPF route towards vertex
   sPrefix.tNetwork= uKey;
   sPrefix.uMaskLen= uKeyLen;

   tDestType = OSPF_DESTINATION_TYPE_NETWORK;
   if (spt_vertex_is_router(pVertex)) {
     if (node_is_BorderRouter(spt_vertex_to_router(pVertex))) {
       tDestType = OSPF_DESTINATION_TYPE_ROUTER;
       //This is the only case in which OSPF ID is used in routing table
       sPrefix.tNetwork = ospf_node_get_id(spt_vertex_to_router(pVertex));
/*       fprintf(stdout, "Installo route verso border routeter con id ");
       ip_address_dump(stdout, ospf_node_get_is(spt_vertex_to_router(pVertex)));
       fprintf(stdout, "\n");*/
       
       /* Set the advertising router field in order to 
	* managed in an omogeneus way INTER AREA Routes. 
	* ADdv. Router will be inherited from this route. 
	* */
       nh_list_set_adv_router(pVertex->pNextHops, sPrefix.tNetwork);
     }
     else// Skip vertex if this is a router on almost a subnet: in this case
       // we reach them with a best-prefix-match on subnet prefix
       if (spt_vertex_is_linked_to_subnet(pVertex)) {
	 return 0;
       }
   }
  //to prevent NextHopList destroy when spt is destroyed
  //note: next hops list are dynamically allocated during spt computation
  //      and linked to routing table during rt computation
  pNHList = pVertex->pNextHops;
  pVertex->pNextHops = NULL;
  
  return node_ospf_rt_add_route(pMyContext->pNode, tDestType, 
                                sPrefix,
				pVertex->uIGPweight,
				pMyContext->tArea, 
				OSPF_PATH_TYPE_INTRA,
				pNHList);
}

// ----- node_ospf_intra_route_single_area -----------------------------------------
/**
 *
 */
int node_ospf_intra_route_single_area(net_node_t * pNode, uint16_t uIGPDomainNumber, ospf_area_t tArea)
{
  int iResult;
  SRadixTree * pTree;
  SOspfIntraRouteContext * pContext;
  
  if (!node_belongs_to_area(pNode, tArea))
    return -1; //TODO define opportune error code
  /* Remove all OSPF routes from node */
  //TODO   node_rt_del_route(pNode, NULL, NULL, NET_ROUTE_IGP);
  
  /* Compute Minimum Spanning Tree */
  pTree= node_ospf_compute_spt(pNode, uIGPDomainNumber, tArea);
  if (pTree == NULL)
    return -1;
  // LOG_DEBUG("start visit of spt\n");
  /* Visit spt and set route in routing table */
  pContext = (SOspfIntraRouteContext *) MALLOC(sizeof(SOspfIntraRouteContext));
  pContext->pNode = pNode;
  pContext->tArea = tArea;
  iResult= radix_tree_for_each(pTree, ospf_intra_route_for_each, pContext);
  radix_tree_destroy(&pTree);
  FREE(pContext);
  return iResult;
}

// ----- node_ospf_intra_route ------------------------------------------------------------------
int node_ospf_intra_route(net_node_t * pNode, uint16_t uIGPDomainNumber)
{
  int iIndex, iStatus = 0;
  ospf_area_t tCurrentArea;
  for (iIndex = 0; iIndex < _array_length((SArray *)pNode->pOSPFAreas); iIndex++){
     _array_get_at((SArray *)(pNode->pOSPFAreas), iIndex, &tCurrentArea);
     iStatus = node_ospf_intra_route_single_area(pNode, uIGPDomainNumber, tCurrentArea );
     if (iStatus < 0) 
       break;
  } 
  return iStatus;
}

// ----- ospf_node_rt_dump ------------------------------------------------------------------
/**  Option:
  *  NET_OSPF_RT_OPTION_SORT_AREA : dump routing table grouping routes by area
  */
void ospf_node_rt_dump(FILE * pStream, net_node_t * pNode, int iOption)
{
  int iIndexArea, iIndexPath;
  ospf_area_t tCurrentArea;
  int printedRoute = 0;
 
  fprintf(stdout, "****************************************************************************\n");
  if (pNode->pcName != NULL){ 
    fprintf(stdout, "** %s (", pNode->pcName);
    ip_address_dump(stdout, pNode->tAddr);
    fprintf(stdout, ") OSPF routing table");
  }
  else {
    fprintf(stdout, "** ");
    ip_address_dump(stdout, pNode->tAddr);
    fprintf(stdout, " OSPF routing table\n");
  fprintf(stdout, "****************************************************************************\n");
  }
	
  switch (iOption) {
    case 0 : 
           OSPF_rt_dump(stdout, pNode->pOspfRT, iOption, 0, 0, &printedRoute);  
         break;
	 
    case OSPF_RT_OPTION_SORT_AREA        : 
           for( iIndexArea = 0; iIndexArea < _array_length((SArray *)(pNode->pOSPFAreas)); iIndexArea++){
             _array_get_at((SArray *)(pNode->pOSPFAreas), iIndexArea, &tCurrentArea);
	     printedRoute = 0;
             OSPF_rt_dump(stdout, pNode->pOspfRT, iOption, 0, tCurrentArea, &printedRoute);
	     if (printedRoute > 0)
               fprintf(stdout,"--------------------------------------------------------------------\n");   
	   }
         break;
	 
    case OSPF_RT_OPTION_SORT_PATH_TYPE   : 
           for( iIndexPath = OSPF_PATH_TYPE_INTRA; iIndexPath < OSPF_PATH_TYPE_EXTERNAL_2; iIndexPath++){
	     printedRoute = 0;
             OSPF_rt_dump(stdout, pNode->pOspfRT, iOption, iIndexPath, 0, &printedRoute);
             if (printedRoute > 0)
                  fprintf(stdout,"----------------------------------------------------------------------------\n");   
           }
         break;
	 
    case (OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE)   : 
	   for( iIndexPath = OSPF_PATH_TYPE_INTRA; iIndexPath < OSPF_PATH_TYPE_EXTERNAL_2; iIndexPath++)
             for( iIndexArea = 0; iIndexArea < _array_length((SArray *)(pNode->pOSPFAreas)); iIndexArea++){
                 _array_get_at((SArray *)(pNode->pOSPFAreas), iIndexArea, &tCurrentArea);
		 printedRoute = 0;
		 OSPF_rt_dump(stdout, pNode->pOspfRT, iOption, iIndexPath, tCurrentArea, &printedRoute);
		 if (printedRoute > 0)
                  fprintf(stdout,"----------------------------------------------------------------------------\n");   
             }
	 break;
  }
}


/////////////////////////////////////////////////////////////////////
///////
/////// COMPUTATION OF INTER-AREA ROUTES
///////
/////////////////////////////////////////////////////////////////////

typedef struct {
  net_node_t          * pSourceNode;  //node for wich inter-route are builded
  net_link_delay_t    tWeightToBR;  //weight from pSourceNode to BR
  ospf_area_t         tOspfArea;    //value for area parameters
  next_hops_list_t  * pNHListToBR;  //list of next hops to BR
} SInterRouteBuilding;

// ----- ospf_route_is_adver_in_area -------------------------------------------
/* 
 * Given a route and an area return 
 *
 *  - OSPF_SUCCESS if route must be advertised in the area with SummaryLSA
 *  - !OSPF_SUCCESS is route must not be advertised in the area with SummaryLSA
 *  
 */ 
int ospf_route_is_adver_in_area(SOSPFRouteInfo * pRI, ospf_area_t tArea) {

/* I must consider only OSPF route: discard other types */
    if (!ospf_ri_route_type_is(pRI, NET_ROUTE_IGP)) 
      return 0;
    
    /* If I am a border router I must not consider INTER-AREA routes of the 
       border router that I'm examing the routing table. I must consider only
       INTRA-AREA routes.
       
       This take in mind OSPF specification (RFC2328 pag. 135) that specify 
       that in SummaryLSA a Border Router announce
       - INTRA-AREA ROUTES of its routing table in backbone and other areas
       - INTER-AREA ROUTES of its touting table only in area != backbone
       */
    /*if (node_is_BorderRouter(pMyContext->pSourceNode)) {
      if (!ospf_ri_pathType_is(pRI, OSPF_PATH_TYPE_INTRA)) 
      //TODO change when will be implemented EXT1 EXT2
        return OSPF_SUCCESS; 
    } */
    switch (ospf_ri_get_pathType(pRI)){
      case OSPF_PATH_TYPE_INTRA : 
	      if (ospf_ri_get_area(pRI) == tArea)
                return 0;
             
	      if (pRI->tOSPFDestinationType == OSPF_DESTINATION_TYPE_ROUTER)
	        return 0; 
      break; 
      case OSPF_PATH_TYPE_INTER : 
	      if (tArea == BACKBONE_AREA)
	        return 0; 
    } 

    return 1;
}

// ----- ospf_br_rt_for_each_search_intra_route --------------------------------
/*
 * Search intra route in Border Routers's Routing Table to build inter route.
 * The intra route MUST don't belongs to an area of pNode router
 * 
*/
int ospf_br_rt_for_each_search_intra_route(uint32_t uKey, uint8_t uKeyLen, void * pItem, void * pContext)
{
  SOSPFRouteInfoList * pRIList = (SOSPFRouteInfoList *) pItem;
  SOSPFRouteInfo * pRI = NULL;
  int iIndex;
  SPrefix sPrefix;

  if (pRIList == NULL)
    return -1;
 
  SInterRouteBuilding * pMyContext = (SInterRouteBuilding *) pContext; 
  
  sPrefix.tNetwork = (net_addr_t) uKey;
  sPrefix.uMaskLen = uKeyLen;
  
  /* Discard route toward myself */
  SPrefix sNodePfx, * pPfx1 = &sPrefix, * pPfx2 = &sNodePfx;
  node_get_prefix(pMyContext->pSourceNode, &sNodePfx);
  if (ip_prefix_cmp(pPfx1, pPfx2) == 0)
    return OSPF_SUCCESS;
  
  /* Analize available routes */
  for (iIndex= 0; iIndex < ptr_array_length((SPtrArray *) pRIList); iIndex++) {
    pRI= (SOSPFRouteInfo *) pRIList->data[iIndex];
    
    if (!ospf_route_is_adver_in_area(pRI, pMyContext->tOspfArea)) 
      continue; 
    /* Now I have a valid route to consider. I must check if I 
     * have a best route yet.   */
    //TODO check that this should be return always INTRA-ROUTE first
    SOSPFRouteInfo * pOldRoute = 
	    OSPF_rt_find_exact(pMyContext->pSourceNode->pOspfRT, sPrefix,
			       NET_ROUTE_IGP, OSPF_NO_AREA);
    /* I have not an older route and I add  the new one to RT 
       next hops are the same for Border Router but I MUST make a copy. 
       I also set the advertisign router field in the NH. */
    if (pOldRoute == NULL) 
    {
      node_ospf_rt_add_route(pMyContext->pSourceNode,   
                             OSPF_DESTINATION_TYPE_NETWORK,
                             sPrefix,   
 		             pMyContext->tWeightToBR + ospf_ri_get_cost(pRI),
 		             pMyContext->tOspfArea, 
			     OSPF_PATH_TYPE_INTER,
 		             ospf_nh_list_copy(pMyContext->pNHListToBR));
    } 
    /* I have an old route with path type > INTER or an 
       INTER route with worst cost: I update the route */
    else if ( pOldRoute->tOSPFPathType > OSPF_PATH_TYPE_INTER || 
	         ((pOldRoute->tOSPFPathType == OSPF_PATH_TYPE_INTER) && 
	          (ospf_ri_get_cost(pOldRoute) > pMyContext->tWeightToBR + 
		   ospf_ri_get_cost(pRI)))) 
    {
      pOldRoute->uWeight = pMyContext->tWeightToBR + ospf_ri_get_cost(pRI);
      pOldRoute->tOSPFArea = pMyContext->tOspfArea;
      pOldRoute->tOSPFPathType = OSPF_PATH_TYPE_INTER;
      ospf_nh_list_destroy(&pOldRoute->aNextHops);
      pOldRoute->aNextHops = ospf_nh_list_copy(pMyContext->pNHListToBR);
    }
   /* I have a old route of type INTER at equal cost: 
       I add the next hops if not present */
    else if ((pOldRoute->tOSPFPathType == OSPF_PATH_TYPE_INTER) && 
               (ospf_ri_get_cost(pOldRoute) == pMyContext->tWeightToBR + 
		ospf_ri_get_cost(pRI))) 
    {	   
      ospf_nh_list_add_list(pOldRoute->aNextHops, pMyContext->pNHListToBR);
    }
    else{ 
//       /* else I have already a best route towards destination: an INTRA AREA route */
 /*      LOG_DEBUG("I have already a best route to ... ");
       ip_prefix_dump(stdout, sPrefix);
       fprintf(stdout, "\n");
       OSPF_route_info_dump(stdout, pOldRoute);
       fprintf(stdout, "\n");
  */   
    }
     

  }
 
  return OSPF_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
///////
/////// OSPF METHOD FOR IGP DOMAIN OBJECT
///////
/////////////////////////////////////////////////////////////////////

// ----- igp_domain_router_for_each_br_scan_rt -----------------------------------------------------------
/* 
 * Scan all the Border Routers in the domain 
 *   -- check if BR is reachable by Source node
 *   -- scan BR routing table to install INTER-AREA routes in Source node routing table 
 */ 
int igp_domain_router_for_each_br_scan_rt(uint32_t uKey, uint8_t uKeyLen, void * pItem, void * pContext)
{
  net_node_t * pCurrentRouter = (net_node_t *) pItem;
  SInterRouteBuilding * pCtx = (SInterRouteBuilding *) pContext;
  net_node_t * pSrcNode = pCtx->pSourceNode;
  SPrefix sPrefix;
  
  /* Consider only border routers */
  if (!node_is_BorderRouter(pCurrentRouter)) 
    return OSPF_SUCCESS;
    
  //TODO route type (last parameter) should be OSPF-IGP
  sPrefix.tNetwork = ospf_node_get_id(pCurrentRouter);//(net_addr_t) uKey;
  sPrefix.uMaskLen = uKeyLen;
  
  SOSPFRouteInfo * pRouteToBR = OSPF_rt_find_exact(pSrcNode->pOspfRT, 
		                sPrefix, NET_ROUTE_IGP, OSPF_NO_AREA);
  // pSrcNode has not a route towards BR... can't use it to learn inter-area destination
  // with this check pSrcNode is discarded is it's a Border Router
  if (pRouteToBR == NULL)
    return OSPF_SUCCESS;
  //route towards BR MUST be an INTRA-AREA route...
  if (pRouteToBR->tOSPFPathType !=  OSPF_PATH_TYPE_INTRA)
    return OSPF_SUCCESS;
  //...and tagged to area router belongs to 
  //(BACKBONE is router is a border router)
  if (node_is_BorderRouter(pCtx->pSourceNode)) {
    if (pRouteToBR->tOSPFArea != 0) //or pCurrentNode Must have a valid virtual link on backbone
      return OSPF_SUCCESS;
  }
  else
    if (pRouteToBR->tOSPFArea != ospf_node_get_area(pCtx->pSourceNode))
      return OSPF_SUCCESS;

  pCtx->tWeightToBR = pRouteToBR->uWeight;
  pCtx->pNHListToBR = pRouteToBR->aNextHops;
  pCtx->tOspfArea = ospf_node_get_area(pSrcNode);
  	     
  return trie_for_each((STrie *) pCurrentRouter->pOspfRT,  ospf_br_rt_for_each_search_intra_route, pContext); 
}


// ----- node_ospf_inter_route ------------------------------------------------------------------
/*
 * Builds INTER-AREA routes for pNode. 
 *
 * Prerequisites:
 *  - All Border Routers had computed INRA-AREA routes.
 */ 
int ospf_node_inter_route(net_node_t * pNode, uint16_t uIGPDomain)
{
  int iResult = OSPF_SUCCESS;
  SIGPDomain * pIGPDomain = get_igp_domain(uIGPDomain);
  SInterRouteBuilding * pContext = (SInterRouteBuilding *) MALLOC(sizeof(SInterRouteBuilding));
  pContext->pSourceNode = pNode;
  //for each border router of the domain give route from their routing table
  //and install INTER-AREA route in pNode routing table
  iResult= radix_tree_for_each((SRadixTree *) pIGPDomain->pRouters, igp_domain_router_for_each_br_scan_rt, pContext);
  return iResult; 
}
 
//----- ospf_domain_build_intra_route_for_each -----------------------------------------------------------
/** 
 *  Helps ospf_domain_build_intra_route function to interate over all the router 
 *  of the domain to build intra route.
 *
*/
int ospf_domain_build_intra_route_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem, void * pContext)
{
  uint16_t uOSPFDomain = *((uint16_t *) pContext);
  net_node_t * pNode   = (net_node_t *) pItem;
 
  int iResult = node_ospf_intra_route(pNode, uOSPFDomain);
//   ospf_node_rt_dump(stdout, pNode, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
  return iResult;
}

//----- ospf_domain_build_intra_route -----------------------------------------------------------
/**
 * Builds intra route for all the routers of the domain.
 */
int ospf_domain_build_intra_route(uint16_t uOSPFDomain)
{
  SIGPDomain * pDomain = get_igp_domain(uOSPFDomain); 
  return igp_domain_routers_for_each(pDomain, ospf_domain_build_intra_route_for_each, &uOSPFDomain);
}

 
//----- ospf_domain_build_inter_route_for_br -----------------------------------------------------------
/** 
 *  Helps ospf_domain_build_inter_route_br_only function to interate over all the border router 
 *  of the domain to build inter-area routes.
*/
int ospf_domain_build_inter_route_for_br(uint32_t uKey, uint8_t uKeyLen, void * pItem, void * pContext)
{
  uint16_t uOSPFDomain = *((uint16_t *) pContext);
  net_node_t * pNode   = (net_node_t *) pItem;
  int iResult = 0;
  if (node_is_BorderRouter(pNode))
   iResult = ospf_node_inter_route(pNode, uOSPFDomain);
  return iResult;
}

//----- ospf_domain_build_inter_route_for_br_only -----------------------------------------------------------
/** 
 *  Computes inter-area routes only for border routers in the domain.
 */
int ospf_domain_build_inter_route_br_only(uint16_t uOSPFDomain){
  SIGPDomain * pDomain = get_igp_domain(uOSPFDomain);
  return igp_domain_routers_for_each(pDomain, ospf_domain_build_inter_route_for_br, &uOSPFDomain);
}

//----- ospf_domain_build_inter_route_for_ir -----------------------------------------------------------
/** 
 *  Helps ospf_node_build_inter_route_ir_only function to interate over all the border router 
 *  of the domain to build intra route.
 *
*/
int ospf_domain_build_inter_route_for_ir(uint32_t uKey, uint8_t uKeyLen, void * pItem, void * pContext)
{
  uint16_t uOSPFDomain = *((uint16_t *) pContext);
  net_node_t * pNode   = (net_node_t *) pItem;
  int iResult = OSPF_SUCCESS;
  if (!node_is_BorderRouter(pNode))
   iResult = ospf_node_inter_route(pNode, uOSPFDomain);
//    ospf_node_rt_dump(stdout, pNode, OSPF_RT_OPTION_SORT_AREA);
  return iResult;
}  

//----- ospf_domain_build_inter_route_ir_only -----------------------------------------------------------
/** 
 *  Calculates inter-area routes only for intra routers of the domain.
*/
int ospf_domain_build_inter_route_ir_only(uint16_t uOSPFDomain){
  SIGPDomain * pDomain = get_igp_domain(uOSPFDomain);
  assert(pDomain != NULL);
  //LOG_DEBUG("ir ok!\n");
  
  return igp_domain_routers_for_each(pDomain, ospf_domain_build_inter_route_for_ir, &uOSPFDomain);
}

//----- ospf_domain_compute_nodes_id_for_each -----------------------------------------------------------
/*int ospf_domain_compute_nodes_id_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem, void * pContext)
{
  net_node_t * pNode   = (net_node_t *) pItem;
  pNode->ospf_id = ospf_node_compute_id(pNode);
  
  return OSPF_SUCCESS;
} 

//----- ospf_domain_compute_nodes_id -----------------------------------------------------------
int ospf_domain_compute_nodes_id(uint16_t uOSPFDomain){
  SIGPDomain * pDomain = get_igp_domain(uOSPFDomain);
  assert(pDomain != NULL);
  return igp_domain_routers_for_each(pDomain, ospf_domain_compute_nodes_id_for_each, &uOSPFDomain);
}
*/
//----- ospf_domain_get_br_for_each ---------------------------------------
int ospf_domain_get_br_for_each(uint32_t uKey, uint8_t uKeyLen, 
		                void * pItem, void * pContext)
{
  net_node_t * pNode   = (net_node_t *) pItem;
  SPtrArray * pBRList = (SPtrArray *) pContext;
  
  if (node_is_BorderRouter(pNode)){
    ptr_array_add(pBRList, &pNode);
  }

  return OSPF_SUCCESS;
}

//----- ospf_domain_get_border_routers -----------------------------------------------------------
SPtrArray * ospf_domain_get_border_routers(uint16_t uOSPFDomain){
  SIGPDomain * pDomain = get_igp_domain(uOSPFDomain);
  SPtrArray * pBRList = ptr_array_create(0, NULL, NULL);
  assert(pDomain != NULL);
  if (igp_domain_routers_for_each(pDomain, ospf_domain_get_br_for_each, pBRList) != OSPF_SUCCESS) 
    return NULL;
  return pBRList;
	  
}

typedef struct {
  SPtrArray * pBRList;
  ospf_area_t tArea;
  uint8_t bOnlyBROnBackbone;
} SBRInAreaInfo; 

//----- ospf_domain_get_br_in_area_for_each -----------------------------------
int ospf_domain_get_br_in_area_for_each(uint32_t uKey, uint8_t uKeyLen, 
		                        void * pItem, void * pContext)
{
  net_node_t * pNode    = (net_node_t *) pItem;
  SBRInAreaInfo * pCtx = (SBRInAreaInfo *) pContext;
  
  if (node_is_BorderRouter(pNode) && node_belongs_to_area(pNode, pCtx->tArea)){
    if (pCtx->bOnlyBROnBackbone && !node_belongs_to_area(pNode, BACKBONE_AREA))
      return OSPF_SUCCESS;
    ptr_array_add(pCtx->pBRList, &pNode);
  }

  return OSPF_SUCCESS;
}
//----- ospf_domain_get_br_on_bb_in_area ---------------------------------------
SPtrArray * ospf_domain_get_br_on_bb_in_area(uint16_t uOSPFDomain, 
		                             ospf_area_t tArea){
  SIGPDomain * pDomain = get_igp_domain(uOSPFDomain);
  SPtrArray * pBRList = ptr_array_create(0, NULL, NULL);
  SBRInAreaInfo sInfo;
  sInfo.pBRList = pBRList;
  sInfo.tArea = tArea;
  sInfo.bOnlyBROnBackbone = 1;
  
  assert(pDomain != NULL);
  if (igp_domain_routers_for_each(pDomain, 
	ospf_domain_get_br_in_area_for_each, &sInfo) != OSPF_SUCCESS) 
    return NULL;
  return pBRList;
	  
}

//----- ospf_domain_build_route ---------------------------------------------------------------
/** 
 *
*/
int ospf_domain_build_route(uint16_t uOSPFDomain) {
  //assert(ospf_domain_compute_nodes_id(uOSPFDomain) == OSPF_SUCCESS);
  assert(ospf_domain_build_intra_route(uOSPFDomain) >= 0);
  assert(!ospf_domain_build_inter_route_br_only(uOSPFDomain));
  assert(!ospf_domain_build_inter_route_ir_only(uOSPFDomain));
  return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////
//////
////// MISC OSPF FUNCTION
//////
//////////////////////////////////////////////////////////////////////////////////////////

void ospf_print_error(FILE * pStream, int iError) {
  char * pMsg = "Unknown error code";
 switch(iError){
    case  OSPF_LINK_OK : 
            pMsg = "No error";
    break;
    
    case  OSPF_LINK_TO_MYSELF_NOT_IN_AREA : 
            pMsg = "Warning: ospf area link is changed";
    break;
    
    case  OSPF_SOURCE_NODE_NOT_IN_AREA    : 
            pMsg = "Error: source node must belong to area";
    break;
    
    case  OSPF_DEST_NODE_NOT_IN_AREA      : 
            pMsg = "Error: destination node must belongs to area";
    break;
    
    case  OSPF_DEST_SUBNET_NOT_IN_AREA    : 
            pMsg = "Error: destination subnet must belongs to area";
    break;
    
    case  OSPF_SOURCE_NODE_LINK_MISSING   : 
            pMsg = "Error: link is not present";
    break;
    default : LOG_SEVERE("Error code %d unknown", iError);
  }
  fprintf(pStream, "%s\n", pMsg);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////
////// TEST OSPF FUNCTION
//////
//////////////////////////////////////////////////////////////////////////////////////////

int ospf_test_rfc2328()
{
  
  LOG_DEBUG("ospf_test_rfc2328(): START\n");
  LOG_DEBUG("ospf_test_rfc2328(): building the sample network of RFC2328 for test...");
//   LOG_DEBUG("point-to-point links attached.\n");
  net_node_t * pNodeRT1= node_create(IPV4_TO_INT(192,168,20,1));
  net_node_t * pNodeRT2= node_create(IPV4_TO_INT(192,168,20,2));
  net_node_t * pNodeRT3= node_create(IPV4_TO_INT(192,168,20,3));
  net_node_t * pNodeRT4= node_create(IPV4_TO_INT(192,168,20,4));
  net_node_t * pNodeRT5= node_create(IPV4_TO_INT(192,168,20,5));
  net_node_t * pNodeRT6= node_create(IPV4_TO_INT(192,168,20,6));
  net_node_t * pNodeRT7= node_create(IPV4_TO_INT(192,168,20,7));
  net_node_t * pNodeRT8= node_create(IPV4_TO_INT(192,168,20,8));
  net_node_t * pNodeRT9= node_create(IPV4_TO_INT(192,168,20,9));
  net_node_t * pNodeRT10= node_create(IPV4_TO_INT(192,168,20,10));
  net_node_t * pNodeRT11= node_create(IPV4_TO_INT(192,168,20,11));
  net_node_t * pNodeRT12= node_create(IPV4_TO_INT(192,168,20,12));
  
  net_subnet_t * pSubnetSN1=  subnet_create(IPV4_TO_INT(192,168,1,0), 24, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSN2=  subnet_create(IPV4_TO_INT(192,168,2,0), 24, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetTN3=  subnet_create(IPV4_TO_INT(192,168,3,0), 24, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubnetSN4=  subnet_create(IPV4_TO_INT(192,168,4,0), 24, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetTN6=  subnet_create(IPV4_TO_INT(192,168,6,0), 24, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubnetSN7=  subnet_create(IPV4_TO_INT(192,168,7,0), 24, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetTN8=  subnet_create(IPV4_TO_INT(192,168,8,0), 24, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubnetTN9=  subnet_create(IPV4_TO_INT(192,168,9,0), 24, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubnetSN10= subnet_create(IPV4_TO_INT(192,168,10,0), 24, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSN11= subnet_create(IPV4_TO_INT(192,168,11,0), 24, NET_SUBNET_TYPE_STUB);
  
  assert(!network_add_node(pNodeRT1));
  assert(!network_add_node(pNodeRT2));
  assert(!network_add_node(pNodeRT3));
  assert(!network_add_node(pNodeRT4));
  assert(!network_add_node(pNodeRT5));
  assert(!network_add_node(pNodeRT6));
  assert(!network_add_node(pNodeRT7));
  assert(!network_add_node(pNodeRT8));
  assert(!network_add_node(pNodeRT9));
  assert(!network_add_node(pNodeRT10));
  assert(!network_add_node(pNodeRT11));
  assert(!network_add_node(pNodeRT12));

  assert(network_add_subnet(pSubnetSN1) >= 0);
  assert(network_add_subnet(pSubnetSN2) >= 0);
  assert(network_add_subnet(pSubnetTN3) >= 0);
  assert(network_add_subnet(pSubnetSN4) >= 0);
  assert(network_add_subnet(pSubnetTN6) >= 0);
  assert(network_add_subnet(pSubnetSN7) >= 0);
  assert(network_add_subnet(pSubnetTN8) >= 0);
  assert(network_add_subnet(pSubnetTN9) >= 0);
  assert(network_add_subnet(pSubnetSN10) >= 0);
  assert(network_add_subnet(pSubnetSN11) >= 0);
  
  assert(node_add_link_to_subnet(pNodeRT1, pSubnetSN1, IPV4_TO_INT(192,168,1,1), 3, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT1, pSubnetTN3, IPV4_TO_INT(192,168,3,1), 1, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT2, pSubnetSN2, IPV4_TO_INT(192,168,2,2), 3, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT2, pSubnetTN3, IPV4_TO_INT(192,168,3,2), 1, 1) >= 0);
  
  assert(node_add_link_to_subnet(pNodeRT3, pSubnetSN4, IPV4_TO_INT(192,168,4,3), 2, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT3, pSubnetTN3, IPV4_TO_INT(192,168,3,3), 1, 1) >= 0);
  assert(node_add_link_to_router(pNodeRT3, pNodeRT6, 8, 0) >= 0);
  assert(node_add_link_to_router(pNodeRT6, pNodeRT3, 6, 0) >= 0);
  assert(node_add_link_to_subnet(pNodeRT4, pSubnetTN3, IPV4_TO_INT(192,168,3,4), 1, 1) >= 0);
 
  assert(node_add_link_to_router(pNodeRT4, pNodeRT5, 8, 1) >= 0);
  assert(node_add_link_to_router(pNodeRT5, pNodeRT6, 7, 0) >= 0);
  assert(node_add_link_to_router(pNodeRT6, pNodeRT5, 6, 0) >= 0);
  
  assert(node_add_link_to_router(pNodeRT5, pNodeRT7, 6, 1) >= 0);
  assert(node_add_link_to_router(pNodeRT6, pNodeRT10, 7, 0) >= 0);
  assert(node_add_link_to_router(pNodeRT10, pNodeRT6, 5, 0) >= 0);
  assert(node_add_link_to_subnet(pNodeRT7, pSubnetTN6, IPV4_TO_INT(192,168,6,7), 1, 1) >= 0);

  assert(node_add_link_to_subnet(pNodeRT8, pSubnetTN6, IPV4_TO_INT(192,168,6,8), 1, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT8, pSubnetSN7, IPV4_TO_INT(192,168,7,8), 4, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT10, pSubnetTN6, IPV4_TO_INT(192,168,6,10), 1, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT10, pSubnetTN8, IPV4_TO_INT(192,168,8,10), 3, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT11, pSubnetTN8, IPV4_TO_INT(192,168,8,11), 2, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT11, pSubnetTN9, IPV4_TO_INT(192,168,9,11), 1, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT9, pSubnetSN11, IPV4_TO_INT(192,168,11,9), 3, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT9, pSubnetTN9, IPV4_TO_INT(192,168,9,9), 1, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT12, pSubnetTN9, IPV4_TO_INT(192,168,9,12), 1, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeRT12, pSubnetSN10, IPV4_TO_INT(192,168,10,12), 2, 1) >= 0);
  
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_test_rfc2328(): Adding other nodes to test igp domain check...");
  net_node_t * pNodeRTE1= node_create(IPV4_TO_INT(192,168,100,1));
  net_node_t * pNodeRTE2= node_create(IPV4_TO_INT(192,168,100,2));
  net_node_t * pNodeRTE3= node_create(IPV4_TO_INT(192,168,100,3));
  net_node_t * pNodeRTE4= node_create(IPV4_TO_INT(192,168,100,4));
  
  assert(!network_add_node(pNodeRTE1));
  assert(!network_add_node(pNodeRTE2));
  assert(!network_add_node(pNodeRTE3));
  assert(!network_add_node(pNodeRTE4));
  
  assert(node_add_link_to_subnet(pNodeRTE1, pSubnetTN3, 0, 1, 1) >= 0);
  assert(node_add_link_to_router(pNodeRTE2, pNodeRT4, 1, 1) >= 0);
  assert(node_add_link_to_router(pNodeRTE3, pNodeRT3, 1, 1) >= 0);
  assert(node_add_link_to_router(pNodeRTE4, pNodeRT6, 1, 1) >= 0);
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_test_rfc2328(): Assigning areas...");
  assert(node_add_OSPFArea(pNodeRT1, 1) >= 0);
  assert(node_add_OSPFArea(pNodeRT2, 1) >= 0);
  assert(node_add_OSPFArea(pNodeRT3, 1) >= 0);
  assert(node_add_OSPFArea(pNodeRT4, 1) >= 0);
  
  assert(node_add_OSPFArea(pNodeRT3,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT4,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT5,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT6,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT7,  BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeRT10, BACKBONE_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeRT7,  2) >= 0);
  assert(node_add_OSPFArea(pNodeRT8,  2) >= 0);
  assert(node_add_OSPFArea(pNodeRT10, 2) >= 0);
  assert(node_add_OSPFArea(pNodeRT11, 2) >= 0);
  
  assert(node_add_OSPFArea(pNodeRT9,  3) >= 0);
  assert(node_add_OSPFArea(pNodeRT11, 3) >= 0);
  assert(node_add_OSPFArea(pNodeRT12, 3) >= 0);
  
  subnet_set_OSPFArea(pSubnetSN1, 1);
  subnet_set_OSPFArea(pSubnetSN2, 1);
  subnet_set_OSPFArea(pSubnetTN3, 1);
  subnet_set_OSPFArea(pSubnetSN4, 1);
  
  subnet_set_OSPFArea(pSubnetTN6, 2);
  subnet_set_OSPFArea(pSubnetSN7, 2);
  subnet_set_OSPFArea(pSubnetTN8, 2);
  
  subnet_set_OSPFArea(pSubnetTN9, 3);
  subnet_set_OSPFArea(pSubnetSN10, 3);
  subnet_set_OSPFArea(pSubnetSN11, 3);
  
  net_iface_t * pLink = node_find_link(pNodeRT1, ip_address_to_dest(IPV4_TO_INT(192,168,1,1)));
  assert(link_set_ospf_area(pLink, 1) == 0);
  
  pLink = node_find_link(pNodeRT1, ip_address_to_dest(IPV4_TO_INT(192,168,3,1)));
  assert(link_set_ospf_area(pLink, 1) == 0);
  
  pLink = node_find_link(pNodeRT2, ip_address_to_dest(IPV4_TO_INT(192,168,2,2)));
  assert(link_set_ospf_area(pLink, 1) == 0);
  
  pLink = node_find_link(pNodeRT2, ip_address_to_dest(IPV4_TO_INT(192,168,3,2)));
  assert(link_set_ospf_area(pLink, 1) == 0);

  pLink = node_find_link(pNodeRT3, ip_address_to_dest(IPV4_TO_INT(192,168,3,3)));
  assert(link_set_ospf_area(pLink, 1) == 0);

  pLink = node_find_link(pNodeRT3, ip_address_to_dest(IPV4_TO_INT(192,168,4,3)));
  assert(link_set_ospf_area(pLink, 1) == 0);
  
  pLink = node_find_link(pNodeRT4, ip_address_to_dest(IPV4_TO_INT(192,168,3,4)));
  assert(link_set_ospf_area(pLink, 1) == 0);
  
  pLink = node_find_link(pNodeRT3, ip_address_to_dest(pNodeRT6->tAddr));
  assert(link_set_ospf_area(pLink, 0) == 0);
  
  pLink = node_find_link(pNodeRT5, ip_address_to_dest(pNodeRT6->tAddr));
  assert(link_set_ospf_area(pLink, 0) == 0);

  pLink = node_find_link(pNodeRT10, ip_address_to_dest(pNodeRT6->tAddr));
  assert(link_set_ospf_area(pLink, 0) == 0);
  SPrefix sPrefix;
  sPrefix.tNetwork = IPV4_TO_INT(192,168,15,9);
  sPrefix.uMaskLen = 32;
  link_set_ip_prefix(pLink, sPrefix); 
 // assert(link_to_router_has_only_iface(pLink) == 0);
 // assert(link_to_router_has_ip_prefix(pLink) != 0);
  
  pLink = node_find_link(pNodeRT6, ip_address_to_dest(pNodeRT10->tAddr));
  sPrefix.tNetwork = IPV4_TO_INT(192,168,15,11);
  sPrefix.uMaskLen = 32;
  link_set_ip_prefix(pLink, sPrefix); 
//  assert(link_to_router_has_only_iface(pLink) == 0);
//  assert(link_to_router_has_ip_prefix(pLink) != 0);
  
  pLink = node_find_link(pNodeRT4, ip_address_to_dest(pNodeRT5->tAddr));
  assert(link_set_ospf_area(pLink, 0) == 0);
  
  pLink = node_find_link(pNodeRT7, ip_address_to_dest(pNodeRT5->tAddr));
  assert(link_set_ospf_area(pLink, 0) == 0);
 
  pLink = node_find_link(pNodeRT7, ip_address_to_dest(IPV4_TO_INT(192,168,6,7)));
  assert(link_set_ospf_area(pLink, 2) == 0);
  
  pLink = node_find_link(pNodeRT10, ip_address_to_dest(IPV4_TO_INT(192,168,6,10)));
  assert(link_set_ospf_area(pLink, 2) == 0);
  
  pLink = node_find_link(pNodeRT8, ip_address_to_dest(IPV4_TO_INT(192,168,6,8)));
  assert(link_set_ospf_area(pLink, 2) == 0);

  pLink = node_find_link(pNodeRT8, ip_address_to_dest(IPV4_TO_INT(192,168,7,8)));
  assert(link_set_ospf_area(pLink, 2) == 0);
  
  pLink = node_find_link(pNodeRT11,ip_address_to_dest(IPV4_TO_INT(192,168,8,11)));
  assert(link_set_ospf_area(pLink, 2) == 0);
  
  pLink = node_find_link(pNodeRT11, ip_address_to_dest(IPV4_TO_INT(192,168,9,11)));
  assert(link_set_ospf_area(pLink, 3) == 0);

  pLink = node_find_link(pNodeRT9, ip_address_to_dest(IPV4_TO_INT(192,168,11,9)));
  assert(link_set_ospf_area(pLink, 3) == 0);
  
  pLink = node_find_link(pNodeRT9, ip_address_to_dest(IPV4_TO_INT(192,168,9,9)));
  assert(link_set_ospf_area(pLink, 3) == 0);
  
  pLink = node_find_link(pNodeRT12, ip_address_to_dest(IPV4_TO_INT(192,168,9,12)));
  assert(link_set_ospf_area(pLink, 3) == 0);
  
  pLink = node_find_link(pNodeRT12, ip_address_to_dest(IPV4_TO_INT(192,168,10,12)));
  assert(link_set_ospf_area(pLink, 3) == 0);
  
  LOG_DEBUG(" done.\n");
  
  LOG_DEBUG("ospf_test_rfc2328(): Creating igp domain...");
  uint16_t uIGPDomain = 10;
  SIGPDomain * pIGPDomain = igp_domain_create(uIGPDomain, DOMAIN_OSPF);
  register_igp_domain(pIGPDomain);
  
  igp_domain_add_router(pIGPDomain, pNodeRT1);
  igp_domain_add_router(pIGPDomain, pNodeRT2);
  igp_domain_add_router(pIGPDomain, pNodeRT3);
  igp_domain_add_router(pIGPDomain, pNodeRT4);
  igp_domain_add_router(pIGPDomain, pNodeRT5);
  igp_domain_add_router(pIGPDomain, pNodeRT6);
  igp_domain_add_router(pIGPDomain, pNodeRT7);
  igp_domain_add_router(pIGPDomain, pNodeRT8);
  igp_domain_add_router(pIGPDomain, pNodeRT9);
  igp_domain_add_router(pIGPDomain, pNodeRT10);
  igp_domain_add_router(pIGPDomain, pNodeRT11);
  igp_domain_add_router(pIGPDomain, pNodeRT12);
  
  LOG_DEBUG(" done.\n");
/*   LOG_DEBUG("ospf_test_rfc2328(): Computing SPT for RT4 (.dot output in spt.dot)...");
  
     SRadixTree * pSpt = node_ospf_compute_spt(pNodeRT4, uIGPDomain, BACKBONE_AREA);
     FILE * pOutDump = fopen("spt.RFC2328.backbone.dot", "w");
     spt_dump_dot(pOutDump, pSpt, pNodeRT4->tAddr);
     fclose(pOutDump);
     radix_tree_destroy(&pSpt);
  
     pSpt = node_ospf_compute_spt(pNodeRT4, uIGPDomain, 1);
     pOutDump = fopen("spt.RFC2328.1.dot", "w");
     spt_dump_dot(pOutDump, pSpt, pNodeRT4->tAddr);
     fclose(pOutDump);
     radix_tree_destroy(&pSpt);
  
   LOG_DEBUG(" ok!\n");
*/  
//   LOG_DEBUG("ospf_test_rfc2328(): Computing Routing Table for RT3-BACKBONE...");
//   node_ospf_intra_route_single_area(pNodeRT3, uIGPDomain, BACKBONE_AREA);
//   fprintf(stdout, "\n");
//   OSPF_rt_dump(stdout, pNodeRT3->pOspfRT, 0, 0);
//   LOG_DEBUG(" ok!\n");
  
//   OSPF_rt_destroy(&(pNodeRT3->pOspfRT));
//   pNodeRT3->pOspfRT = OSPF_rt_create();
//   LOG_DEBUG("ospf_test_rfc2328(): Computing Routing Table for RT3-AREA 1...");
//   node_ospf_intra_route_single_area(pNodeRT3, uIGPDomain, 1);
//   fprintf(stdout, "\n");
//   OSPF_rt_dump(stdout, pNodeRT3->pOspfRT, 0, 0);
//   LOG_DEBUG(" ok!\n");

//   LOG_DEBUG("ospf_test_rfc2328(): Computing Routing Table for RT6-AREA 1 (should fail)...");
//   assert(node_ospf_intra_route_single_area(pNodeRT6, uIGPDomain, 1) < 0);
//   LOG_DEBUG(" ok!\n");
   
   //pLink = node_find_link(pNodeRT10, ip_address_to_dest(IPV4_TO_INT(192,168,6,10)));
/*   pLink = node_find_link(pNodeRT10, ip_address_to_dest(pNodeRT6->tAddr));
   pLink->uFlags = 0;
   pLink = node_find_link(pNodeRT6, ip_address_to_dest(pNodeRT10->tAddr));
   pLink->uFlags = 0;
   */
  //ospf_domain_compute_nodes_id(uIGPDomain); 
/*    
    LOG_DEBUG("ospf_test_rfc2328(): Try computing intra route for each area on RT1...\n");
    fprintf(stdout, "\n");
    node_ospf_intra_route(pNodeRT10, uIGPDomain);
    ospf_node_rt_dump(stdout, pNodeRT10, OSPF_RT_OPTION_SORT_AREA);
    LOG_DEBUG(" ok!\n");
*/
  
   LOG_DEBUG("ospf_test_rfc2328(): Try to build INTRA-AREA route in the domain...");
   assert(ospf_domain_build_intra_route(uIGPDomain) >= 0);
   LOG_DEBUG(" ok!\n");

/*    LOG_DEBUG("ospf_test_rfc2328(): Try computing inter area route RT3... \n");
    ospf_node_inter_route(pNodeRT3, uIGPDomain);
    ospf_node_rt_dump(stdout, pNodeRT3, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
 
   LOG_DEBUG("ospf_test_rfc2328(): Try computing inter area route RT4... \n");
   ospf_node_inter_route(pNodeRT4, uIGPDomain);
   fprintf(stdout, "RT4 routing table...\n");
   ospf_node_rt_dump(stdout, pNodeRT4, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
  
      ospf_node_rt_dump(stdout, pNodeRT1, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
      LOG_DEBUG("ospf_test_rfc2328(): Try computing inter area route for RT1... \n");
      ospf_node_inter_route(pNodeRT1, uIGPDomain);
      ospf_node_rt_dump(stdout, pNodeRT1, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
*/  
    LOG_DEBUG("ospf_test_rfc2328(): Try to build INTER-AREA route only for border router...");
    assert(ospf_domain_build_inter_route_br_only(uIGPDomain) >= 0);
    LOG_DEBUG(" ok!\n");
     
    LOG_DEBUG("ospf_test_rfc2328(): Try to build INTER-AREA route only for intra router...");
    assert(ospf_domain_build_inter_route_ir_only(uIGPDomain) >= 0);
    LOG_DEBUG(" ok!\n");
   
    ospf_node_rt_dump(stdout, pNodeRT4, OSPF_RT_OPTION_SORT_AREA | 
		                        OSPF_RT_OPTION_SORT_PATH_TYPE);
 
//   LOG_DEBUG(" ok!\n");
  
  LOG_DEBUG("ospf_test_rfc2328: STOP\n");
  
  return 1;
}

int ospf_info_test() {
  
  const int startAddr = 1024;
  net_node_t * pNodeB1= node_create(startAddr);
  net_node_t * pNodeB2= node_create(startAddr + 1);
  net_node_t * pNodeB3= node_create(startAddr + 2);
  net_node_t * pNodeB4= node_create(startAddr + 3);
  net_node_t * pNodeX1= node_create(startAddr + 4);
  net_node_t * pNodeX2= node_create(startAddr + 5);
  net_node_t * pNodeX3= node_create(startAddr + 6);
  net_node_t * pNodeY1= node_create(startAddr + 7);
  net_node_t * pNodeY2= node_create(startAddr + 8);
  net_node_t * pNodeK1= node_create(startAddr + 9);
  net_node_t * pNodeK2= node_create(startAddr + 10);
  net_node_t * pNodeK3= node_create(startAddr + 11);
  
  LOG_DEBUG("nodes created...\n");
  net_subnet_t * pSubnetTB1= subnet_create(4, 30, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubnetTX1= subnet_create(8, 30, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubnetTY1= subnet_create(12, 30, NET_SUBNET_TYPE_TRANSIT);
  net_subnet_t * pSubnetTK1= subnet_create(16, 30, NET_SUBNET_TYPE_TRANSIT);
  
  net_subnet_t * pSubnetSB1= subnet_create(20, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSB2= subnet_create(24, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSX1= subnet_create(28, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSX2= subnet_create(32, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSX3= subnet_create(36, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSY1= subnet_create(40, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSY2= subnet_create(44, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSY3= subnet_create(48, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSK1= subnet_create(52, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSK2= subnet_create(56, 30, NET_SUBNET_TYPE_STUB);
  net_subnet_t * pSubnetSK3= subnet_create(60, 30, NET_SUBNET_TYPE_STUB);

  
  LOG_DEBUG("subnet created...\n");
  assert(!network_add_node(pNodeB1));
  assert(!network_add_node(pNodeB2));
  assert(!network_add_node(pNodeB3));
  assert(!network_add_node(pNodeB4));
  assert(!network_add_node(pNodeX1));
  assert(!network_add_node(pNodeX2));
  assert(!network_add_node(pNodeX3));
  assert(!network_add_node(pNodeY1));
  assert(!network_add_node(pNodeY2));
  assert(!network_add_node(pNodeK1));
  assert(!network_add_node(pNodeK2));
  assert(!network_add_node(pNodeK3));
 
  LOG_DEBUG("nodes attached...\n");
  assert(network_add_subnet(pSubnetTB1) >= 0);
  assert(network_add_subnet(pSubnetTX1) >= 0);
  assert(network_add_subnet(pSubnetTY1) >= 0);
  assert(network_add_subnet(pSubnetTK1) >= 0);
  
  assert(network_add_subnet(pSubnetSB1) >= 0);
  assert(network_add_subnet(pSubnetSB2) >= 0);
  assert(network_add_subnet(pSubnetSX1) >= 0);
  assert(network_add_subnet(pSubnetSX2) >= 0);
  assert(network_add_subnet(pSubnetSX3) >= 0);
  assert(network_add_subnet(pSubnetSY1) >= 0);
  assert(network_add_subnet(pSubnetSY2) >= 0);
  assert(network_add_subnet(pSubnetSY3) >= 0);
  assert(network_add_subnet(pSubnetSK1) >= 0);
  assert(network_add_subnet(pSubnetSK2) >= 0);
  assert(network_add_subnet(pSubnetSK3) >= 0);
  
  LOG_DEBUG("subnet attached...\n");
  assert(node_add_link_to_router(pNodeB1, pNodeB2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB2, pNodeB3, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB3, pNodeB4, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB4, pNodeB1, 100, 1) >= 0);
  
  assert(node_add_link_to_router(pNodeB2, pNodeX2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB3, pNodeX2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeX2, pNodeX1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeX2, pNodeX3, 100, 1) >= 0);

  assert(node_add_link_to_router(pNodeB3, pNodeY1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeY1, pNodeY2, 100, 1) >= 0);
  
  assert(node_add_link_to_router(pNodeB4, pNodeK1, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB4, pNodeK2, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeB1, pNodeK3, 100, 1) >= 0);
  assert(node_add_link_to_router(pNodeK3, pNodeK2, 100, 1) >= 0);
  
  
  assert(node_add_link_to_subnet(pNodeB1, pSubnetTB1, 0, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, pSubnetTB1, 1, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX1, pSubnetTX1, 2, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX3, pSubnetTX1, 3, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, pSubnetTY1, 4, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY2, pSubnetTY1, 5, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, pSubnetTK1, 6, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK1, pSubnetTK1, 7, 100, 1) >= 0);
  
  LOG_DEBUG("transit-network links attached.\n");
  
  assert(node_add_link_to_subnet(pNodeB2, pSubnetSB1, 8, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB1, pSubnetSB2, 9, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB2, pSubnetSX1, 10, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX2, pSubnetSX2, 11, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeX3, pSubnetSX3, 12, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeB3, pSubnetSY1, 13, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY1, pSubnetSY2, 14, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeY1, pSubnetSY3, 15, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK3, pSubnetSK1, 16, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, pSubnetSK2, 17, 100, 1) >= 0);
  assert(node_add_link_to_subnet(pNodeK2, pSubnetSK3, 18, 100, 1) >= 0);
  
  LOG_DEBUG("stub-network links attached.\n");
  
  assert(node_add_OSPFArea(pNodeB1, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB1, K_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB2, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB2, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB3, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB3, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB3, Y_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB4, BACKBONE_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeB4, K_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeX1, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeX2, X_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeX3, X_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeY1, Y_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeY2, Y_AREA) >= 0);
  
  assert(node_add_OSPFArea(pNodeK1, K_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeK2, K_AREA) >= 0);
  assert(node_add_OSPFArea(pNodeK3, K_AREA) >= 0);
  
  subnet_set_OSPFArea(pSubnetTB1, BACKBONE_AREA);
  subnet_set_OSPFArea(pSubnetTX1, X_AREA);
  subnet_set_OSPFArea(pSubnetTY1, Y_AREA);
  subnet_set_OSPFArea(pSubnetTK1, K_AREA);
  
  subnet_set_OSPFArea(pSubnetSB1, BACKBONE_AREA);
  subnet_set_OSPFArea(pSubnetSB2, BACKBONE_AREA);
  subnet_set_OSPFArea(pSubnetSX1, X_AREA);
  subnet_set_OSPFArea(pSubnetSX2, X_AREA);
  subnet_set_OSPFArea(pSubnetSX3, X_AREA);
  subnet_set_OSPFArea(pSubnetSY1, Y_AREA);
  subnet_set_OSPFArea(pSubnetSY2, Y_AREA);
  subnet_set_OSPFArea(pSubnetSY3, Y_AREA);
  subnet_set_OSPFArea(pSubnetSK1, K_AREA);
  subnet_set_OSPFArea(pSubnetSK2, K_AREA);
  subnet_set_OSPFArea(pSubnetSK3, K_AREA);
  
  LOG_DEBUG("OSPF area assigned.\n");
  
  assert(node_find_link_to_subnet(pNodeY1, pSubnetSY2, 14) != NULL);

  SPrefix sPrefixToFound;
  //sPrefixToFound.tNetwork = pNodeB2->tAddr;
  //sPrefixToFound.uMaskLen = 32;
  assert(node_find_link_to_router(pNodeB1, pNodeB2->tAddr) != NULL) ;
  //sPrefixToFound.tNetwork = pNodeX2->tAddr;
  assert(node_find_link_to_router(pNodeB1, pNodeX2->tAddr) == NULL) ;
  assert(node_find_link_to_subnet(pNodeB1, pSubnetTB1, 0) != NULL);
  
  //OK: source and destination node are in the backbone
  sPrefixToFound.tNetwork = pNodeB2->tAddr;
  sPrefixToFound.uMaskLen = 32;
  net_iface_t * pLink = node_find_link(pNodeB3, ip_prefix_to_dest(sPrefixToFound));
  assert(link_set_ospf_area(pLink, BACKBONE_AREA) == OSPF_SUCCESS);
  
  //OK: source node and dest subnet are in the backbone
  pLink = node_find_link(pNodeB1, ip_address_to_dest(9));
  assert(link_set_ospf_area(pLink, BACKBONE_AREA) == OSPF_SUCCESS);
  
  //OK: source node and dest subnet are in the backbone
  pLink = node_find_link(pNodeB1,ip_address_to_dest(0));
  assert(link_set_ospf_area(pLink, BACKBONE_AREA) == OSPF_SUCCESS);
  
  //FAILS: dest subnet is not in X_AREA
  pLink = node_find_link(pNodeB3, ip_prefix_to_dest(*(subnet_get_prefix(pSubnetTB1))));
  assert(link_set_ospf_area(pLink, X_AREA) == OSPF_DEST_SUBNET_NOT_IN_AREA);
  
  //FAILS: B1 is not linked with Y1
  pLink = node_find_link(pNodeB1, ip_address_to_dest(pNodeY1->tAddr));
  assert(link_set_ospf_area(pLink, BACKBONE_AREA) ==  OSPF_SOURCE_NODE_LINK_MISSING);
  
  //FAILS: K3 is not in BACKBONE_AREA
  pLink = node_find_link(pNodeK3, ip_address_to_dest(pNodeB1->tAddr));
  assert(link_set_ospf_area(pLink, BACKBONE_AREA) == OSPF_SOURCE_NODE_NOT_IN_AREA);
  
  //OK: but warning beacause the area attribute of the link change
  pLink = node_find_link(pNodeB3, ip_address_to_dest(pNodeB2->tAddr));
  assert(link_set_ospf_area(pLink, X_AREA) == OSPF_LINK_TO_MYSELF_NOT_IN_AREA);
  
  assert(node_is_BorderRouter(pNodeB1));
  assert(node_is_BorderRouter(pNodeB2));
  assert(node_is_BorderRouter(pNodeB3));
  assert(node_is_BorderRouter(pNodeB4));
  
  assert(!node_is_BorderRouter(pNodeX1));
  assert(!node_is_BorderRouter(pNodeX2));
  assert(!node_is_BorderRouter(pNodeX3));
  assert(!node_is_BorderRouter(pNodeY1));
  assert(!node_is_BorderRouter(pNodeY2));
  assert(!node_is_BorderRouter(pNodeK1));
  assert(!node_is_BorderRouter(pNodeK2));
  assert(!node_is_BorderRouter(pNodeK3));
  
  assert(!node_is_InternalRouter(pNodeB1));
  assert(!node_is_InternalRouter(pNodeB2));
  assert(!node_is_InternalRouter(pNodeB3));
  assert(!node_is_InternalRouter(pNodeB4));
    
  assert(node_is_InternalRouter(pNodeX1));
  assert(node_is_InternalRouter(pNodeX2));
  assert(node_is_InternalRouter(pNodeX3));
  assert(node_is_InternalRouter(pNodeY1));
  assert(node_is_InternalRouter(pNodeY2));
  assert(node_is_InternalRouter(pNodeK1));
  assert(node_is_InternalRouter(pNodeK2));
  assert(node_is_InternalRouter(pNodeK3));
  
  LOG_DEBUG("all node OSPF method tested... they seem ok!\n");
  
  assert(subnet_get_OSPFArea(pSubnetTX1) == X_AREA);
  assert(subnet_get_OSPFArea(pSubnetTB1) != X_AREA);
  assert(subnet_get_OSPFArea(pSubnetTB1) == BACKBONE_AREA);
  
  assert(subnet_get_OSPFArea(pSubnetSB1) == BACKBONE_AREA);
  assert(subnet_get_OSPFArea(pSubnetSB2) != X_AREA);
  assert(subnet_get_OSPFArea(pSubnetSY1) == Y_AREA);
  assert(subnet_get_OSPFArea(pSubnetSK2) != X_AREA);
  assert(subnet_get_OSPFArea(pSubnetSX2) == X_AREA);
    
  assert(subnet_is_transit(pSubnetTK1));
  assert(!subnet_is_stub(pSubnetTK1));
  
  _network_destroy();
  return 1; 
}

// ----- test_ospf -----------------------------------------------
int ospf_test()
{
  
  log_set_level(pMainLog, LOG_LEVEL_EVERYTHING);
  log_set_stream(pMainLog, stderr);
   
 // _subnet_test();
  LOG_DEBUG("all info OSPF methods tested ... they seem ok!\n");
  
  _network_destroy();
  _network_create();
  ospf_info_test();
  LOG_DEBUG("all subnet OSPF methods tested ... they seem ok!\n");
  
  _network_destroy();
  _network_create();
  ospf_rt_test();
  LOG_DEBUG("all routing table OSPF methods tested ... they seem ok!\n");
 
  _network_destroy();
  _network_create();
  _igp_domain_test();
  LOG_DEBUG("all IGPDomain methods tested... seem ok!\n");
   
//   ospf_test_sample_net();
//   LOG_DEBUG("test on sample network... seem ok!\n");

  _network_destroy();
  _network_create();
  ospf_test_rfc2328();

  ospf_deflection_test(10);
  LOG_DEBUG("test on sample network in RFC2328... seems ok!\n");
  
  return 1;
}

#endif

