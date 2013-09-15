// ==================================================================
// @(#)bgp_filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @date 27/02/2008
// $Id: bgp_filter.c,v 1.3 2009-03-24 15:58:13 bqu Exp $
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

#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>

#include <bgp/filter/filter.h>
#include <bgp/filter/parser.h>
#include <bgp/filter/predicate_parser.h>
#include <cli/common.h>
#include <cli/context.h>

// -----[ cli_bgp_filter_rule_match ]--------------------------------
/**
 * context: {?, rule}
 * tokens: {?, predicate}
 */
static int cli_bgp_filter_rule_match(cli_ctx_t * ctx,
				     cli_cmd_t * cmd)
{
  bgp_ft_rule_t * rule= _rule_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_ft_matcher_t * matcher;
  int result;

  // Parse predicate
  result= predicate_parser(arg, &matcher);
  if (result != PREDICATE_PARSER_SUCCESS) {
    cli_set_user_error(cli_get(), "invalid predicate \"%s\" (%s)",
		       arg, predicate_parser_strerror(result));
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (rule->matcher != NULL)
    filter_matcher_destroy(&rule->matcher);
  rule->matcher= matcher;

  return CLI_SUCCESS;
}

// -----[ cli_bgp_filter_rule_action ]-------------------------------
/**
 * context: {?, rule}
 * tokens: {?, action}
 */
static int cli_bgp_filter_rule_action(cli_ctx_t * ctx,
				      cli_cmd_t * cmd)
{
  bgp_ft_rule_t * rule= _rule_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  bgp_ft_action_t * action;
  bgp_ft_action_t * prev;

  // Parse action
  if (filter_parser_action(arg, &action)) {
    cli_set_user_error(cli_get(), "invalid action \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  if (rule->action == NULL) {
    rule->action= action;
  } else {
    prev= rule->action;
    while (prev->next_action != NULL)
      prev= prev->next_action;
    prev->next_action= action;
  }
  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_filter_add_rule -------------------------
/**
 * context: {?, &filter}
 * tokens: {?}
 */
int cli_ctx_create_bgp_filter_add_rule(cli_ctx_t * ctx, cli_cmd_t * cmd,
				       void ** item)
{
  bgp_filter_t ** filter= _filter_from_context(ctx);
  bgp_ft_rule_t * rule;

  // If the filter is empty, create it
  if (*filter == NULL)
    *filter= filter_create();

  // Create rule and put it on the context
  rule= filter_rule_create(NULL, NULL);
  filter_add_rule2(*filter, rule);

  *item= rule;

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_filter_insert_rule ----------------------
/**
 * context: {?, &filter}
 * tokens: {?, pos}
 */
int cli_ctx_create_bgp_filter_insert_rule(cli_ctx_t * ctx, cli_cmd_t * cmd,
					  void ** item)
{
  bgp_filter_t ** filter= _filter_from_context(ctx);
  bgp_ft_rule_t * rule;
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int index;

  // Get insertion position
  if (str_as_uint(arg, &index)) {
    cli_set_user_error(cli_get(), "invalid index \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // If the filter is empty, create it
  if (*filter == NULL)
    *filter= filter_create();

  // Create rule and put it on the context
  rule= filter_rule_create(NULL, NULL);
  filter_insert_rule(*filter, index, rule);

  *item= rule;

  return CLI_SUCCESS;
}

// ----- cli_bgp_filter_remove_rule ---------------------------------
/**
 * context: {?, &filter}
 * tokens: {?, pos}
 */
int cli_bgp_filter_remove_rule(cli_ctx_t * ctx,
			       cli_cmd_t * cmd)
{
  bgp_filter_t ** filter= _filter_from_context(ctx);
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int index;

  if (*filter == NULL) {
    cli_set_user_error(cli_get(), "filter contains no rule");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  // Get index of rule to be removed
  if (str_as_uint(arg, &index)) {
    cli_set_user_error(cli_get(), "invalid index \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Remove rule
  if (filter_remove_rule(*filter, index)) {
    cli_set_user_error(cli_get(), "could not remove rule %u", index);
    return CLI_ERROR_COMMAND_FAILED;
  }

  return CLI_SUCCESS;
}

// ----- cli_ctx_create_bgp_filter_rule -----------------------------
/**
 * context: {?, &filter}
 * tokens: {?}
 */
int cli_ctx_create_bgp_filter_rule(cli_ctx_t * ctx, cli_cmd_t * cmd,
				   void ** item)
{
  cli_set_user_error(cli_get(), "not yet implemented, sorry.");
  return CLI_ERROR_CTX_CREATE;
}

// ----- cli_ctx_destroy_bgp_filter_rule ----------------------------
/**
 *
 */
void cli_ctx_destroy_bgp_filter_rule(void ** item)
{
}

// ----- cli_bgp_filter_show-----------------------------------------
/**
 * context: {?, &filter}
 * tokens: {?}
 */
int cli_bgp_filter_show(cli_ctx_t * ctx,
			cli_cmd_t * cmd)
{
  bgp_filter_t ** filter= _filter_from_context(ctx);
  filter_dump(gdsout, *filter);
  return CLI_SUCCESS;
}

// -----[ cli_register_bgp_filter_rule ]-----------------------------
void cli_register_bgp_filter_rule(cli_cmd_t * parent)
{
  cli_cmd_t * cmd;

  cmd= cli_add_cmd(parent, cli_cmd("match", cli_bgp_filter_rule_match));
  cli_add_arg(cmd, cli_arg("predicate", NULL));
  cmd= cli_add_cmd(parent, cli_cmd("action", cli_bgp_filter_rule_action));
  cli_add_arg(cmd, cli_arg("actions", NULL));
}
