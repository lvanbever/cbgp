// ==================================================================
// @(#)bgp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 15/07/2003
// $Id: bgp.c,v 1.60 2009-08-31 09:37:26 bqu Exp $
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

#include <bgp/as.h>
#include <bgp/attr/path.h>
#include <bgp/bgp_assert.h>
#include <bgp/bgp_debug.h>
#include <bgp/domain.h>
#include <bgp/dp_rules.h>
#include <bgp/filter/filter.h>
#include <bgp/filter/parser.h>
#include <bgp/filter/predicate_parser.h>
#include <bgp/message.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/peer-list.h>
#include <bgp/qos.h>
#include <bgp/record-route.h>
#include <bgp/route.h>
#include <bgp/route_map.h>
#include <bgp/tie_breaks.h>

#include <cli/bgp.h>
#include <cli/bgp_filter.h>
#include <cli/bgp_router_peer.h>
#include <cli/bgp_topology.h>
#include <cli/common.h>
#include <cli/context.h>
#include <cli/enum.h>
#include <cli/net.h>
#include <ui/rl.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/stream.h>
#include <libgds/memory.h>
#include <libgds/str_util.h>
#include <net/error.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/util.h>
#include <string.h>
#include <bgp/aslevel/as-level.h>

// ----- cli_bgp_add_router -----------------------------------------
/**
 * This function adds a BGP protocol instance to the given node
 * (identified by its address).
 *
 * context: {}
 * tokens: {addr, as-num}
 */
int cli_bgp_add_router(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_asn = cli_get_arg_value(cmd, 0);
  const char * arg_addr= cli_get_arg_value(cmd, 1);
  net_node_t * node;
  asn_t asn;
  net_error_t error;

  // Check AS-Number
  if (str2asn(arg_asn, &asn)) {
    cli_set_user_error(cli_get(), "invalid AS number (%u)", arg_asn);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Find node
  if (str2node(arg_addr, &node)) {
    cli_set_user_error(cli_get(), "invalid node address \"%s\"", arg_addr);
    return CLI_ERROR_COMMAND_FAILED;
  }

  error= bgp_add_router(asn, node, NULL);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add BGP router (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_assert_peerings ------------------------------------
/**
 * This function checks that all the BGP peerings defined in BGP
 * instances are valid, i.e. the peer exists and is reachable.
 */
int cli_bgp_assert_peerings(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  // Test the validity of peerings in all BGP instances
  if (bgp_assert_peerings()) {
    cli_set_user_error(cli_get(), "peerings assertion failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_assert_reachability --------------------------------
/**
 * This function checks that all the BGP instances have at leat one
 * BGP route towards all the advertised networks.
 *
 * context: {}
 * tokens: {}
 */
int cli_bgp_assert_reachability(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  // Test the reachability of all advertised networks from all BGP
  // instances
  if (bgp_assert_reachability()) {
    cli_set_user_error(cli_get(), "reachability assertion failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_domain ----------------------------------
/**
 * Create a context for the given BGP domain.
 *
 * context: {}
 * tokens: {asn}
 */
int cli_ctx_create_bgp_domain(cli_ctx_t * ctx, cli_cmd_t * cmd, void ** item)
{
  asn_t asn;

  /* Get BGP domain from the top of the context */
  if (str2asn(cli_get_arg_value(cmd, 0), &asn)) {
    cli_set_user_error(cli_get(), "invalid AS number \"%s\"",
		       cli_get_arg_value(cmd, 0));
    return CLI_ERROR_CTX_CREATE;
  }

  /* Check that the BGP domain exists */
  if (!exists_bgp_domain((uint16_t) asn)) {
    cli_set_user_error(cli_get(), "domain \"%u\" does not exist.", asn);
    return CLI_ERROR_CTX_CREATE;
  }

  *item= get_bgp_domain(asn);
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_domain ---------------------------------
void cli_ctx_destroy_bgp_domain(void ** item)
{
  // Nothing to destroy, context item was only a reference.
}

// ----- cli_bgp_domain_full_mesh -----------------------------------
/**
 * Generate a full-mesh of iBGP sessions between the routers of the
 * domain.
 *
 * context: {as}
 * tokens: {}
 */
int cli_bgp_domain_full_mesh(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_domain_t * domain= _domain_from_context(ctx);
  // Generate the full-mesh of iBGP sessions
  bgp_domain_full_mesh(domain);
  return CLI_SUCCESS;
}

// ----- cli_bgp_domain_rescan --------------------------------------
/**
 * Rescan all the routers in the given BGP domain.
 *
 * context: {as}
 * tokens: {}
 */
int cli_bgp_domain_rescan(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_domain_t * domain= _domain_from_context(ctx);
  // Rescan all the routers in the domain.
  bgp_domain_rescan(domain);
  return CLI_SUCCESS;
}

// ----- cli_net_node_recordroute ------------------------------------
/**
 * context: {as}
 * tokens: {prefix|address|*}
 */
int cli_bgp_domain_recordroute(cli_ctx_t * ctx,
			       cli_cmd_t * cmd)
{
  bgp_domain_t * domain= _domain_from_context(ctx);
  const char * arg;
  ip_dest_t dest;
  ip_opt_t * opts;

  opts= ip_options_create();

  // Optional deflection ?
  if (cli_opts_has_value(cmd->opts, "deflection"))
    ip_options_set(opts, IP_OPT_DEFLECTION);

  // Get destination address
  arg= cli_get_arg_value(cmd, 0);
  if (ip_string_to_dest(arg, &dest)) {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  /* Check that the destination type is adress/prefix */
  if ((dest.type != NET_DEST_ADDRESS) &&
      (dest.type != NET_DEST_PREFIX)) {
    cli_set_user_error(cli_get(), "can not use this destination");
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (dest.type == NET_DEST_PREFIX)
    ip_options_alt_dest(opts, dest.prefix);
  bgp_domain_record_route(gdsout, domain, dest, opts);

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_route_map --------------------------------
/**
 * context: {} -> {&filter}
 * tokens: {}
 */
int cli_ctx_create_bgp_route_map(cli_ctx_t * ctx, cli_cmd_t * cmd,
				 void ** item)
{
  const char * arg;
  bgp_filter_t * filter;

  arg= cli_get_arg_value(cmd, 0);
  filter= filter_create();
  if (route_map_add(arg, filter) < 0) {
    cli_set_user_error(cli_get(), "route-map already exists.", arg);
    filter_destroy(&filter);
    return CLI_ERROR_CTX_CREATE;
  }

  // ----------------------------------
  // Tricky pointer stuff explained !!!
  // ----------------------------------
  // Here, we need to store the address of the filter reference in the
  // context item. This trick is used to be able to plug the filter
  // commands on top of the route map command, without change.
  // The memory allocated for the address will be freed when the
  // route-map context is exited (see 'cli_ctx_destroy_bgp_route_map'
  // below).
  *item= MALLOC(sizeof(bgp_filter_t *));
  *((bgp_filter_t **) *item)= filter;

  // Note: an alternative way would be to store in the context item
  // the address of the route-map filter field (as it is done when peer
  // filters are put on the context stack).
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_route_map -------------------------------
/**
 *
 *
 */
void cli_ctx_destroy_bgp_route_map(void ** item)
{
  bgp_filter_t ** filter_ptr= (bgp_filter_t **) *item;
  
  if (filter_ptr != NULL)
    FREE(filter_ptr);
}

// ----- cli_ctx_create_bgp_router ----------------------------------
/**
 * This function adds the BGP instance of the given node (identified
 * by its address) on the CLI context.
 *
 * context: {} -> {router}
 * tokens: {addr}
 */
int cli_ctx_create_bgp_router(cli_ctx_t * ctx, cli_cmd_t * cmd, void ** item)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  net_node_t * node;
  net_protocol_t * protocol;

  // Find node
  if (str2node_id(arg, &node)) {
    cli_set_user_error(cli_get(), "invalid node id \"%s\"", arg);
    return CLI_ERROR_CTX_CREATE;
  }

  // Get BGP protocol instance
  protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP);
  if (protocol == NULL) {
    cli_set_user_error(cli_get(), "BGP is not supported on node \"%s\"", arg);
    return CLI_ERROR_CTX_CREATE;
  }

  *item= protocol->handler;

  cli_enum_ctx_bgp_router((bgp_router_t *) protocol->handler);

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_router ---------------------------------
void cli_ctx_destroy_bgp_router(void ** item)
{
  cli_enum_ctx_bgp_router(NULL);
}

// -----[ cli_bgp_options_autocreate ]-------------------------------
/**
 * context: {}
 * tokens: {on/off}
 */
int cli_bgp_options_autocreate(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg;

  arg= cli_get_arg_value(cmd, 0);
  if (!strcmp(arg, "on"))
    bgp_options_flag_set(BGP_OPT_AUTO_CREATE);
  else if (!strcmp(arg, "off"))
    bgp_options_flag_reset(BGP_OPT_AUTO_CREATE);
  else {
    cli_set_user_error(cli_get(), "invalid value \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_options_showmode ]---------------------------------
/**
 * Change the BGP route "show" mode.
 *
 * context: {}
 * tokens : {"cisco"|"mrt"|format}
 *
 * Where the mode can be one of the following:
 *   - "cisco"
 *   - "mrt"
 *   - a custom format string
 *
 * See function bgp_route_dump_custom() [src/bgp/route.c] for more
 * details on the format specifier string.
 */
int cli_bgp_options_showmode(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg;
  uint8_t mode;

  // Get mode
  arg= cli_get_arg_value(cmd, 0);
  if (route_str2mode(arg, &mode) != 0)
    mode= BGP_ROUTES_OUTPUT_CUSTOM;

  route_set_show_mode(mode, arg);

  return CLI_SUCCESS;
}

// ----- cli_bgp_options_med ----------------------------------------
/**
 * context: {}
 * tokens: {med-type}
 */
int cli_bgp_options_med(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_med_type_t med_type;
  
  if (dp_rules_str2med_type(arg, &med_type) !=  ESUCCESS) {
    cli_set_user_error(cli_get(), "unknown med-type \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  dp_rules_options_set_med_type(med_type);
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_localpref ----------------------------------
/**
 * context: {}
 * tokens: {local-pref}
 */
int cli_bgp_options_localpref(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned long int local_pref;

  if (str_as_ulong(arg, &local_pref)) {
    cli_set_user_error(cli_get(), "invalid default local-pref \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  bgp_options_set_local_pref(local_pref);
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_msgmonitor ---------------------------------
/**
 * context: {}
 * tokens: {output-file|"-"}
 */
int cli_bgp_options_msgmonitor(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);

  if (strcmp(arg, "-") == 0) {
    bgp_msg_monitor_close();
  } else {
    bgp_msg_monitor_open(arg);
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_options_qosaggrlimit -------------------------------
/**
 * context: {}
 * tokens: {limit}
 */
#ifdef BGP_QOS
int cli_bgp_options_qosaggrlimit(cli_ctx_t * ctx,
				 cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int limit;

  if (str_as_uint(arg, &limit))
    return CLI_ERROR_COMMAND_FAILED;
  BGP_OPTIONS_QOS_AGGR_LIMIT= limit;
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_options_walton_convergence ------------------------
/**
 *
 */
#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
int cli_bgp_options_walton_convergence(cli_ctx_t * ctx,
				       cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);

  if (!strcmp(arg, "best"))
    BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST = 1;
  else if (!strcmp(arg, "all"))
    BGP_OPTIONS_WALTON_CONVERGENCE_ON_BEST = 0;
  else {
    cli_set_user_error(cli_get(), "invalid value \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_options_advertise_external_best -------------------
/**
 * context : {}
 * tokens: {}
 */
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
int cli_bgp_options_advertise_external_best(cli_ctx_t * ctx,
					    cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);

  if (!strcmp(arg, "on"))
    BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST= 1;
  else if (!strcmp(arg, "off"))
    BGP_OPTIONS_ADVERTISE_EXTERNAL_BEST= 0;
  else {
    cli_set_user_error(cli_get(), "invalid value \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_router_set_clusterid -------------------------------
/**
 * This function changes the router's cluster-id. This also allows
 * route-reflection in this router.
 *
 * context: {router}
 * tokens: {cluster-id}
 */
int cli_bgp_router_set_clusterid(cli_ctx_t * ctx,
				 cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int cluster_id;

  // Get the cluster-id
  if (str_as_uint(arg, &cluster_id)) {
    cli_set_user_error(cli_get(), "invalid cluster-id \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  router->cluster_id= cluster_id;
  router->reflector= 1;
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_debug_dp ------------------------------------
/**
 * context: {router}
 * tokens: {prefix}
 */
int cli_bgp_router_debug_dp(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  ip_pfx_t prefix;

  // Get the prefix
  if (str2prefix(arg, &prefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  bgp_debug_dp(gdsout, router, prefix);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_load_rib ------------------------------------
/**
 * This function loads an MRTD table dump into the given BGP
 * instance.
 *
 * context: {router}
 * tokens: {file}
 * options: {--autoconf,--format,--force,--summary}
 */
static int cli_bgp_router_load_rib(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * filename= cli_get_arg_value(cmd, 0);
  const char * arg;
  bgp_input_type_t format= BGP_ROUTES_INPUT_MRT_ASC;
  uint8_t options= 0;

  // Get the optional format
  arg= cli_get_opt_value(cmd, "format");
  if (arg != NULL) {
    if (bgp_routes_str2format(arg, &format) != 0) {
      cli_set_user_error(cli_get(), "invalid input format \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Get option --autoconf ?
  if (cli_has_opt_value(cmd, "autoconf"))
    options|= BGP_ROUTER_LOAD_OPTIONS_AUTOCONF;

  // Get option --force ?
  if (cli_has_opt_value(cmd, "force"))
    options|= BGP_ROUTER_LOAD_OPTIONS_FORCE;
    
  // Get option --summary ?
  if (cli_has_opt_value(cmd, "summary"))
    options|= BGP_ROUTER_LOAD_OPTIONS_SUMMARY;

  // Load the MRTD file 
  if (bgp_router_load_rib(router, filename, format, options) != 0) {
    cli_set_user_error(cli_get(), "could not load \"%s\" (%s%s)",
		       filename, "plop", ", at line XX");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_set_tiebreak --------------------------------
/**
 * This function changes the tie-breaking rule of the given BGP
 * instance.
 *
 * context: {router}
 * tokens: {addr, function}
 */
/*int cli_bgp_router_set_tiebreak(cli_ctx_t * ctx,
				cli_cmd_t * cmd)
{
  char * pcParam;
  bgp_router_t * router= (bgp_router_t *) _router_from_context(ctx);

  // Get the tie-breaking function name
  pcParam= tokens_get_string_at(pTokens, 1);
  if (!strcmp(pcParam, "hash"))
    router->fTieBreak= tie_break_hash;
  else if (!strcmp(pcParam, "low-isp"))
    router->fTieBreak= tie_break_low_ISP;
  else if (!strcmp(pcParam, "high-isp"))
    router->fTieBreak= tie_break_high_ISP;
  else {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: invalid tie-breaking function \"%s\"\n", pcParam);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
  }*/

// ----- cli_bgp_router_show_info -----------------------------------
/**
 * context: {router}
 * tokens: {}
 */
static int cli_bgp_router_show_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  // Show information
  bgp_router_info(gdsout, router);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_networks -------------------------------
/** 
 * This function shows the list of locally originated networks in the
 * given BGP instance.
 *
 * context: {router}
 * tokens: {}
 */
static int cli_bgp_router_show_networks(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  bgp_router_dump_networks(gdsout, router);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_peers ----------------------------------
/**
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_show_peers(cli_ctx_t * ctx,
			      cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  // Dump peers
  bgp_router_dump_peers(gdsout, router);

  return CLI_SUCCESS;
}

static inline int _opt_output_init(const cli_cmd_t * cmd, gds_stream_t ** stream_ref) {
  const char * output;
  gds_stream_t * stream= gdsout;

  if (cli_has_opt_value(cmd, "output")) {
    output= cli_get_opt_value(cmd, "output");
    stream= stream_create_file(output);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "could not create \"%d\"", output);	\
      return -1;
    }
  }

  *stream_ref= stream;
  return 0;
}

static inline void _opt_output_release(gds_stream_t ** stream_ref) {
  if (*stream_ref != gdsout) {
    stream_destroy(stream_ref);
    *stream_ref= gdsout;
  }
}

// ----- cli_bgp_router_show_rib ------------------------------------
/**
 * This function shows the list of routes in the given BGP instance's
 * Loc-RIB (best routes). The function takes a single parameter which
 * can be
 *   - an address, meaning show only the route which best matches this
 *     address 
 *   - a prefix, meaning show only the route towards this exact prefix
 *   - an asterisk ('*'), meaning show everything
 *
 * context: {router}
 * tokens: {prefix|address|*}
 * options: {--output=filename}
 */
int cli_bgp_router_show_rib(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  ip_pfx_t prefix;
  gds_stream_t * stream;
  
  if (_opt_output_init(cmd, &stream))
    return CLI_ERROR_COMMAND_FAILED;

  // Get the prefix/address/*
  if (!strcmp(arg, "*")) {
    bgp_router_dump_rib(stream, router);
  } else if (!str2prefix(arg, &prefix)) {
    bgp_router_dump_rib_prefix(stream, router, prefix);
  } else if (!str2address(arg, &prefix.network)) {
    bgp_router_dump_rib_address(stream, router, prefix.network);
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"", arg);
    _opt_output_release(&stream);
    return CLI_ERROR_COMMAND_FAILED;
  }

  _opt_output_release(&stream);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_adjrib ---------------------------------
/**
 * This function shows the list of routes in the given BGP instance's
 * Adj-RIB-In. The function takes two parameters. The first one
 * selects the peer(s) for which the Adj-RIB-In will be shown. The
 * second one selects the routes which will be shown. The 'peer'
 * parameter can be 
 *   - a peer address
 *   - an asterisk ('*')
 * The second parameter can be
 *   - an address, meaning show only the route which best matches this
 *     address 
 *   - a prefix, meaning show only the route towards this exact prefix
 *   - an asterisk ('*'), meaning show everything
 *
 * context: {router}
 * tokens: {in|out, addr, prefix|address|*}
 * options: {--output=filename}
 */
static int cli_bgp_router_show_adjrib(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg;
  net_addr_t peer_addr;
  bgp_peer_t * peer;
  ip_pfx_t prefix;
  bgp_rib_dir_t dir;
  gds_stream_t * stream;

  // Get the adjrib direction: in|out
  arg= cli_get_arg_value(cmd, 0);
  if (!strcmp(arg, "in")) {
    dir= RIB_IN;
  } else if (!strcmp(arg, "out")) {
    dir= RIB_OUT;
  } else {
    cli_set_user_error(cli_get(), "invalid adj-rib side \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the peer|*
  arg= cli_get_arg_value(cmd, 1);
  if (!strcmp(arg, "*")) {
    peer= NULL;
  } else if (!str2addr_id(arg, &peer_addr)) {
    peer= bgp_router_find_peer(router, peer_addr);
    if (peer == NULL) {
      cli_set_user_error(cli_get(), "unknown peer \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    cli_set_user_error(cli_get(), "invalid peer address \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the prefix|address|*
  arg= cli_get_arg_value(cmd, 2);
  if (!strcmp(arg, "*")) {
    prefix.mask= 0;
  } else if (!str2prefix(arg, &prefix)) {
  } else if (!str2address(arg, &prefix.network)) {
    prefix.mask= 32;
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|address|* \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

    if (_opt_output_init(cmd, &stream))
    return CLI_ERROR_COMMAND_FAILED;
  
  bgp_router_dump_adjrib(stream, router, peer, prefix, dir);

  _opt_output_release(&stream);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_routeinfo ------------------------------
/**
 * Display a detailled information about the best route towards the
 * given prefix. Information includes communities, and so on...
 *
 * context: {router}
 * tokens : {prefix|address|*}
 * options: {--output=filename}
 */
static int cli_bgp_router_show_routeinfo(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg;
  ip_dest_t dest;
  gds_stream_t * stream;

  // Get the destination
  arg= cli_get_arg_value(cmd, 0);
  if (ip_string_to_dest(arg, &dest)) {
    cli_set_user_error(cli_get(), "invalid destination \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Show the route information
  if (_opt_output_init(cmd, &stream))
    return CLI_ERROR_COMMAND_FAILED;

  bgp_router_show_routes_info(stream, router, dest);

  _opt_output_release(&stream);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_show_stats ----------------------------------
/**
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_show_stats(cli_ctx_t * ctx,
			      cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  bgp_router_show_stats(gdsout, router);

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_recordroute ---------------------------------
/**
 * context: {router}
 * tokens : {prefix}
 * options: {--exact-match, --preserve-dups}
 */
int cli_bgp_router_recordroute(cli_ctx_t * ctx,
			       cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char *   arg= cli_get_arg_value(cmd, 0);
  ip_pfx_t prefix;
  int result;
  bgp_path_t * path= NULL;
  uint8_t options= 0;

  // Get prefix
  if (str2prefix(arg, &prefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (cli_has_opt_value(cmd, "exact-match"))
    options|= AS_RECORD_ROUTE_OPT_EXACT_MATCH;
  if (cli_has_opt_value(cmd, "preserve-dups"))
    options|= AS_RECORD_ROUTE_OPT_PRESERVE_DUPS;

  // Record route
  result= bgp_record_route(router, prefix, &path, options);
  // Display recorded-route
  bgp_dump_recorded_route(gdsout, router, prefix, path, result);
  path_destroy(&path);
  return CLI_SUCCESS;
}

// ----- cli_bgp_router_rescan --------------------------------------
/**
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_rescan(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  if (bgp_router_scan_rib(router)) {
    cli_set_user_error(cli_get(), "RIB scan failed");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_rerun ---------------------------------------
/**
 * context: {router}
 * tokens: {prefix|*}
 */
int cli_bgp_router_rerun(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  ip_pfx_t prefix;

  // Get prefix
  if (!strcmp(arg, "*")) {
    prefix.mask= 0;
  } else if (!str2prefix(arg, &prefix)) {
  } else {
    cli_set_user_error(cli_get(), "invalid prefix|* \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Rerun the decision process for all known prefixes
  if (bgp_router_rerun(router, prefix)) {
    cli_set_user_error(cli_get(), "reset failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_reset ---------------------------------------
/**
 * This function shuts down all the router's peerings then restart
 * them.
 *
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_reset(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  // Reset the router
  if (bgp_router_reset(router)) {
    cli_set_user_error(cli_get(), "reset failed.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_start ---------------------------------------
/**
 * This function opens all the defined peerings of the given router.
 *
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_start(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  // Start the router
  if (bgp_router_start(router)) {
    cli_set_user_error(cli_get(), "could not start the router.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_stop ----------------------------------------
/**
 * This function closes all the defined peerings of the given router.
 *
 * context: {router}
 * tokens: {}
 */
int cli_bgp_router_stop(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);

  // Stop the router
  if (bgp_router_stop(router)) {
    cli_set_user_error(cli_get(), "could not stop the router.");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_peer ------------------------------------
/**
 * context: {router}
 * tokens: {as-num, addr}
 */
int cli_bgp_router_add_peer(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg;
  asn_t asn;
  net_addr_t addr;

  // Get the new peer's AS number
  arg= cli_get_arg_value(cmd, 0);
  if (str2asn(arg, &asn)) {
    cli_set_user_error(cli_get(), "invalid AS number \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the new peer's address
  arg= cli_get_arg_value(cmd, 1);
  if (str2address(arg, &addr) < 0) {
    cli_set_user_error(cli_get(), "invalid address \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (bgp_router_add_peer(router, asn, addr, NULL) != 0) {
    cli_set_user_error(cli_get(), "peer already exists");
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_network ---------------------------------
/**
 * This function adds a local network to the BGP instance.
 *
 * context: {router}
 * tokens: {prefix}
 */
int cli_bgp_router_add_network(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  ip_pfx_t prefix;
  net_error_t error;

  // Get the prefix
  if (str2prefix(arg, &prefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add the network
  error= bgp_router_add_network(router, prefix);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add network (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_del_network ---------------------------------
/**
 * This function removes a previously added local network.
 *
 * context: {router}
 * tokens: {prefix}
 */
int cli_bgp_router_del_network(cli_ctx_t * ctx,
			       cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  ip_pfx_t prefix;

  // Get the prefix
  if (str2prefix(arg, &prefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove the network
  if (bgp_router_del_network(router, prefix))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_add_qosnetwork ------------------------------
/**
 * context: {router}
 * tokens: {prefix, delay}
 */
#ifdef BGP_QOS
int cli_bgp_router_add_qosnetwork(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  ip_pfx_t prefix;
  unsigned int uDelay;

  if (str2prefix(tokens_get_string_at(pTokens, 0), &prefix) < 0)
    return CLI_ERROR_COMMAND_FAILED;

  if (tokens_get_uint_at(pTokens, 1, &uDelay))
    return CLI_ERROR_COMMAND_FAILED;

  if (bgp_router_add_qos_network(router, prefix, (net_link_delay_t) uDelay))
    return CLI_ERROR_COMMAND_FAILED;
  return CLI_SUCCESS;
}
#endif

// ----- cli_bgp_router_assert_routes_match --------------------------
/**
 * context: {router, routes}
 * tokens: {predicate}
 */
int cli_bgp_router_assert_routes_match(cli_ctx_t * ctx,
				       cli_cmd_t * cmd)
{
  bgp_routes_t * routes= _routes_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_ft_matcher_t * matcher;
  int matches= 0;
  unsigned int index;
  bgp_route_t * route;
  int result;

  // Parse predicate
  result= predicate_parser(arg, &matcher);
  if (result != PREDICATE_PARSER_SUCCESS) {
    cli_set_user_error(cli_get(), "invalid predicate \"%s\" (%s)",
		       arg, predicate_parser_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  for (index= 0; index < bgp_routes_size(routes); index++) {
    route= bgp_routes_at(routes, index);
    if (filter_matcher_apply(matcher, NULL, route)) {
      matches++;
    }
  }

  filter_matcher_destroy(&matcher);

  if (matches < 1) {
    cli_set_user_error(cli_get(), "no route matched \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_bgp_router_assert_routes_show --------------------------
/**
 * context: {router, routes}
 * tokens: {}
 */
int cli_bgp_router_assert_routes_show(cli_ctx_t * ctx,
				      cli_cmd_t * cmd)
{
  bgp_routes_t * routes= _routes_from_context(ctx);
  routes_list_dump(gdsout, routes);
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_router_assert_routes --------------------
/**
 * Check that the given router has a feasible route towards a given
 * prefix that goes through a given next-hop.
 *
 * context: {router}
 * tokens: {prefix, type}
 */
int cli_ctx_create_bgp_router_assert_routes(cli_ctx_t * ctx, cli_cmd_t * cmd,
					    void ** item_ref)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg;
  ip_pfx_t prefix;
  bgp_routes_t * routes;

  // Get the prefix
  arg= cli_get_arg_value(cmd, 0);
  if (str2prefix(arg, &prefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the routes type
  arg= cli_get_arg_value(cmd, 1);
  // Get the requested routes of the router towards the prefix
  if (!strcmp(arg, "best")) {
    routes= bgp_router_get_best_routes(router, prefix);
  } else if (!strcmp(arg, "feasible")) {
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
    routes= bgp_router_get_feasible_routes(router, prefix, 0);
#else
    routes= bgp_router_get_feasible_routes(router, prefix);
#endif
  } else {
    cli_set_user_error(cli_get(), "unsupported type of routes \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check that there is at leat one route
  if (bgp_routes_size(routes) < 1) {
    cli_set_user_error(cli_get(), "no feasible routes towards %s",
		       cli_get_arg_value(cmd, 0));
    routes_list_destroy(&routes);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Store list of routes on context...
  *item_ref= routes;

  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_bgp_router_assert_routes -----------------
void cli_ctx_destroy_bgp_router_assert_routes(void ** item_ref)
{
  routes_list_destroy((bgp_routes_t **) item_ref);
}

// ----- cli_bgp_route_map_filter_add --------------------------------
/**
 * context: {route-map name}
 * tokens: {filter}
 */
int cli_bgp_route_map_filter_add(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  char * name= (char *) cli_context_top_data(ctx);
  assert(name != NULL);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_ft_rule_t * rule;
  bgp_filter_t * filter= NULL;

  if (filter_parser_rule(arg, &rule) != FILTER_PARSER_SUCCESS) {
    return CLI_ERROR_COMMAND_FAILED;
  }

  filter = route_map_get(name);
  if (filter == NULL) {
    filter= filter_create();
    if (route_map_add(name, filter) < 0) {
      cli_set_user_error(cli_get(), "could not add the route-map filter.");
      return CLI_ERROR_COMMAND_FAILED;
    }
  }
  filter_add_rule2(filter, rule);
  return CLI_SUCCESS;
}

#ifdef __EXPERIMENTAL__
// ----- cli_bgp_router_load_ribs_in --------------------------------
/**
 * context: {router}
 * tokens: {file}
 */
/* DE-ACTIVATED by BQU on 2007/10/04 */
#ifdef COMMENT_BQU
int cli_bgp_router_load_ribs_in(cli_ctx_t * ctx,
				cli_cmd_t * cmd)
{
  bgp_router_t * router= _router_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);

  if (bgp_router_load_ribs_in(arg, router) == -1) {
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}
#endif /* COMMENT_BQU */
#endif /* __EXPERIMENTAL__ */

// ----- cli_bgp_show_route_maps ------------------------------------
/**
 * Show the list of route-maps.
 *
 * context: {}
 * tokens: {}
 */
int cli_bgp_show_route_maps(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  gds_enum_t * enu= route_map_enum();
  _route_map_t * rm;
  
  while (enum_has_next(enu)) {
    rm= *((_route_map_t **) enum_get_next(enu));
    stream_printf(gdsout, "route-map \"%s\"\n", rm->name);
    filter_dump(gdsout, rm->filter);
    stream_printf(gdsout, "\n");
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_show_routers ---------------------------------------
/**
 * Show the list of routers matching the given criterion which is a
 * prefix or an asterisk (meaning "all routers").
 *
 * context: {}
 * tokens: {prefix}
 */
int cli_bgp_show_routers(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  char * addr;
  int state= 0;

  // Get prefix
  if (!strcmp(arg, "*"))
    arg= NULL;

  while ((addr= cli_enum_bgp_routers_addr(arg, state++)) != NULL) {
    stream_printf(gdsout, "%s\n", addr);
  }
  return CLI_SUCCESS;
}

// ----- cli_bgp_show_sessions --------------------------------------
/**
 * Show the list of sessions.
 *
 * context: {}
 * tokens:  {}
 * options: {--output=<filename>}
 */
int cli_bgp_show_sessions(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router;
  bgp_peer_t * peer;
  int state= 0;
  gds_stream_t * stream;
  unsigned int index;

  if (_opt_output_init(cmd, &stream))
    return CLI_ERROR_COMMAND_FAILED;

  while ((router= cli_enum_bgp_routers(NULL, state++)) != NULL) {
    for (index= 0; index < bgp_peers_size(router->peers); index++) {
      peer= bgp_peers_at(router->peers, index);
      bgp_router_dump_id(stream, router);
      stream_printf(stream, "\t");
      ip_address_dump(stream, peer->addr);
      stream_printf(stream, "\t%d\t%d\n", peer->send_seq_num,
		 peer->recv_seq_num);
    }
  }

  _opt_output_release(&stream);

  return CLI_SUCCESS;
}

// -----[ cli_bgp_clearrib ]-----------------------------------------
/**
 * context: ---
 * tokens : ---
 */
int cli_bgp_clearrib(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router;
  int iState= 0;

  while ((router= cli_enum_bgp_routers(NULL, iState++)) != NULL) {
    bgp_router_clear_rib(router);
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_clearadjrib ]--------------------------------------
/**
 * context: ---
 * tokens : ---
 */
int cli_bgp_clearadjrib(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_router_t * router;
  int iState= 0;

  while ((router= cli_enum_bgp_routers(NULL, iState++)) != NULL) {
    bgp_router_clear_adj_rib(router);
  }

  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// REGISTRATION OF CLI COMMANDS
//
/////////////////////////////////////////////////////////////////////

// ----- _register_bgp_add ---------------------------------------
static void _register_bgp_add(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  
  group= cli_add_cmd(parent, cli_cmd_group("add"));
  cmd= cli_add_cmd(group, cli_cmd("router", cli_bgp_add_router));
  cli_add_arg(cmd, cli_arg("as-num", NULL));
  cli_add_arg(cmd, cli_arg2("addr", NULL, cli_enum_net_nodes_addr));
}

// ----- _register_bgp_assert ------------------------------------
static void _register_bgp_assert(cli_cmd_t * parent)
{
  cli_cmd_t * group;

  group= cli_add_cmd(parent, cli_cmd_group("assert"));
  cli_add_cmd(group, cli_cmd("peerings-ok", cli_bgp_assert_peerings));
  cli_add_cmd(group, cli_cmd("reachability-ok", cli_bgp_assert_reachability));
}

// ----- _register_bgp_domain ------------------------------------
static void _register_bgp_domain(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  
  group= cli_add_cmd(parent, cli_cmd_ctx("domain",
					 cli_ctx_create_bgp_domain,
					 cli_ctx_destroy_bgp_domain));
  cli_add_arg(group, cli_arg("as-number", NULL));
  cmd= cli_add_cmd(group, cli_cmd("full-mesh", cli_bgp_domain_full_mesh));
  cmd= cli_add_cmd(group, cli_cmd("rescan", cli_bgp_domain_rescan));
  cmd= cli_add_cmd(group, cli_cmd("record-route", cli_bgp_domain_recordroute));
  cli_add_arg(cmd, cli_arg("address|prefix", NULL));
  cli_add_opt(cmd, cli_opt("deflection", NULL));
}

// ----- _register_bgp_router_add --------------------------------
static void _register_bgp_router_add(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("add"));
  cmd= cli_add_cmd(group, cli_cmd("peer", cli_bgp_router_add_peer));
  cli_add_arg(cmd, cli_arg("as-num", NULL));
  cli_add_arg(cmd, cli_arg("addr", NULL));
  cmd= cli_add_cmd(group, cli_cmd("network", cli_bgp_router_add_network));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
#ifdef BGP_QOS
  cmd= cli_add_cmd(group, cli_cmd("qos-network",
				  cli_bgp_router_add_qosnetwork));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_arg(cmd, cli_arg("delay", NULL));
#endif /* BGP_QOS */
}

// ----- _register_bgp_router_assert -----------------------------
static void _register_bgp_router_assert(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("assert"));
  group= cli_add_cmd(group,
		     cli_cmd_ctx("routes",
				 cli_ctx_create_bgp_router_assert_routes,
				 cli_ctx_destroy_bgp_router_assert_routes));
  cli_add_arg(group, cli_arg("prefix", NULL));
  cli_add_arg(group, cli_arg("best|feasible", NULL));
  cmd= cli_add_cmd(group, cli_cmd("match",
				  cli_bgp_router_assert_routes_match));
  cli_add_arg(cmd, cli_arg("predicate", NULL));
  cmd= cli_add_cmd(group, cli_cmd("show", cli_bgp_router_assert_routes_show));
}

// ----- _register_bgp_router_debug ------------------------------
static void _register_bgp_router_debug(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("debug"));
  cmd= cli_add_cmd(group, cli_cmd("dp", cli_bgp_router_debug_dp));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
}

// ----- _register_bgp_router_del --------------------------------
static void _register_bgp_router_del(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("del"));
  cmd= cli_add_cmd(group, cli_cmd("network", cli_bgp_router_del_network));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
}

// ----- _register_bgp_route_map ------------------------------------
static void _register_bgp_route_map(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_ctx("route-map", 
					 cli_ctx_create_bgp_route_map, 
					 cli_ctx_destroy_bgp_route_map));
  cli_add_arg(group, cli_arg("name", NULL));
  cmd= cli_add_cmd(group, cli_cmd_ctx("add-rule",
				 cli_ctx_create_bgp_filter_add_rule,
				 cli_ctx_destroy_bgp_filter_rule));
  cli_register_bgp_filter_rule(cmd);
}

// ----- _register_bgp_options -----------------------------------
static void _register_bgp_options(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("options"));
  cmd= cli_add_cmd(group, cli_cmd("auto-create", cli_bgp_options_autocreate));
  cli_add_arg(cmd, cli_arg("on-off", NULL));
  cmd= cli_add_cmd(group, cli_cmd("med", cli_bgp_options_med));
  cli_add_arg(cmd, cli_arg("med-type", NULL));
  cmd= cli_add_cmd(group, cli_cmd("local-pref", cli_bgp_options_localpref));
  cli_add_arg(cmd, cli_arg("local-pref", NULL));
  cmd= cli_add_cmd(group, cli_cmd("msg-monitor", cli_bgp_options_msgmonitor));
  cli_add_arg(cmd, cli_arg("output-file", NULL));
  cmd= cli_add_cmd(group, cli_cmd("show-mode", cli_bgp_options_showmode));
  cli_add_arg(cmd, cli_arg("cisco|mrt|custom", NULL));

#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  cmd= cli_add_cmd(group, cli_cmd("advertise-external-best",
				  cli_bgp_options_advertise_external_best));
  cli_add_arg(cmd, cli_arg("on-off", NULL));
#endif /* __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__ */

#if defined __EXPERIMENTAL__ && defined __EXPERIMENTAL_WALTON__
  cmd= cli_add_cmd(group, cli_cmd("walton-convergence",
				  cli_bgp_options_walton_convergence));
  cli_add_arg(cmd, cli_arg("all|best", NULL));
#endif /* __EXPERIMENTAL__ && __EXPERIMENTAL_WALTON__ */
}

// ----- _register_bgp_router_load -------------------------------
static void _register_bgp_router_load(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("load"));
  cmd= cli_add_cmd(group, cli_cmd("rib", cli_bgp_router_load_rib));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
  cli_add_opt(cmd, cli_opt("autoconf", NULL));
  cli_add_opt(cmd, cli_opt("force", NULL));
  cli_add_opt(cmd, cli_opt("format=", NULL));
  cli_add_opt(cmd, cli_opt("summary", NULL));
}

// ----- _register_bgp_router_set --------------------------------
static void _register_bgp_router_set(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("set"));
  cmd= cli_add_cmd(group, cli_cmd("cluster-id", cli_bgp_router_set_clusterid));
  cli_add_arg(cmd, cli_arg("cluster-id", NULL));
}

// ----- _register_bgp_router_show -------------------------------
static void _register_bgp_router_show(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("show"));
  cmd= cli_add_cmd(group, cli_cmd("info", cli_bgp_router_show_info));
  cmd= cli_add_cmd(group, cli_cmd("networks", cli_bgp_router_show_networks));
  cmd= cli_add_cmd(group, cli_cmd("peers", cli_bgp_router_show_peers));
  cmd= cli_add_cmd(group, cli_cmd("adj-rib", cli_bgp_router_show_adjrib));
  cli_add_arg(cmd, cli_arg("in|out", NULL));
  cli_add_arg(cmd, cli_arg2("peer", NULL, cli_enum_bgp_peers_addr));
  cli_add_arg(cmd, cli_arg("prefix|address|*", NULL));
  cli_add_opt(cmd, cli_opt("output=", NULL));
  cmd= cli_add_cmd(group, cli_cmd("rib", cli_bgp_router_show_rib));
  cli_add_arg(cmd, cli_arg("prefix|address|*", NULL));
  cli_add_opt(cmd, cli_opt("output=", NULL));
  cmd= cli_add_cmd(group, cli_cmd("route-info",
				  cli_bgp_router_show_routeinfo));
  cli_add_arg(cmd, cli_arg("prefix|address|*", NULL));
  cli_add_opt(cmd, cli_opt("output=", NULL));
  cmd= cli_add_cmd(group, cli_cmd("stats", cli_bgp_router_show_stats));
}

// ----- _register_bgp_router ------------------------------------
static void _register_bgp_router(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_ctx("router",
					 cli_ctx_create_bgp_router,
					 cli_ctx_destroy_bgp_router));
  cli_add_arg(group, cli_arg2("addr", NULL, cli_enum_bgp_routers_addr_id));
  _register_bgp_router_add(group);
  _register_bgp_router_assert(group);
  _register_bgp_router_debug(group);
  _register_bgp_router_del(group);
  cli_register_bgp_router_peer(group);
  _register_bgp_router_load(group);
  _register_bgp_router_set(group);
  _register_bgp_router_show(group);
  cmd= cli_add_cmd(group, cli_cmd("record-route", cli_bgp_router_recordroute));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_opt(cmd, cli_opt("exact-match", NULL));
  cli_add_opt(cmd, cli_opt("preserve-dups", NULL));
  cmd= cli_add_cmd(group, cli_cmd("rerun", cli_bgp_router_rerun));
  cli_add_arg(cmd, cli_arg("prefix|*", NULL));
  cmd= cli_add_cmd(group, cli_cmd("rescan", cli_bgp_router_rescan));
  cmd= cli_add_cmd(group, cli_cmd("reset", cli_bgp_router_reset));
  cmd= cli_add_cmd(group, cli_cmd("start", cli_bgp_router_start));
  cmd= cli_add_cmd(group, cli_cmd("stop", cli_bgp_router_stop));
}

// ----- _register_bgp_show --------------------------------------
static void _register_bgp_show(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  
  group= cli_add_cmd(parent, cli_cmd_group("show"));
  cmd= cli_add_cmd(group, cli_cmd("route-maps", cli_bgp_show_route_maps));
  cmd= cli_add_cmd(group, cli_cmd("routers", cli_bgp_show_routers));
  cli_add_arg(cmd, cli_arg("prefix|*", NULL));
  cmd= cli_add_cmd(group, cli_cmd("sessions", cli_bgp_show_sessions));
  cli_add_opt(cmd, cli_opt("output", NULL));
}

// -----[ cli_register_bgp ]-----------------------------------------
void cli_register_bgp(cli_cmd_t * parent)
{
  cli_cmd_t * group;

  group= cli_add_cmd(parent, cli_cmd_group("bgp"));
  _register_bgp_route_map(group);
  _register_bgp_add(group);
  _register_bgp_assert(group);
  _register_bgp_domain(group);
  _register_bgp_options(group);
  cli_register_bgp_topology(group);
  _register_bgp_router(group);
  _register_bgp_show(group);
  cli_add_cmd(group, cli_cmd("clear-rib", cli_bgp_clearrib));
  cli_add_cmd(group, cli_cmd("clear-adj-rib", cli_bgp_clearadjrib));
}
