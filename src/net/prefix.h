// ==================================================================
// @(#)prefix.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/12/2002
// $Id: prefix.h,v 1.19 2009-03-24 16:22:30 bqu Exp $
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

#ifndef __IP_PREFIX_H__
#define __IP_PREFIX_H__

#include <libgds/stream.h>
#include <libgds/types.h>

#include <stdio.h>

#include <net/ip.h>
#include <net/ip6.h>

typedef uint8_t net_afi_t;
#define NET_AFI_IP4 0
#define NET_AFI_IP6 1

#ifdef __cplusplus
extern "C" {
#endif

  // ----- ip_address_to_string ----------------------------------------
  int ip_address_to_string(net_addr_t addr, char * str, size_t str_size);
  // ----- create_ip_prefix ----------------------------------------
  ip_pfx_t * create_ip_prefix(net_addr_t addr, net_mask_t mask_len);
  // ----- ip_address_dump --------------------------------------------
  void ip_address_dump(gds_stream_t * stream, net_addr_t addr);
  // ----- ip_string_to_address ---------------------------------------
  int ip_string_to_address(const char * str, char ** end_ptr,
			   net_addr_t * addr_ref);
  // ----- uint32_to_prefix -------------------------------------------
  ip_pfx_t uint32_to_prefix(net_addr_t addr, net_mask_t mask_len);
  // ----- ip_prefix_dump ---------------------------------------------
  void ip_prefix_dump(gds_stream_t * stream, ip_pfx_t prefix);
  // ----- ip_prefix_to_string ----------------------------------------
  int ip_prefix_to_string(const ip_pfx_t * prefix, char * str,
			  size_t str_size);
  // ----- ip_string_to_prefix ----------------------------------------
  int ip_string_to_prefix(const char * str, char ** end_ptr,
			  ip_pfx_t * prefix_ref);
  // ----- ip_string_to_dest ------------------------------------------
  int ip_string_to_dest(const char * prefix, ip_dest_t * dest);
  // ----- ip_dest_dump -----------------------------------------------
  void ip_dest_dump(gds_stream_t * stream, ip_dest_t dest);
  // -----[ ip_prefix_cmp ]--------------------------------------------
  int ip_prefix_cmp(ip_pfx_t * prefix1, ip_pfx_t * prefix2);
  // ----- ip_address_in_prefix ---------------------------------------
  int ip_address_in_prefix(net_addr_t addr, ip_pfx_t prefix);
  // ----- ip_prefix_in_prefix ----------------------------------------
  int ip_prefix_in_prefix(ip_pfx_t prefix1, ip_pfx_t prefix2);
  // ----- ip_prefix_ge_prefix ----------------------------------------
  int ip_prefix_ge_prefix(ip_pfx_t prefix1, ip_pfx_t prefix2,
			  uint8_t mask_len);
  // ----- ip_prefix_le_prefix ----------------------------------------
  int ip_prefix_le_prefix(ip_pfx_t prefix1, ip_pfx_t prefix2,
			  uint8_t mask_len);
  // ----- ip_prefix_copy ---------------------------------------------
  ip_pfx_t * ip_prefix_copy(ip_pfx_t * prefix);
  // ----- ip_prefix_destroy ------------------------------------------
  void ip_prefix_destroy(ip_pfx_t ** prefix_ref);
  // ----- ip_build_mask ----------------------------------------------
  net_addr_t ip_build_mask(uint8_t mask_len);
  // ----- ip_prefix_mask ---------------------------------------------
  void ip_prefix_mask(ip_pfx_t * prefix);
  // ----- ip_prefix_to_dest ------------------------------------------
  ip_dest_t ip_prefix_to_dest(ip_pfx_t prefix);
  // ----- ip_address_to_dest -----------------------------------------
  ip_dest_t ip_address_to_dest(net_addr_t addr);

#ifdef __cplusplus
}
#endif

#endif /* __IP_PREFIX_H__ */
