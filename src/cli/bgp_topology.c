// ==================================================================
// @(#)bgp_topology.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/04/2007
// $Id: bgp_topology.c,v 1.5 2009-06-25 14:34:20 bqu Exp $
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

#include <string.h>

#include <libgds/types.h>
#include <libgds/cli.h>
#include <libgds/cli_params.h>

#include <bgp/record-route.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/aslevel/filter.h>
#include <bgp/aslevel/stat.h>
#include <cli/common.h>
#include <net/util.h>

// -----[ cli_bgp_topology_load ]------------------------------------
/**
 * context: {}
 * tokens: {file}
 * options:
 *   --addr-sch=default|local
 *   --format=default|caida
 */
static int cli_bgp_topology_load(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg;
  uint8_t addr_scheme= ASLEVEL_ADDR_SCH_DEFAULT;
  uint8_t format= ASLEVEL_FORMAT_DEFAULT;
  int line_number;
  int result;

  // Optional addressing scheme specified ?
  arg= cli_opts_get_value(cmd->opts, "addr-sch");
  if (arg != NULL) {
    if (aslevel_str2addr_sch(arg, &addr_scheme) != ASLEVEL_SUCCESS) {
      cli_set_user_error(cli_get(), "invalid addressing scheme \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Optional format specified ?
  arg= cli_opts_get_value(cmd->opts, "format");
  if (arg != NULL) {
    if (aslevel_topo_str2format(arg, &format) != ASLEVEL_SUCCESS) {
      cli_set_user_error(cli_get(), "invalid format \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  // Load AS-level topology
  arg= cli_get_arg_value(cmd, 0);
  result= aslevel_topo_load(arg, format, addr_scheme, &line_number);
  if (result != ASLEVEL_SUCCESS) {
    if (line_number > 0)
      cli_set_user_error(cli_get(), "could not load topology \"%s\" "
			 "(%s, at line %u)", arg, aslevel_strerror(result),
			 line_number);
    else
      cli_set_user_error(cli_get(), "could not load topology \"%s\" "
			 "(%s)", arg, aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_check ]-----------------------------------
/**
 * context: {}
 * tokens: {}
 * options: {--verbose}
 */
static int cli_bgp_topology_check(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result;
  int verbose= 0;

  // Verbose check ?
  if (cli_opts_has_value(cmd->opts, "verbose"))
    verbose= 1;

  result= aslevel_topo_check(verbose);
  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "check failed (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_filter_asn ]------------------------------
/**
 * context: {}
 * tokens: {<asn>}
 */
static int cli_bgp_topology_filter_asn(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  asn_t asn;
  int result;
  
  // Get ASN
  if (str2asn(arg, &asn)) {
    cli_set_user_error(cli_get(), "invalid ASN (%s)", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove AS from topology
  result= aslevel_topo_filter_as(asn);
  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not filter ASN (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_filter_group ]----------------------------
/**
 * context: {}
 * tokens: {<what>}
 */
static int cli_bgp_topology_filter_group(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  int result;
  uint8_t filter;

  // Get filter parameter
  result= aslevel_filter_str2filter(arg, &filter);
  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not filter topology (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Filter topology
  result= aslevel_topo_filter_group(filter);
  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not filter topology (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_policies ]--------------------------------
/**
 * context: {}
 * tokens: {}
 */
static int cli_bgp_topology_policies(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result= aslevel_topo_policies();

  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not setup policies (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_info ]------------------------------------
/**
 * context: {}
 * tokens: {}
 */
static int cli_bgp_topology_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result= aslevel_topo_info(gdsout);

  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not get topology info (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_recordroute ]-----------------------------
/**
 * context: {}
 * tokens : {prefix}
 * options: {--output=FILE,--exact-match,--preserve-dups}
 */
static int cli_bgp_topology_recordroute(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  ip_pfx_t prefix;
  gds_stream_t * stream= gdsout;
  const char * arg= cli_get_arg_value(cmd, 0);
  int result;
  uint8_t options= 0;

  // Destination prefix
  if (str2prefix(arg, &prefix) < 0) {
    cli_set_user_error(cli_get(), "invalid prefix \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Optional output file ?
  arg= cli_get_opt_value(cmd, "output");
  if (arg != NULL) {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  if (cli_has_opt_value(cmd, "exact-match"))
    options|= AS_RECORD_ROUTE_OPT_EXACT_MATCH;
  if (cli_has_opt_value(cmd, "preserve-dups"))
    options|= AS_RECORD_ROUTE_OPT_PRESERVE_DUPS;

  // Record routes
  result= aslevel_topo_record_route(stream, prefix, options);
  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not record route (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (stream != gdsout)
    stream_destroy(&stream);

  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_route_dp_rule ]---------------------------
/**
 * context: {}
 * tokens: {prefix, file-out}
 */
/*
static int cli_bgp_topology_route_dp_rule(cli_ctx_t * ctx,
				   cli_cmd_t * cmd)
{
  ip_pfx_t prefix;
  FILE * fOutput;
  int result= CLI_SUCCESS;

  if (str2prefix(tokens_get_string_at(pTokens, 0), &prefix) < 0)
    return CLI_ERROR_COMMAND_FAILED;
  if ((fOutput= fopen(tokens_get_string_at(pTokens, 1), "w")) == NULL)
    return CLI_ERROR_COMMAND_FAILED;
  if (rexford_route_dp_rule(fOutput, prefix))
    result= CLI_ERROR_COMMAND_FAILED;
  fclose(fOutput);
  return result;
}
*/

// -----[ cli_bgp_topology_install ]---------------------------------
/**
 * context: {}
 * tokens: {}
 */
static int cli_bgp_topology_install(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result= aslevel_topo_install();

  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not install topology (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_run ]-------------------------------------
/**
 * context: {}
 * tokens: {}
 */
static int cli_bgp_topology_run(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  int result= aslevel_topo_run();

  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not run topology (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_dump ]------------------------------------
/**
 * context: {}
 * tokens : {}
 * options: {--output=FILE, --format=graphviz}
 */
static int cli_bgp_topology_dump(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  gds_stream_t * stream= gdsout;
  const char * arg;
  int result;

  arg= cli_get_opt_value(cmd, "output");
  if (arg != NULL) {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  //  result= aslevel_topo_dump_stubs(stream);
  arg= cli_get_opt_value(cmd, "format");
  if ((arg != NULL) && !strcmp(arg, "graphviz"))
    result= aslevel_topo_dump_graphviz(stream);
  else
    result= aslevel_topo_dump(stream, cli_get_opt_value(cmd, "verbose") != NULL);

  if (stream != gdsout)
    stream_destroy(&stream);

  if (result != ASLEVEL_SUCCESS) {
    cli_set_user_error(cli_get(), "could not dump topology (%s)",
		       aslevel_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// -----[ cli_bgp_topology_stat ]------------------------------------
/**
 * context: {}
 * tokens : {}
 * options: {--output=FILE}
 */
static int cli_bgp_topology_stat(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  gds_stream_t * stream= gdsout;
  as_level_topo_t * topo= aslevel_get_topo();
  const char * arg;

  if (topo == NULL) {
    cli_set_user_error(cli_get(), "no topology loaded");
    return CLI_ERROR_COMMAND_FAILED;
  }

  arg= cli_get_opt_value(cmd, "output");
  if (arg != NULL) {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  aslevel_stat_degree(stream, topo, 1, 1, 1);
  
  if (stream != gdsout)
    stream_destroy(&stream);

  return CLI_SUCCESS;
}

// -----[ cli_register_bgp_topology_filter ]-------------------------
static void _cli_register_bgp_topology_filter(cli_cmd_t * parent)
{
  cli_cmd_t * group, *cmd;

  group= cli_add_cmd(parent, cli_cmd_group("filter"));
  cmd= cli_add_cmd(group, cli_cmd("asn", cli_bgp_topology_filter_asn));
  cli_add_arg(cmd, cli_arg("asn", NULL));
  cmd= cli_add_cmd(group, cli_cmd("group", cli_bgp_topology_filter_group));
  cli_add_arg(cmd, cli_arg("what", NULL));
}

// ----- cli_register_bgp_topology ----------------------------------
void cli_register_bgp_topology(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("topology"));
  cmd= cli_add_cmd(group, cli_cmd("load", cli_bgp_topology_load));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
  cli_add_opt(cmd, cli_opt("addr-sch=", NULL));
  cli_add_opt(cmd, cli_opt("format=", NULL));
  cmd= cli_add_cmd(group, cli_cmd("check", cli_bgp_topology_check));
  cli_add_opt(cmd, cli_opt("verbose", NULL));
  cmd= cli_add_cmd(group, cli_cmd("dump", cli_bgp_topology_dump));
  cli_add_opt(cmd, cli_opt("output=", NULL));
  cli_add_opt(cmd, cli_opt("format=", NULL));
  cli_add_opt(cmd, cli_opt("verbose", NULL));
  _cli_register_bgp_topology_filter(group);
  cmd= cli_add_cmd(group, cli_cmd("info", cli_bgp_topology_info));
  cmd= cli_add_cmd(group, cli_cmd("install", cli_bgp_topology_install));
  cmd= cli_add_cmd(group, cli_cmd("policies", cli_bgp_topology_policies));
  cmd= cli_add_cmd(group, cli_cmd("stat-degree", cli_bgp_topology_stat));
  cli_add_opt(cmd, cli_opt("output=", NULL));
  cmd= cli_add_cmd(group, cli_cmd("record-route",
				  cli_bgp_topology_recordroute));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_opt(cmd, cli_opt("output=", NULL));
  cli_add_opt(cmd, cli_opt("exact-match", NULL));
  cli_add_opt(cmd, cli_opt("preserve-dups", NULL));
  /*
  cmd= cli_add_cmd(group, cli_cmd("show-rib", cli_bgp_topology_showrib));
  cmd= cli_add_cmd(group, cli_cmd("route-dp-rule",
  cli_bgp_topology_route_dp_rule));
  cli_add_arg(cmd, cli_arg("<prefix>", NULL));
  cli_add_arg(cmd, cli_arg("<output>", NULL));
  */
  cmd= cli_add_cmd(group, cli_cmd("run", cli_bgp_topology_run));
}
