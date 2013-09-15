// ==================================================================
// @(#)registry.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Pradeep Bangera (pradeep.bangera@imdea.org)
// @date 01/03/2004
// $Id: registry.c,v 1.2 2009-08-31 09:37:08 bqu Exp $
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
#include <libgds/hash.h>
#include <libgds/memory.h>
#include <libgds/stream.h>

#include <bgp/attr/comm.h>
#include <bgp/attr/ecomm.h>
#include <bgp/filter/filter.h>
#include <bgp/filter/registry.h>
#include <bgp/route_map.h>
#include <net/util.h>
#include <util/regex.h>

#include <string.h>

static cli_t * _cli_predicate= NULL;
static cli_t * _cli_action= NULL;

static inline bgp_ft_action_t ** _action_from_context(cli_ctx_t * ctx) {
  bgp_ft_action_t ** ref= (bgp_ft_action_t **) cli_context_top_data(ctx);
  if (ref == NULL)
    cli_context_dump(gdsout, ctx);
  assert(ref != NULL);
  return ref;
}

static inline bgp_ft_matcher_t ** _matcher_from_context(cli_ctx_t * ctx) {
  bgp_ft_matcher_t ** ref= (bgp_ft_matcher_t **) cli_context_top_data(ctx);
  assert(ref != NULL);
  return ref;
}

/////////////////////////////////////////////////////////////////////
//
// PUBLIC REGISTRY API
//
/////////////////////////////////////////////////////////////////////

// -----[ ft_registry_get_error ]------------------------------------
void ft_registry_get_error(cli_error_t * error)
{
  cli_get_error_details(_cli_predicate, error);
}

// -----[ ft_registry_predicate_parser ]-----------------------------
/**
 * This function takes a string, parses it and if it matches the
 * definition of a predicate, creates and returns it.
 */
int ft_registry_predicate_parser(const char * expr,
				 bgp_ft_matcher_t ** matcher)
{
  cli_set_user_data(_cli_predicate, matcher);
  return cli_execute(_cli_predicate, expr);
}

// -----[ ft_registry_action_parser ]--------------------------------
/**
 * This function takes a string, parses it and if it matches the
 * definition of an action, creates and returns it.
 */
int ft_registry_action_parser(const char * expr,
			      bgp_ft_action_t ** action)
{
  cli_set_user_data(_cli_action, action);
  return cli_execute(_cli_action, expr);
}


/////////////////////////////////////////////////////////////////////
//
// CLI PREDICATE COMMANDS (PRIVATE)
//
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_predicate_any ---------------------------------------
static int ft_cli_predicate_any(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_comm_is -----------------------------------
static int ft_cli_predicate_comm_is(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  const char * arg;
  bgp_comm_t comm;

  // Get community
  arg= cli_get_arg_value(cmd, 0);
  if (comm_value_from_string(arg, &comm)) {
    cli_set_user_error(_cli_predicate, "invalid community \"%s\"\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *matcher= filter_match_comm_contains(comm);
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_nexthop_in --------------------------------
static int ft_cli_predicate_nexthop_in(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  const char * arg;
  ip_pfx_t pfx;
  
  // Get prefix
  arg= cli_get_arg_value(cmd, 0);
  if (str2prefix(arg,&pfx)) {
    cli_set_user_error(_cli_predicate, "invalid prefix \"%s\"\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *matcher= filter_match_nexthop_in(pfx);
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_nexthop_is --------------------------------
static int ft_cli_predicate_nexthop_is(cli_ctx_t * ctx,
				       cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  const char * arg;
  net_addr_t nh;

  // Get IP address
  arg= cli_get_arg_value(cmd, 0);
  if (str2address(arg, &nh)) {
    cli_set_user_error(_cli_predicate, "invalid next-hop \"%s\"\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *matcher= filter_match_nexthop_equals(nh);
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_prefix_is ---------------------------------
static int ft_cli_predicate_prefix_is(cli_ctx_t * ctx,
				      cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  const char * arg;
  ip_pfx_t pfx;
  
  // Get prefix
  arg= cli_get_arg_value(cmd, 0);
  if (str2prefix(arg, &pfx)) {
    cli_set_user_error(_cli_predicate, "invalid prefix \"%s\"\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *matcher= filter_match_prefix_equals(pfx);
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_prefix_in ---------------------------------
static int ft_cli_predicate_prefix_in(cli_ctx_t * ctx,
				      cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  const char * arg;
  ip_pfx_t pfx;
  
  // Get prefix
  arg= cli_get_arg_value(cmd, 0);
  if (str2prefix(arg, &pfx)) {
    cli_set_user_error(_cli_predicate, "invalid prefix \"%s\"\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *matcher= filter_match_prefix_in(pfx);
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_prefix_ge ---------------------------------
static int ft_cli_predicate_prefix_ge(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  const char * arg_pfx= cli_get_arg_value(cmd, 0);
  const char * arg_len= cli_get_arg_value(cmd, 1);
  ip_pfx_t pfx;
  unsigned int len;

  // Get prefix
  if (str2prefix(arg_pfx, &pfx)) {
    cli_set_user_error(_cli_predicate, "invalid prefix \"%s\"\n", arg_pfx);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get prefix length
  if (str_as_uint(arg_len, &len) < 0) {
    cli_set_user_error(_cli_predicate, "invalid prefix length \"%s\"\n",
	       arg_len);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Prefix length must not be longer than 32
  if (len > 32) {
    cli_set_user_error(_cli_predicate, "prefix length (%u) larger than 32\n",
		       len);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *matcher= filter_match_prefix_ge(pfx, (uint8_t) len);
  return CLI_SUCCESS;
}

// ----- ft_cli_predicate_prefix_le ---------------------------------
static int ft_cli_predicate_prefix_le(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  const char * arg_pfx= cli_get_arg_value(cmd, 0);
  const char * arg_len= cli_get_arg_value(cmd, 1);
  ip_pfx_t pfx;
  unsigned int len;
  
  // Get prefix
  if (str2prefix(arg_pfx, &pfx)) {
    cli_set_user_error(_cli_predicate, "invalid prefix \"%s\"\n", arg_pfx);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get prefix length
  if (str_as_uint(arg_len, &len) < 0) {
    cli_set_user_error(_cli_predicate, "invalid prefix length \"%s\"\n", arg_len);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Prefix length must not be longer than 32
  if (len > 32) {
    cli_set_user_error(_cli_predicate,"prefix length (%u) larger than 32\n", len);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *matcher= filter_match_prefix_le(pfx, (uint8_t) len);
  return CLI_SUCCESS;
}

extern gds_hash_set_t * pHashPathExpr;
extern ptr_array_t * paPathExpr;

// ----- ft_cli_predicate_path ---------------------------------------
static int ft_cli_predicate_path (cli_ctx_t * ctx, 
				  cli_cmd_t * cmd)
{
  bgp_ft_matcher_t ** matcher= _matcher_from_context(ctx);
  int pos = 0;
  const char * arg= cli_get_arg_value(cmd, 0);
  SPathMatch * pFilterRegEx, * pHashFilterRegEx;

  pFilterRegEx = MALLOC(sizeof(SPathMatch));
  pFilterRegEx->pcPattern= str_create(arg);

  /* Try to find the expression in the hash table 
   * if not found, insert it in the hash table and in the array.
   * else the we have found the structure added in the hash table. 
   * This structure contains the position in the array.
   */
  if ((pHashFilterRegEx= hash_set_search(pHashPathExpr, pFilterRegEx))
      == NULL) {
    pHashFilterRegEx= pFilterRegEx;
    if ((pHashFilterRegEx->pRegEx= regex_init(arg, 20)) == NULL) {
      STREAM_ERR(STREAM_LEVEL_SEVERE,
		 "Error: Invalid Regular Expression : \"%s\"\n", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }

    assert(hash_set_add(pHashPathExpr, pHashFilterRegEx) != NULL);
    if ((pos= ptr_array_add(paPathExpr, &pHashFilterRegEx)) == -1) {
      STREAM_ERR(STREAM_LEVEL_SEVERE,
		 "Error: Could not insert path reg exp in the array\n");
      return CLI_ERROR_COMMAND_FAILED;
    }
    pHashFilterRegEx->uArrayPos= pos;
  } else {
    if (pFilterRegEx->pcPattern != NULL)
      FREE(pFilterRegEx->pcPattern);
    FREE(pFilterRegEx);
    pos= pHashFilterRegEx->uArrayPos;
  }

  *matcher= filter_match_path(pos);
  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI PREDICATE REGISTRATION (PRIVATE)
//
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_register_predicate_any ------------------------------
static void ft_cli_register_predicate_any(cli_cmd_t * root)
{
  cli_add_cmd(root, cli_cmd("any", ft_cli_predicate_any));
}

// ----- ft_cli_register_predicate_comm -----------------------------
static void ft_cli_register_predicate_comm(cli_cmd_t * root)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(root, cli_cmd_group("community"));
  cmd= cli_add_cmd(group, cli_cmd("is", ft_cli_predicate_comm_is));
  cli_add_arg(cmd, cli_arg("community", NULL));
}

// ----- ft_cli_register_predicate_nexthop --------------------------
static void ft_cli_register_predicate_nexthop(cli_cmd_t * root)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(root, cli_cmd_group("next-hop"));
  cmd= cli_add_cmd(group, cli_cmd("in", ft_cli_predicate_nexthop_in));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cmd= cli_add_cmd(group, cli_cmd("is", ft_cli_predicate_nexthop_is));
  cli_add_arg(cmd, cli_arg("next-hop", NULL));
}

// ----- ft_cli_register_predicate_prefix ---------------------------
static void ft_cli_register_predicate_prefix(cli_cmd_t * root)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(root, cli_cmd_group("prefix"));
  cmd= cli_add_cmd(group, cli_cmd("is", ft_cli_predicate_prefix_is));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cmd= cli_add_cmd(group, cli_cmd("in", ft_cli_predicate_prefix_in));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cmd= cli_add_cmd(group, cli_cmd("ge", ft_cli_predicate_prefix_ge));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_arg(cmd, cli_arg("prefix-length", NULL));
  cmd= cli_add_cmd(group, cli_cmd("le", ft_cli_predicate_prefix_le));
  cli_add_arg(cmd, cli_arg("prefix", NULL));
  cli_add_arg(cmd, cli_arg("prefix-length", NULL));
}

// ----- ft_cli_register_predicate_path ------------------------------
static void ft_cli_register_predicate_path(cli_cmd_t * root)
{
  cli_cmd_t * cmd;

  cmd= cli_add_cmd(root, cli_cmd("path", ft_cli_predicate_path));
  cli_add_arg(cmd, cli_arg("regex", NULL));
}


/////////////////////////////////////////////////////////////////////
//
// CLI ACTION COMMANDS (PRIVATE)
//
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_action_accept ---------------------------------------
static int ft_cli_action_accept(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);

  *action= filter_action_accept();
  return CLI_SUCCESS;
}

// ----- ft_cli_action_deny -----------------------------------------
static int ft_cli_action_deny(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);

  *action= filter_action_deny();
  return CLI_SUCCESS;
}

// ----- ft_cli_action_jump -----------------------------------------
static int ft_cli_action_jump(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_filter_t * filter;

  filter= route_map_get(arg);
  if (filter == NULL) {
    *action= NULL;
    cli_set_user_error(_cli_action, "no route-map named \"%s\".\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action= filter_action_jump(filter);
  return CLI_SUCCESS;
}

// ----- ft_cli_action_call -----------------------------------------
static int ft_cli_action_call(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_filter_t * filter;

  filter= route_map_get(arg);
  if (filter == NULL) {
    *action= NULL;
    cli_set_user_error(_cli_action, "no route-map named \"%s\".\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action= filter_action_call(filter);
  return CLI_SUCCESS;
}

// ----- ft_cli_action_local_pref -----------------------------------
static int ft_cli_action_local_pref(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned long int pref;

  if (str_as_ulong(arg, &pref)) {
    *action= NULL;
    cli_set_user_error(_cli_action, "invalid local-pref %s\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action= filter_action_pref_set(pref);
  return CLI_SUCCESS;
}

// ----- ft_cli_action_metric ---------------------------------------
static int ft_cli_action_metric(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned long int metric;

  if (!strcmp(arg, "internal")) {
    *action= filter_action_metric_internal();
  } else {
    if (str_as_ulong(arg, &metric)) {
      *action= NULL;
      cli_set_user_error(_cli_action, "invalid metric %d\n", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
    *action= filter_action_metric_set(metric);
  }
  return CLI_SUCCESS;
}

// ----- ft_cli_action_path_prepend ---------------------------------
static int ft_cli_action_path_prepend(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0); 
  unsigned long int amount;

  if (str_as_ulong(arg, &amount)) {
    *action= NULL;
    cli_set_user_error(_cli_action, "invalid prepending amount %s\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  *action= filter_action_path_prepend(amount);
   return CLI_SUCCESS;
}

// ----- ft_cli_action_path_insert -------------------------------------
static int ft_cli_action_path_insert(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg_asn= cli_get_arg_value(cmd, 0); //get the ASN
  const char * arg_amount= cli_get_arg_value(cmd, 1); //get the amount
  asn_t asn;
  unsigned long int amount;

  if (str2asn(arg_asn, &asn)) {
    *action= NULL;
    cli_set_user_error(_cli_action, "invalid ASN %s\n", arg_asn);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (str_as_ulong(arg_amount, &amount)) {
    *action= NULL;
    cli_set_user_error(_cli_action, "invalid amount %s\n", arg_amount);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action= filter_action_path_insert(asn, amount);
  return CLI_SUCCESS;
}

// ----- ft_cli_action_path_rem_private -----------------------------
static int ft_cli_action_path_rem_private(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);

  *action= filter_action_path_rem_private();
   return CLI_SUCCESS;
}

// ----- ft_cli_action_comm_add -------------------------------------
static int ft_cli_action_comm_add(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_comm_t comm;

  if (comm_value_from_string(arg, &comm)) {
    *action= NULL;
    cli_set_user_error(_cli_action, "invalid community \"%s\"\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action= filter_action_comm_append(comm);
  return CLI_SUCCESS;
}

// ----- ft_cli_action_comm_remove ----------------------------------
static int ft_cli_action_comm_remove(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_comm_t comm;

  if (comm_value_from_string(arg, &comm)) {
    *action= NULL;
    cli_set_user_error(_cli_action, "invalid community \"%s\"\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action= filter_action_comm_remove(comm);
  return CLI_SUCCESS;
}

// ----- ft_cli_action_comm_strip -----------------------------------
static int ft_cli_action_comm_strip(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);

  *action= filter_action_comm_strip();
  return CLI_SUCCESS;
}

// ----- ft_cli_action_red_comm_ignore ------------------------------
static int ft_cli_action_red_comm_ignore(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int target;

  if (str_as_uint(arg, &target)) {
    cli_set_user_error(_cli_action, "invalid target %s\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action=
    filter_action_ecomm_append(ecomm_red_create_as(ECOMM_RED_ACTION_IGNORE,
						   0, target));
  return CLI_SUCCESS;
}

// ----- ft_cli_action_red_comm_prepend -----------------------------
static int ft_cli_action_red_comm_prepend(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg_amount= cli_get_arg_value(cmd, 0);
  const char * arg_target= cli_get_arg_value(cmd, 1);
  unsigned int amount;
  unsigned int target;
  
  if (str_as_uint(arg_amount, &amount)) {
    cli_set_user_error(_cli_action, "invalid prepending amount %s\n", arg_amount);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (str_as_uint(arg_target, &target)) {
    cli_set_user_error(_cli_action, "invalid target %s\n", arg_target);
    return CLI_ERROR_COMMAND_FAILED;
  }

  *action=
    filter_action_ecomm_append(ecomm_red_create_as(ECOMM_RED_ACTION_PREPEND,
						   amount, target));
  return CLI_SUCCESS;
}

#ifdef __EXPERIMENTAL__
// ----- ft_cli_action_pref_comm ------------------------------------
static int ft_cli_action_pref_comm(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  bgp_ft_action_t ** action= _action_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int pref;
  
  if (str_as_uint(arg, &pref)) {
    cli_set_user_error(_cli_action, "invalid preference value %s\n", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  *action= filter_action_ecomm_append(ecomm_pref_create(pref));
  return CLI_SUCCESS;
}
#endif

/////////////////////////////////////////////////////////////////////
//
// CLI ACTION REGISTRATION (PRIVATE)
//
/////////////////////////////////////////////////////////////////////

// ----- ft_cli_register_action_accept_deny -------------------------
static void ft_cli_register_action_accept_deny(cli_cmd_t * root)
{
  cli_add_cmd(root, cli_cmd("accept", ft_cli_action_accept));
  cli_add_cmd(root, cli_cmd("deny", ft_cli_action_deny));
}

// ----- ft_cli_register_action_local_pref --------------------------
static void ft_cli_register_action_local_pref(cli_cmd_t * root)
{
  cli_cmd_t * cmd;

  cmd= cli_add_cmd(root, cli_cmd("local-pref", ft_cli_action_local_pref));
  cli_add_arg(cmd, cli_arg("preference", NULL));
}

// ----- ft_cli_register_action_jump ---------------------------------
static void ft_cli_register_action_jump(cli_cmd_t * root)
{
  cli_cmd_t * cmd;

  cmd= cli_add_cmd(root, cli_cmd("jump", ft_cli_action_jump));
  cli_add_arg(cmd, cli_arg("name", NULL));
}

// ----- ft_cli_register_action_call ---------------------------------
static void ft_cli_register_action_call(cli_cmd_t * root)
{
  cli_cmd_t * cmd;

  cmd= cli_add_cmd(root, cli_cmd("call", ft_cli_action_call));
  cli_add_arg(cmd, cli_arg("name", NULL));
}

// ----- ft_cli_register_action_metric ------------------------------
static void ft_cli_register_action_metric(cli_cmd_t * root)
{
  cli_cmd_t * cmd;

  cmd= cli_add_cmd(root, cli_cmd("metric", ft_cli_action_metric));
  cli_add_arg(cmd, cli_arg("metric", NULL));
}

// ----- ft_cli_register_action_path --------------------------------
static void ft_cli_register_action_path(cli_cmd_t * root)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(root, cli_cmd_group("as-path"));
  cmd= cli_add_cmd(group, cli_cmd("prepend",
				  ft_cli_action_path_prepend));
  cli_add_arg(cmd, cli_arg("amount", NULL));
  cmd= cli_add_cmd(group, cli_cmd("insert",
				  ft_cli_action_path_insert));
  cli_add_arg(cmd, cli_arg("asn", NULL));
  cli_add_arg(cmd, cli_arg("amount", NULL));
  cmd= cli_add_cmd(group, cli_cmd("remove-private",
				  ft_cli_action_path_rem_private));
}

// ----- ft_cli_register_action_comm --------------------------------
static void ft_cli_register_action_comm(cli_cmd_t * root)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(root, cli_cmd_group("community"));
  cmd= cli_add_cmd(group, cli_cmd("strip", ft_cli_action_comm_strip));
  cmd= cli_add_cmd(group, cli_cmd("add", ft_cli_action_comm_add));
  cli_add_arg(cmd, cli_arg("community", NULL));
  cmd= cli_add_cmd(group, cli_cmd("remove", ft_cli_action_comm_remove));
  cli_add_arg(cmd, cli_arg("community", NULL));
}

// ----- ft_cli_register_action_red_comm ----------------------------
static void ft_cli_register_action_red_comm(cli_cmd_t * root)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(root, cli_cmd_group("red-community"));
  group= cli_add_cmd(group, cli_cmd_group("add"));
  cmd= cli_add_cmd(group, cli_cmd("ignore", ft_cli_action_red_comm_ignore));
  cli_add_arg(cmd, cli_arg("as-num|address", NULL));
  cmd= cli_add_cmd(group, cli_cmd("prepend", ft_cli_action_red_comm_prepend));
  cli_add_arg(cmd, cli_arg("amount", NULL));
  cli_add_arg(cmd, cli_arg("as-num|address", NULL));
}

#ifdef __EXPERIMENTAL__
// ----- ft_cli_register_action_pref_comm ---------------------------
static void ft_cli_register_action_pref_comm(cli_cmd_t * root)
{
  cli_cmd_t * group, * cmd;
  
  group= cli_add_cmd(root, cli_cmd_group("pref-community"));
  cmd= cli_add_cmd(group, cli_cmd("add", ft_cli_action_pref_comm));
  cli_add_arg(cmd, cli_arg("pref", NULL));
}
#endif /* __EXPERIMENTAL__ */

// ----- ft_cli_register_action_ext_community -----------------------
/*static void ft_cli_register_action_ext_community(cli_cmd_t * root)
{
cli_cmd_t * group,  cmd;

group= cli_add_cmd(root, cli_cmd_group("ext-community"));
cmd= cli_add_cmd(group, cli_cmd("add", ???));
}*/


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(A) sizeof(A)/sizeof(A[0])

typedef void (*cli_register_func)(cli_cmd_t * root);

static cli_register_func _cli_predicates[]= {
  ft_cli_register_predicate_any,
  ft_cli_register_predicate_comm,
  ft_cli_register_predicate_nexthop,
  ft_cli_register_predicate_prefix,
  ft_cli_register_predicate_path,
};

static cli_register_func _cli_actions[]= {
  ft_cli_register_action_accept_deny,
  ft_cli_register_action_local_pref,
  ft_cli_register_action_metric,
  ft_cli_register_action_path,
  ft_cli_register_action_comm,
  //ft_cli_register_action_ext_community,
  ft_cli_register_action_red_comm,
#ifdef __EXPERIMENTAL__
  ft_cli_register_action_pref_comm,
#endif
  ft_cli_register_action_call,
  ft_cli_register_action_jump,
};

// -----[ _ft_registry_init ]----------------------------------------
void _ft_registry_init()
{
  unsigned int index;

  // Register predicates
  _cli_predicate= cli_create();
  for (index= 0; index < ARRAY_SIZE(_cli_predicates); index++)
    _cli_predicates[index](cli_get_root_cmd(_cli_predicate));

  // Register actions
  _cli_action= cli_create();
  for (index= 0; index < ARRAY_SIZE(_cli_actions); index++)
    _cli_actions[index](cli_get_root_cmd(_cli_action));
}

// -----[ _ft_registry_done ]----------------------------------------
void _ft_registry_done()
{
  cli_destroy(&_cli_predicate);
  cli_destroy(&_cli_action);
}
