// ==================================================================
// @(#)nlri.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 12/09/2008
// $Id$
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

#ifndef __BGP_NLRI_H__
#define __BGP_NLRI_H__

#define NLRI_IPV4        0x01
#define NLRI_IPV4_WALTON 0x02

#include <net/error.h>
#include <net/ip.h>

typedef uint8_t bgp_nlri_type_t;

typedef struct bgp_nlri_t {
  bgp_nlri_type_t type;
} bgp_nlri_t;

typedef struct bgp_nlri_ipv4_t {
  bgp_nlri_type_t type;
  ip_pfx_t        prefix;
} bgp_nlri_ipv4_t;

typedef struct bgp_nlri_ipv4_walton_t {
  bgp_nlri_type_t type;
  ip_pfx_t        prefix;
  net_addr_t      nexthop;
} bgp_nlri_ipv4_walton_t;

// -----[ bgp_nlri_size ]--------------------------------------------
static inline size_t bgp_nlri_size(bgp_nlri_t * nlri) {
  switch (nlri->type) {
  case NLRI_IPV4:
    return sizeof(bgp_nlri_ipv4_t);
  case NLRI_IPV4_WALTON:
    return sizeof(bgp_nlri_ipv4_t);
  default:
    cbgp_fatal("Unsupported NLRI type (%u)", nlri->type);
  }
  return 0;
}

#endif /* __BGP_NLRI_H__ */
