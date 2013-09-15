// ==================================================================
// @(#)net.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 15/07/2003
// $Id: net.c,v 1.40 2009-08-31 09:37:56 bqu Exp $
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
#include <errno.h>
#include <string.h>

#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/stream.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <cli/context.h>
#include <cli/enum.h>
#include <cli/net.h>
#include <cli/net_domain.h>
#include <cli/net_node.h>
#include <cli/net_ospf.h>
#include <net/error.h>
#include <net/export.h>
#include <net/netflow.h>
#include <net/node.h>
#include <net/ntf.h>
#include <net/prefix.h>
#include <net/subnet.h>
#include <net/igp.h>
#include <net/igp_domain.h>
#include <net/network.h>
#include <net/ospf.h>
#include <net/ospf_rt.h>
#include <net/tm.h>
#include <net/util.h>
#include <ui/rl.h>

#include <bgp/aslevel/types.h>
#include <bgp/aslevel/as-level.h>


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS
//
/////////////////////////////////////////////////////////////////////

// ----- cli_net_add_node -------------------------------------------
/**
 * context: {}
 * tokens : {addr}
 * options: {no-loopback}
 */
int cli_net_add_node(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_addr_t addr;
  net_node_t * node;
  int error;
  int options= 0; // default is: create loopback with address = node ID

  // Node address ?
  if (str2address(cli_get_arg_value(cmd, 0), &addr)) {
    cli_set_user_error(cli_get(), "could not add node (invalid address)");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Option: no-loopback ?
  if (!cli_opts_has_value(cmd->opts, "no-loopback")) {
    options|= NODE_OPTIONS_LOOPBACK;
  }

  // Create new node
  error= node_create(addr, &node, options);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add node (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;    
  }

  // Add node
  error= network_add_node(network_get_default(), node);
  if (error != ESUCCESS) {
    node_destroy(&node);
    cli_set_user_error(cli_get(), "could not add node (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_net_add_subnet -------------------------------------------
/**
 * context: {}
 * tokens: {prefix, type}
 */
int cli_net_add_subnet(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  ip_pfx_t prefix;
  const char * arg;
  uint8_t type;
  net_subnet_t * subnet;
  int result;

  // Subnet prefix
  arg= cli_get_arg_value(cmd, 0);
  if ((str2prefix(arg, &prefix))) {
    cli_set_user_error(cli_get(), "could not add subnet (invalid prefix)");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check the prefix length (!= 32)
  // THE CHECK FOR PREFIX LENGTH SHOULD BE MOVED TO THE NETWORK MGMT PART.
  if (prefix.mask == 32) {
    cli_set_user_error(cli_get(), "could not add subnet (invalid subnet)");
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Subnet type: transit / stub ?
  arg= cli_get_arg_value(cmd, 1);
  if (!strcmp(arg, "transit"))
    type= NET_SUBNET_TYPE_TRANSIT;
  else if (!strcmp(arg, "stub"))
    type = NET_SUBNET_TYPE_STUB;
  else {
    cli_set_user_error(cli_get(), "invalid subnet type \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Create new subnet
  subnet= subnet_create(prefix.network, prefix.mask, type);

  // Add the subnet
  result= network_add_subnet(network_get_default(), subnet);
  if (result != ESUCCESS) {
    subnet_destroy(&subnet);
    cli_set_user_error(cli_get(), "could not add subnet (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_add_link -------------------------------------------
/**
 * context: {}
 * tokens: {addr-src, prefix-dst}
 * options: [--depth=, --capacity=, delay=]
 */
int cli_net_add_link(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_src= cli_get_arg_value(cmd, 0);
  const char * arg_dst= cli_get_arg_value(cmd, 1);
  const char * opt;
  net_node_t * src_node;
  net_node_t * dst_node;
  ip_dest_t dest;
  int result;
  uint8_t depth= 1;
  net_link_delay_t delay= 0;
  net_link_load_t capacity= 0;
  net_iface_dir_t dir;
  net_iface_t * iface;
  net_subnet_t * subnet;

  // Get optional link depth
  opt= cli_get_opt_value(cmd, "depth");
  if (opt != NULL) {
    if (str2depth(opt, &depth)) {
      cli_set_user_error(cli_get(), "invalid link depth \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }
  
  // Get optional link capacity
  opt= cli_get_opt_value(cmd, "bw");
  if (opt != NULL) {
    if (str2capacity(opt, &capacity)) {
      cli_set_user_error(cli_get(), "invalid link capacity \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get optional link delay
  opt= cli_get_opt_value(cmd, "delay");
  if (opt != NULL) {
    if (str2delay(opt, &delay)) {
      cli_set_user_error(cli_get(), "invalid link delay \"%s\"", opt);
      return CLI_ERROR_CMD_FAILED;
    }
  }

  // Get source node
  if (str2node(arg_src, &src_node)) {
    cli_set_user_error(cli_get(), "could not find node \"%s\"", arg_src);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get destination: can be a node / a subnet
  if ((ip_string_to_dest(arg_dst, &dest) < 0) ||
      (dest.type == NET_DEST_ANY)) {
    cli_set_user_error(cli_get(), "invalid destination \"%s\".", arg_dst);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Add link: RTR (to node) / PTMP (to subnet)
  if (dest.type == NET_DEST_ADDRESS) {
    dst_node= network_find_node(network_get_default(), dest.addr);
    if (dst_node == NULL) {
      cli_set_user_error(cli_get(), "tail-end \"%s\" does not exist.",
			 arg_dst);
      return CLI_ERROR_COMMAND_FAILED;
    }
    dir= BIDIR;
    result= net_link_create_rtr(src_node, dst_node, dir, &iface);
  } else {
    subnet= network_find_subnet(network_get_default(), dest.prefix);
    if (subnet == NULL) {
      cli_set_user_error(cli_get(), "tail-end \"%s\" does not exist.",
			 arg_dst);
      return CLI_ERROR_COMMAND_FAILED;
    }
    dir= UNIDIR;
    result= net_link_create_ptmp(src_node, subnet,
				 dest.prefix.network,
				 &iface);
  }
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add link %s -> %s (%s)",
		       arg_src, arg_dst, network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  result= net_link_set_phys_attr(iface, delay, capacity, dir);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not set physical attributes (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }  

  return CLI_SUCCESS;
}

// -----[ cli_net_add_linkptp ]--------------------------------------
/**
 * context: {}
 * tokens: {src-addr, src-iface-id, dst-addr, dst-iface-id}
 */
int cli_net_add_linkptp(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_src   = cli_get_arg_value(cmd, 0);
  const char * arg_src_if= cli_get_arg_value(cmd, 1);
  const char * arg_dst   = cli_get_arg_value(cmd, 2);
  const char * arg_dst_if= cli_get_arg_value(cmd, 3);
  const char * opt;
  net_node_t * src_node;
  net_node_t * dst_node;
  net_iface_id_t src_if;
  net_iface_id_t dst_if;
  int result;
  net_iface_t * iface;
  net_link_delay_t delay= 0;
  net_link_load_t capacity= 0;

  // Get source node
  if (str2node(arg_src, &src_node)) {
    cli_set_user_error(cli_get(), "could not find node \"%s\"", arg_src);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get source address
  if (net_iface_str2id(arg_src_if, &src_if)) {
    cli_set_user_error(cli_get(), "invalid interface id \"%\"", arg_src_if);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get destination node
  if (str2node(arg_dst, &dst_node)) {
    cli_set_user_error(cli_get(), "could not find node \"%s\"", arg_dst);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // get destination address
  if (net_iface_str2id(arg_dst_if, &dst_if)) {
    cli_set_user_error(cli_get(), "invalid interface id \"%\"", arg_dst_if);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get optional link capacity
  opt= cli_get_opt_value(cmd, "bw");
  if (opt != NULL) {
    if (str2capacity(opt, &capacity)) {
      cli_set_user_error(cli_get(), "invalid link capacity \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get optional link delay
  opt= cli_get_opt_value(cmd, "delay");
  if (opt != NULL) {
    if (str2delay(opt, &delay)) {
      cli_set_user_error(cli_get(), "invalid link delay \"%s\"", opt);
      return CLI_ERROR_CMD_FAILED;
    }
  }
  
  // Create link
  result= net_link_create_ptp(src_node, src_if, dst_node, dst_if,
			      BIDIR, &iface);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not create ptp link "
		       "from %s (%s) to %s (%s) (%s)",
		       arg_src, arg_src_if, arg_dst, arg_dst_if,
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  result= net_link_set_phys_attr(iface, delay, capacity, BIDIR);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not set physical attributes (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}


// ----- cli_ctx_create_net_link ------------------------------------
/**
 * context: {}
 * tokens: {src-addr, dst-addr}
 */
int cli_ctx_create_net_link(cli_ctx_t * ctx, cli_cmd_t * cmd, void ** item)
{
  const char * arg_src= cli_get_arg_value(cmd, 0);
  const char * arg_dst= cli_get_arg_value(cmd, 1);
  net_node_t * node;
  ip_dest_t dest;
  net_iface_t * iface = NULL;

  if (str2node_id(arg_src, &node)) {
    cli_set_user_error(cli_get(), "unable to find node \"%s\"", arg_src);
    return CLI_ERROR_CTX_CREATE;
  }

  if (str2dest_id(arg_dst, &dest) < 0 ||
      dest.type == NET_DEST_ANY) {
    cli_set_user_error(cli_get(), "destination id is wrong \"%s\"", arg_dst);
    return CLI_ERROR_CTX_CREATE;
  }

  iface= node_find_iface(node, net_dest_to_prefix(dest));
  if (iface == NULL) {
    cli_set_user_error(cli_get(), "unable to find link %s -> %s",
		       arg_src, arg_dst);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *item= iface;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_link -----------------------------------
void cli_ctx_destroy_net_link(void ** item)
{
}

// -----[ cli_net_export ]-------------------------------------------
/**
 * Context: {}
 * tokens: {}
 * options: {-format=<format>, -output=file}
 */
int cli_net_export(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  char * filename;
  net_export_format_t format= NET_EXPORT_FORMAT_CLI;
  char * option;
  net_error_t result;

  // Get optional export format
  option= cli_opts_get_value(cmd->opts, "format");
  if (option != NULL) {
    if (net_export_str2format(option, &format) != ESUCCESS) {
      cli_set_user_error(cli_get(), "invalid export format \"%s\"", option);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get name of the output file
  filename= cli_opts_get_value(cmd->opts, "output");
  if (filename != NULL) {
    result= net_export_file(filename, network_get_default(), format);
    if (result != ESUCCESS) {
      cli_set_user_error(cli_get(), "could not export to \"%s\"", filename);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    result= net_export_stream(gdsout, network_get_default(), format);
    if (result != ESUCCESS) {
      cli_set_user_error(cli_get(), "could not export");
      return CLI_ERROR_COMMAND_FAILED;
    }
  }
  

  return CLI_SUCCESS;
}

// ----- cli_net_ntf_load -------------------------------------------
/**
 * context: {}
 * tokens: {ntf_file}
 */
int cli_net_ntf_load(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);

  // Load given NTF file
  if (ntf_load(arg) < 0) {
    cli_set_user_error(cli_get(), "could not load NTF file \"%s\" (%s%s)",
		       arg, ntf_strerror(), ntf_strerrorloc());
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_link_up --------------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_up(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * link= _link_from_context(ctx);
  net_iface_set_enabled(link, 1);
  return CLI_SUCCESS;
}

// ----- cli_net_link_down ------------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_down(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * link= _link_from_context(ctx);
  net_iface_set_enabled(link, 0);
  return CLI_SUCCESS;
}

// ----- cli_net_link_ipprefix -------------------------------------
/**
 * context: {link}
 * tokens: {iface-prefix}
 */
/* COMMENT ON 17/01/2007
int cli_net_link_ipprefix(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * pLink;
  char * pcIfaceAddr;
  char * pcEndChar;
  ip_pfx_t sIfacePrefix;
  
  pLink= (net_iface_t *) cli_context_get_item_at_top(ctx);
  if (pLink == NULL)
    return CLI_ERROR_COMMAND_FAILED;

  if (!link_is_to_router(pLink))
    return CLI_ERROR_COMMAND_FAILED;
  
  // Get src prefix
  pcIfaceAddr= tokens_get_string_at(ctx->cmd->arg_values, 0);
  if (ip_string_to_prefix(pcIfaceAddr, &pcEndChar, &sIfacePrefix) ||
      (*pcEndChar != 0)) {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: invalid prefix \"%s\"\n",
	       pcIfaceAddr);
    return CLI_ERROR_COMMAND_FAILED;
  }
  

  link_set_ip_prefix(pLink, sIfacePrefix);
  return CLI_SUCCESS;
  }*/

// ----- cli_net_link_igpweight -------------------------------------
/**
 * context: {link}
 * tokens : {igp-weight}
 * options: {--bidir --tos=<>}
 */
int cli_net_link_igpweight(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * link= _link_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  const char * opt;
  igp_weight_t weight;
  net_tos_t tos= 0;
  int bidir= 0;
  int result;

  //cli_args_dump(gdsout, cmd->opts);

  // Get optional TOS
  opt= cli_get_opt_value(cmd, "tos");
  if (opt != NULL)
    if (!str2tos(opt, &tos)) {
      cli_set_user_error(cli_get(), "invalid TOS \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }

  // Check option --bidir
  if (cli_has_opt_value(cmd, "bidir")) {
    if (link->type != NET_IFACE_RTR) {
      cli_set_user_error(cli_get(), ": --bidir only works with ptp links");
      return CLI_ERROR_COMMAND_FAILED;
    }
    bidir= 1;
  }

  // Get new IGP weight
  if (str2weight(arg, &weight)) {
    cli_set_user_error(cli_get(), "invalid weight \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Change link weight
  result= net_iface_set_metric(link, tos, weight, bidir);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "cannot set metric (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_net_link_show_info -------------------------------------
/**
 * context: {link}
 * tokens: {}
 */
int cli_net_link_show_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * pLink= _link_from_context(ctx);

  net_link_dump_info(gdsout, pLink);

  return CLI_SUCCESS;
}

// ----- cli_net_links_load_clear -----------------------------------
/**
 * context: {}
 * tokens: {}
 */
int cli_net_links_load_clear(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  network_ifaces_load_clear(network_get_default());
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_subnet ----------------------------------
int cli_ctx_create_net_subnet(cli_ctx_t * ctx, cli_cmd_t * cmd, void ** item)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  net_subnet_t * subnet;

  if (str2subnet(arg, &subnet)) {
    cli_set_user_error(cli_get(), "unable to find subnet \"%s\"", arg);
    return CLI_ERROR_CTX_CREATE;
  }
  *item= subnet;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_subnet -----------------------------------
void cli_ctx_destroy_net_subnet(void ** item)
{
}

// -----[ cli_net_subnet_show_info ]---------------------------------
int cli_net_subnet_show_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_subnet_t * subnet= _subnet_from_context(ctx);
  subnet_info(gdsout, subnet);
  return CLI_SUCCESS;
}

// -----[ cli_net_subnet_show_links ]--------------------------------
int cli_net_subnet_show_links(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_subnet_t * subnet= _subnet_from_context(ctx);
  subnet_links_dump(gdsout, subnet);
  return CLI_SUCCESS;
}

// ----- cli_net_show_nodes -----------------------------------------
/**
 * Display all nodes matching the given criterion, which is a prefix
 * or an asterisk (meaning "all nodes").
 *
 * context: {}
 * tokens: {prefix}
 */
int cli_net_show_nodes(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  network_to_file(gdsout, network_get_default());
  return CLI_SUCCESS;
}

// ----- cli_net_show_subnets ---------------------------------------
/**
 * Display all subnets matching the given criterion, which is a
 * prefix or an asterisk (meaning "all subnets").
 *
 * context: {}
 * tokens: {}
 */
int cli_net_show_subnets(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  network_dump_subnets(gdsout, network_get_default());
  return CLI_SUCCESS;
}

static int _net_flow_src_ip_handler(flow_t * flow, flow_field_map_t * map,
				    void * ctx)
{
  flow_stats_t * stats= (flow_stats_t *) ctx;
  ip_trace_t * trace= NULL;
  net_error_t result;
  ip_opt_t opts;

  net_node_t * src_node= network_find_node(network_get_default(), flow->src_addr);
  if (src_node == NULL) {
    printf("Source node not found\n");
    return -1;
  }

  ip_options_init(&opts);
  if (flow_field_map_isset(map, FLOW_FIELD_DST_MASK)) {
    ip_pfx_t pfx= {
      .network=flow->dst_addr,
      .mask=flow->dst_mask,
    };
    ip_options_alt_dest(&opts, pfx);
  }

  result= node_load_flow(src_node, NET_ADDR_ANY, flow->dst_addr, flow->bytes,
			 stats, &trace, &opts);
  if (result < 0)
    return 0;

  if (trace != NULL) {
    stream_printf(gdsout, "TRACE=[");
    ip_trace_dump(gdsout, trace, 0);
    stream_printf(gdsout, "]\n");
    ip_trace_destroy(&trace);
  }
  return 0;
}

static int _net_flow_src_asn_handler(flow_t * flow, flow_field_map_t * map,
				     void * ctx)
{
  // Find source based on source ASN
  as_level_topo_t * topo= aslevel_get_topo();
  if (topo == NULL) {
    printf("no AS-level topology loaded\n");
    return -1;
  }
  flow->src_addr= topo->addr_mapper(flow->src_asn);
  return _net_flow_src_ip_handler(flow, map, ctx);
}

// ----- cli_net_traffic_load ---------------------------------------
/**
 * context: {}
 * tokens : {file}
 * options: {--src=ip|asn, --dst=ip|pfx, --summary}
 */
int cli_net_traffic_load(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  const char * opt;
  int result;
  flow_handler_f handler= _net_flow_src_ip_handler;
  flow_field_map_t map;
  flow_stats_t stats;

  flow_stats_init(&stats);

  flow_field_map_init(&map);
  flow_field_map_set(&map, FLOW_FIELD_OCTETS);
  flow_field_map_set(&map, FLOW_FIELD_DST_IP);

  // Get option "src"
  if (cli_has_opt_value(cmd, "src")) {
    opt= cli_get_opt_value(cmd, "src");
    if (!strcmp(opt, "ip")) {
      flow_field_map_set(&map, FLOW_FIELD_SRC_IP);
      handler= _net_flow_src_ip_handler;
    } else if (!strcmp(opt, "asn")) {
      flow_field_map_set(&map, FLOW_FIELD_SRC_ASN);
      handler= _net_flow_src_asn_handler;
    } else {
      cli_set_user_error(cli_get(), "invalid source type \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    flow_field_map_set(&map, FLOW_FIELD_SRC_IP);
  }

  // Get option "dst"
  if (cli_has_opt_value(cmd, "dst")) {
    opt= cli_get_opt_value(cmd, "dst");
    if (!strcmp(opt, "ip")) {
    } else if (!strcmp(opt, "pfx")) {
      flow_field_map_set(&map, FLOW_FIELD_DST_MASK);
    } else {
      cli_set_user_error(cli_get(), "invalid destination type \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Load flows
  result= netflow_load(arg, &map, handler, &stats);
  if (result != 0) {
    cli_set_user_error(cli_get(), "could not load traffic matrix \"%s\" (%s)",
		       arg, netflow_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get option "--summary" ?
  if (cli_has_opt_value(cmd, "summary"))
    flow_stats_dump(gdsout, &stats);

  return CLI_SUCCESS;
}

// ----- cli_net_traffic_save ---------------------------------------
/**
 * context: {}
 * tokens : {file}
 */
int cli_net_traffic_save(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  gds_stream_t * stream;
  int result;

  // Get the optional parameter
  stream= stream_create_file(arg);
  if (stream == NULL) {
    cli_set_user_error(cli_get(), "could not create \"%d\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Save traffic matrix
  result= network_links_save(stream, network_get_default());
  if (result != 0) {
    cli_set_user_error(cli_get(), "could not save traffic matrix (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS REGISTRATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _register_net_add ]----------------------------------------
static void _register_net_add(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("add"));
  cmd= cli_add_cmd(group, cli_cmd("domain", cli_net_add_domain));
  cli_add_arg(cmd, cli_arg("id", NULL));
  cli_add_arg(cmd, cli_arg("type", NULL));
  cmd= cli_add_cmd(group, cli_cmd("node", cli_net_add_node));
  cli_add_arg(cmd, cli_arg("addr", NULL));
  cli_add_opt(cmd, cli_opt("no-loopback", NULL));
  cmd= cli_add_cmd(group, cli_cmd("subnet", cli_net_add_subnet));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_arg(cmd, cli_arg("transit|stub", NULL));
  cmd= cli_add_cmd(group, cli_cmd("link", cli_net_add_link));
  cli_add_arg(cmd, cli_arg2("addr-src", NULL, cli_enum_net_nodes_addr));
  cli_add_arg(cmd, cli_arg2("addr-dst", NULL, cli_enum_net_nodes_addr));
  cli_add_opt(cmd, cli_opt("bw=", NULL));
  cli_add_opt(cmd, cli_opt("delay=", NULL));
  cli_add_opt(cmd, cli_opt("depth=", NULL));

  cmd= cli_add_cmd(group, cli_cmd("link-ptp", cli_net_add_linkptp));
  cli_add_arg(cmd, cli_arg2("addr-src", NULL, cli_enum_net_nodes_addr));
  cli_add_arg(cmd, cli_arg("iface-id", NULL));
  cli_add_arg(cmd, cli_arg2("addr-dst", NULL, cli_enum_net_nodes_addr));
  cli_add_arg(cmd, cli_arg("iface-id", NULL));
  cli_add_opt(cmd, cli_opt("bw=", NULL));
  cli_add_opt(cmd, cli_opt("delay=", NULL));
  cli_add_opt(cmd, cli_opt("depth=", NULL));
}

// -----[ _register_net_export ]-------------------------------------
static void _register_net_export(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("export", cli_net_export));
  cli_add_opt(cmd, cli_opt("format=", NULL));
  cli_add_opt(cmd, cli_opt("output=", NULL));
}

// -----[ _register_net_link_show ]----------------------------------
static void cli_register_net_link_show(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_prefix("show"));
  cli_add_cmd(group, cli_cmd("info", cli_net_link_show_info));
}

// -----[ _register_net_link ]---------------------------------------
static void _register_net_link(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_ctx("link",
					 cli_ctx_create_net_link,
					 cli_ctx_destroy_net_link));
  cli_add_arg(group, cli_arg2("addr-src", NULL, cli_enum_net_nodes_addr));
  cli_add_arg(group, cli_arg2("addr-dst", NULL, cli_enum_net_nodes_addr));
  cmd= cli_add_cmd(group, cli_cmd("down", cli_net_link_down));
  cmd= cli_add_cmd(group, cli_cmd("up", cli_net_link_up));
  cmd= cli_add_cmd(group, cli_cmd("igp-weight", cli_net_link_igpweight));
  cli_add_arg(cmd, cli_arg("weight", NULL));
  cli_add_opt(cmd, cli_opt("bidir", NULL));
  cli_add_opt(cmd, cli_opt("tos", NULL));
  cli_register_net_link_show(group);
}

// -----[ _register_net_links ]--------------------------------------
static void _register_net_links(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("links"));
  cli_add_cmd(group, cli_cmd("load-clear", cli_net_links_load_clear));
}

// -----[ _register_net_ntf ]----------------------------------------
static void _register_net_ntf(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("ntf"));
  cli_cmd_t * cmd= cli_add_cmd(group, cli_cmd("load", cli_net_ntf_load));
  cli_add_arg(cmd, cli_arg_file("filename", NULL));
}

// -----[ _register_net_subnet_show ]--------------------------------
static void  _register_net_subnet_show(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("show"));
  cli_cmd_t * cmd= cli_add_cmd(group, cli_cmd("info",
					      cli_net_subnet_show_info));
  cmd= cli_add_cmd(group, cli_cmd("links", cli_net_subnet_show_links));
}

// -----[ _register_net_subnet ]-------------------------------------
static void _register_net_subnet(cli_cmd_t * parent)
{
  cli_cmd_t * group;
  
  group= cli_add_cmd(parent, cli_cmd_ctx("subnet",
					 cli_ctx_create_net_subnet,
					 cli_ctx_destroy_net_subnet));
  cli_add_arg(group, cli_arg("prefix", NULL));
#ifdef OSPF_SUPPORT  
  cli_register_net_subnet_ospf(group);
#endif /* OSPF_SUPPORT */
  _register_net_subnet_show(group);
}

// -----[ _register_net_show ]---------------------------------------
static void _register_net_show(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_prefix("show"));
  cmd= cli_add_cmd(group, cli_cmd("nodes", cli_net_show_nodes));
  cli_add_arg(cmd, cli_arg("prefix|*", NULL));
  cmd= cli_add_cmd(group, cli_cmd("subnets", cli_net_show_subnets));
}

// -----[ _register_net_traffic ]------------------------------------
static void _register_net_traffic(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("traffic"));
  cmd= cli_add_cmd(group, cli_cmd("load", cli_net_traffic_load));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
  cli_add_opt(cmd, cli_opt("src=", NULL));
  cli_add_opt(cmd, cli_opt("dst=", NULL));
  cli_add_opt(cmd, cli_opt("summary", NULL));
  cmd= cli_add_cmd(group, cli_cmd("save", cli_net_traffic_save));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
}

// -----[ cli_register_net ]-----------------------------------------
void cli_register_net(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("net"));
  _register_net_add(group);
  cli_register_net_domain(group);
  _register_net_export(group);
  _register_net_link(group);
  _register_net_links(group);
  _register_net_ntf(group);
  cli_register_net_node(group);
  _register_net_subnet(group);
  _register_net_show(group);
  _register_net_traffic(group);
//#ifdef OSPF_SUPPORT
//  cli_register_net_ospf(group);
//#endif
}
