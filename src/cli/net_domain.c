// ==================================================================
// @(#)net_domain.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Stefano Iasi (stefanoia@tin.it)
// @date 29/07/2005
// $Id: net_domain.c,v 1.11 2009-03-24 15:58:43 bqu Exp $
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

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/stream.h>

#include <cli/common.h>
#include <cli/context.h>
#include <net/igp_domain.h>
#include <net/ospf_deflection.h>
#include <net/util.h>

// -----[ cli_net_add_domain ]---------------------------------------
/**
 * context: {}
 * tokens: {id, type}
 */
int cli_net_add_domain(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_id= cli_get_arg_value(cmd, 0);
  const char * arg_type= cli_get_arg_value(cmd, 1);
  unsigned int id;
  igp_domain_type_t type;
  igp_domain_t* domain;

  // Check domain id
  if (str_as_uint(arg_id, &id) || (id > 65535)) {
    cli_set_user_error(cli_get(), "invalid domain id %s", arg_id);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Check domain type
  if (!strcmp(arg_type, "igp")) {
    type= IGP_DOMAIN_IGP;
  } else if (!strcmp(arg_type, "ospf")) {
#ifdef OSPF_SUPPORT 
    type= IGP_DOMAIN_OSPF;
#else 
    cli_set_user_error(cli_get(), "not compiled with OSPF support");
    return CLI_ERROR_COMMAND_FAILED;
#endif    
  } else {
    cli_set_user_error(cli_get(), "unknown domain type %s", arg_type);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Check that a domain with the same id does not exist
  domain= igp_domain_create(id, type);
  if (network_add_igp_domain(network_get_default(), domain) != ESUCCESS) {
    igp_domain_destroy(&domain);
    cli_set_user_error(cli_get(), "domain %d already exists", id);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_net_domain ----------------------------------
/*
 * context: {}
 * tokens: {id}
 */
static int cli_ctx_create_net_domain(cli_ctx_t * ctx, cli_cmd_t * cmd,
				     void ** ppItem)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int id;
  igp_domain_t* domain;

  // Get domain id
  if (str2domain_id(arg, &id)) {
    cli_set_user_error(cli_get(), "invalid domain id \"%s\"", arg);
    return CLI_ERROR_CTX_CREATE;
  }

  domain= network_find_igp_domain(network_get_default(), id);
  if (domain == NULL) {
    cli_set_user_error(cli_get(), "unable to find domain \"%d\"", id);
    return CLI_ERROR_CTX_CREATE;
  }
  *ppItem= domain;
  return CLI_SUCCESS;
}

// ----- cli_ctx_destroy_net_domain ---------------------------------
static void cli_ctx_destroy_net_domain(void ** ppItem)
{
}

// ----- cli_net_domain_show_info -----------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
static int cli_net_domain_show_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  igp_domain_t* domain= _igp_domain_from_context(ctx);
  igp_domain_info(gdsout, domain);
  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_nodes ----------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
static int cli_net_domain_show_nodes(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  igp_domain_t* domain= _igp_domain_from_context(ctx);
  igp_domain_dump(gdsout, domain);
  return CLI_SUCCESS;
}

// ----- cli_net_domain_show_subnets --------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
/*int cli_net_domain_show_subnets(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  igp_domain_t* pDomain;

  // Get domain from context
  pDomain= cli_context_get_item_at_top(ctx);

  igp_domain_dump(stdout, pDomain);
  return CLI_SUCCESS;
  }*/

// ----- cli_net_domain_compute -------------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
static int cli_net_domain_compute(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  igp_domain_t * domain= _igp_domain_from_context(ctx);
  int keep_spt= cli_has_opt_value(cmd, "keep-spt");
  if (igp_domain_compute(domain, keep_spt) != CLI_SUCCESS) {
    cli_set_user_error(cli_get(), "IGP routes computation failed.\n");
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_net_domain_links_igp_weight ]--------------------------
/**
 * context: {domain}
 * tokens : {}
 */
/*static int cli_net_domain_links_igp_weight(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  igp_domain_t * domain= _igp_domain_from_context(ctx);
  return CLI_SUCCESS;
  }*/

#ifdef OSPF_SUPPORT
// ----- cli_net_domain_check-deflection -------------------------------------
/**
 * context: {domain}
 * tokens: {}
 */
static int cli_net_domain_check_deflection(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  igp_domain_t * domain= _igp_domain_from_context(ctx);
  ospf_domain_deflection(domain);
  return CLI_SUCCESS;
}
#endif /* OSPF_SUPPORT */


/////////////////////////////////////////////////////////////////////
//
// CLI_COMMANDS REGISTRATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _register_net_domain_show ]--------------------------------
static void _register_net_domain_show(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("show"));
  cli_add_cmd(group, cli_cmd("info", cli_net_domain_show_info));
  cli_add_cmd(group, cli_cmd("nodes", cli_net_domain_show_nodes));
  //cli_add_cmd(group, cli_cmd("subnets", cli_net_domain_show_subnets));
}

// ----- cli_register_net_domain ------------------------------------
void cli_register_net_domain(cli_cmd_t * parent)
{
  cli_cmd_t * group;
  cli_cmd_t * cmd;
  group= cli_add_cmd(parent, cli_cmd_ctx("domain",
					 cli_ctx_create_net_domain,
					 cli_ctx_destroy_net_domain));
  cli_add_arg(group, cli_arg("id", NULL));
  cmd= cli_cmd("compute", cli_net_domain_compute);
  cli_add_opt(cmd, cli_opt("keep-spt", NULL));
  cli_add_cmd(group, cmd);
  /*cli_add_cmd(group, cli_cmd("links-igp-weight",
    cli_net_domain_links_igp_weight));*/
#ifdef OSPF_SUPPORT
  cli_cmds_add(sub_cmds, cli_cmd_create("check-deflection",
					cli_net_domain_check_deflection,
					NULL, NULL));
#endif /* OSPF_SUPPORT */
  _register_net_domain_show(group);
}
