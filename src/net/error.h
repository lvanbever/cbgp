// ==================================================================
// @(#)error.h
//
// List of error codes.
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 30/05/2007
// $Id: error.h,v 1.7 2009-03-24 16:06:30 bqu Exp $
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

#ifndef __NET_ERROR_H__
#define __NET_ERROR_H__

#include <libgds/stream.h>

// -----[ Error codes ]----------------------------------------------
typedef enum {
  ESUCCESS                = 0,
  EUNEXPECTED             = -1,   /* Unexpected error (generic) */
  EUNSUPPORTED            = -2,   /* Unsupported feature (generic) */
  EUNSPECIFIED            = -3,   /* Unspecified error (generic) */

  ENET_NET_UNREACH        = -100, /* Network unreachable */
  ENET_HOST_UNREACH       = -101, /* Host unreachable */
  ENET_PROTO_UNREACH      = -102, /* Protocol unreachable */
  ENET_TIME_EXCEEDED      = -103, /* Time exceeded (TTL = 0) */
  ENET_LINK_DOWN          = -104, /* Link is down (disabled) */
  ENET_LINK_LOOP          = -105, /* Cannot create a link loop */
  ENET_LINK_DUPLICATE     = -106,

  ENET_NODE_DUPLICATE     = -200,
  ENET_SUBNET_DUPLICATE   = -201,
  ENET_PROTO_UNKNOWN      = -202,
  ENET_PROTO_DUPLICATE    = -203,
  ENET_IGP_DOMAIN_UNKNOWN = -204,
  ENET_IGP_DOMAIN_DUPLICATE = -205,

  ENET_IFACE_UNKNOWN      = -300,
  ENET_IFACE_DUPLICATE    = -301,
  ENET_IFACE_INVALID_ID   = -302,
  ENET_IFACE_INVALID_TYPE = -303,
  ENET_IFACE_NOT_SUPPORTED= -304,
  ENET_IFACE_INCOMPATIBLE = -305,
  ENET_IFACE_CONNECTED    = -306,
  ENET_IFACE_NOT_CONNECTED= -307,

  EBGP_PEER_DUPLICATE     = -400, /* Peer already exists */
  EBGP_PEER_INVALID_ADDR  = -401, /* Cannot add peer with this address */
  EBGP_PEER_UNKNOWN       = -402, /* Peer does not exist */
  EBGP_PEER_INCOMPATIBLE  = -403, /* Peer not compatible with operation */
  EBGP_PEER_UNREACHABLE   = -404, /* Peer could not be reached */
  EBGP_PEER_INVALID_STATE = -405, /* Peer state incompatible with operation */
  EBGP_PEER_OUT_OF_SEQ    = -406, /* Message received out-of-sequence */

  EBGP_NETWORK_DUPLICATE  = -420, /* Network already originated from router */

  ENET_RT_NH_UNREACH      = -500,
  ENET_RT_IFACE_UNKNOWN   = -501,
  ENET_RT_DUPLICATE       = -502,
  ENET_RT_UNKNOWN         = -503,
  ENET_RT_NO_GW_NO_OIF    = -504,

  ENET_ICMP_NET_UNREACH   = -601, /* ICMP net-unreach received */
  ENET_ICMP_HOST_UNREACH  = -602, /* ICMP host-unreach received */
  ENET_ICMP_PROTO_UNREACH = -603, /* ICMP proto-unreach received */
  ENET_ICMP_TIME_EXCEEDED = -604, /* ICMP time-exceeded received */
  ENET_PROTOCOL_ERROR     = -605,
  ENET_NO_REPLY           = -606,
  ENET_FWD_LOOP           = -607,
  ENET_PFX_REACHED        = -608,

  ENET_NODE_INVALID_ID    = -700,

  ESIM_TIME_LIMIT         = -1000,
} net_error_t;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- network_perror -------------------------------------------
  void network_perror(gds_stream_t * stream, int error);
  // ----- network_strerror -----------------------------------------
  const char * network_strerror(int error);
  // -----[ cbgp_fatal ]---------------------------------------------
  void cbgp_fatal(const char * msg, ...);
  // -----[ cbgp_warn ]----------------------------------------------
  void cbgp_warn(const char * msg, ...);

#ifdef __cplusplus
}
#endif

#endif /* __NET_ERROR_H__ */
