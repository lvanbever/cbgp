// ==================================================================
// @(#)error.c
//
// List of error codes.
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 30/05/2007
// $Id: error.c,v 1.7 2009-03-24 16:06:30 bqu Exp $
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
# include <config.h>
#endif

#include <stdarg.h>
#include <stdlib.h>

#include <net/error.h>

// ----- network_perror ---------------------------------------------
/**
 * Dump an error message for the given error code.
 */
void network_perror(gds_stream_t * stream, int error)
{
  const char * error_msg= network_strerror(error);
  if (error_msg != NULL)
    stream_printf(stream, error_msg);
  else
    stream_printf(stream, "unknown error (%i)", error);    
}

// ----- network_strerror -------------------------------------------
const char * network_strerror(int error)
{
  switch (error) {
  case ESUCCESS:
    return "success";
  case EUNEXPECTED:
    return "unexpected error";
  case EUNSUPPORTED:
    return "not implemented";
  case EUNSPECIFIED:
    return "unspecified error";

  case ENET_NET_UNREACH:
    return "network unreachable";
  case ENET_HOST_UNREACH:
    return "host unreachable";
  case ENET_PROTO_UNREACH:
    return "protocol unreachable";
  case ENET_TIME_EXCEEDED:
    return "time exceeded";
  case ENET_ICMP_NET_UNREACH:
    return "icmp error (network-unreachable)";
  case ENET_ICMP_HOST_UNREACH:
    return "icmp error (host-unreachable)";
  case ENET_ICMP_PROTO_UNREACH:
    return "icmp error (proto-unreachable)";
  case ENET_ICMP_TIME_EXCEEDED:
    return "icmp error (time-exceeded)";
  case ENET_NO_REPLY:
    return "no reply";
  case ENET_FWD_LOOP:
    return "forwarding loop";
  case ENET_LINK_DOWN:
    return "link down";
  case ENET_PROTOCOL_ERROR:
    return "protocol error";
  case ENET_NODE_DUPLICATE:
    return "node already exists";
  case ENET_SUBNET_DUPLICATE:
    return "subnet already exists";
  case ENET_LINK_DUPLICATE:
    return "link already exists";
  case ENET_LINK_LOOP:
    return "link endpoints are equal";
  case ENET_IGP_DOMAIN_DUPLICATE:
    return "igp domain already exists";
  case ENET_PROTO_UNKNOWN:
    return "invalid protocol ID";
  case ENET_PROTO_DUPLICATE:
    return "protocol already registered";
  case ENET_IFACE_UNKNOWN:
    return "unknown interface";
  case ENET_IFACE_DUPLICATE:
    return "interface already exists";
  case ENET_IFACE_INVALID_ID:
    return "invalid interface ID";
  case ENET_IFACE_INVALID_TYPE:
    return "invalid interface type";
  case ENET_IFACE_NOT_SUPPORTED:
    return "operation not supported by interface";
  case ENET_IFACE_INCOMPATIBLE:
    return "incompatible interface";
  case ENET_IFACE_CONNECTED:
    return "interface is connected";
  case ENET_IFACE_NOT_CONNECTED:
    return "interface is not connected";
  case EBGP_PEER_DUPLICATE:
    return "peer already exists";
  case EBGP_PEER_INVALID_ADDR:
    return "cannot add peer with this address";
  case EBGP_PEER_UNKNOWN:
    return "peer does not exist";
  case EBGP_PEER_INCOMPATIBLE:
    return "peer not compatible with operation";
  case EBGP_PEER_UNREACHABLE:
    return "peer could not be reached";
  case EBGP_PEER_INVALID_STATE:
    return "peer state incompatible with operation";
  case EBGP_PEER_OUT_OF_SEQ:
    return "BGP message received out-of-sequence";
  case ENET_RT_NH_UNREACH:
    return "next-hop is unreachable";
  case ENET_RT_IFACE_UNKNOWN:
    return "interface is unknown";
  case ENET_RT_DUPLICATE:
    return "route already exists";
  case ENET_RT_UNKNOWN:
    return "route does not exist";
  case ENET_RT_NO_GW_NO_OIF:
    return "route must have oif or gw";
  case EBGP_NETWORK_DUPLICATE:
    return "network already exists";
  case ENET_NODE_INVALID_ID:
    return "invalid identifier";
  }
  return NULL;
}

// -----[ cbgp_fatal ]-----------------------------------------------
void cbgp_fatal(const char * msg, ...)
{
  va_list ap;

  va_start(ap, msg);
  fprintf(stderr, "CBGP FATAL ERROR: ");
  vfprintf(stderr, msg, ap);
  va_end(ap);
  abort();
}

// -----[ cbgp_warn ]------------------------------------------------
void cbgp_warn(const char * msg, ...)
{
  va_list ap;

  va_start(ap, msg);
  fprintf(stderr, "CBGP WARNING: ");
  vfprintf(stderr, msg, ap);
  va_end(ap);
}
