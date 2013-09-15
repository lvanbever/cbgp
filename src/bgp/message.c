// ==================================================================
// @(#)message.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 19/05/2003
// $Id: message.c,v 1.25 2009-03-24 14:23:55 bqu Exp $
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

#include <assert.h>
#include <string.h>
#include <time.h>

#include <libgds/stream.h>
#include <libgds/memory.h>

#include <net/network.h>
#include <net/node.h>
#include <net/protocol.h>

#include <bgp/as.h>
#include <bgp/message.h>
#include <bgp/attr/origin.h>
#include <bgp/attr/path.h>
#include <bgp/route.h>

#include <sim/simulator.h>

typedef struct {
  uint16_t    uRemoteAS;
  bgp_msg_t * msg;
} bgp_msg_send_ctx_t;

static char * bgp_msg_names[BGP_MSG_TYPE_MAX]= {
  "A",
  "W",
  "CLOSE",
  "OPEN",
};

static gds_stream_t * pMonitor= NULL;

// ----- bgp_msg_update_create --------------------------------------
/**
 *
 */
bgp_msg_t * bgp_msg_update_create(uint16_t peer_asn,
				  bgp_route_t * route)
{
  bgp_msg_update_t * msg=
    (bgp_msg_update_t *) MALLOC(sizeof(bgp_msg_update_t));
  msg->header.type= BGP_MSG_TYPE_UPDATE;
  msg->header.peer_asn= peer_asn;
  msg->route= route;
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_withdraw_create ------------------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
bgp_msg_t * bgp_msg_withdraw_create(uint16_t peer_asn,
				    ip_pfx_t prefix,
				    net_addr_t * next_hop)
#else
bgp_msg_t * bgp_msg_withdraw_create(uint16_t peer_asn,
				    ip_pfx_t prefix)
#endif
{
  bgp_msg_withdraw_t * msg=
    (bgp_msg_withdraw_t *) MALLOC(sizeof(bgp_msg_withdraw_t));
  msg->header.type= BGP_MSG_TYPE_WITHDRAW;
  msg->header.peer_asn= peer_asn;
  memcpy(&(msg->prefix), &prefix, sizeof(ip_pfx_t));
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  //It corresponds to the path identifier described in the walton draft 
  //... nevertheless, we keep the variable name NextHop!
//  STREAM_DEBUG("creation of withdraw : ");
//  STREAM_ENABLED_DEBUG() ip_address_dump(stream_get_stream(pMainLog), tNextHop);
//  STREAM_DEBUG("\n");
  if (next_hop != NULL) {
    msg->next_hop = MALLOC(sizeof(net_addr_t));
    memcpy(msg->next_hop, next_hop, sizeof(net_addr_t));
  } else {
    msg->next_hop = NULL;
  }
#endif
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_close_create ---------------------------------------
bgp_msg_t * bgp_msg_close_create(uint16_t peer_asn)
{
  bgp_msg_close_t * msg=
    (bgp_msg_close_t *) MALLOC(sizeof(bgp_msg_close_t));
  msg->header.type= BGP_MSG_TYPE_CLOSE;
  msg->header.peer_asn= peer_asn;
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_open_create ----------------------------------------
bgp_msg_t * bgp_msg_open_create(uint16_t peer_asn, net_addr_t router_id)
{
  bgp_msg_open_t * msg=
    (bgp_msg_open_t *) MALLOC(sizeof(bgp_msg_open_t));
  msg->header.type= BGP_MSG_TYPE_OPEN;
  msg->header.peer_asn= peer_asn;
  msg->router_id= router_id;
  return (bgp_msg_t *) msg;
}

// ----- bgp_msg_destroy --------------------------------------------
/**
 *
 */
void bgp_msg_destroy(bgp_msg_t ** msg_ref)
{
  if (*msg_ref != NULL) {
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
    if (((*msg_ref)->type == BGP_MSG_TYPE_WITHDRAW) &&
	((bgp_msg_withdraw_t *)(*msg_ref))->next_hop != NULL)
      FREE( ((SBGPMsgWithdraw *)(*msg_ref))->next_hop );
#endif
    FREE(*msg_ref);
    *msg_ref= NULL;
  }
}

// ----- bgp_msg_send -----------------------------------------------
/**
 *
 */
int bgp_msg_send(net_node_t * node, net_addr_t src_addr,
		 net_addr_t dst_addr, bgp_msg_t * msg)
{
  bgp_msg_monitor_write(msg, node, dst_addr);

  //fprintf(stdout, "(");
  //ip_address_dump(stdout, node->addr);
  //fprintf(stdout, ") bgp-msg-send from ");
  //ip_address_dump(stdout, node->addr);
  //fprintf(stdout, " to ");
  //ip_address_dump(stdout, dst_addr);
  //fprintf(stdout, "\n");
  
  return node_send_msg(node, src_addr, dst_addr, NET_PROTOCOL_BGP, 255,
		       msg, (FPayLoadDestroy) bgp_msg_destroy,
		       NULL, network_get_simulator(node->network));
}

// -----[ _bgp_msg_header_dump ]-------------------------------------
static inline void _bgp_msg_header_dump(gds_stream_t * stream,
					net_node_t * node,
					bgp_msg_t * msg)
{
  stream_printf(stream, "|%s", bgp_msg_names[msg->type]);
  // Peer IP
  stream_printf(stream, "|");
  if (node != NULL) {
    node_dump_id(stream, node);
  } else {
    stream_printf(stream, "?");
  }
  // Peer AS
  stream_printf(stream, "|%d", msg->peer_asn);
}

// -----[ _bgp_msg_update_dump ]-------------------------------------
static inline void _bgp_msg_update_dump(gds_stream_t * stream,
					bgp_msg_update_t * msg)
{
  unsigned int index;
  uint32_t comm;
  bgp_route_t * route= msg->route;

  // Prefix
  stream_printf(stream, "|");
  ip_prefix_dump(stream, route->prefix);
  // AS-PATH
  stream_printf(stream, "|");
  path_dump(stream, route_get_path(route), 1);
  // ORIGIN
  stream_printf(stream, "|");
  stream_printf(stream, bgp_origin_to_str(route->attr->origin));
  // NEXT-HOP
  stream_printf(stream, "|");
  ip_address_dump(stream, route->attr->next_hop);
  // LOCAL-PREF
  stream_printf(stream, "|%u", route->attr->local_pref);
  // MULTI-EXIT-DISCRIMINATOR
  if (route->attr->med == ROUTE_MED_MISSING)
    stream_printf(stream, "|");
  else
    stream_printf(stream, "|%u", route->attr->med);
  // COMMUNITY
  stream_printf(stream, "|");
  if (route->attr->comms != NULL) {
    for (index= 0; index < route->attr->comms->num; index++) {
      comm= (uint32_t) route->attr->comms->values[index];
      stream_printf(stream, "%u ", comm);
    }
  }
  
  // Route-reflectors: Originator
  if (route->attr->originator != NULL) {
    stream_printf(stream, "originator:");
    ip_address_dump(stream, *route->attr->originator);
  }
  stream_printf(stream, "|");
  
  if (route->attr->cluster_list != NULL) {
    stream_printf(stream, "cluster_id_list:");
    cluster_list_dump(stream, route->attr->cluster_list);
  }
  stream_printf(stream, "|");
}

// -----[ _bgp_msg_withdraw_dump ]-----------------------------------
static inline void _bgp_msg_withdraw_dump(gds_stream_t * stream,
					  bgp_msg_withdraw_t * msg)
{
  // Prefix
  stream_printf(stream, "|");
  ip_prefix_dump(stream, ((bgp_msg_withdraw_t *) msg)->prefix);
}

// -----[ _bgp_msg_close_dump ]--------------------------------------
static inline void _bgp_msg_close_dump(gds_stream_t * stream,
				       bgp_msg_close_t * msg)
{
}

// -----[ _bgp_msg_open_dump ]---------------------------------------
static inline void _bgp_msg_open_dump(gds_stream_t * stream,
				      bgp_msg_open_t * msg)
{
  /* Router ID */
  stream_printf(stream, "|");
  ip_address_dump(stream, msg->router_id);
}

// ----- bgp_msg_dump -----------------------------------------------
/**
 *
 */
void bgp_msg_dump(gds_stream_t * stream,
		  net_node_t * node,
		  bgp_msg_t * msg)
{
  assert(msg->type < BGP_MSG_TYPE_MAX);

  /* Dump header */
  _bgp_msg_header_dump(stream, node, msg);

  /* Dump message content */
  switch (msg->type) {
  case BGP_MSG_TYPE_UPDATE:
    _bgp_msg_update_dump(stream, (bgp_msg_update_t *) msg);
    break;
  case BGP_MSG_TYPE_WITHDRAW:
    _bgp_msg_withdraw_dump(stream, (bgp_msg_withdraw_t *) msg);
    break;
  case BGP_MSG_TYPE_OPEN:
    _bgp_msg_open_dump(stream, (bgp_msg_open_t *) msg);
    break;
  case BGP_MSG_TYPE_CLOSE:
    _bgp_msg_close_dump(stream, (bgp_msg_close_t *) msg);
    break;
  default:
    stream_printf(stream, "should never reach this code");
  } 
}

/////////////////////////////////////////////////////////////////////
//
// BGP MESSAGES MONITORING SECTION
//
/////////////////////////////////////////////////////////////////////
//
// This is enabled through the CLI with the following command
// (see cli/bgp.c): bgp options msg-monitor <output-file>

// ----- bgp_msg_monitor_open ---------------------------------------
/**
 *
 */
int bgp_msg_monitor_open(const char * file_name)
{
  time_t tTime= time(NULL);

  bgp_msg_monitor_close();

  // Create new monitor
  pMonitor= stream_create_file((char *) file_name);
  if (pMonitor == NULL) {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "Unable to create monitor file\n");
    return -1;
  }

  // Write monitor file header
  stream_set_level(pMonitor, STREAM_LEVEL_EVERYTHING);
  stream_printf(pMonitor, "# BGP message trace\n");
  stream_printf(pMonitor, "# generated by C-BGP on %s",
	  ctime(&tTime));
  stream_printf(pMonitor, "# <dest-ip>|BGP4|<event-time>|<type>|"
	  "<peer-ip>|<peer-as>|<prefix>|...\n");
  return 0;
}

// -----[ bgp_msg_monitor_close ]------------------------------------
/**
 *
 */
void bgp_msg_monitor_close()
{
  // If monitor was previously enabled, stop it.
  if (pMonitor != NULL)
    stream_destroy(&pMonitor);
}

// ----- bgp_msg_monitor_write --------------------------------------
/**
 * Write the given BGP update/withdraw message in MRTD format, i.e.
 * a BGP update message is written as
 *
 *   BGP4|<time>|A|<peer-ip>|<peer-as>|<prefix>|<as-path>|<origin>|
 *   <next-hop>|<local-pref>|<med>|<communities>
 *
 * and a BGP withdraw message is written as
 *
 *   BGP4|<time>|W|<peer-ip>|<peer-as>|<prefix>
 *
 * In addition to this, each message is prefixed with the
 * destination's IP address
 *
 *   <dest-ip>|
 */
void bgp_msg_monitor_write(bgp_msg_t * msg, net_node_t * node,
			   net_addr_t addr)
{
  if (pMonitor != NULL) {

    // Destination router (): this is not MRTD format but required to
    // identify the destination of the messages
    ip_address_dump(pMonitor, addr);
    // Protocol and Time
    stream_printf(pMonitor, "|BGP4|%.2f",
	       sim_get_time(network_get_simulator(node->network)));

    bgp_msg_dump(pMonitor, node, msg);

    stream_printf(pMonitor, "\n");
  }
}

// ----- bgp_msg_monitor_destroy ------------------------------------
/**
 *
 */
static inline void bgp_msg_monitor_destroy(gds_stream_t ** stream_ref)
{
  stream_destroy(stream_ref);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _message_destroy ]-----------------------------------------
void _message_destroy()
{
  bgp_msg_monitor_destroy(&pMonitor);
}
