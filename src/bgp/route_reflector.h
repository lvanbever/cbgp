// ==================================================================
// @(#)route_reflector.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/12/2003
// $Id: route_reflector.h,v 1.6 2009-03-24 15:52:37 bqu Exp $
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

#ifndef __BGP_ROUTE_REFLECTOR_H__
#define __BGP_ROUTE_REFLECTOR_H__

#include <libgds/array.h>
#include <libgds/stream.h>
#include <libgds/types.h>

#include <stdio.h>

#include <net/prefix.h>

// A CLUSTER_ID is a 4-bytes value (RFC1966)
typedef uint32_t bgp_cluster_id_t;

// A CLUSTER_ID_LIST is the list of clusters traversed by a route (RFC1966)
typedef uint32_array_t bgp_cluster_list_t;

typedef net_addr_t bgp_originator_t;

#ifdef __cplusplus
extern "C" {
#endif

  // ---- cluster_list_create ---------------------------------------
#define cluster_list_create()				\
  uint32_array_create(0)
  // ---- cluster_list_append ---------------------------------------
  inline int cluster_list_append(bgp_cluster_list_t * cl,
				 bgp_cluster_id_t cluster_id);
  // ----- cluster_list_copy ------------------------------------------
#define cluster_list_copy(PL)				\
  uint32_array_copy(PL)
  // ----- cluster_list_length ----------------------------------------
#define cluster_list_length(PL)			\
  uint32_array_size(PL)
  
  // ----- cluster_list_destroy -------------------------------------
  void cluster_list_destroy(bgp_cluster_list_t ** cl_ref);
  // ----- cluster_list_dump ----------------------------------------
  void cluster_list_dump(gds_stream_t * stream, bgp_cluster_list_t * cl);
  // ----- cluster_list_equals --------------------------------------
  int cluster_list_equals(bgp_cluster_list_t * cl1,
			  bgp_cluster_list_t * cl2);
  // ----- cluster_list_contains ------------------------------------
  int cluster_list_contains(bgp_cluster_list_t * cl,
			    bgp_cluster_id_t cluster_id);

  // -----[ originator_equals ]--------------------------------------
  int originator_equals(bgp_originator_t * orig1,
			bgp_originator_t * orig2);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_ROUTE_REFLECTOR_H__ */
