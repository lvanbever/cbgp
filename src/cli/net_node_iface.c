// ==================================================================
// @(#)net_node_iface.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/02/2008
// $Id: net_node_iface.c,v 1.4 2009-03-24 15:58:43 bqu Exp $
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
#include <net/iface.h>
#include <net/net_types.h>
#include <net/node.h>
#include <net/error.h>
#include <net/util.h>

// -----[ cli_ctx_create_iface ]-------------------------------------
/**
 * context: {node}
 * tokens: {prefix|address}
 */
static int cli_ctx_create_iface(cli_ctx_t * ctx, cli_cmd_t * cmd,
				void ** item_ref)
{
  net_node_t * node= _node_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  net_iface_t * iface= NULL;
  net_iface_id_t iface_id;
  int result;

  assert(node != NULL);
  
  // Get interface identifier
  result= net_iface_str2id(arg, &iface_id);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "invalid input \"%s\" (%s)", arg,
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Find interface in current node
  iface= node_find_iface(node, iface_id);
  if (iface == NULL) {
    cli_set_user_error(cli_get(), "no such interface \"%s\".", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *item_ref= iface;
  return CLI_SUCCESS;
}

// -----[ cli_ctx_destroy_iface ]------------------------------------
static void cli_ctx_destroy_iface(void ** item_ref)
{
  // Nothing to do here (context is only a reference).
}

// -----[ cli_iface_igpweight ]--------------------------------------
/**
 * context: {iface}
 * tokens: {weight}
 */
static int cli_iface_igpweight(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * iface= _iface_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  igp_weight_t weight;
  int result;

  // Get IGP weight
  if (str2weight(arg, &weight)) {
    cli_set_user_error(cli_get(), "invalid weight \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Change link weight in forward direction
  result= net_iface_set_metric(iface, 0, weight, 0);
  if (result != ESUCCESS) {
    cli_set_user_error(cli_get(), "cannot set weight (%s)",
		       network_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_iface_down ]-------------------------------------------
/**
 * context: {iface}
 */
static int cli_iface_down(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * iface= _iface_from_context(ctx);

  // Change interface's state
  net_iface_set_enabled(iface, 0);
  return CLI_SUCCESS;
}

// -----[ cli_iface_up ]---------------------------------------------
/**
 * context: {iface}
 */
static int cli_iface_up(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * iface= _iface_from_context(ctx);

  // Change interface's state
  net_iface_set_enabled(iface, 1);
  return CLI_SUCCESS;
}

// -----[ cli_iface_connect ]----------------------------------------
/**
 * context: {iface}
 * tokens: {}
 */
static int cli_iface_connect(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * iface= _iface_from_context(ctx);

  iface= NULL;

  return CLI_SUCCESS;
}

// -----[ cli_iface_load_clear ]-------------------------------------
/**
 * context: {iface}
 * tokens: {}
 */
static int cli_iface_load_clear(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * iface= _iface_from_context(ctx);
  net_iface_set_load(iface, 0);
  return CLI_SUCCESS;
}

// -----[ cli_iface_load_add ]---------------------------------------
/**
 * context: {iface}
 * tokens: {load}
 */
static int cli_iface_load_add(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * iface= _iface_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  net_link_load_t load;

  // Get load
  if (str2capacity(arg, &load)) {
    cli_set_user_error(cli_get(), "invalid load \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  net_iface_set_load(iface, load);
  return CLI_SUCCESS;
}

// -----[ cli_iface_load_show ]--------------------------------------
/**
 * context: {iface}
 * tokens: {}
 */
static int cli_iface_load_show(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_iface_t * iface= _iface_from_context(ctx);
  net_link_dump_load(gdsout, iface);
  stream_printf(gdsout, "\n");
  return CLI_SUCCESS;
}

// -----[ cli_net_node_add_iface ]----------------------------------
/**
 * context: {node}
 * tokens: {address|prefix, type}
 * type: {rtr, ptp, ptmp, virtual}
 */
static int cli_net_node_add_iface(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  net_node_t * node= _node_from_context(ctx);
  const char * arg_if= cli_get_arg_value(cmd, 0);
  const char * arg_type= cli_get_arg_value(cmd, 1);
  net_error_t error;
  net_iface_id_t iface;
  net_iface_type_t type;

  // Get interface identifier
  error= net_iface_str2id(arg_if, &iface);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add interface (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get interface type
  error= net_iface_str2type(arg_type, &type);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add interface (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Add new interface
  error= node_add_iface(node, iface, type);
  if (error != ESUCCESS) {
    cli_set_user_error(cli_get(), "could not add interface (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS REGISTRATION
//
/////////////////////////////////////////////////////////////////////

// -----[ cli_register_net_node_add_iface ]--------------------------
void cli_register_net_node_add_iface(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("iface",
					       cli_net_node_add_iface));
  cli_add_arg(cmd, cli_arg("address|prefix", NULL));
  cli_add_arg(cmd, cli_arg("type", NULL));
}

// -----[ _register_net_node_iface_load ]----------------------------
static void _register_net_node_iface_load(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  
  group= cli_add_cmd(parent, cli_cmd_group("load"));
  cmd= cli_add_cmd(group, cli_cmd("add", cli_iface_load_add));
  cli_add_arg(cmd, cli_arg("load", NULL));
  cmd= cli_add_cmd(group, cli_cmd("clear", cli_iface_load_clear));
  cmd= cli_add_cmd(group, cli_cmd("show", cli_iface_load_show));
}

// -----[ cli_register_net_node_iface ]------------------------------
void cli_register_net_node_iface(cli_cmd_t * parent)
{
  cli_cmd_t * group, *cmd;
  group= cli_add_cmd(parent, cli_cmd_ctx("iface",
					 cli_ctx_create_iface,
					 cli_ctx_destroy_iface));
  cli_add_arg(group, cli_arg("address|prefix", NULL));
  cmd= cli_add_cmd(group, cli_cmd("connect", cli_iface_connect));
  cmd= cli_add_cmd(group, cli_cmd("igp-weight", cli_iface_igpweight));
  cli_add_arg(cmd, cli_arg("weight", NULL));
  cmd= cli_add_cmd(group, cli_cmd("down", cli_iface_down));
  cmd= cli_add_cmd(group, cli_cmd("up", cli_iface_up));
  _register_net_node_iface_load(group);
}
