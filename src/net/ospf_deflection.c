// ==================================================================
// @(#)ospf_deflection.c
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 8/09/2005
// $Id: ospf_deflection.c,v 1.4 2009-03-24 16:22:01 bqu Exp $
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
#include <net/link-list.h>
#include <net/link.h>
#include <net/prefix.h>
#include <net/igp_domain.h>
#include <net/spt_vertex.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL DESTINATION INFO OBJECT 
///////
///////  Info on destination that can have forwarding deflection
///////
////////////////////////////////////////////////////////////////////////////////

typedef SPrefix SCDestInfo; //Info on critical destination

// ----- _cd_info_create -------------------------------------------------------
SCDestInfo * _cd_info_create(SPrefix sDestPrefix) {
  SCDestInfo * pCDInfo = (SCDestInfo *) MALLOC(sizeof(SCDestInfo));
  *pCDInfo = sDestPrefix;
  return pCDInfo;
} 

// ----- _cd_info_destroy ------------------------------------------------------
void _cd_info_destroy(void * pItem) {  
  ip_prefix_destroy((SPrefix **) pItem);
}

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL DESTINATION INFO LIST OBJECT 
///////
////////////////////////////////////////////////////////////////////////////////

typedef SPtrArray * cd_info_list_t;

// ----- _cd_list_create -------------------------------------------------------
cd_info_list_t _cd_list_create() {
  cd_info_list_t pList = ptr_array_create(0, NULL, _cd_info_destroy);
  return pList;
} 

// ----- _cd_list_add ----------------------------------------------------------
int _cd_list_add(cd_info_list_t tList, SCDestInfo sDInfo) {
  SCDestInfo * pDInfo = (SCDestInfo *) MALLOC(sizeof(SCDestInfo));
  *pDInfo = sDInfo;
  fprintf(stdout, "Aggiungo dest ");
  ip_prefix_dump(stdout, *pDInfo);
fprintf(stdout, "\n");
  return ptr_array_add(tList, &pDInfo);
} 

// ----- _cd_list_destroy ------------------------------------------------------
void _cd_list_destroy(cd_info_list_t tList){ 
  ptr_array_destroy(&tList);	
} 

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL ROUTER INFO OBJECT 
///////
///////  Info on couple of a border router that in a given area
///////  could be cause of deflection.
///////
////////////////////////////////////////////////////////////////////////////////

typedef struct { 
  net_node_t         * pBR;         //router causes deflection
  net_node_t         * pER;         //exit router that can be selected
  net_link_delay_t   tRCost_BR_ER;
  ospf_area_t        tArea;       //checked area
  cd_info_list_t     tCDList;     //Critical Destination List
} SCRouterInfo;


// ----- _crouter_info_create --------------------------------------------------
SCRouterInfo * _crouter_info_create(net_node_t * pBR, net_node_t * pER, 
                            net_link_delay_t tRCost_BR_ER, ospf_area_t tArea){
  SCRouterInfo * pCRInfo = (SCRouterInfo *) MALLOC(sizeof(SCRouterInfo));
  pCRInfo->pBR = pBR;
  pCRInfo->pER = pER;
  pCRInfo->tRCost_BR_ER = tRCost_BR_ER;
  pCRInfo->tArea = tArea;
  pCRInfo->tCDList = _cd_list_create();
  return pCRInfo;
}

// ----- _crouter_info_destroy -------------------------------------------------
void _crouter_info_destroy(void * pItem) {  
  SCRouterInfo** ppCRInfo = (SCRouterInfo**) (pItem);
  if (*ppCRInfo != NULL) {
    _cd_list_destroy((*ppCRInfo)->tCDList);
    free(*ppCRInfo);
    *ppCRInfo = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////
///////
///////  CRITICAL ROUTER INFO LIST OBJECT 
///////
////////////////////////////////////////////////////////////////////////////////

typedef SPtrArray * cr_info_list_t;

// ----- _cr_info_list_create -------------------------------------------------
cr_info_list_t _cr_info_list_create() {
  cr_info_list_t pList = ptr_array_create(0, NULL, _crouter_info_destroy);
  return pList;
} 

// ----- _cr_info_list_add ----------------------------------------------------
int _cr_info_list_add(cr_info_list_t tList, SCRouterInfo * pCRInfo) {
  return ptr_array_add(tList, &pCRInfo);
} 

// ----- cr_info_list_destroy --------------------------------------------------
void cr_info_list_destroy(cr_info_list_t tList){ 
  ptr_array_destroy(&tList);	
} 


////////////////////////////////////////////////////////////////////////////////
///////
///////  OSPF DEFLECTION CHECK FUNCTION
///////
////////////////////////////////////////////////////////////////////////////////

// ----- ospf_build_cd_for_each ---------------------------------------------
int build_cd_for_each(uint32_t uKey, uint8_t uKeyLen, void * pItem, 
		                                              void * pContext){ 
  SCRouterInfo * pCtx = (SCRouterInfo *) pContext;
  SOSPFRouteInfoList * pRIList = (SOSPFRouteInfoList *) pItem;
  SOSPFRouteInfo * pRIExitRouter = NULL;// = ospf_ri_list_get_route(pRIList);
  SOSPFRouteInfo * pRIBorderRouter;
  SPrefix sRoutePrefix;
  sRoutePrefix.tNetwork = (net_addr_t) uKey;
  sRoutePrefix.uMaskLen = uKeyLen;
  int iIndex;

  for (iIndex = 0; iIndex < ptr_array_length(pRIList); iIndex++){
    ptr_array_get_at(pRIList, iIndex, &pRIExitRouter);
    if (ospf_ri_route_type_is(pRIExitRouter, NET_ROUTE_IGP))
      break;
    else{
      pRIExitRouter = NULL;
    }
  }
  
  if (pRIExitRouter != NULL && 
      ospf_route_is_adver_in_area(pRIExitRouter, pCtx->tArea)) {
    
    pRIBorderRouter = OSPF_rt_find_exact(pCtx->pBR->pOspfRT, 
		                  sRoutePrefix, NET_ROUTE_ANY, pCtx->tArea);
    if (pRIBorderRouter == NULL){

        _cd_list_add(pCtx->tCDList, sRoutePrefix);
    }
    else if (ospf_ri_get_cost(pRIExitRouter) + pCtx->tRCost_BR_ER < 
		                         ospf_ri_get_cost(pRIBorderRouter)){
        _cd_list_add(pCtx->tCDList, sRoutePrefix);
    } 
  }

  return OSPF_SUCCESS;
}


// ----- build_critical_destination_list ---------------------------------------
int build_critical_destination_list(SCRouterInfo * pCRInfo)
{
 LOG_DEBUG("bcdl on ER "); 
 ip_address_dump(stdout, pCRInfo->pER->tAddr);
 fprintf(stdout, " area %d BR ", pCRInfo->tArea);
 ip_address_dump(stdout, pCRInfo->pBR->tAddr);
 fprintf(stdout, "\n");
 if (OSPF_rt_for_each(pCRInfo->pER->pOspfRT, build_cd_for_each, pCRInfo) 
		                                                == OSPF_SUCCESS)
   return ptr_array_length(pCRInfo->tCDList);
 return 0;
}



// ----- build_critical_routers_list -------------------------------------------
int build_critical_routers_list(uint16_t uOspfDomain,
                                    ospf_area_t tArea, cr_info_list_t * tCRList) {
  int iIndexI, iIndexJ;
  SOSPFRouteInfo * pRouteIJArea, * pRouteIJBackbone;
  *tCRList = _cr_info_list_create();
  SCRouterInfo * pCRInfo;
  /* Builds the set of BR that belongs to area and are on the backbone */
  SPtrArray * pBRList = ospf_domain_get_br_on_bb_in_area(uOspfDomain, tArea);
  net_node_t * pBRI, *pBRJ;
  SPrefix sPrefix;
  
  /* Build all possibile couple of border router in the set. 
   * For each couple check if condition 1 is verified
   * If is verified check condition 2, too.
   * */
  for(iIndexI = 0; iIndexI < ptr_array_length(pBRList); iIndexI++){ 
    ptr_array_get_at(pBRList, iIndexI, &pBRI);
    for(iIndexJ = 0; iIndexJ < ptr_array_length(pBRList); iIndexJ++){ 
      if (iIndexI == iIndexJ)
        continue;
      ptr_array_get_at(pBRList, iIndexJ, &pBRJ);
      sPrefix.tNetwork = ospf_node_get_id(pBRJ);
      sPrefix.uMaskLen = 32;
      pRouteIJArea = OSPF_rt_find_exact(pBRI->pOspfRT, sPrefix,
		                        NET_ROUTE_IGP, tArea);

      /* If BR1 has not a route towards BR2 in area tArea this wants 
       * to say that there is a partition in the area. In this case no
       * deflection is possibile because there not will be a source that
       * has BR1 on its path towards BR2, if BR2.
       * */
      if (pRouteIJArea == NULL)
	continue;
      
      pRouteIJBackbone = OSPF_rt_find_exact(pBRI->pOspfRT, sPrefix, 
		                            NET_ROUTE_IGP, BACKBONE_AREA);
      /* If BR1 has not a route towards BR2 in backbone this wants 
       * to say that there is a partition in backbone.
       * Deflection can be possibile if BR2 will be choosen 
       * as exit router to exit area for some couple source-destination
       * where source has BR1 on the path towards BR2.
       * This happends because BR2 announces destination in area tArea
       * but BR1 has not a route towards it because he does not consider
       * summary in area != backbone.
       *
       * If we assume that when a route does not exist in a routing table
       * it has a cost infinite, condition 1 we try to verify is still
       * verified.
       * */
      if (pRouteIJBackbone == NULL) {      
	fprintf(stdout, "partizione nel backbone - verificata la cond 1\n");
	pCRInfo = _crouter_info_create(pBRI, pBRJ,
			               ospf_ri_get_cost(pRouteIJArea), tArea);
	int iRes =build_critical_destination_list(pCRInfo);
	fprintf(stdout, "Returned %d", iRes);
	if (iRes)   
	  _cr_info_list_add(*tCRList, pCRInfo);
	else
	  _crouter_info_destroy(&pCRInfo);
        continue;
      }   
      
      /* Checking condition 2. 
       * If it's true computing critical destination list for this 
       * couple of routers and adding them to critical routers list. 
       * */
      if (ospf_ri_get_cost(pRouteIJArea) < ospf_ri_get_cost(pRouteIJBackbone))
        fprintf(stdout, "Verificata condizione 1\n");
        pCRInfo = _crouter_info_create(pBRI, pBRJ,
			               ospf_ri_get_cost(pRouteIJArea), tArea);
        
	int iRes =build_critical_destination_list(pCRInfo); 
	fprintf(stdout, "Returned %d", iRes);
        if (iRes)   
	  _cr_info_list_add(*tCRList, pCRInfo);
	else
	  _crouter_info_destroy(&pCRInfo);
    }
  }
  return ptr_array_length(*tCRList);
}
//------ csource_check_deflection ----------------------------------------------
/**
 *  Visit the revers shorthes path tree starting from pVertex to its soon.
 *  
 */
void csource_check_deflection(SSptVertex * pVertex, SCRouterInfo * pCRInfo){
  SSptVertex * pSon;
  int iLeafIdx;
  if (spt_vertex_is_router(pVertex) && 
       (spt_vertex_to_router(pVertex)->tAddr != pCRInfo->pBR->tAddr)) {
    SCDestInfo * pCDInfo;
    int iDestIdx;
    SPrefix vertex_id = spt_vertex_get_id(pVertex);
    fprintf(stdout, "Visit ");
    ip_prefix_dump(stdout, vertex_id);
    fprintf(stdout, "\nCritical dest list: \n");
    for (iDestIdx = 0; iDestIdx < ptr_array_length(pCRInfo->tCDList); 
		                                                   iDestIdx++){
      SOSPFRouteInfo * pCDRoute;
      ptr_array_get_at(pCRInfo->tCDList, iDestIdx, &pCDInfo);
      ip_prefix_dump(stdout, *pCDInfo);
      fprintf(stdout, "\n");

      pCDRoute = OSPF_rt_find_exact(spt_vertex_to_router(pVertex)->pOspfRT, 
	  	                    *pCDInfo, NET_ROUTE_IGP, OSPF_NO_AREA);
      assert(pCDRoute != NULL);
      
      int iNHIdx;
      SOSPFNextHop * pNH;
      for (iNHIdx = 0; iNHIdx < ptr_array_length(pCDRoute->aNextHops); iNHIdx++)      {
        ptr_array_get_at(pCDRoute->aNextHops, iNHIdx, &pNH);
	if (pNH->tAdvRouterAddr != 0)
	  ip_prefix_dump(stdout, pCDRoute->sPrefix);
	if (pNH->tAdvRouterAddr == pCRInfo->pER->tAddr)
          fprintf(stdout, "Trovata route con deflection!");
      }   
    } 
  }
  //visit children
  for (iLeafIdx = 0; iLeafIdx < ptr_array_length(pVertex->sons); iLeafIdx++){
    ptr_array_get_at(pVertex->sons, iLeafIdx, &pSon);
    csource_check_deflection(pSon, pCRInfo); 
  }
}

// ----- cr_list_check_deflection ----------------------------------------------
int cr_list_check_deflection(cr_info_list_t tList,uint16_t IGPDomainNumber) {
  LOG_DEBUG("check actual deflection\n");
  int iIndex;
  SCRouterInfo * pCRInfo;

  for (iIndex = 0; iIndex < ptr_array_length(tList); iIndex++){
    ptr_array_get_at(tList, iIndex, &pCRInfo);
    
    fprintf(stdout, "CRInfo BR: ");
    ip_address_dump(stdout, pCRInfo->pBR->tAddr);
    fprintf(stdout, "\n");
    fprintf(stdout, "ERInfo ER: ");
    ip_address_dump(stdout, pCRInfo->pER->tAddr);
    
    fprintf(stdout, "Compute RSPT on ER\n");
    SRadixTree * pRspt = ospf_node_compute_rspt(pCRInfo->pER, 
		                                 IGPDomainNumber, 
						 pCRInfo->tArea);
    assert(pRspt != NULL);
    SSptVertex * pBRVertex = radix_tree_get_exact(pRspt, 
		                                  pCRInfo->pBR->tAddr, 32);
    assert(pBRVertex != NULL);
    csource_check_deflection(pBRVertex, pCRInfo); 
    radix_tree_destroy(&pRspt);
    
        
    //fprintf(stdout, "--- Per ogni dest in CRInfo\n");
    //fprintf(stdout, "------- Trova route in routing table in foglia\n");
    //fprintf(stdout, "------- if (advRouter(route) == ER(CRInfo) deflection!\n");
  }
return 0;
}




// ----- check_deflection ----------------------------------------------
int check_deflection(uint16_t IGPDomainNumber, cr_info_list_t tList) {
  LOG_DEBUG("check actual deflection in domain %d\n", IGPDomainNumber);
  int iIndex;
  SCRouterInfo * pCRInfo;

  for (iIndex = 0; iIndex < ptr_array_length(tList); iIndex++){
    ptr_array_get_at(tList, iIndex, &pCRInfo);
    
    fprintf(stdout, "CRInfo BR: ");
    ip_address_dump(stdout, pCRInfo->pBR->tAddr);
    fprintf(stdout, "\n");
    fprintf(stdout, "ERInfo ER: ");
    ip_address_dump(stdout, pCRInfo->pER->tAddr);
    
    fprintf(stdout, "Compute RSPT on ER\n");
    SRadixTree * pRspt = ospf_node_compute_rspt(pCRInfo->pER, 
		                                 IGPDomainNumber, 
						 pCRInfo->tArea);
    assert(pRspt != NULL);
    SSptVertex * pBRVertex = radix_tree_get_exact(pRspt, 
		                                  pCRInfo->pBR->tAddr, 32);
    assert(pBRVertex != NULL);
    csource_check_deflection(pBRVertex, pCRInfo); 
    radix_tree_destroy(&pRspt);
    
        
    //fprintf(stdout, "--- Per ogni dest in CRInfo\n");
    //fprintf(stdout, "------- Trova route in routing table in foglia\n");
    //fprintf(stdout, "------- if (advRouter(route) == ER(CRInfo) deflection!\n");
  }
return 0;
}


// ----- check_deflection_in_area -------------------------------------------
int check_deflection_in_area(uint16_t uOspfDomain, ospf_area_t tArea)
{
  int iIndexI, iIndexJ;
  /* Builds the set of BR that belongs to area and are on the backbone */
  SPtrArray * pBRList = ospf_domain_get_br_on_bb_in_area(uOspfDomain, tArea);
  net_node_t * pBR, * pER;
  
  /* Build all possibile couple of border router in the set. 
   * For each couple build RSPT on ER 
   * */
  for(iIndexI = 0; iIndexI < ptr_array_length(pBRList); iIndexI++) { 
    ptr_array_get_at(pBRList, iIndexI, &pBR);
    for(iIndexJ = 0; iIndexJ < ptr_array_length(pBRList); iIndexJ++) { 
      if (iIndexI == iIndexJ)
        continue;
      ptr_array_get_at(pBRList, iIndexJ, &pER);
      fprintf(stdout, "BR ");
      ip_address_dump(stdout, pBR->tAddr);
      fprintf(stdout, " - ER ");
      ip_address_dump(stdout, pER->tAddr);
      fprintf(stdout, "\n Computing rspt on ER\n");
      SRadixTree * pRspt = ospf_node_compute_rspt(pER, uOspfDomain, tArea);
      assert(pRspt != NULL);
      SSptVertex * pBRVertex = radix_tree_get_exact(pRspt, 
		                                    pBR->tAddr, 32);
      
      assert(pBRVertex != NULL);
      
      
      radix_tree_destroy(&pRspt);
    }
  }
  return 0;
}

// ----- ospf_domain_deflection ------------------------------------------------
void ospf_domain_deflection(SIGPDomain * pDomain) { 

  check_deflection_in_area(pDomain->uNumber, 2);

/*    if (cs_check_deflection(tCBRList, uOspfDomain))
      fprintf(stdout, "Deflection!\n");
  //}
  cr_info_list_destroy(tCBRList);*/
}

////////////////////////////////////////////////////////////////////////////////
///////
///////  OSPF DEFLECTION TEST FUNCTION
///////
////////////////////////////////////////////////////////////////////////////////

//----- ospf_deflection_test() -------------------------------------------------
int ospf_deflection_test(uint16_t uOspfDomain) { 
  //cr_info_list_t tCBRList = NULL;
  //if (build_critical_routers_list(uOspfDomain, 2, &tCBRList)){  
  //  assert(tCBRList != NULL);
  check_deflection_in_area(uOspfDomain, 2);

/*    if (cs_check_deflection(tCBRList, uOspfDomain))
      fprintf(stdout, "Deflection!\n");
  //}
  cr_info_list_destroy(tCBRList);*/
  return OSPF_SUCCESS;
}




#endif


