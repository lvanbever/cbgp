// ==================================================================
// @(#)spt_vertex.c
//
// @author Stefano Iasi (stefanoia@tin.it), (C) 2005
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 4 July 2005
// $Id: spt_vertex.c,v 1.12 2009-03-24 16:27:19 bqu Exp $
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

#include <net/spt_vertex.h>
#include <net/node.h>
#include <net/ospf.h>
#include <net/igp_domain.h>
#include <net/link.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
///////  spt computation: vertex object 
//////////////////////////////////////////////////////////////////////////

// ----- spt_vertex_create_byRouter ---------------------------------------
SSptVertex * spt_vertex_create_byRouter(net_node_t * pNode, net_link_delay_t uIGPweight)
{
  SSptVertex * pVertex = (SSptVertex *) MALLOC(sizeof(SSptVertex));
  pVertex->uDestinationType = NET_LINK_TYPE_ROUTER;
//   (pVertex->UDestId).pNode = pNode;
  pVertex->pObject = pNode;
  pVertex->uIGPweight = uIGPweight;
  pVertex->pNextHops = ospf_nh_list_create();
  pVertex->aSubnets = ptr_array_create(0, NULL, NULL);
  pVertex->fathers = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  pVertex->sons = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  pVertex->tVertexInfo = 0;
  return pVertex;
}

// ----- spt_vertex_create_bySubnet ---------------------------------------
SSptVertex * spt_vertex_create_bySubnet(net_subnet_t * pSubnet, net_link_delay_t uIGPweight)
{
  SSptVertex * pVertex = (SSptVertex *) MALLOC(sizeof(SSptVertex));
  pVertex->uDestinationType = NET_LINK_TYPE_TRANSIT;
//   (pVertex->UDestId).pSubnet = pSubnet;
  pVertex->pObject = pSubnet;
  pVertex->uIGPweight = uIGPweight;
  pVertex->pNextHops = ospf_nh_list_create();
  pVertex->aSubnets = ptr_array_create(0, NULL, NULL);
  pVertex->fathers = ptr_array_create(ARRAY_OPTION_UNIQUE | ARRAY_OPTION_SORTED, spt_vertex_compare, NULL);
  pVertex->sons = ptr_array_create(ARRAY_OPTION_UNIQUE, spt_vertex_compare, NULL);
  pVertex->tVertexInfo = 0;
  return pVertex;
}

// ----- spt_vertex_create ----------------------------------------------------
/* Create a new vertex starting from link info.
 * There are three possibility:
 *
 * 1) Link source is a subnet: in this case destination node is 
 *    available in link info (link is the same from node to subnet)
 *    and cost of the link must be consider 0
 * 2) Link source is a node and destination is a router: in this case 
 *    destination node must be found in network starting from 
 *    address stored in link
 * 3) Link source is a node and destination is a subnet: in this case
 *    destination prefix can be obtain by subnet pointer stored in link
 */    
SSptVertex * spt_vertex_create(SNetwork * pNetwork, net_iface_t * pLink, 
			       SSptVertex * pVFather)
{
  SSptVertex * pVertex;
  
  if (spt_vertex_is_subnet(pVFather)){
    pVertex = spt_vertex_create_byRouter(pLink->pSrcNode, 
		                         pVFather->uIGPweight);
  }
  else if (pLink->tType == NET_IFACE_RTR){
    net_node_t * pNode;
    pNode = network_find_node(link_get_address(pLink));
    assert(pNode != NULL);
    pVertex = spt_vertex_create_byRouter(pNode, pVFather->uIGPweight + 
		                                pLink->uIGPweight);
  }
  else if (pLink->tType == NET_IFACE_PTMP) {
    net_subnet_t * pSubnet;
    pSubnet = pLink->tDest.pSubnet;
    pVertex = spt_vertex_create_bySubnet(pSubnet, pVFather->uIGPweight + 
		                                  pLink->uIGPweight);
  }
  else
    return NULL;
    
  return pVertex;
}

// ----- spt_vertex_destroy ---------------------------------------
void spt_vertex_destroy(SSptVertex ** ppVertex){
  if (*ppVertex != NULL){
    if ((*ppVertex)->pNextHops != NULL)
      ospf_nh_list_destroy(&((*ppVertex)->pNextHops));
    if ((*ppVertex)->aSubnets != NULL)
      ptr_array_destroy(&((*ppVertex)->aSubnets));
    if ((*ppVertex)->sons != NULL)
      ptr_array_destroy(&((*ppVertex)->sons));
    if ((*ppVertex)->fathers != NULL)
      ptr_array_destroy(&((*ppVertex)->fathers));
    FREE(*ppVertex);
     
    *ppVertex = NULL;
  }    
}

// ----- spt_vertex_dst ---------------------------------------
void spt_vertex_dst(void ** ppItem){
  SSptVertex * pVertex = *((SSptVertex **)(ppItem));
  spt_vertex_destroy(&pVertex); 
}


// ----- spt_vertex_get_links ---------------------------------------
SPtrArray * spt_vertex_get_links(SSptVertex * pVertex)
{
  if (spt_vertex_is_router(pVertex)){
    return  ((net_node_t *)(pVertex->pObject))->pLinks;
  }
  return((net_subnet_t *)(pVertex->pObject))->pLinks; 
}

// ----- spt_vertex_belongs_to_area ---------------------------------------
int spt_vertex_belongs_to_area(SSptVertex * pVertex, ospf_area_t tArea)
{
  if (spt_vertex_is_router(pVertex))
    return  node_belongs_to_area(spt_vertex_to_router(pVertex), tArea);
  return  subnet_belongs_to_area(spt_vertex_to_subnet(pVertex), tArea);
}

// ----- spt_vertex_get_id ---------------------------------------
SPrefix spt_vertex_get_id(SSptVertex * pVertex)
{     
  SPrefix p;
  if (spt_vertex_is_router(pVertex)) {
    p.tNetwork = (spt_vertex_to_router(pVertex))->tAddr;
    p.uMaskLen = 32;
  }
  else {
    p = (spt_vertex_to_subnet(pVertex))->sPrefix;
  }
  return p;
}


// ----- spt_vertex_compare ---------------------------------------
int spt_vertex_compare(void * pItem1, void * pItem2, unsigned int uEltSize)
{
  SSptVertex * pVertex1= *((SSptVertex **) pItem1);
  SSptVertex * pVertex2= *((SSptVertex **) pItem2); 
  
  if (pVertex1->uDestinationType == pVertex2->uDestinationType) {
    if (pVertex1->uDestinationType == NET_LINK_TYPE_ROUTER){
      if (((net_node_t *)(pVertex1->pObject))->tAddr < ((net_node_t *)(pVertex2->pObject))->tAddr)
        return -1;
      else if (((net_node_t *)(pVertex1->pObject))->tAddr > ((net_node_t *)(pVertex2->pObject))->tAddr)
        return 1;
      else
	return 0;
    }
    else {
      SPrefix sPrefV1 = spt_vertex_get_id(pVertex1); 
      SPrefix sPrefV2 = spt_vertex_get_id(pVertex2); 
      SPrefix * pPrefV1 = &sPrefV1;
      SPrefix * pPrefV2 = &sPrefV2;
      return ip_prefix_cmp(pPrefV1, pPrefV2);
    }
  }
  else if (pVertex1->uDestinationType == NET_LINK_TYPE_ROUTER)
   return -1;
  else 
   return  1;
}



// ----- spt_get_best_candidate -----------------------------------------------
/*
  Search for the best candidate to add to SPT.
  - vertex with smallest cost are preferred
  - transit network are preferred on router
*/
SSptVertex * spt_get_best_candidate(SPtrArray * paGrayVertexes)
{
  SSptVertex * pBestVertex = NULL;
  SSptVertex * pCurrentVertex = NULL;
  
  int iBestIndex = 0, iIndex;
  
  if (ptr_array_length(paGrayVertexes) > 0) {
    
    for (iIndex = 0	; iIndex < ptr_array_length(paGrayVertexes); iIndex++) {
      ptr_array_get_at(paGrayVertexes, iIndex, &pCurrentVertex);
      assert(pCurrentVertex != NULL);
      if (pBestVertex == NULL){
// 	ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
// 	fprintf(stdout, "\n");
	pBestVertex = pCurrentVertex;
	iBestIndex = iIndex;
      }
      else if (pCurrentVertex->uIGPweight <  pBestVertex->uIGPweight){
// 	ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
// 	fprintf(stdout, "\n");
	pBestVertex = pCurrentVertex;
	iBestIndex = iIndex;
      }
      else if (pCurrentVertex->uIGPweight == pBestVertex->uIGPweight){ 
        if (pCurrentVertex->uDestinationType > pBestVertex->uDestinationType){ 
// 	  ip_prefix_dump(stdout, spt_vertex_get_prefix(pCurrentVertex));
// 	  fprintf(stdout, "\n");
	  pBestVertex = pCurrentVertex;
	  iBestIndex = iIndex;
	}
      }
   }  
//    fprintf(stdout, "rimuovo da aGrayVertex ...\n");
   ptr_array_remove_at(paGrayVertexes, iBestIndex);
  } 
  return pBestVertex;
}

// ----- spt_vertex_has_father -------------------------------------------------
int spt_vertex_has_father(SSptVertex * pParent, SSptVertex * pRoot){
  unsigned int uIndex;

  if (ptr_array_sorted_find_index(pParent->fathers, &pRoot, &uIndex) == 0)
    return 1;
  return 0;
}


// ----- calculate_next_hop ----------------------------------------------------
/**
 * Calculates next hop as explained in RFC2328 (p. 167)
 *
 * pRoot        = SPT's root node
 * pParent      = parent node of pDestination (beetween pRoot and pDestination)
 * pDestination = node procedure calculate next hops for
 * pLink        = pParent's link towards pDestination
 */
 
void spt_calculate_next_hop(SSptVertex * pRoot, SSptVertex * pParent, 
			    SSptVertex * pDestination,
			    net_iface_t * pLink){
  SOSPFNextHop * pNH = NULL, * pNHCopy = NULL;
  int iLink;
  
  if (pRoot == pParent){
    pNH = ospf_next_hop_create(pLink, OSPF_NO_IP_NEXT_HOP);
    ospf_nh_list_add(pDestination->pNextHops, pNH);
  }
  else if (pParent->uDestinationType == NET_LINK_TYPE_TRANSIT && 
           spt_vertex_has_father(pParent, pRoot)) {
    for(iLink = 0; iLink < ptr_array_length(pParent->pNextHops); iLink++){
      ptr_array_get_at(pParent->pNextHops, iLink, &pNHCopy);
      pNH = ospf_next_hop_create(pNHCopy->pLink, pLink->tIfaceAddr);
      ospf_nh_list_add(pDestination->pNextHops, pNH);
    }
  }
  else {
    //there is at least a router beetween root and destination
    //fprintf(stdout, "spt_calculate_next_hop(): inherited from parent\n");
    ospf_nh_list_add_list(pDestination->pNextHops, pParent->pNextHops);
  }
}

// ----- spt_vertex_add_subnet -------------------------------------------------
int spt_vertex_add_subnet(SSptVertex * pCurrentVertex,
			  net_iface_t * pCurrentLink){
  return ptr_array_add(pCurrentVertex->aSubnets, &pCurrentLink);
}


// ----- spt_candidate_compute_id -------------------------------------------
SPrefix spt_candidate_compute_id(SSptVertex * pParentVertex, 
				 net_iface_t * pLink){
  SPrefix sVertexId;
  if (spt_vertex_is_subnet(pParentVertex)) { 
    sVertexId.tNetwork = pLink->pSrcNode->tAddr;
    sVertexId.uMaskLen = 32;
  }
  else
    link_get_prefix(pLink, &sVertexId);
  return sVertexId;
}

#define spt_candidate_is_router(C) (C.uMaskLen == 32)

// ----- node_ospf_compute_spt -------------------------------------------------
/**
 * Compute the Shortest Path Tree from the given source router
 * towards all the other routers that belong to the given domain.
 * 
 * TODO - improve computation
 *
 * PREREQUISITE:
 *      1. igp domain MUST exist
 *      2. ospf areas configuration is correct
 *      
 */
SRadixTree * node_ospf_compute_spt(net_node_t * pNode, uint16_t IGPDomainNumber, ospf_area_t tArea)
{
  SPrefix        sCandidateId; 
  int            iIndex = 0;
  unsigned int   uOldVertexPos;
  SNetwork     * pNetwork = pNode->pNetwork;
  SIGPDomain   * pIGPDomain = get_igp_domain(IGPDomainNumber);
  
  SPtrArray    * aLinks = NULL;
  net_iface_t     * pCurrentLink = NULL;
  SSptVertex   * pCurrentVertex = NULL, * pNewVertex = NULL;
  SSptVertex   * pOldVertex = NULL, * pRootVertex = NULL;
  SRadixTree   * pSpt = radix_tree_create(32, spt_vertex_dst);
  net_node_t     * pRootNode = pNode;
  assert(pRootNode != NULL);
  spt_vertex_list_t * aGrayVertexes = ptr_array_create(ARRAY_OPTION_SORTED|
		                                       ARRAY_OPTION_UNIQUE,
                                                       spt_vertex_compare,
     	    				               NULL);
  //ADD root to SPT
  pRootVertex            = spt_vertex_create_byRouter(pRootNode, 0);
  pCurrentVertex         = pRootVertex;
  assert(pCurrentVertex != NULL);
  
  while (pCurrentVertex != NULL) {
    sCandidateId = spt_vertex_get_id(pCurrentVertex);
    radix_tree_add(pSpt, sCandidateId.tNetwork, sCandidateId.uMaskLen,
		         pCurrentVertex);
    
    aLinks = spt_vertex_get_links(pCurrentVertex);
    for (iIndex = 0; iIndex < ptr_array_length(aLinks); iIndex++){
      ptr_array_get_at(aLinks, iIndex, &pCurrentLink);
      
      /* Consider only the links that have the following properties:
	 - NET_LINK_FLAG_UP (the link must be UP)
	 - the end-point belongs to the given prefix */
      if (!(link_get_state(pCurrentLink, NET_LINK_FLAG_UP)) || 
          !(link_belongs_to_area(pCurrentLink, tArea))){
        continue;
      }
      //I must check the status of the link in the opposite direction
      net_iface_t * pBckLink = link_find_backword(pCurrentLink);
      if (!link_get_state(pBckLink, NET_LINK_FLAG_UP))
        continue;

      //first time consider only Transit Link... but we store subnet in a list
      //so we have not to recheck all the links durign routing table computation
      if (pCurrentLink->uDestinationType == NET_LINK_TYPE_STUB ||
	  (pCurrentLink->uDestinationType == NET_LINK_TYPE_ROUTER && 
	  /*TODO controlla non venga da subnet*/ 
	   (link_to_router_has_ip_prefix(pCurrentLink) || 
	     link_to_router_has_only_iface(pCurrentLink)))) {
	spt_vertex_linked_to_subnet(pCurrentVertex);
        spt_vertex_add_subnet(pCurrentVertex, pCurrentLink);
        if (pCurrentLink->uDestinationType != NET_LINK_TYPE_ROUTER) 
	  continue;
      }
      
      //compute vertex id 
      sCandidateId = spt_candidate_compute_id(pCurrentVertex, pCurrentLink);

      //ROUTER should belongs to ospf domain 
      if (spt_candidate_is_router(sCandidateId)) {
         if (!igp_domain_contains_router_by_addr(pIGPDomain, 
				                sCandidateId.tNetwork)) 
           continue;
      }
      else 
	 //if is a subnet I remember vertex is linked to it 
	 //(see route building process)
	 spt_vertex_linked_to_subnet(pCurrentVertex); 
      
      //check if vertex is in spt	
      pOldVertex = (SSptVertex *)radix_tree_get_exact(pSpt, 
                                                          sCandidateId.tNetwork, 
							  sCandidateId.uMaskLen);
      
      if(pOldVertex != NULL)
       continue;
      
      //create a vertex object to compare with grayVertex
      pNewVertex = spt_vertex_create(pNetwork, pCurrentLink, pCurrentVertex);
      assert(pNewVertex != NULL);
      
      //check if vertex is in grayVertexes
      int srchRes = -1;
      if (ptr_array_length(aGrayVertexes) > 0)
        srchRes = ptr_array_sorted_find_index(aGrayVertexes, &pNewVertex, &uOldVertexPos);
      else ;
//         fprintf(stdout, "aGrays è vuoto\n");
      //srchres == -1 ==> item not found
      //srchres == 0  ==> item found
      pOldVertex = NULL;
      if(srchRes == 0) {
//         fprintf(stdout, "vertex è già in aGrayVertexes\n");
        ptr_array_get_at(aGrayVertexes, uOldVertexPos, &pOldVertex);
      }
      
      if (pOldVertex == NULL){
//         fprintf(stdout, "vertex da aggiungere\n");
//         if (spt_vertex_belongs_to_area(pNewVertex, tArea)){
          spt_calculate_next_hop(pRootVertex, pCurrentVertex, pNewVertex, pCurrentLink);
        
	  ptr_array_add(aGrayVertexes, &pNewVertex);
	  //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - START
	  //set father and sons relationship
  	  ptr_array_add(pCurrentVertex->sons, &pNewVertex);
	  ptr_array_add(pNewVertex->fathers, &pCurrentVertex);
	  //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - STOP
// }
//  	else
//  	  spt_vertex_destroy(&pNewVertex);
	
      }
      else if (pOldVertex->uIGPweight > pNewVertex->uIGPweight) {
//         fprintf(stdout, "vertex da aggiornare (peso minore)\n");
        unsigned int uPos;
	int iIndex;
	SSptVertex * pFather;
	pOldVertex->uIGPweight = pNewVertex->uIGPweight;
// 	Remove old next hops
        if (ptr_array_length(pOldVertex->pNextHops) > 0){
          ospf_nh_list_destroy(&(pOldVertex->pNextHops));
 	  pOldVertex->pNextHops = ospf_nh_list_create();
        }
        spt_calculate_next_hop(pRootVertex, pCurrentVertex, pOldVertex, 
			                                    pCurrentLink);
        //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR 
	//   FASTER RECOMPUTATION) - START
	//remove old father->sons relationship
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_get_at(pOldVertex->fathers, iIndex, &pFather);
	  assert(ptr_array_sorted_find_index(pFather->sons, &pOldVertex, &uPos) == 0);
	  ptr_array_remove_at(pFather->sons, uPos);
	}
	//remove sons->father relationship
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_remove_at(pOldVertex->fathers, iIndex);
	}
	//set new sons->father and father->sons relationship
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
        //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - STOP
	spt_vertex_destroy(&pNewVertex);
      }
      else if (pOldVertex->uIGPweight == pNewVertex->uIGPweight) {
//         fprintf(stdout, "vertex da aggiornare (peso uguale)\n");
        spt_calculate_next_hop(pRootVertex, pCurrentVertex, pOldVertex, pCurrentLink);
// 	fprintf(stdout, "next hops calcolati\n");
        //// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - START
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
	//// THIS IS GOOD IF WE WOULD TO STORE SPT (FOR PRINT OR FOR FASTER RECOMPUTATION) - STOP
        spt_vertex_destroy(&pNewVertex);
      }
      else {
//         fprintf(stdout, "new vertex eliminato\n");
        spt_vertex_destroy(&pNewVertex);
      }  
    }//end for on links
    //links for subnet are dinamically created and MUST be removed
     /*if (pCurrentVertex->uDestinationType == NET_LINK_TYPE_TRANSIT ||
         pCurrentVertex->uDestinationType == NET_LINK_TYPE_STUB)
 	ptr_array_destroy(&aLinks);*/
    //select node with smallest weight to add to spt
    pCurrentVertex = spt_get_best_candidate(aGrayVertexes);
  }
  ptr_array_destroy(&aGrayVertexes);
  return pSpt;
}

// ----- ospf_node_compute_rspt -----------------------------------------------
/**
 * Compute the Reverse Shortest Path Tree from the given source router
 * towards all the other routers that belong to the given domain.
 * Algorithm is the same of SPT computation but use link in the 
 * destination->source direction.
 * TODO improve computation
 *
 * PREREQUISITE:
 *      1. igp domain MUST exist
 *      2. ospf areas configuration is correct
 *      3. when a link is down it is down in all the two direction
 */
SRadixTree * ospf_node_compute_rspt(net_node_t * pNode, uint16_t IGPDomainNumber, 
  	                                                     ospf_area_t tArea)
{
  SPrefix        sCandidateId; 
  int            iIndex         = 0;
  unsigned int   uOldVertexPos;
  SNetwork     * pNetwork       = pNode->pNetwork;
  SIGPDomain   * pIGPDomain     = get_igp_domain(IGPDomainNumber);
  
  SPtrArray    * aLinks         = NULL;
  net_iface_t     * pCurrentLink   = NULL;
  SSptVertex   * pCurrentVertex = NULL, * pNewVertex = NULL;
  SSptVertex   * pOldVertex     = NULL, * pRootVertex = NULL;
  SRadixTree   * pRspt           = radix_tree_create(32, spt_vertex_dst);
  net_node_t     * pRootNode      = pNode;
  
  spt_vertex_list_t * aGrayVertexes = ptr_array_create(ARRAY_OPTION_SORTED|
		                                       ARRAY_OPTION_UNIQUE,
                                                       spt_vertex_compare,
     	    				               NULL);
  //ADD root to SPT
  pRootVertex            = spt_vertex_create_byRouter(pRootNode, 0);
  pCurrentVertex         = pRootVertex;
  assert(pCurrentVertex != NULL);
  
  while (pCurrentVertex != NULL) {
    sCandidateId = spt_vertex_get_id(pCurrentVertex);
    radix_tree_add(pRspt, sCandidateId.tNetwork, sCandidateId.uMaskLen,
		         pCurrentVertex);
    
    aLinks = spt_vertex_get_links(pCurrentVertex);
    for (iIndex = 0; iIndex < ptr_array_length(aLinks); iIndex++){
      ptr_array_get_at(aLinks, iIndex, &pCurrentLink);
      
      //for rspt we are intered in transit vertex only
      if (pCurrentLink->uDestinationType == NET_LINK_TYPE_STUB) {
	  continue;
      }

      /* Consider only the links that have the following properties:
	 - NET_LINK_FLAG_UP (the link must be UP)
	 - the end-point belongs to the given prefix */
      if (!(link_get_state(pCurrentLink, NET_LINK_FLAG_UP)) || 
          !(link_belongs_to_area(pCurrentLink, tArea))){
        continue;
      }
      
      //I must check the status of the link in the opposite direction
      net_iface_t * pBckLink = link_find_backword(pCurrentLink);
      if (!link_get_state(pBckLink, NET_LINK_FLAG_UP))
        continue;
           
      //compute vertex id 
      sCandidateId = spt_candidate_compute_id(pCurrentVertex, pCurrentLink);

      //a router should belongs to ospf domain 
      //while if pCurrentVertex is linked to a subnet its loopback
      //should not to be used to build a route
      if (spt_candidate_is_router(sCandidateId)) {
         if (!igp_domain_contains_router_by_addr(pIGPDomain, 
				                 sCandidateId.tNetwork)) 
           continue;
      }
      else 
	 spt_vertex_linked_to_subnet(pCurrentVertex); 
      
      //check if vertex is in spt	
      pOldVertex = (SSptVertex *)radix_tree_get_exact(pRspt,                                                           sCandidateId.tNetwork,						               sCandidateId.uMaskLen);
      
      if(pOldVertex != NULL)
       continue;
      
      //create a vertex object to compare with grayVertexs
      //for rspt we have to consider the cost of the backward link
      pNewVertex = spt_vertex_create(pNetwork, pCurrentLink, pCurrentVertex);
      assert(pNewVertex != NULL);
      
      //check if vertex is in grayVertexes
      int srchRes = -1;
      if (ptr_array_length(aGrayVertexes) > 0)
        srchRes = ptr_array_sorted_find_index(aGrayVertexes, &pNewVertex, &uOldVertexPos);
      else ;
      //srchres == -1 ==> item not found
      //srchres == 0  ==> item found
      pOldVertex = NULL;
      if(srchRes == 0) {
        ptr_array_get_at(aGrayVertexes, uOldVertexPos, &pOldVertex);
      }
      
      if (pOldVertex == NULL){
	  ptr_array_add(aGrayVertexes, &pNewVertex);
	  //set father and sons relationship
  	  ptr_array_add(pCurrentVertex->sons, &pNewVertex);
	  ptr_array_add(pNewVertex->fathers, &pCurrentVertex);
      }
      else if (pOldVertex->uIGPweight > pNewVertex->uIGPweight) {
        unsigned int uPos;
	int iIndex;
	SSptVertex * pFather;
	pOldVertex->uIGPweight = pNewVertex->uIGPweight;
	//remove old father->sons relationship
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_get_at(pOldVertex->fathers, iIndex, &pFather);
	  assert(ptr_array_sorted_find_index(pFather->sons, &pOldVertex, &uPos) == 0);
	  ptr_array_remove_at(pFather->sons, uPos);
	}
	//remove sons->father relationship
	for( iIndex = 0; iIndex < ptr_array_length(pOldVertex->fathers); iIndex++){
	  ptr_array_remove_at(pOldVertex->fathers, iIndex);
	}
	//set new sons->father and father->sons relationship
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
	spt_vertex_destroy(&pNewVertex);
      }
      else if (pOldVertex->uIGPweight == pNewVertex->uIGPweight) {
        ptr_array_add(pOldVertex->fathers, &pCurrentVertex);
        ptr_array_add(pCurrentVertex->sons, &pOldVertex);
        spt_vertex_destroy(&pNewVertex);
      }
      else {
        spt_vertex_destroy(&pNewVertex);
      }  
    }//end for on links
    //select node with smallest weight to add to spt
    pCurrentVertex = spt_get_best_candidate(aGrayVertexes);
  }
  ptr_array_destroy(&aGrayVertexes);
  return pRspt;
}


//------ compute_vertical_bars -------------------------------------------
/**
 *  Compute string to dump a text-based graphics spt
 */
void compute_vertical_bars(FILE * pStream, int iLevel, char * pcBars){
  int iIndex;
//   strcpy(pcPrefix, "");
  strcpy(pcBars, "");
  if (iLevel != 0){
    for (iIndex = 0; iIndex < iLevel; iIndex++)
    	strcat(pcBars, "            |");
//     strcat(pcSpace, "|");
    
  }
}

//------ visit_vertex -------------------------------------------
/**
 *  Visit a node of the spt to dump it
 */
void visit_vertex(SSptVertex * pVertex, int iLevel, int lastChild, char * pcSpace, 
                  SRadixTree * pVisited, FILE * pStream){
  char * pcPrefix = MALLOC(30);
  SPrefix sPrefix, sChildPfx;
  SSptVertex * pChild;
  int iIndex; 
  strcpy(pcSpace, "");
  strcpy(pcPrefix, "");
  compute_vertical_bars(pStream, iLevel, pcSpace);
  

  
  fprintf(pStream, "%s\n%s", pcSpace, pcSpace);
  
  
  sPrefix = spt_vertex_get_id(pVertex);
  ip_prefix_to_string(pcPrefix, &sPrefix);
  
  fprintf(pStream, "---> [ %s ]-[ %d ] -- NH >> ", pcPrefix, pVertex->uIGPweight);
  
  if (pVertex->pNextHops != NULL){
//     if (lastChild)
//       pcSpace[strlen(pcSpace) - 1] = ' ';
      for(iIndex = 0; iIndex < strlen(pcPrefix) + 24 + 3; iIndex++) //last term should be dynamic
         strcat(pcSpace, " ");
    ospf_nh_list_dump(pStream, pVertex->pNextHops, 0, pcSpace);
  }
  radix_tree_add(pVisited, sPrefix.tNetwork, sPrefix.uMaskLen, pVertex);
  iLevel++;
  for(iIndex = 0; iIndex < ptr_array_length(pVertex->sons); iIndex++){
     ptr_array_get_at(pVertex->sons, iIndex, &pChild);
     sChildPfx = spt_vertex_get_id(pChild);
     if (radix_tree_get_exact(pVisited, sChildPfx.tNetwork, sChildPfx.uMaskLen) != NULL)
       continue;
//      if (iIndex == ptr_array_length(pVertex->sons) - 1)
       visit_vertex(pChild, iLevel, 1, pcSpace, pVisited, pStream);
//      else
//        visit_vertex(pChild, iLevel, 0, pcSpace, pVisited, pStream);
  }
  
  FREE(pcPrefix);
}
#define startAddr 1024

char * ip_to_name(int tAddr){
// const int startAddr = 1024;

 switch(tAddr){
    case startAddr      : return "RT1"; break;
    case startAddr + 1  : return "RT2"; break;
    case startAddr + 2  : return "RT3"; break;
    case startAddr + 3  : return "RT4"; break;
    case startAddr + 4  : return "RT5"; break;
    case startAddr + 5  : return "RT6"; break;
    case startAddr + 6  : return "RT7"; break;
    case startAddr + 7  : return "RT8"; break;
    case startAddr + 8  : return "RT9"; break;
    case startAddr + 9  : return "RT10"; break;
    case startAddr + 10 : return "RT11"; break;
    case startAddr + 11 : return "RT12"; break;
    case 4              : return "N1"; break;
    case 8              : return "N2"; break;
    case 12             : return "N3"; break;
    case 16             : return "N4"; break;
    case 20             : return "N6"; break;
    case 24             : return "N7"; break;
    case 28             : return "N8"; break;
    case 32             : return "N9"; break;
    case 36             : return "N10"; break;
    case 40             : return "N11"; break;
    default             : return "?" ; 
  }
}
//------ visit_vertex -------------------------------------------
/**
 *  Visit a node of the spt to dump it
 */
void visit_vertex_dot(SSptVertex * pVertex, char * pcPfxFather, SRadixTree * pVisited, net_link_delay_t tWFather, FILE * pStream){
  char * pcMyPrefix = MALLOC(30);
  char cNextHop[25] = "", cNextHops[300] = "", * pcNextHops = cNextHops, * pcNH = cNextHop;
  SSptVertex * pChild;
  SPrefix sMyPrefix = spt_vertex_get_id(pVertex), sChildPfx;
  SOSPFNextHop * pNH;
  
  ip_prefix_to_string(pcMyPrefix, &sMyPrefix);
  int iIndex;
  
  
  //print node
  fprintf(pStream, "\"%s\" -> \"%s\" [label=\"%d\"];\n", pcPfxFather, pcMyPrefix, pVertex->uIGPweight - tWFather);
   assert(pVertex->pNextHops != NULL);
  if (ptr_array_length(pVertex->pNextHops) > 0){
    //print next hop
    for (iIndex = 0; iIndex < ptr_array_length(pVertex->pNextHops); iIndex++){
      ptr_array_get_at(pVertex->pNextHops, iIndex, &pNH);
      assert(pNH->pLink != NULL);
      ospf_next_hop_to_string(pcNH, pNH);
      strcat(pcNextHops, pcNH);
      strcat(pcNextHops, "\\n");
    }
    fprintf(pStream, "\"%sNH\" [shape = box, label=\"%s\", style=filled, color=\".7 .3 1.0\"] \n", pcMyPrefix, pcNextHops);
    fprintf(pStream, "\"%s\" -> \"%sNH\" [style = dotted];\n", pcMyPrefix, pcMyPrefix);
  }
  
  radix_tree_add(pVisited, sMyPrefix.tNetwork, sMyPrefix.uMaskLen, pVertex);
  assert(pVertex->sons != 0);
  for(iIndex = 0; iIndex < ptr_array_length(pVertex->sons); iIndex++){
     ptr_array_get_at(pVertex->sons, iIndex, &pChild);
     sChildPfx = spt_vertex_get_id(pChild);
     if (radix_tree_get_exact(pVisited, sChildPfx.tNetwork, sChildPfx.uMaskLen) != NULL)
       continue;
     visit_vertex_dot(pChild, pcMyPrefix, pVisited, pVertex->uIGPweight, pStream);
  }
  
  FREE(pcMyPrefix);
}

// ----- spt_dump ------------------------------------------
void spt_dump(FILE * pStream, SRadixTree * pSpt, net_addr_t tRadixAddr)
{
  SSptVertex * pRadix = (SSptVertex *)radix_tree_get_exact(pSpt, tRadixAddr, 32);
  char * pcSpace = MALLOC(500);
  SRadixTree * pVisited =  radix_tree_create(32, NULL);
  visit_vertex(pRadix, 0, 1, pcSpace, pVisited, pStream);
}

// ----- spt_dump_dot ------------------------------------------
void spt_dump_dot(FILE * pStream, SRadixTree * pSpt, net_addr_t tRadixAddr)
{
  SSptVertex * pRadix = (SSptVertex *)radix_tree_get_exact(pSpt, tRadixAddr, 32);
//   char * pcSpace = MALLOC(500);
  SPrefix sPref;
  SRadixTree * pVisited =  radix_tree_create(32, NULL);
  sPref.tNetwork = tRadixAddr;
  sPref.uMaskLen = 32;
  char pcRootPrefix[30];
  ip_prefix_to_string(pcRootPrefix, &sPref);
  fprintf(pStream, "digraph G {\n");
  visit_vertex_dot(pRadix, pcRootPrefix, pVisited, 0, pStream);
  fprintf(pStream, "}\n");
  radix_tree_destroy(&pVisited);
}
#endif
