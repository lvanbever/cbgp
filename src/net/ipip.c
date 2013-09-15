// ==================================================================
// @(#)ipip.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 25/02/2004
// $Id: ipip.c,v 1.10 2009-03-24 16:14:28 bqu Exp $
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

#include <net/error.h>
#include <net/icmp_options.h>
#include <net/iface.h>
#include <net/ipip.h>
#include <net/link.h>
#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>
#include <util/str_format.h>

//#define IPIP_DEBUG

#ifdef IPIP_DEBUG
static int _debug_for_each(gds_stream_t * stream, void * context,
			   char format)
{
  va_list * ap= (va_list*) context;
  net_node_t * node;
  net_addr_t addr;
  net_msg_t * msg;
  net_iface_t * iface;
  int error;

  switch (format) {
  case 'a':
    addr= va_arg(*ap, net_addr_t);
    ip_address_dump(stream, addr);
    break;
  case 'e':
    error= va_arg(*ap, int);
    network_perror(stream, error);
    break;
  case 'i':
    iface= va_arg(*ap, net_iface_t *);
    net_iface_dump_id(stream, iface);
    break;
  case 'm':
    msg= va_arg(*ap, net_msg_t *);
    stream_printf(stream, "[");
    message_dump(stream, msg);
    stream_printf(stream, "]");
    break;
  case 'n':
    node= va_arg(*ap, net_node_t *);
    node_dump_id(stream, node);
    break;
  }
  return 0;
}
#endif /* IPIP_DEBUG */

static inline void ___ipip_debug(const char * msg, ...)
{
#ifdef IPIP_DEBUG
  va_list ap;

  va_start(ap, msg);
  stream_printf(gdsout, "IPIP_DBG::");
  str_format_for_each(gdsout, _debug_for_each, &ap, msg);
  stream_flush(gdsout);
  va_end(ap);
#endif /* IPIP_DEBUG */
}

typedef struct {
  net_iface_t * oif;      /* iface used to send traffic (by-pass routing) */
  net_addr_t    gateway;
  net_addr_t    src_addr;
} ipip_data_t;

// -----[ _ipip_iface_destroy ]--------------------------------------
static void _ipip_iface_destroy(void * ctx)
{
  // Destroy iface->user_data (ctx)
  FREE(ctx);
}

// -----[ _ipip_msg_destroy ]----------------------------------------
/**
 *
 */
static void _ipip_msg_destroy(void ** payload_ref)
{
  net_msg_t * msg= *((net_msg_t **) payload_ref);

  ___ipip_debug("_ipip_msg_destroy msg=%m\n", msg);
  message_destroy(&msg);
}

// -----[ ipip_iface_send ]------------------------------------------
/**
 * This is the tunnel interface send function.
 */
static int _ipip_iface_send(net_iface_t * self,
			    net_addr_t next_hop,
			    net_msg_t * msg)
{
  ipip_data_t * ctx= (ipip_data_t *) self->user_data;
  net_addr_t src_addr= ctx->src_addr;
  net_msg_t * outer_msg;

  ___ipip_debug("_ipip_iface_send msg=%m\n", msg);

  if (ctx->oif != NULL) {
    // Default IP encap source address = outgoing interface's address.
    if (src_addr == NET_ADDR_ANY)
	src_addr= ctx->oif->addr;

    //TO BE WRITTEN: return node_ip_output(); ...
    return EUNSUPPORTED;

  } else {

    outer_msg= message_create(src_addr, self->dest.end_point,
			      NET_PROTOCOL_IPIP, 255, msg,
			      _ipip_msg_destroy);

    ip_opt_hook_msg_encap(self->owner, outer_msg, msg);

    node_send(self->owner, outer_msg, NULL, NULL);
    return ESUCCESS;
  }
}

// -----[ ipip_iface_recv ]------------------------------------------
/**
 * This is the tunnel interface receive function.
 *
 * Note: this function is responsible for destroying the received
 *       message. This is a bit different from a protocol handler,
 *       but is exactly the same behavior as an interface.
 */
static int _ipip_iface_recv(net_iface_t * self, net_msg_t * msg)
{
  net_msg_t * outer_msg;
  net_msg_t * inner_msg;

  ___ipip_debug("_ipip_iface_recv msg=%m\n", msg);

  if (msg->protocol != NET_PROTOCOL_IPIP) {
    /* Discard packet silently ? should log */
    stream_printf(gdserr, "non-IPIP packet received on tunnel interface\n");
    return -1;
  }

  outer_msg= msg;
  inner_msg= (net_msg_t *) outer_msg->payload;

  ip_opt_hook_msg_decap(self->owner, outer_msg, inner_msg);

  outer_msg->payload= NULL;
  message_destroy(&outer_msg);

  return node_recv_msg(self->owner, self, inner_msg);
}

// -----[ ipip_link_create ]-----------------------------------------
/**
 * Create a tunnel interface.
 *
 * Arguments:
 *   node     : source node (where the interface is attached.
 *   dst-point: remote tunnel endpoint
 *   addr     : tunnel identifier
 *   oif      : optional outgoing interface
 *   src_addr : optional IPIP packet source address
 */
net_error_t ipip_link_create(net_node_t * node,
			     net_addr_t end_point,
			     net_addr_t addr,
			     net_iface_t * oif,
			     net_addr_t src_addr,
			     net_iface_t ** iface_ref)
{
  net_iface_t * tunnel;
  ipip_data_t * ctx;
  net_error_t error;

  // Check that the source address corresponds to a local interface
  if (src_addr != NET_ADDR_ANY) {
    if (node_find_iface(node, net_prefix(src_addr, 32)) == NULL) {
      abort();
    }
  }

  // Create the tunnel interface
  error= net_iface_factory(node, net_prefix(addr, 32), NET_IFACE_VIRTUAL,
			   &tunnel);
  if (error != ESUCCESS)
    return error;

  // Connect tunnel interface
  tunnel->dest.end_point= end_point;
  tunnel->connected= 1;

  // Create the IPIP context (will be destroyed by _ipip_iface_destroy)
  ctx= (ipip_data_t *) MALLOC(sizeof(ipip_data_t));
  ctx->oif= oif;
  ctx->gateway= NET_ADDR_ANY;
  ctx->src_addr= src_addr;

  tunnel->user_data= ctx;
  tunnel->ops.send= _ipip_iface_send;
  tunnel->ops.recv= _ipip_iface_recv;
  tunnel->ops.destroy= _ipip_iface_destroy;

  *iface_ref= tunnel;

  return ESUCCESS;
}



// -----[ _ipip_proto_handle ]---------------------------------------
/**
 * This handler is used when an IPIP packet is delivered for
 * processing to the destination node. This is not the expected way
 * to receive IPIP packets. The normal way is through the tunnel
 * endpoint, i.e. a tunnel interface on the destination node.
 *
 * When a packet is received by this function it is discarded. It's
 * main purpose is to free IPIP packets from memory.
 */
static int _ipip_proto_handle(simulator_t * sim,
			      void * handler,
			      net_msg_t * msg)
{
  ___ipip_debug("_ipip_proto_handle msg=%m\n", msg);

  net_msg_t * inner_msg= (net_msg_t *) msg->payload;
  message_destroy(&inner_msg);
  return ESUCCESS;

}

// -----[ _ipip_proto_dump_msg ]-------------------------------------
static void _ipip_proto_dump_msg(gds_stream_t * stream, net_msg_t * msg)
{
  net_msg_t * encap_msg= (net_msg_t *) msg->payload;
  stream_printf(stream, "msg:[");
  if (encap_msg != NULL)
    message_dump(stream, encap_msg);
  else
    stream_printf(stream, "null");
  stream_printf(stream, "]");
}

// -----[ _ipip_proto_copy_payload ]---------------------------------
static void * _ipip_proto_copy_payload(net_msg_t * msg)
{
  net_msg_t * inner_msg= message_copy((net_msg_t *) msg->payload);
  return inner_msg;
}

const net_protocol_def_t PROTOCOL_IPIP= {
  .name= "ipip",
  .ops= {
    .handle      = _ipip_proto_handle,
    .destroy     = NULL,
    .dump_msg    = _ipip_proto_dump_msg,
    .destroy_msg = NULL,
    .copy_payload= _ipip_proto_copy_payload,
  }
};
