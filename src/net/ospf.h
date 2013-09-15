// ==================================================================
// @(#)ospf.h
//
// @author Stefano Iasi (stefanoia@tin.it)
// @date 14/06/2005
// $Id: ospf.h,v 1.9 2009-03-24 16:21:28 bqu Exp $
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

#ifndef __NET_OSPF_H__
#define __NET_OSPF_H__

#ifdef OSPF_SUPPORT

#include <net/net_types.h>
#include <net/subnet.h>
#include <net/link.h>

#define OSPF_NO_AREA	      0xffffffff 
#define BACKBONE_AREA 0

//First is only a warning
#define OSPF_SUCCESS                        0
#define OSPF_LINK_OK                        0
#define OSPF_LINK_TO_MYSELF_NOT_IN_AREA    -1 
#define OSPF_SOURCE_NODE_NOT_IN_AREA       -2
#define OSPF_DEST_NODE_NOT_IN_AREA         -3
#define OSPF_DEST_SUBNET_NOT_IN_AREA       -4
#define OSPF_SOURCE_NODE_LINK_MISSING      -5

#define OSPF_BORDER_ROUTER 0x01
#define OSPF_INTRA_ROUTER  0x02

/*
   Assumption: pSubNet is valid only for destination type Subnet.
   The filed can be used only if pPrefix has a 32 bit netmask.
*/
typedef struct {
  net_iface_t * pLink;  //link towards next-hop
  net_addr_t tAddr;  //ip address of next hop
  net_addr_t tAdvRouterAddr; //Router that advertise 
                         //destination with LSA (for INTER-AREA only) 
} SOSPFNextHop;

// ----- ospf test function --------------------------------------------
extern int ospf_test();

///////////////////////////////////////////////////////////////////////////
//////  NODE OSPF FUNCTION
///////////////////////////////////////////////////////////////////////////

// ----- node_add_OSPFArea --------------------------------------------
extern int node_add_OSPFArea(net_node_t * pNode, uint32_t OSPFArea);
// ----- node_is_BorderRouter --------------------------------------------
extern int node_is_BorderRouter(net_node_t * pNode);
// ----- node_is_InternalRouter --------------------------------------------
extern int node_is_InternalRouter(net_node_t * pNode);
// ----- node_ospf_rt_add_route --------------------------------------------
extern int node_ospf_rt_add_route(net_node_t     * pNode,     ospf_dest_type_t  tOSPFDestinationType,
                       ip_pfx_t prefix,   net_link_delay_t        uWeight,
		       ospf_area_t    tOSPFArea, ospf_path_type_t  tOSPFPathType,
		       next_hops_list_t * pNHList);
// ----- node_belongs_to_area -----------------------------------------------
extern int node_belongs_to_area(net_node_t * pNode, uint32_t tArea);
// ----- link_set_ospf_area ------------------------------------------
extern int link_set_ospf_area(net_iface_t * pLink, ospf_area_t tArea);

// ----- ospf_node_get_id ------------------------------------------
extern net_addr_t ospf_node_get_id(net_node_t * pNode);

///////////////////////////////////////////////////////////////////////////
//////  SUBNET OSPF FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- subnet_OSPFArea -------------------------------------------------
extern int subnet_set_OSPFArea(net_subnet_t * pSubnet, uint32_t uOSPFArea);
// ----- subnet_getOSPFArea ----------------------------------------------
extern uint32_t subnet_get_OSPFArea(net_subnet_t * pSubnet);
// ----- subnet_belongs_to_area ---------------------------------------------
int subnet_belongs_to_area(net_subnet_t * pSubnet, uint32_t tArea);

///////////////////////////////////////////////////////////////////////////
//////  OSPF ROUTING TABLE FUNCTION
///////////////////////////////////////////////////////////////////////////
// ----- ospf_node_rt_dump ------------------------------------------------------------------
extern void ospf_node_rt_dump(FILE * pStream, net_node_t * pNode, int iOption);
// ----- ospf_route_is_adver_in_area -------------------------------------------
int ospf_route_is_adver_in_area(SOSPFRouteInfo * pRI, ospf_area_t tArea);
//////////////////////////////////////////////////////////////////////////
//////  OSPF DOMAIN TABLE FUNCTION
///////////////////////////////////////////////////////////////////////////
//----- ospf_domain_build_route ---------------------------------------------------------------
extern int ospf_domain_build_route(uint16_t uOSPFDomain);

//----- ospf_print_error --------------------------------------------------------------------
extern void ospf_print_error(FILE* pStream, int iError);
//----- ospf_domain_get_border_routers -----------------------------------------------------------
SPtrArray * ospf_domain_get_border_routers(uint16_t uOSPFDomain);
//----- ospf_domain_get_br_on_bb_in_area ---------------------------------------------
SPtrArray * ospf_domain_get_br_on_bb_in_area(uint16_t uOSPFDomain, ospf_area_t tArea);
#endif
#endif
