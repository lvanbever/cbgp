// ==================================================================
// @(#)bgp_router_peer.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @date 27/02/2008
// $Id: bgp_router_peer.c,v 1.5 2009-08-24 10:33:30 bqu Exp $
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
#include <string.h>

#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>

#include <bgp/aslevel/as-level.h>
#include <bgp/filter/filter.h>
#include <bgp/filter/parser.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/as.h>
#include <cli/bgp_filter.h>
#include <cli/common.h>
#include <cli/context.h>
#include <cli/enum.h>
#include <net/util.h>

// -----[ cli_ctx_create_peer ]--------------------------------------
/**
 * context: {router} -> {router, peer}
 * tokens: {addr}
 */
static int cli_ctx_create_peer(cli_ctx_t * ctx, cli_cmd_t * cmd, void ** item)
{
  bgp_router_t* router= _router_from_context(ctx);
  bgp_peer_t * peer;
  const char * arg= cli_get_arg_value(cmd, 0);
  net_addr_t addr;

  // Get the peer ID (address, ASN, ...)
  if (str2addr_id(arg, &addr)) {
    cli_set_user_error(cli_get(), "invalid peer address \"%s\"", arg);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get the peer
  peer= bgp_router_find_peer(router, addr);
  if (peer == NULL) {
    cli_set_user_error(cli_get(), "unknown peer");
    return CLI_ERROR_CTX_CREATE;
  }

  *item= peer;
  return CLI_SUCCESS;
}

// -----[ cli_ctx_destroy_peer ]-------------------------------------
static void cli_ctx_destroy_peer(void ** item_ref)
{
  // NOTHING TO REMOVE.
} 

// -----[ cli_ctx_create_peer_filter ]-------------------------------
/**
 * context: {router, peer}
 * tokens: {in|out}
 */
static int cli_ctx_create_peer_filter(cli_ctx_t * ctx, cli_cmd_t * cmd,
				      void ** item_ref)
{
  bgp_peer_t * peer= _peer_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);

  // Create filter context
  if (!strcmp(arg, "in"))
    *item_ref= &(peer->filter[FILTER_IN]);
  else if (!strcmp(arg, "out"))
    *item_ref= &(peer->filter[FILTER_OUT]);
  else {
    cli_set_user_error(cli_get(), "invalid filter type \"%s\"", arg);
    return CLI_ERROR_CTX_CREATE;
  }

  return CLI_SUCCESS;
}

// -----[ cli_ctx_destroy_peer_filter ]------------------------------
static void cli_ctx_destroy_peer_filter(void ** item_ref)
{
}

// -----[ cli_peer_show_info ]---------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_show_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  bgp_peer_show_info(gdsout, peer);
  return CLI_SUCCESS;
}

// -----[ cli_peer_rrclient ]----------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_rrclient(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  peer->router->reflector= 1;
  bgp_peer_flag_set(peer, PEER_FLAG_RR_CLIENT, 1);
  return CLI_SUCCESS;
}

// -----[ cli_peer_nexthop ]-----------------------------------------
/**
 * context: {router, peer}
 * tokens: {addr}
 */
static int cli_peer_nexthop(cli_ctx_t * ctx,
			    cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  net_addr_t next_hop;

  // Get next-hop address
  if (str2address(arg, &next_hop)) {
    cli_set_user_error(cli_get(), "invalid next-hop \"%s\".", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Set the next-hop to be used for routes advertised to this peer
  if (bgp_peer_set_nexthop(peer, next_hop)) {
    cli_set_user_error(cli_get(), "could not change next-hop");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_peer_nexthopself ]-------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_nexthopself(cli_ctx_t * ctx,
				    cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  bgp_peer_flag_set(peer, PEER_FLAG_NEXT_HOP_SELF, 1);
  return CLI_SUCCESS;
}

// -----[ cli_peer_record ]------------------------------------------
/**
 * context: {router, peer}
 * tokens: {file|-}
 */
static int cli_peer_record(cli_ctx_t * ctx,
			   cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  gds_stream_t * stream;
  
  /* Get filename */
  if (strcmp(arg, "-") == 0) {
    bgp_peer_set_record_stream(peer, NULL);
  } else {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "could not open \"%s\" for writing", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
    if (bgp_peer_set_record_stream(peer, stream) < 0) {
      stream_destroy(&stream);
      cli_set_user_error(cli_get(), "could not set the peer record stream");
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  return CLI_SUCCESS;
}

// -----[ cli_peer_recv ]--------------------------------------------
/**
 * context: {router, peer}
 * tokens: {mrt-record}
 */
static int cli_peer_recv(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_msg_t * msg;
  int result;

  /* Check that the peer is virtual */
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    cli_set_user_error(cli_get(), "only virtual peers can do that");
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Build a message from the MRT-record */
  result= mrtd_msg_from_line(peer->router, peer, arg, &msg);

  if (result < 0) {
    cli_set_user_error(cli_get(), "could not build message from input (%s)",
		       mrtd_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_peer_handle_message(peer, msg);

  return CLI_SUCCESS;
}

// -----[ cli_peer_walton_limit ]------------------------------------
/**
 * context: {router, peer}
 * tokens: {announce-limit}
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
static int cli_peer_walton_limit(cli_ctx_t * ctx, 
				 cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int limit;

  if (str_as_uint(arg, &limit)) {
    cli_set_user_error(cli_get(), "invalid walton limit \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  peer_set_walton_limit(peer, limit);
  return CLI_SUCCESS;
}
#endif

// -----[ cli_peer_up ]----------------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_up(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  // Try to open session
  if (bgp_peer_open_session(peer)) {
    cli_set_user_error(cli_get(), "could not open session");
    return CLI_ERROR_COMMAND_FAILED;
  }
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_update_source ]-----------------------------------
/**
 * context: {router, peer}
 * tokens: {src-addr}
 */
static int cli_peer_update_source(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  net_addr_t src_addr;

  // Get source address
  if (str2address(arg, &src_addr)) {
    cli_set_user_error(cli_get(), "invalid source address \"%s\".", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Try to open session
  if (bgp_peer_set_source(peer, src_addr)) {
    cli_set_user_error(cli_get(), "could not set source address");
    return CLI_ERROR_COMMAND_FAILED;
  }
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_softrestart ]-------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_softrestart(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  // Set the virtual flag of this peer
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    cli_set_user_error(cli_get(), "soft-restart option only available to virtual peers");
    return CLI_ERROR_COMMAND_FAILED;
  }
  bgp_peer_flag_set(peer, PEER_FLAG_SOFT_RESTART, 1);
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_virtual ]-----------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_virtual(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  // Set the virtual flag of this peer
  bgp_peer_flag_set(peer, PEER_FLAG_VIRTUAL, 1);
    
  return CLI_SUCCESS;
}

// -----[ cli_peer_down ]--------------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_down(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  // Try to close session
  if (bgp_peer_close_session(peer)) {
    cli_set_user_error(cli_get(), "could not close session");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_peer_readv ]-------------------------------------------
/**
 * context: {router, peer}
 * tokens: {prefix|*}
 */
#ifdef __EXPERIMENTAL__
static int cli_peer_readv(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);
  bgp_router_t* router= peer->router;
  const char * arg= cli_get_arg_value(cmd, 0);
  ip_pfx_t prefix;

  /* Get prefix */
  if (!strcmp(arg, "*")) {
    prefix.mask= 0;
  } else if (!str2prefix(arg, &prefix)) {
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|* \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Re-advertise */
  if (bgp_router_peer_readv_prefix(router, peer, prefix)) {
    cli_set_user_error(cli_get(), "could not re-advertise");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}
#endif /* __EXPERIMENTAL__ */

// -----[ cli_peer_reset ]-------------------------------------------
/**
 * context: {router, peer}
 * tokens: {}
 */
static int cli_peer_reset(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_peer_t * peer= _peer_from_context(ctx);

  // Try to close session
  if (bgp_peer_close_session(peer)) {
    cli_set_user_error(cli_get(), "could not close session");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Try to open session
  if (bgp_peer_open_session(peer)) {
    cli_set_user_error(cli_get(), "could not close session");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ _register_peer_filter ]------------------------------------
/**
 * filter add
 * filter insert <pos>
 * filter remove <pos>
 *
 * filter X
 *   match ""
 *   action ""
 *   (...)
 */
static void _register_peer_filter(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_ctx("filter",
					 cli_ctx_create_peer_filter,
					 cli_ctx_destroy_peer_filter));
  cli_add_arg(group, cli_arg("in|out", NULL));
  cmd= cli_add_cmd(group, cli_cmd_ctx("add-rule",
				      cli_ctx_create_bgp_filter_add_rule,
				      cli_ctx_destroy_bgp_filter_rule));
  cli_register_bgp_filter_rule(cmd);
  cmd= cli_add_cmd(group, cli_cmd_ctx("insert-rule",
				      cli_ctx_create_bgp_filter_insert_rule,
				      cli_ctx_destroy_bgp_filter_rule));
  cli_add_arg(cmd, cli_arg("index", NULL));
  cmd= cli_add_cmd(group, cli_cmd("remove-rule", cli_bgp_filter_remove_rule));
  cli_add_arg(cmd, cli_arg("index", NULL));
  cmd= cli_add_cmd(group, cli_cmd("show", cli_bgp_filter_show));
}

// -----[ _register_peer_show ]--------------------------------------
static inline void _register_peer_show(cli_cmd_t * parent)
{
  cli_cmd_t * group;

  group= cli_add_cmd(parent, cli_cmd_group("show"));
  cli_add_cmd(group, cli_cmd("info", cli_peer_show_info));
}

// -----[ cli_register_bgp_router_peer ]-----------------------------
void cli_register_bgp_router_peer(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_ctx("peer",
					 cli_ctx_create_peer,
					 cli_ctx_destroy_peer));
  cli_add_arg(group, cli_arg2("addr", NULL, cli_enum_bgp_peers_addr_id));
  _register_peer_filter(group);
  _register_peer_show(group);
  cmd= cli_add_cmd(group, cli_cmd("down", cli_peer_down));
  cmd= cli_add_cmd(group, cli_cmd("next-hop", cli_peer_nexthop));
  cli_add_arg(cmd, cli_arg("next-hop", NULL));
  cmd= cli_add_cmd(group, cli_cmd("next-hop-self", cli_peer_nexthopself));
#ifdef __EXPERIMENTAL__
  cmd= cli_add_cmd(group, cli_cmd("re-adv", cli_peer_readv));
  cli_add_arg(cmd, cli_arg("prefix|*", NULL));
#endif /* __EXPERIMENTAL__ */
  cmd= cli_add_cmd(group, cli_cmd("record", cli_peer_record));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  cmd= cli_add_cmd(group, cli_cmd("walton-limit", cli_peer_walton_limit));
  cli_add_arg(cmd, cli_arg("announce-limit", NULL));
#endif /* __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__ */
  cmd= cli_add_cmd(group, cli_cmd("recv", cli_peer_recv));
  cli_add_arg(cmd, cli_arg("mrt-record", NULL));
  cmd= cli_add_cmd(group, cli_cmd("reset", cli_peer_reset));
  cmd= cli_add_cmd(group, cli_cmd("rr-client", cli_peer_rrclient));
  cmd= cli_add_cmd(group, cli_cmd("soft-restart", cli_peer_softrestart));
  cmd= cli_add_cmd(group, cli_cmd("up", cli_peer_up));
  cmd= cli_add_cmd(group, cli_cmd("update-source", cli_peer_update_source));
  cli_add_arg(cmd, cli_arg("iface-address", NULL));
  cmd= cli_add_cmd(group, cli_cmd("virtual", cli_peer_virtual));
}
