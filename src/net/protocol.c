// ==================================================================
// @(#)protocol.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 25/02/2004
// $Id: protocol.c,v 1.8 2009-03-24 16:23:40 bqu Exp $
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

#include <libgds/array.h>
#include <libgds/memory.h>
#include <string.h>

#include <net/error.h>
#include <net/net_types.h>
#include <net/protocol.h>

#include <net/icmp.h>
#include <net/ipip.h>
#include <bgp/as.h>

const net_protocol_def_t PROTOCOL_RAW= {
  .name= "raw",
  .ops= {
    .handle      = NULL,
    .destroy     = NULL,
    .dump_msg    = NULL,
    .destroy_msg = NULL,
    .copy_payload= NULL,
  }
};

typedef int (*debug_proto_handler)(net_msg_t * msg);

static int _debug_proto_handle(simulator_t * sim,
			      void * handler,
			      net_msg_t * msg)
{
  return ((debug_proto_handler) handler)(msg);
}

static void _debug_proto_destroy_msg(net_msg_t * msg)
{
  if (msg->payload != NULL)
    FREE(msg->payload);
}

const net_protocol_def_t PROTOCOL_DEBUG= {
  .name= "debug",
  .ops= {
    .handle      = _debug_proto_handle,
    .destroy     = NULL,
    .dump_msg    = NULL,
    .destroy_msg = _debug_proto_destroy_msg,
    .copy_payload= NULL,
  }
};

const net_protocol_def_t * PROTOCOL_DEFS[NET_PROTOCOL_MAX]= {
  &PROTOCOL_RAW,
  &PROTOCOL_DEBUG,
  &PROTOCOL_ICMP,
  &PROTOCOL_BGP,
  &PROTOCOL_IPIP,
};

// -----[ _protocol_create ]-----------------------------------------
/**
 *
 */
static inline
net_protocol_t * _protocol_create(net_protocol_id_t id,
				  void * handler,
				  const net_protocol_def_t * def)
{
  net_protocol_t * proto=
    (net_protocol_t *) MALLOC(sizeof(net_protocol_t));
  proto->id= id;
  proto->handler= handler;
  proto->def= def;
  return proto;
}

// -----[ _protocol_destroy ]----------------------------------------
/**
 *
 */
static inline
void _protocol_destroy(net_protocol_t ** proto_ref)
{
  if (*proto_ref != NULL) {
    if ((*proto_ref)->def->ops.destroy != NULL)
      (*proto_ref)->def->ops.destroy(&(*proto_ref)->handler);
    FREE(*proto_ref);
    *proto_ref= NULL;
  }
}

// -----[ protocol_recv ]------------------------------------------
net_error_t protocol_recv(net_protocol_t * proto, net_msg_t * msg,
			  simulator_t * sim)
{
  //assert(proto->def->ops.handle != NULL);
  if (proto->def->ops.handle != NULL)
    return proto->def->ops.handle(sim, proto->handler, msg);
  return ESUCCESS;
}

// -----[ _protocols_cmp ]-------------------------------------------
static int _protocols_cmp(const void * item1,
			  const void * item2,
			  unsigned int item_size)
{
  net_protocol_t * proto1= *((net_protocol_t **) item1);
  net_protocol_t * proto2= *((net_protocol_t **) item2);
  
  if (proto1->id > proto2->id)
    return 1;
  else if (proto1->id < proto2->id)
    return -1;
  return 0;
}

// -----[ _protocols_destroy ]---------------------------------------
static void _protocols_destroy(void * item, const void * ctx)
{
  _protocol_destroy((net_protocol_t **) item);
}

// -----[ protocols_create ]-----------------------------------------
net_protocols_t * protocols_create()
{
  net_protocols_t * protocols=
    (net_protocols_t *) ptr_array_create(ARRAY_OPTION_SORTED|
					 ARRAY_OPTION_UNIQUE,
					 _protocols_cmp,
					 _protocols_destroy,
					 NULL);
  return protocols;
}

// -----[ protocols_destroy ]----------------------------------------
/**
 * This function destroys the list of protocols and destroys all the
 * related protocol instances if required.
 */
void protocols_destroy(net_protocols_t ** protocols_ref)
{
  ptr_array_destroy(protocols_ref);
}

// -----[ protocols_add ]--------------------------------------------
/**
 * Add a protocol handler to the protocol list.
 */
net_error_t protocols_add(net_protocols_t * protocols,
			  net_protocol_id_t id,
			  void * handler)
{
  net_protocol_t * protocol;

  if (id >= NET_PROTOCOL_MAX)
    return ENET_PROTO_UNKNOWN;

  protocol= _protocol_create(id, handler, PROTOCOL_DEFS[id]);

  if (ptr_array_add(protocols, &protocol) < 0) {
    _protocol_destroy(&protocol);
    return ENET_PROTO_DUPLICATE;
  }

  return ESUCCESS;
}

// -----[ protocols_get ]--------------------------------------------
/**
 *
 */
net_protocol_t * protocols_get(net_protocols_t * protocols,
			       net_protocol_id_t id)
{
  net_protocol_t proto= { .id= id };
  net_protocol_t * proto_ref= &proto;
  unsigned int index;

  if (ptr_array_sorted_find_index(protocols, &proto_ref, &index) < 0)
    return NULL;
  return protocols->data[index];
}

// -----[ net_protocol2str ]-----------------------------------------
const char * net_protocol2str(net_protocol_id_t id)
{
  if (id >= NET_PROTOCOL_MAX)
    return "unknown";
  return PROTOCOL_DEFS[id]->name;
}

// -----[ net_str2protocol ]-----------------------------------------
net_error_t net_str2protocol(const char * str, net_protocol_id_t * id_ref)
{
  net_protocol_id_t id= 0;
  while (id < NET_PROTOCOL_MAX) {
    if (!strcmp(PROTOCOL_DEFS[id]->name, str)) {
      *id_ref= id;
      return ESUCCESS;
    }
    id++;
  }
  return ENET_PROTO_UNKNOWN;
}

// -----[ net_protocols_get_def ]----------------------------------
const net_protocol_def_t * net_protocols_get_def(net_protocol_id_t id)
{
  assert(id < NET_PROTOCOL_MAX);
  return PROTOCOL_DEFS[id];
}
