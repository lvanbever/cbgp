// ==================================================================
// @(#)net_node.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 15/05/2007
// $Id: net_node.c,v 1.11 2009-08-31 09:37:56 bqu Exp $
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

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <cli/context.h>
#include <cli/enum.h>
#include <cli/net_node_iface.h>
#include <net/error.h>
#include <net/icmp.h>
#include <net/igp_domain.h>
#include <net/net_types.h>
#include <net/netflow.h>
#include <net/node.h>
#include <net/spt.h>
#include <net/util.h>

#define DEBUG
#include <libgds/debug.h>

/////////////////////////////////////////////////////////////////////
//
// CONTEXT CREATION/DESTRUCTION
//
/////////////////////////////////////////////////////////////////////

// ----- cli_ctx_create_net_node ------------------------------------
static int cli_ctx_create_net_node(cli_ctx_t * ctx, cli_cmd_t * cmd,
				   void ** item)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  net_node_t * node;

  if (str2node_id(arg, &node)) {
    cli_set_user_error(cli_get(), "unable to find node \"%s\"", arg);
    return CLI_ERROR_CTX_CREATE;
  }
  *item= node;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_node -----------------------------------
static void cli_ctx_destroy_net_node(void ** item)
{
  // Nothing to do (context is only a reference)
}


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS
//
/////////////////////////////////////////////////////////////////////

// ----- cli_net_node_coord -----------------------------------------
/**
 * context: {node}
 * tokens: {latitude,longitude}
 */
static int cli_net_node_coord(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_lat= cli_get_arg_value(cmd, 0);
  const char * arg_long= cli_get_arg_value(cmd, 1);
  net_node_t * node= _node_from_context(ctx);
  double latitude, longitude;

  // Get latitude
  if (str_as_double(arg_lat, &latitude)) {
    cli_set_user_error(cli_get(), "invalid latitude \"%s\"", arg_lat);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get latitude
  if (str_as_double(arg_long, &longitude)) {
    cli_set_user_error(cli_get(), "invalid longitude \"%s\"", arg_long);
    return CLI_ERROR_COMMAND_FAILED;
  }

  node->coord.latitude= latitude;
  node->coord.longitude= longitude;

  return CLI_SUCCESS;
}

// ----- cli_net_node_domain ----------------------------------------
/**
 * context: {node}
 * tokens: {id}
 */
static int cli_net_node_domain(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  net_node_t * node= _node_from_context(ctx);
  unsigned int id;
  igp_domain_t * domain;

  // Get domain ID
  if (str2domain_id(arg, &id)) {
    cli_set_user_error(cli_get(), "invalid domain id \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get domain from ID. Check if domain exists...
  domain= network_find_igp_domain(network_get_default(), id);
  if (domain == NULL) {
    cli_set_user_error(cli_get(), "unknown domain \"%d\"", id);
    return CLI_ERROR_COMMAND_FAILED;    
  }
  
  // Check if domain already contains this node
  if (igp_domain_contains_router(domain, node)) {
    cli_set_user_error(cli_get(), "could not add to domain \"%d\"", id);
    return CLI_ERROR_COMMAND_FAILED;
  }

  igp_domain_add_router(domain, node);
  return CLI_SUCCESS;
}

// ----- cli_net_node_ipip_enable -----------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_ipip_enable(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  int result;

  // Enable IP-in-IP
  result= node_ipip_enable(node);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not enable ip-ip (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_node_name ------------------------------------------
/**
 * context: {node}
 * tokens: {string}
 */
static int cli_net_node_name(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  node_set_name(node, cli_get_arg_value(cmd, 0));
  return CLI_SUCCESS;
}

// ----- cli_net_node_ping ------------------------------------------
/**
 * context: {node}
 * tokens : {addr}
 * options: {--ttl=TTL}
 */
static int cli_net_node_ping(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  const char * opt;
  net_node_t * node= _node_from_context(ctx);
  net_addr_t dst_addr;
  int error;
  uint8_t ttl= 0;

  // Get destination address
  if (str2address(arg, &dst_addr) != 0) {
    cli_set_user_error(cli_get(), "invalid address \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Optional: TTL ?
  opt= cli_get_opt_value(cmd, "ttl");
  if (opt != NULL) {
    if (str2ttl(opt, &ttl)) {
      cli_set_user_error(cli_get(), "invalid TTL \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Perform ping (note that errors are ignored)
  error= icmp_ping(gdsout, node, NET_ADDR_ANY, dst_addr, ttl);
  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroute -----------------------------------
/**
 * context: {node}
 * tokens : {prefix|address|*}
 * options: [--capacity]
 *          [--check-loop]
 *          [--deflection]
 *          [--delay]
 *          [--ecmp]
 *          [--load=V]
 *          [--tos]
 *          [--ttl=]
 *          [--tunnel]
 *          [--weight]
 */
static int cli_net_node_recordroute(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  const char * opt;
  ip_dest_t dest;
  net_tos_t tos= 0;
  ip_opt_t * opts;
  net_link_load_t load= 0;
  uint8_t ttl= 255;

  opts= ip_options_create();

  // Optional capacity ?
  if (cli_has_opt_value(cmd, "capacity"))
    ip_options_set(opts, IP_OPT_CAPACITY);

  // Optional loop-check ?
  if (cli_has_opt_value(cmd, "check-loop"))
    ip_options_set(opts, IP_OPT_QUICK_LOOP);

  // Optional deflection-check ?
  if (cli_has_opt_value(cmd, "deflection"))
    ip_options_set(opts, IP_OPT_DEFLECTION);

  // Optional delay ?
  if (cli_has_opt_value(cmd, "delay"))
    ip_options_set(opts, IP_OPT_DELAY);

  // All equal-cost paths ?
  if (cli_has_opt_value(cmd, "ecmp"))
    ip_options_set(opts, IP_OPT_ECMP);

  // TTL value provided ?
  opt= cli_get_opt_value(cmd, "ttl");
  if (opt != NULL) {
    if (str2ttl(opt, &ttl)) {
      cli_set_user_error(cli_get(), "invalid TTL \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }    

  // Show path inside tunnels ?
  if (cli_has_opt_value(cmd, "tunnel"))
    ip_options_set(opts, IP_OPT_TUNNEL);

  // Load path with value ?
  opt= cli_get_opt_value(cmd, "load");
  if (opt != NULL) {
    if (str2capacity(opt, &load)) {
      cli_set_user_error(cli_get(), "invalid load \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
    ip_options_load(opts, load);
  }

  // Perform record-route in particular plane (based on TOS) ?
  opt= cli_get_opt_value(cmd, "tos");
  if (opt != NULL)
    if (str2tos(opt, &tos)) {
      cli_set_user_error(cli_get(), "invalid TOS \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }

  // Optional weight ?
  if (cli_has_opt_value(cmd, "weight"))
    ip_options_set(opts, IP_OPT_WEIGHT);

  // Get destination address
    if (ip_string_to_dest(arg, &dest)) {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

    // Check that the destination type is adress/prefix
  if ((dest.type != NET_DEST_ADDRESS) &&
      (dest.type != NET_DEST_PREFIX)) {
    cli_set_user_error(cli_get(), "can not use such destination");
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (dest.type == NET_DEST_PREFIX)
    ip_options_alt_dest(opts, dest.prefix);
  icmp_record_route(gdsout, node, IP_ADDR_ANY, dest, ttl, tos, opts);

  return CLI_SUCCESS;
}

// ----- cli_net_node_traceroute ------------------------------------
/**
 * context: {node}
 * tokens : {addr}
 */
static int cli_net_node_traceroute(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  net_node_t * node= _node_from_context(ctx);
  net_addr_t dst_addr;
  int error;

  // Get destination address
  if (str2address(arg, &dst_addr) != 0) {
    cli_set_user_error(cli_get(), "invalid address \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Perform traceroute (note that errors are ignored)
  error= icmp_trace_route(gdsout, node, NET_ADDR_ANY, dst_addr, 0, NULL);
  return CLI_SUCCESS;
}

// ----- cli_net_node_route_add -------------------------------------
/**
 * This function adds a new static route to the given node.
 *
 * context: {node}
 * tokens : {prefix, metric}
 * options: --gw=addr --oif=addr
 */
static int cli_net_node_route_add(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_pfx= cli_get_arg_value(cmd, 0);
  const char * arg_metric= cli_get_arg_value(cmd, 2);
  const char * opt;
  net_node_t * node= _node_from_context(ctx);
  ip_pfx_t prefix;
  net_addr_t gateway= NET_ADDR_ANY;
  unsigned long metric;
  net_iface_id_t oif= { .network= IP_ADDR_ANY, .mask= 0 };
  int result;

  // Get the route's prefix
  if (str2prefix(arg_pfx, &prefix) != 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg_pfx);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the gateway (optional)
  opt= cli_opts_get_value(cmd->opts, "gw");
  if (opt != NULL) {
    if (str2address(opt, &gateway) != 0) {
      cli_set_user_error(cli_get(), "invalid gateway \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get the outgoing interface identifier (optional)
  opt= cli_opts_get_value(cmd->opts, "oif");
  if (opt != NULL) {
    result= net_iface_str2id(opt, &oif);
    if (result != ESUCCESS) {
      cli_set_user_error(cli_get(), "invalid interface \"%s\" (%s)", opt,
			 network_strerror(result));
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get the metric
  if (str_as_ulong(arg_metric, &metric)) {
    cli_set_user_error(cli_get(), "invalid metric \"%s\"", arg_metric);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the route
  result= node_rt_add_route(node, prefix, oif, gateway,
			    metric, NET_ROUTE_STATIC);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add static route (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_node_route_del -------------------------------------
/**
 * This function removes a static route from the given node.
 *
 * context: {node}
 * tokens: {prefix, iface|*}
 * option: --gw=addr
 */
static int cli_net_node_route_del(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_pfx= cli_get_arg_value(cmd, 0);
  const char * arg_oif= cli_get_arg_value(cmd, 2);
  const char * opt;
  net_node_t * node= _node_from_context(ctx);
  ip_pfx_t prefix;
  net_addr_t gateway= NET_ADDR_ANY;
  net_iface_id_t oif;
  net_iface_id_t * oif_ref= &oif;
  int result;

  // Get the route's prefix
  if (str2prefix(arg_pfx, &prefix) != 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg_pfx);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get the gateway (optional)
  opt= cli_opts_get_value(cmd->opts, "gw");
  if (opt != NULL) {
    if (str2address(opt, &gateway) != 0) {
      cli_set_user_error(cli_get(), "invalid gateway \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get the outgoing interface
  if (!strcmp(arg_oif, "*")) {
    oif_ref= NULL;
  } else {
    result= net_iface_str2id(arg_oif, oif_ref);
    if (result != ESUCCESS) {
      cli_set_user_error(cli_get(), "invalid interface \"%s\" (%s)", arg_oif,
			 network_strerror(result));
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Remove the route
  if (node_rt_del_route(node, &prefix, oif_ref, &gateway,
			NET_ROUTE_STATIC)) {
    cli_set_user_error(cli_get(), "could not remove static route");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_ifaces -----------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_show_ifaces(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  node_ifaces_dump(gdsout, node);
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_info -------------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_show_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  node_info(gdsout, node);
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_links ------------------------------------
/**
 * context: {node}
 * tokens: {}
 */
static int cli_net_node_show_links(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  node_links_dump(gdsout, node);
  return CLI_SUCCESS;
}

// -----[ cli_net_node_show_spt ]------------------------------------
/**
 * context: {node}
 * tokens : {}
 */
static int cli_net_node_show_spt(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  gds_stream_t * stream= gdsout;

  if (node->spt == NULL) {
    cli_set_user_error(cli_get(), "no SPT stored for this node");
    return CLI_ERROR_CMD_FAILED;
  }

  if (cli_has_opt_value(cmd, "output")) {
    stream= stream_create_file(cli_get_opt_value(cmd, "output"));
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create output file");
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  spt_to_graphviz(stream, node->spt);

  if (stream != gdsout) {
    stream_destroy(&stream);
  }
  return CLI_SUCCESS;
}

// ----- cli_net_node_show_rt ---------------------------------------
/**
 * context: {node}
 * tokens: {prefix|address|*}
 */
static int cli_net_node_show_rt(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  ip_dest_t dest;

  // Get the prefix/address/*
  if (ip_string_to_dest(arg, &dest)) {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

#ifndef OSPF_SUPPORT
  // Dump routing table
  node_rt_dump(gdsout, node, dest);
#else
  // Dump OSPF routing table
  // Options: OSPF_RT_OPTION_SORT_AREA : dump routing table grouping routes by area
  ospf_node_rt_dump(gdsout, node, OSPF_RT_OPTION_SORT_AREA | OSPF_RT_OPTION_SORT_PATH_TYPE);
#endif
  return CLI_SUCCESS;
}

// ----- cli_net_node_tunnel_add ------------------------------------
/**
 * context: {node}
 * tokens: {end-point, if-addr}
 * option: oif=<addr>, src=<addr>
 */
static int cli_net_node_tunnel_add(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  const char * arg_ep= cli_get_arg_value(cmd, 0);
  const char * arg_tun= cli_get_arg_value(cmd, 1);
  const char * opt;
  net_addr_t end_point;
  net_addr_t addr;
  net_iface_id_t oif;
  net_iface_id_t * oif_ref= &oif;
  net_addr_t src_addr= 0;
  int result;

  // Get tunnel end-point (remote)
  if (str2address(arg_ep, &end_point) != 0) {
    cli_set_user_error(cli_get(), "invalid end-point address \"%s\"", arg_ep);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get tunnel identifier: address (local)
  if (str2address(arg_tun, &addr) != 0) {
    cli_set_user_error(cli_get(), "invalid local address \"%s\"", arg_tun);
    return CLI_ERROR_COMMAND_FAILED;    
  }

  // get optional outgoing interface
  opt= cli_get_opt_value(cmd, "oif");
  if (opt != NULL) {
    result= net_iface_str2id(opt, oif_ref);
    if (result != ESUCCESS) {
      cli_set_user_error(cli_get(), "invalid outgoing interface \"%s\" (%s)",
			 opt, network_strerror(result));
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else
   oif_ref= NULL;

  // get optional source address
  opt= cli_opts_get_value(cmd->opts, "src");
  if (opt != NULL) {
    if (str2address(opt, &src_addr) < 0) {
      cli_set_user_error(cli_get(), "invalid source address \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Add tunnel
  result= node_add_tunnel(node, end_point, addr, oif_ref, src_addr);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add tunnel (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_net_node_traffic_load ] -------------------------------
/**
 * context: {node}
 * tokens: {<filename>}
 * option: --summary, --details
 */
static int cli_net_node_traffic_load(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  int result;
  uint8_t options= 0;
  flow_stats_t stats;

  // Get option "--details" ?
  if (cli_has_opt_value(cmd, "details"))
    options|= NET_NODE_NETFLOW_OPTIONS_DETAILS;

  // Load Netflow from file
  flow_stats_init(&stats);
  result= node_load_netflow(node, arg, options, &stats);
  if (result != NETFLOW_SUCCESS) {
    cli_set_user_error(cli_get(), "could not load traffic (%s)",
		       netflow_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get option "--summary" ?
  if (cli_has_opt_value(cmd, "summary"))
    flow_stats_dump(gdsout, &stats);

  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI REGISTRATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _register_net_node_add ]-----------------------------------
static void _register_net_node_add(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("add"));
  cli_register_net_node_add_iface(group);
}

// -----[ _register_net_node_traffic ]-------------------------------
static void _register_net_node_traffic(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  group= cli_add_cmd(parent, cli_cmd_group("traffic"));
  cmd= cli_add_cmd(group, cli_cmd("load", cli_net_node_traffic_load));
  cli_add_arg(cmd, cli_arg_file("filename", NULL));
  cli_add_opt(cmd, cli_opt("details", NULL));
  cli_add_opt(cmd, cli_opt("summary", NULL));
}

// -----[ _register_net_node_show ]----------------------------------
static void _register_net_node_show(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  group= cli_add_cmd(parent, cli_cmd_group("show"));
  cmd= cli_add_cmd(group, cli_cmd("ifaces", cli_net_node_show_ifaces));
  cmd= cli_add_cmd(group, cli_cmd("info", cli_net_node_show_info));
  cmd= cli_add_cmd(group, cli_cmd("links", cli_net_node_show_links));
  cmd= cli_cmd("spt", cli_net_node_show_spt);
  cli_add_opt(cmd, cli_opt("output=", NULL));
  cmd= cli_add_cmd(group, cmd);
  cmd= cli_add_cmd(group, cli_cmd("rt", cli_net_node_show_rt));
  cli_add_arg(cmd, cli_arg("prefix|address|*", NULL));
}

// -----[ _register_net_node_route ]---------------------------------
static void _register_net_node_route(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  group= cli_add_cmd(parent, cli_cmd_group("route"));
  cmd= cli_add_cmd(group, cli_cmd("add", cli_net_node_route_add));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_arg(cmd, cli_arg("weight", NULL));
  cli_add_opt(cmd, cli_opt("gw=", NULL));
  cli_add_opt(cmd, cli_opt("oif=", NULL));
  cmd= cli_add_cmd(group, cli_cmd("del", cli_net_node_route_del));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_arg(cmd, cli_arg("iface", NULL));
  cli_add_opt(cmd, cli_opt("gw=", NULL));
}

// -----[ _register_net_node_tunnel ]--------------------------------
static void _register_net_node_tunnel(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  group= cli_add_cmd(parent, cli_cmd_group("tunnel"));
  cmd= cli_add_cmd(group, cli_cmd("add", cli_net_node_tunnel_add));
  cli_add_arg(cmd, cli_arg("end-point", NULL));
  cli_add_arg(cmd, cli_arg("addr", NULL));
  cli_add_opt(cmd, cli_opt("oif", NULL));
  cli_add_opt(cmd, cli_opt("src", NULL));
}

// -----[ cli_register_net_node ]------------------------------------
void cli_register_net_node(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  group= cli_add_cmd(parent, cli_cmd_ctx("node",
					 cli_ctx_create_net_node,
					 cli_ctx_destroy_net_node));
  cli_add_arg(group, cli_arg2("addr", NULL, cli_enum_net_nodes_addr_id));

  _register_net_node_add(group);  
  cli_register_net_node_iface(group);
  _register_net_node_route(group);
  cmd= cli_add_cmd(group, cli_cmd("coord", cli_net_node_coord));
  cli_add_arg(cmd, cli_arg("latitude", NULL));
  cli_add_arg(cmd, cli_arg("longitude", NULL));
  cmd= cli_add_cmd(group, cli_cmd("domain", cli_net_node_domain));
  cli_add_arg(cmd, cli_arg("id", NULL));
  cmd= cli_add_cmd(group, cli_cmd("ipip-enable", cli_net_node_ipip_enable));
  cmd= cli_add_cmd(group, cli_cmd("name", cli_net_node_name));
  cli_add_arg(cmd, cli_arg("name", NULL));
  cmd= cli_add_cmd(group, cli_cmd("ping", cli_net_node_ping));
  cli_add_arg(cmd, cli_arg2("addr", NULL, cli_enum_net_nodes_addr_id));
  cli_add_opt(cmd, cli_opt("ttl=", NULL));
  cmd= cli_add_cmd(group, cli_cmd("record-route", cli_net_node_recordroute));
  cli_add_arg(cmd, cli_arg("address|prefix", NULL));
  cli_add_opt(cmd, cli_opt("capacity", NULL));
  cli_add_opt(cmd, cli_opt("check-loop", NULL));
  cli_add_opt(cmd, cli_opt("deflection", NULL));
  cli_add_opt(cmd, cli_opt("delay", NULL));
  cli_add_opt(cmd, cli_opt("ecmp", NULL));
  cli_add_opt(cmd, cli_opt("load=", NULL));
  cli_add_opt(cmd, cli_opt("ttl=", NULL));
  cli_add_opt(cmd, cli_opt("tos=", NULL));
  cli_add_opt(cmd, cli_opt("tunnel", NULL));
  cli_add_opt(cmd, cli_opt("weight", NULL));
  cmd= cli_add_cmd(group, cli_cmd("traceroute", cli_net_node_traceroute));
  cli_add_arg(cmd, cli_arg2("addr", NULL, cli_enum_net_nodes_addr_id));

  _register_net_node_show(group);
  _register_net_node_traffic(group);
  _register_net_node_tunnel(group);
#ifdef OSPF_SUPPORT
  cli_register_net_node_ospf(group);
#endif /* OSPF_SUPPORT */
}
