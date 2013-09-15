// ==================================================================
// @(#)ospf_rt.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// $Id: ospf_rt.h,v 1.13 2009-03-24 16:22:02 bqu Exp $
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

#ifndef __NET_OSPF_RT_H__
#define __NET_OSPF_RT_H__

#ifdef OSPF_SUPPORT

#include <libgds/types.h>
#include <libgds/array.h>
#include <libgds/memory.h>
#include <net/net_types.h>
#include <net/prefix.h>
#include <net/ospf.h>
#include <net/link.h>

#define OSPF_RT_SUCCESS               0
#define OSPF_RT_ERROR_NH_UNREACH     -1
#define OSPF_RT_ERROR_IF_UNKNOWN     -2
#define OSPF_RT_ERROR_ADD_DUP        -3
#define OSPF_RT_ERROR_DEL_UNEXISTING -4

#define OSPF_DESTINATION_TYPE_NETWORK 0
#define OSPF_DESTINATION_TYPE_ROUTER  1
#define OSPF_DESTINATION_TYPE_ASBR    2

#define OSPF_PATH_TYPE_INTRA       0
#define OSPF_PATH_TYPE_INTER       1
#define OSPF_PATH_TYPE_EXTERNAL_1  2
#define OSPF_PATH_TYPE_EXTERNAL_2  3
#define OSPF_PATH_TYPE_ANY         4

#define OSPF_NO_IP_NEXT_HOP 0
#define OSPF_RT_OPTION_SORT_AREA      0x01
#define OSPF_RT_OPTION_SORT_PATH_TYPE 0x02

// ----- ospf_rt_test() -----------------------------------------------
extern int ospf_rt_test();

///////////////////////////////////////////////////////////////////////////
//////  OSPF NEXT HOP FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- ospf_next_hop_create --------------------------------------------------
extern SOSPFNextHop * ospf_next_hop_create(net_iface_t * pLink, net_addr_t tAddr);
// ----- ospf_next_hop_destroy -------------------------------------------------
extern void ospf_next_hop_destroy(SOSPFNextHop ** ppNH);

// ----- OSPF_next_hop_dump --------------------------------------------
extern void ospf_next_hop_dump(FILE* pStream, SOSPFNextHop * pNH, int iPathType); 
// ----- ospf_next_hops_compare -----------------------------------------------
extern int ospf_next_hops_compare(void * pItem1, void * pItem2, unsigned int uEltSize);
// ----- ospf_next_hops_compare -----------------------------------------------
void ospf_next_hop_to_string(char * pString, SOSPFNextHop * pNH);

// ----- ospf_nh_list_create --------------------------------------------------
extern next_hops_list_t * ospf_nh_list_create();
// ----- ospf_nh_list_destroy --------------------------------------------------
extern void ospf_nh_list_destroy(next_hops_list_t ** pNHList);
// ----- ospf_nh_list_add --------------------------------------------------
extern int ospf_nh_list_add(next_hops_list_t * pNHList, SOSPFNextHop * pNH);

// ----- ospf_nh_list_copy --------------------------------------------------
extern next_hops_list_t * ospf_nh_list_copy(next_hops_list_t * pNHList);
// ----- ospf_nh_list_add_list ------------------------------------------------
extern void ospf_nh_list_add_list(next_hops_list_t * pNHListDest, next_hops_list_t * pNHListSource);

// ----- ospf_nh_list_dump --------------------------------------------------
/*
   pcSpace should not be NULL! 
   USAGE pcSapace = "" or 
         pcSpace = "\t"
*/
extern void ospf_nh_list_dump(FILE * pStream, next_hops_list_t * pNHList, 
		       int iPathType, char * pcSpace);

///////////////////////////////////////////////////////////////////////////
//////  ROUTING TABLE OSPF FUNCTION
///////////////////////////////////////////////////////////////////////////

// ----- rt_perror -------------------------------------------------------
extern void OSPF_rt_perror(FILE * pStream, int iErrorCode);
// ----- route_info_create ------------------------------------------
extern SOSPFRouteInfo * OSPF_route_info_create(ospf_dest_type_t  tOSPFDestinationType,
                                       ip_pfx_t prefix,
				       uint32_t           uWeight,
				       ospf_area_t        tOSPFArea,
				       ospf_path_type_t   tOSPFPathType,
				       next_hops_list_t * pNHList);
				       
// ----- OSPF_route_info_add_nextHop -----------------------------------------
int OSPF_route_info_add_nextHop(SOSPFRouteInfo * pRouteInfo, SOSPFNextHop * pNH);
// ----- OSPF_route_info_dump ------------------------------------------------
extern void OSPF_route_info_dump(FILE * pStream, SOSPFRouteInfo * pRouteInfo);
// ----- OSPF_rt_find_exact --------------------------------------------------
//extern SOSPFRouteInfo * OSPF_rt_find_exact(SOSPFRT * pRT, ip_pfx_t prefix,
//			      net_route_type_t tType, ospf_area_t tArea);
// ----- OSPF_rt_dump -------------------------------------------------------------
extern int OSPF_rt_dump(FILE * pStream, SOSPFRT * pRT, int iOption, ospf_path_type_t tPathType, 
                 ospf_area_t tArea, int * piPrintedRoutes);

// ----- route_info_destroy --------------------------------------------------
extern void OSPF_route_info_destroy(SOSPFRouteInfo ** ppOSPFRouteInfo);


// ----- rt_create --------------------------------------------------
extern SOSPFRT * OSPF_rt_create();
// ----- rt_destroy -------------------------------------------------
extern void OSPF_rt_destroy(SOSPFRT ** ppRT);
// ----- rt_find_best -----------------------------------------------
extern SOSPFRouteInfo * OSPF_rt_find_best(SOSPFRT * pRT, net_addr_t tAddr,
				    net_route_type_t tType);
// ----- rt_find_exact ----------------------------------------------
extern SOSPFRouteInfo * OSPF_rt_find_exact(SOSPFRT * pRT, ip_pfx_t prefix,
				     net_route_type_t tType, ospf_area_t tArea);
// ----- rt_add_route -----------------------------------------------
extern int OSPF_rt_add_route(SOSPFRT * pRT, ip_pfx_t prefix,
			SOSPFRouteInfo * pRouteInfo);
// ----- rt_del_route -----------------------------------------------
extern int OSPF_rt_del_route(SOSPFRT * pRT, ip_pfx_t * prefix, SOSPFNextHop * pNextHop,
		 net_route_type_t tType);


// ----- rt_for_each -------------------------------------------------------
extern int OSPF_rt_for_each(SNetRT * pRT, FRadixTreeForEach fForEach, void * pContext);

#endif
#endif
