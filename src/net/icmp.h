// ==================================================================
// @(#)icmp.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 25/02/2004
// $Id: icmp.h,v 1.7 2009-03-24 16:09:09 bqu Exp $
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
 * Provide data structures and functions to send and receive ICMP
 * messages. This implementation does not at all stick to the ICMP
 * specifications. However, much of the functionnality is here.
 *
 * The main use of these functions is to perform end-to-end
 * reachability tests and to trace IP paths.
 */

#ifndef __NET_ICMP_H__
#define __NET_ICMP_H__

#include <libgds/array.h>
#include <libgds/enumerator.h>

#include <net/ip_trace.h>
#include <net/prefix.h>
#include <net/message.h>
#include <net/network.h>
#include <net/net_path.h>
#include <net/icmp_options.h>


// -----[ icmp_error_type_t ]----------------------------------------
/** Definition of the possible ICMP error codes. */
typedef enum {
  ICMP_ERROR_NET_UNREACH  = 1,
  ICMP_ERROR_HOST_UNREACH = 2,
  ICMP_ERROR_PROTO_UNREACH= 3,
  ICMP_ERROR_TIME_EXCEEDED= 4,
} icmp_error_type_t;


// -----[ icmp_msg_type_t ]------------------------------------------
/** Definition of the possible ICMP message types. */
typedef enum {
  ICMP_ECHO_REQUEST= 0,
  ICMP_ECHO_REPLY  = 1,
  ICMP_ERROR       = 2,
  ICMP_TRACE       = 3, /* This is a non-standard type used to perform
			 * "record-route" actions */
} icmp_msg_type_t;


// -----[ icmp_msg_t ]-----------------------------------------------
/** Definition of an ICMP message. */
typedef struct {
  icmp_msg_type_t     type;     /* ICMP type */
  icmp_error_type_t   sub_type; /* ICMP subtype (error) */
  net_node_t        * node;     /* Origin node */
  ip_trace_t        * trace;
} icmp_msg_t;


extern const net_protocol_def_t PROTOCOL_ICMP;
 

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ icmp_send_error ]----------------------------------------
  int icmp_send_error(net_node_t * node, net_addr_t src_addr,
		      net_addr_t dst_addr, icmp_error_type_t error,
		      simulator_t * sim);
  // -----[ icmp_send_echo_request ]---------------------------------
  int icmp_send_echo_request(net_node_t * node, net_addr_t src_addr,
			     net_addr_t dst_addr, uint8_t ttl,
			     simulator_t * sim);
  // -----[ is_icmp_error ]------------------------------------------
  int is_icmp_error(net_msg_t * msg);
  // -----[ icmp_ping_send_recv ]------------------------------------
  int icmp_ping_send_recv(net_node_t * node, net_addr_t src_addr,
			  net_addr_t dst_addr, uint8_t ttl);
  // -----[ icmp_trace_send ]----------------------------------------
  array_t * icmp_trace_send(net_node_t * node, net_addr_t dst_addr,
			    uint8_t max_ttl, ip_opt_t * opts);
  
  
  ///////////////////////////////////////////////////////////////////
  // PROBES
  ///////////////////////////////////////////////////////////////////

  // -----[ icmp_ping ]----------------------------------------------
  /**
   * Perform a "ping" test from a node to a destination address.
   *
   * This function works by sending an "ICMP echo request" within
   * an ad-hoc simulator. It then checks if an answer ("ICMP echo
   * response") has been received from the probed destination.
   * This has no impact on the main simulator's queue.
   *
   * \param stream   is the output stream.
   * \param node     is the source node.
   * \param src_addr is the source address for sending probes.
   * \param dst_adr  is the destination IP address.
   * \param ttl      is the ICMP echo request's TTL.
   * \retval an error code.
   *
   * \attention
   * This function is intended to be used within the user-interface of
   * the simulator, i.e. it will return results on the console (output
   * stream). However, the backend of this function can also be
   * accessed through the API.
   */
  int icmp_ping(gds_stream_t * stream,
		net_node_t * node, net_addr_t src_addr,
		net_addr_t dst_addr, uint8_t ttl);

  // -----[ icmp_trace_route ]---------------------------------------
  /**
   * Trace the route from a node to a destination address.
   *
   * This function works in the same way as the "real-world"
   * traceroute utility, i.e. by sending successive probes
   * (ICMP echo request) with an increasing TTL value. Probes are
   * sent in the same way as with the \c icmp_ping function.
   *
   * \param stream   is the output stream.
   * \param node     is the source node.
   * \param src_addr is the source address for sending probes. The
   *   source address can be left unspecified with \c IP_ADDR_ANY.
   *   If a source address is specified, it must be assigned to one
   *   of the node's network interfaces.
   * \param dst_addr is the destination IP address.
   * \param trace_ref is a pointer to the returned IP trace.
   * \retval an error code.
   *
   * \attention
   * This function is intended to be used within the user-interface of
   * the simulator, i.e. it will return results on the console (output
   * stream). However, the backend of this function can also be
   * accessed through the API.
   */
  int icmp_trace_route(gds_stream_t * stream,
		       net_node_t * node, net_addr_t src_addr,
		       net_addr_t dst_addr, uint8_t max_ttl,
		       ip_trace_t ** trace_ref);

  // -----[ icmp_record_route ]--------------------------------------
  /**
   * Trace the route from a node to a destination.
   *
   * This function has additional features compared to the
   * \c icmp_trace_route function:
   * \li The destination can be an IP address or an IP prefix.
   * \li The traversed path can be loaded with a given amount of "traffic".
   * \li All the equal-cost paths can be traced.
   * \li Tunnel paths can be traced (outer encapsulation).
   *
   * \param stream   is the output stream.
   * \param node     is the source node.
   * \param src_addr is the source address for sending probes. The
   *   source address can be left unspecified with \c IP_ADDR_ANY.
   *   If a source address is specified, it must be assigned to one
   *   of the node's network interfaces.
   * \param dest     is a destination (IP address or IP prefix).
   * \param ttl      is the initial TTL value (0 < ttl <= 255)
   * \param tos      is a type of service. It identifies which
   *   topology must be used (if available).
   * \param opts     is a set of options.
   * \retval is an error code.
   *
   * \attention
   * This function is intended to be used within the user-interface of
   * the simulator, i.e. it will return results on the console (output
   * stream). However, the backend of this function can also be
   * accessed through the API.
   */
  int icmp_record_route(gds_stream_t * stream,
			net_node_t * node, net_addr_t src_addr,
			ip_dest_t dest, uint8_t ttl,
			net_tos_t tos, ip_opt_t * opts);
  
#ifdef __cplusplus
}
#endif

#endif /* __NET_ICMP_H__ */
