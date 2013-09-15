// ==================================================================
// @(#)ip.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 01/12/2002
// $Id: ip.h,v 1.6 2009-03-24 16:13:18 bqu Exp $
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

/**
 * \file
 * Provide data structures to represent IPv4 addresses and prefixes.
 */

#ifndef __NET_IP_H__
#define __NET_IP_H__

#include <stdlib.h>

/** Build an IPv4 address from its 4 bytes form. */
#define IPV4(A,B,C,D)				\
  (((((uint32_t)(A))*256 +			\
     (uint32_t)(B))*256 +			\
    (uint32_t)( C))*256 +			\
   (uint32_t)(D))

/** Build an IPv4 prefix from its 5 bytes form. */
#define IPV4PFX(A,B,C,D,L) net_prefix(IPV4(A,B,C,D),L)

/** Definition of a "don't care" IPv4 address value. */
#define IP_ADDR_ANY 0
#define NET_ADDR_ANY IP_ADDR_ANY

/** Definition of the maximum IPv4 address value. */
#define MAX_ADDR UINT32_MAX


// -----[ net_addr_t ]-----------------------------------------------
/** Definition of an IPv4 address. */
typedef uint32_t net_addr_t;


// -----[ net_mask_t ]-----------------------------------------------
/** Definition of a network mask length. */
typedef uint8_t  net_mask_t;


// -----[ ip_pfx_t ]-------------------------------------------------
/** Definition of an IPv4 network prefix. */
typedef struct {
  net_addr_t network;
  net_mask_t mask;
} ip_pfx_t;


// -----[ ip_dest_type_t ]-------------------------------------------
/** Possible IPv4 destination. */
typedef enum {
  /** Invalid destination. */
  NET_DEST_INVALID,
  /** IP address. */
  NET_DEST_ADDRESS,
  /** IP prefix. */
  NET_DEST_PREFIX,
  /** Any destination (*). */
  NET_DEST_ANY,
} ip_dest_type_t;


// -----[ ip_dest_t ]------------------------------------------------
/** Definition of an IPv4 destination. */
typedef struct {
  /** Destination type. */
  ip_dest_type_t type;
  union {
    /** Destination as an IP prefix. */
    ip_pfx_t   prefix;
    /** Destination as an IP address. */
    net_addr_t addr;
  };
} ip_dest_t;


// -----[ net_prefix ]-----------------------------------------------
static inline ip_pfx_t net_prefix(net_addr_t network, net_mask_t mask)
{
  ip_pfx_t pfx= {
    .network= network,
    .mask= mask
  };
  return pfx;
}

// -----[ ip_dest_addr ]---------------------------------------------
static inline ip_dest_t net_dest_addr(net_addr_t addr)
{
  ip_dest_t dest;
  dest.type= NET_DEST_ADDRESS;
  dest.addr= addr;
  return dest;
}

// -----[ net_dest_prefix ]------------------------------------------
static inline ip_dest_t net_dest_prefix(net_addr_t addr, uint8_t mask)
{
  ip_dest_t dest;
  dest.type= NET_DEST_PREFIX;
  dest.prefix.network= addr;
  dest.prefix.mask= mask;
  return dest;
}

// -----[ net_dest_to_prefix ]---------------------------------------
static inline ip_pfx_t net_dest_to_prefix(ip_dest_t dest)
{
  ip_pfx_t pfx;
  switch (dest.type) {
  case NET_DEST_ADDRESS:
    pfx.network= dest.addr;
    pfx.mask= 32;
    break;
  case NET_DEST_PREFIX:
    pfx= dest.prefix;
    break;
  default:
    abort();
  }
  return pfx;
}

#endif /** __NET_IP_H__ */
