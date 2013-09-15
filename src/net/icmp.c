// ==================================================================
// @(#)icmp.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 25/02/2004
// $Id: icmp.c,v 1.15 2009-08-31 09:48:28 bqu Exp $
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
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>

#include <libgds/fifo.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/protocol.h>
#include <net/network.h>
#include <net/net_types.h>

//#define ICMP_DEBUG

static inline void __debug(const char * msg, ...) {
#ifdef ICMP_DEBUG
  va_list ap;

  va_start(ap, msg);
  stream_printf(gdsout, "ICMP_DBG::");
  stream_vprintf(gdsout, msg, ap);
  stream_printf(gdsout, "\n");
  stream_flush(gdsout);
  va_end(ap);
#endif
}

static struct {
  net_node_t * node;      // Node that received the message
  net_addr_t   src_addr;  // Source address of the message
  net_addr_t   dst_addr;  // Destination address of the message
  icmp_msg_t   msg;
  int          received;
} _rcvd_ctx= {
  .received= 0,
};

// -----[ icmp_perror ]----------------------------------------------
/**
 * Report comprehensive ICMP error description.
 */
void icmp_perror(gds_stream_t * stream, icmp_error_type_t error)
{
  switch (error) {
  case ICMP_ERROR_NET_UNREACH:
    stream_printf(stream, "network unreachable"); break;
  case ICMP_ERROR_HOST_UNREACH:
    stream_printf(stream, "host unreachable"); break;
  case ICMP_ERROR_PROTO_UNREACH:
    stream_printf(stream, "protocol unreachable"); break;
  case ICMP_ERROR_TIME_EXCEEDED:
    stream_printf(stream, "time exceeded"); break;
  default:
    stream_printf(stream, "unknown error code (%d)", error);
  }
}

// -----[ is_icmp_error ]--------------------------------------------
/**
 * Check if the message is an ICMP error. This is used in the node
 * forwarding function to avoid sending an ICMP error message when
 * we fail to forward/deliver another ICMP error message.
 */
int is_icmp_error(net_msg_t * msg)
{
  return ((msg->protocol == NET_PROTOCOL_ICMP) &&
	  (((icmp_msg_t *) msg->payload)->type == ICMP_ERROR));
}

// -----[ _icmp_msg_create ]-----------------------------------------
static inline icmp_msg_t * _icmp_msg_create(icmp_msg_type_t type,
					    icmp_error_type_t sub_type,
					    net_node_t * node)
{
  __debug("_icmp_msg_create");
  icmp_msg_t * icmp_msg= MALLOC(sizeof(icmp_msg_t));
  icmp_msg->type= type;
  icmp_msg->sub_type= sub_type;
  icmp_msg->node= node;
  icmp_msg->trace= NULL;
  return icmp_msg;
}

// -----[ _icmp_msg_destroy ]----------------------------------------
static inline void _icmp_msg_destroy(icmp_msg_t ** msg_ref)
{
  __debug("_icmp_msg_destroy");
  if (*msg_ref != NULL) {
    FREE(*msg_ref);
    *msg_ref= NULL;
  }
}

// -----[ icmp_send_error ]------------------------------------------
/**
 * Send an ICMP error message.
 */
int icmp_send_error(net_node_t * node,
		    net_addr_t src_addr,
		    net_addr_t dst_addr,
		    icmp_error_type_t error,
		    simulator_t * sim)
{
  return node_send_msg(node, src_addr, dst_addr, NET_PROTOCOL_ICMP,
		       255, _icmp_msg_create(ICMP_ERROR, error, node),
		       (FPayLoadDestroy) _icmp_msg_destroy,
		       NULL, sim);
}

// -----[ _icmp_send ]-----------------------------------------------
static inline int _icmp_send(net_node_t * node,
			     net_addr_t src_addr,
			     net_addr_t dst_addr,
			     uint8_t ttl,
			     icmp_msg_t * msg,
			     simulator_t * sim,
			     ip_opt_t * opts)
{
  __debug("_icmp_send");
  return node_send_msg(node, src_addr, dst_addr, NET_PROTOCOL_ICMP,
		       ttl, msg,
		       (FPayLoadDestroy) _icmp_msg_destroy, opts, sim);
}

// -----[ icmp_send_echo_request ]-----------------------------------
/**
 * Send an ICMP echo request to the given destination address.
 */
int icmp_send_echo_request(net_node_t * node,
			   net_addr_t src_addr,
			   net_addr_t dst_addr,
			   uint8_t ttl,
			   simulator_t * sim)
{
  return _icmp_send(node, src_addr, dst_addr, ttl,
		    _icmp_msg_create(ICMP_ECHO_REQUEST, 0, node),
		    sim, NULL);
}

// -----[ icmp_send_echo_reply ]-------------------------------------
/**
 * Send an ICMP echo reply to the given destination address. The
 * source address of the ICMP message is set to the given source
 * address. Depending on the implementation, the source address might
 * be the same as the destination address in the ICMP echo request
 * message instead of the address of the outgoing interface.
 */
int icmp_send_echo_reply(net_node_t * node,
			 net_addr_t src_addr,
			 net_addr_t dst_addr,
			 uint8_t ttl,
			 simulator_t * sim)
{
  return _icmp_send(node, src_addr, dst_addr, ttl,
		    _icmp_msg_create(ICMP_ECHO_REPLY, 0, node),
		    sim, NULL);
}

// -----[ _icmp_proto_copy_payload ]---------------------------------
static void * _icmp_proto_copy_payload(net_msg_t * msg)
{
  icmp_msg_t * icmp_msg= (icmp_msg_t *) msg->payload;
  return _icmp_msg_create(icmp_msg->type,
			  icmp_msg->sub_type,
			  icmp_msg->node);
}

// -----[ _icmp_proto_dump_msg ]--------------------------------------
static void _icmp_proto_dump_msg(gds_stream_t * stream, net_msg_t * msg)
{
  icmp_msg_t * icmp_msg= (icmp_msg_t *) msg->payload;

  switch (icmp_msg->type) {
  case ICMP_ECHO_REQUEST:
    stream_printf(stream, "echo-request"); break;
  case ICMP_ECHO_REPLY:
    stream_printf(stream, "echo-reply"); break;
  case ICMP_TRACE:
    stream_printf(stream, "trace"); break;
  case ICMP_ERROR:
    stream_printf(stream, "error(");
    icmp_perror(stream, icmp_msg->sub_type);
    stream_printf(stream, ")");
    break;
  default:
    stream_printf(stream, "???");
  }
}

// -----[ icmp_proto_handle ]----------------------------------------
/**
 * This function handles ICMP messages received by a node.
 */
static int _icmp_proto_handle(simulator_t * sim,
			      void * handler,
			      net_msg_t * msg)
{
  __debug("_icmp_proto_handle");
  net_node_t * node= (net_node_t *) handler;
  icmp_msg_t * icmp_msg= (icmp_msg_t *) msg->payload;

  // Record received ICMP messages. This is used by the icmp_ping()
  // and icmp_traceroute() functions to check for replies.
  _rcvd_ctx.received= 1;
  _rcvd_ctx.node= node;
  _rcvd_ctx.src_addr= msg->src_addr;
  _rcvd_ctx.dst_addr= msg->dst_addr;
  _rcvd_ctx.msg= *icmp_msg;

  switch (_rcvd_ctx.msg.type) {
  case ICMP_ECHO_REQUEST:
    icmp_send_echo_reply(node, msg->dst_addr, msg->src_addr, 255, sim);
    break;

  case ICMP_TRACE:
    if (msg->opts != NULL)
      msg->opts->trace->status= ESUCCESS;
    break;

  case ICMP_ECHO_REPLY:
  case ICMP_ERROR:
    break;

  default:
    cbgp_fatal("unsupported ICMP message type (%d)\n", _rcvd_ctx.msg.type);
  }

  _icmp_msg_destroy(&icmp_msg);

  return 0;
}

// -----[ icmp_ping_send_recv ]--------------------------------------
/**
 * This function sends an ICMP echo-request and checks that an answer
 * is received. It can be used to silently check that a remote host
 * is reachable (e.g. in BGP session establishment and refresh
 * checks).
 *
 * For this purpose, the function instanciates a local simulator that
 * is used to schedule the messages exchanged in reaction to the ICMP
 * echo-request.
 */
net_error_t icmp_ping_send_recv(net_node_t * node, net_addr_t src_addr,
				net_addr_t dst_addr, uint8_t ttl)
{
  simulator_t * sim= NULL;
  net_error_t error;

  sim= sim_create(SCHEDULER_STATIC);

  _rcvd_ctx.received= 0;
  error= icmp_send_echo_request(node, src_addr, dst_addr, ttl, sim);
  if (error == ESUCCESS) {
    sim_run(sim);

    if ((_rcvd_ctx.received) && (_rcvd_ctx.node == node)) {
      switch (_rcvd_ctx.msg.type) {
      case ICMP_ECHO_REPLY:
	error= ESUCCESS;
	break;
      case ICMP_ERROR:
	switch (_rcvd_ctx.msg.sub_type) {
	case ICMP_ERROR_NET_UNREACH:
	  error= ENET_ICMP_NET_UNREACH; break;
	case ICMP_ERROR_HOST_UNREACH:
	  error= ENET_ICMP_HOST_UNREACH; break;
	case ICMP_ERROR_PROTO_UNREACH:
	  error= ENET_ICMP_PROTO_UNREACH; break;
	case ICMP_ERROR_TIME_EXCEEDED:
	  error= ENET_ICMP_TIME_EXCEEDED; break;
	default:
	  abort();
	}
	break;
      default:
	error= ENET_NO_REPLY;
      }
    } else {
      error= ENET_NO_REPLY;
    }
  }

  sim_destroy(&sim);
  return error;
}

// -----[ icmp_ping ]------------------------------------------------
/**
 * Send an ICMP echo-request to the given destination and checks if a
 * reply is received. This function mimics the well-known 'ping'
 * application.
 */
int icmp_ping(gds_stream_t * stream,
	      net_node_t * node, net_addr_t src_addr,
	      net_addr_t dst_addr, uint8_t ttl)
{
  int iResult;

  if (ttl == 0)
    ttl= 255;

  iResult= icmp_ping_send_recv(node, src_addr, dst_addr, ttl);

  stream_printf(stream, "ping: ");
  switch (iResult) {
  case ESUCCESS:
    stream_printf(stream, "reply from ");
    ip_address_dump(stream, _rcvd_ctx.src_addr);
    break;
  case ENET_ICMP_NET_UNREACH:
  case ENET_ICMP_HOST_UNREACH:
  case ENET_ICMP_PROTO_UNREACH:
  case ENET_ICMP_TIME_EXCEEDED:
    network_perror(stream, iResult);
    stream_printf(stream, " from ");
    ip_address_dump(stream, _rcvd_ctx.src_addr);
    break;
  default:
    network_perror(stream, iResult);
  }
  stream_printf(stream, "\n");
  stream_flush(stream);

  return iResult;
}

// -----[ icmp_trace_route ]-----------------------------------------
/**
 * Send an ICMP echo-request with increasing TTL to the given
 * destination and checks if a 'time-exceeded' error or an echo-reply
 * is received. This function mimics the well-known 'traceroute'
 * application.
 */
net_error_t icmp_trace_route(gds_stream_t * stream,
			     net_node_t * node, net_addr_t src_addr,
			     net_addr_t dst_addr, uint8_t max_ttl,
			     ip_trace_t ** trace_ref)
{
  ip_trace_t * trace= ip_trace_create(NULL);
  net_error_t error= EUNEXPECTED;
  uint8_t ttl= 1;

  if (max_ttl == 0)
    max_ttl= 255;

  ip_trace_add_node(trace, node, NULL, NULL);

  while (ttl <= max_ttl) {

    error= icmp_ping_send_recv(node, src_addr, dst_addr, ttl);

    if (stream != NULL)
      stream_printf(stream, "%3d\t", ttl);
    switch (error) {
    case ESUCCESS:
    case ENET_ICMP_NET_UNREACH:
    case ENET_ICMP_HOST_UNREACH:
    case ENET_ICMP_PROTO_UNREACH:
    case ENET_ICMP_TIME_EXCEEDED:
      if (stream != NULL) {
	ip_address_dump(stream, _rcvd_ctx.src_addr);
	stream_printf(stream, " (");
	node_dump_id(stream, _rcvd_ctx.msg.node);
	stream_printf(stream, ")");
      }
      ip_trace_add_node(trace, _rcvd_ctx.msg.node,
			node_find_iface(_rcvd_ctx.msg.node,
					net_prefix(_rcvd_ctx.src_addr, 32)),
			NULL);
      break;
    default:
      if (stream != NULL)
	stream_printf(stream, "* (*)");
      ip_trace_add_node(trace, NULL, NET_ADDR_ANY, NET_ADDR_ANY);
    }

    if (stream != NULL) {
      stream_printf(stream, "\t");
      if (error == ESUCCESS)
	stream_printf(stream, "reply");
      else
	network_perror(stream, error);
      stream_printf(stream, "\n");
    }
    
    if (error != ENET_ICMP_TIME_EXCEEDED)
      break;
      
    ttl++;
  }

  if (stream != NULL)
    stream_flush(stream);

  if (trace_ref != NULL)
    *trace_ref= trace;
  else
    ip_trace_destroy(&trace);

  return error;
}

// -----[ icmp_trace_send ]------------------------------------------
/**
 * How to handle the IP options ?
 * - the caller provided IP options must be non-NULL
 * - shall the caller allocate the options on the stack on the heap ?
 *   => both must be supported
 * - if a copy of the IP options is performed, it should be released
 *   => when the enumerator is destroyed (decrease reference).
 *
 * To enable ECMP search, the caller must set the IP_OPT_ECMP option.
 */
array_t * icmp_trace_send(net_node_t * node, net_addr_t dst_addr,
			  uint8_t max_ttl, ip_opt_t * opts)
{
  __debug("_icmp_trace_send");
  net_msg_t * msg;
  icmp_msg_t * icmp_msg= _icmp_msg_create(ICMP_TRACE, 0, node);
  ip_opt_t *_opts;

  if (opts != NULL)
    _opts= ip_options_copy(opts, 1);
  else
    _opts= ip_options_create();
  ip_options_trace(_opts);
 
  // Prepare the ICMP message and associate the IP options
  msg= message_create(NET_ADDR_ANY, dst_addr,
		      NET_PROTOCOL_ICMP, max_ttl,
		      icmp_msg, (FPayLoadDestroy) _icmp_msg_destroy);
  message_set_options(msg, _opts);


  array_t * traces= ip_opt_ecmp_run(_opts, msg, node);

  ip_options_destroy(&_opts);
  __debug("_icmp_trace_send::END");
  return traces;
}

// -----[ _icmp_record_route_dump ]----------------------------------
/*
 * Output format:
 *   <src-node> <dest> <status> <length> <path>
 *     [delay] [weight] [capacity]
 */
static inline
void _icmp_record_route_dump(gds_stream_t * stream, net_node_t * node,
			     ip_dest_t dest, ip_opt_t * opts,
			     ip_trace_t * trace)
{
  node_dump_id(stream, node);
  stream_printf(stream, "\t");
  ip_dest_dump(stream, dest);
  stream_printf(stream, "\t");

  switch (trace->status) {
  case ESUCCESS:
    stream_printf(stream, "SUCCESS"); break;
  case ENET_FWD_LOOP:
    stream_printf(stream, "LOOP"); break;
  default:
    stream_printf(stream, "UNREACH");
  }

  // Dump each (node) hop to output
  ip_trace_dump(stream, trace, IP_TRACE_DUMP_LENGTH);

  // Total propagation delay requested ?
  if (opts->flags & IP_OPT_DELAY)
    stream_printf(stream, "\tdelay:%u", trace->delay);

  // Total IGP weight requested ?
  if (opts->flags & IP_OPT_WEIGHT)
    stream_printf(stream, "\tweight:%u", trace->weight);

  // Maximum capacity requested ?
  if (opts->flags & IP_OPT_CAPACITY)
    stream_printf(stream, "\tcapacity:%u", trace->capacity);

  /*if ((options.flags & IP_OPT_DEFLECTION) &&
      (net_path_length(pRRInfo->pDeflectedPath) > 0)) {
    stream_printf(pStream, "\tDEFLECTION\t");
    pDeflectedDump.pStream= pStream;
    pDeflectedDump.uAddrType= 0;
    net_path_for_each(pRRInfo->pDeflectedPath,
		      _print_deflected_path_for_each,
		      &pDeflectedDump);
		      }*/

  stream_printf(stream, "\n");

  //net_record_route_info_destroy(&pRRInfo);

  stream_flush(stream);
}

// -----[ icmp_record_route ]----------------------------------------
int icmp_record_route(gds_stream_t * stream,
		      net_node_t * node, net_addr_t src_addr,
		      ip_dest_t dest, uint8_t ttl, net_tos_t tos,
		      ip_opt_t * opts)
{
  int i;
  ip_trace_t * trace= NULL;
  array_t * traces= icmp_trace_send(node, dest.addr, ttl, opts);

  for (i= 0; i < _array_length(traces); i++) {
    _array_get_at(traces, i, &trace);
    _icmp_record_route_dump(stream, node, dest, opts, trace);
  }
  _array_destroy(&traces);
  return ESUCCESS;
}


const net_protocol_def_t PROTOCOL_ICMP= {
  .name= "icmp",
  .ops= {
    .handle      = _icmp_proto_handle,
    .destroy     = NULL,
    .dump_msg    = _icmp_proto_dump_msg,
    .destroy_msg = NULL/*icmp_msg_destroy*/,
    .copy_payload= _icmp_proto_copy_payload,
  }
};
