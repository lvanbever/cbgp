// ==================================================================
// @(#)ip6.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/01/2008
// $Id: ip6.h,v 1.3 2009-03-24 16:13:18 bqu Exp $
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

#ifndef __NET_IP6_H__
#define __NET_IP6_H__

// ----- IPv6 address -----
typedef uint8_t ip6_addr_t[8];
typedef uint8_t ip6_mask_t;
#define IP6_ADDR_MAX (net_addr6_t) {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

// ----- IPv6 prefix -----
typedef struct {
  ip6_addr_t network;
  ip6_mask_t mask_len;
} ip6_pfx_t;

// ----- IPv6 destination -----
typedef struct {
  uint8_t type;
  union {
    ip6_pfx_t prefix;
    ip6_addr_t addr;  
    };
} ip6_dest_t;

#endif /* __NET_IP6_H__ */
