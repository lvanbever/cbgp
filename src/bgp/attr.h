// ==================================================================
// @(#)attr.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/11/2005
// $Id: attr.h,v 1.6 2009-03-24 14:25:33 bqu Exp $
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

#ifndef __BGP_ATTR_H__
#define __BGP_ATTR_H__

#include <bgp/attr/comm.h>
#include <bgp/attr/ecomm.h>
#include <bgp/attr/path.h>
#include <bgp/types.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ bgp_attr_create ]----------------------------------------
  bgp_attr_t * bgp_attr_create(net_addr_t next_hop,
			     bgp_origin_t origin,
			     uint32_t local_pref,
			     uint32_t med);
  // -----[ bgp_attr_destroy ]---------------------------------------
  void bgp_attr_destroy(bgp_attr_t ** attr_ref);
  // -----[ bgp_attr_set_nexthop ]-----------------------------------
  void bgp_attr_set_nexthop(bgp_attr_t ** attr_ref, net_addr_t next_hop);
  // -----[ bgp_attr_set_origin ]------------------------------------
  void bgp_attr_set_origin(bgp_attr_t ** attr_ref, bgp_origin_t origin);
  // -----[ bgp_attr_set_path ]--------------------------------------
  void bgp_attr_set_path(bgp_attr_t ** attr_ref, bgp_path_t * path);
  // -----[ bgp_attr_path_prepend ]----------------------------------
  int bgp_attr_path_prepend(bgp_attr_t ** attr_ref, asn_t asn,
			    uint8_t amount);
  // -----[ bgp_attr_path_rem_private ]------------------------------
  int bgp_attr_path_rem_private(bgp_attr_t ** attr_ref);
  // -----[ bgp_attr_set_comm ]--------------------------------------
  void bgp_attr_set_comm(bgp_attr_t ** attr_ref, bgp_comms_t * comms);
  // -----[ bgp_attr_comm_append ]-----------------------------------
  int bgp_attr_comm_append(bgp_attr_t ** attr_ref, bgp_comm_t comm);
  // -----[ bgp_attr_comm_remove ]-----------------------------------
  void bgp_attr_comm_remove(bgp_attr_t ** attr_ref, bgp_comm_t comm);
  // -----[ bgp_attr_comm_strip ]------------------------------------
  void bgp_attr_comm_strip(bgp_attr_t ** attr_ref);
  // -----[ bgp_attr_ecomm_append ]----------------------------------
  int bgp_attr_ecomm_append(bgp_attr_t ** attr_ref, bgp_ecomm_t * ecomm);
  // -----[ bgp_attr_originator_destroy ]----------------------------
  void bgp_attr_originator_destroy(bgp_attr_t * attr);
  // -----[ bgp_attr_cluster_list_destroy ]--------------------------
  void bgp_attr_cluster_list_destroy(bgp_attr_t * attr);
  
  // -----[ bgp_attr_cmp ]-------------------------------------------
  int bgp_attr_cmp(bgp_attr_t * attr1, bgp_attr_t * attr2);
  // -----[ bgp_attr_copy ]------------------------------------------
  bgp_attr_t * bgp_attr_copy(bgp_attr_t * attr);

#ifdef __cplusplus
}
#endif
  
#endif /* __BGP_ATTR_H__ */
