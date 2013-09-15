// ==================================================================
// @(#)completion.c
//
// Provides functions to auto-complete commands/parameters in the CLI
// in interactive mode.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/11/2002
// $Id: completion.c,v 1.3 2009-03-24 16:29:41 bqu Exp $
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
// NOTE 1:
//   The CLI auto-completion feature will not work if called from
//   multiple threads. This is due to how GNU readline handles the
//   so-called completion generators.
//
//   For the time being, c-bgp has a single command-line interface at
//   a time. However, in the future, it could evolve towards a
//   route computation server with multiple clients.
//
// NOTE 2:
//   The completion generators must use malloc() instead of libgds's
//   MALLOC() since GNU readline is responsible for freeing the
//   memory and will not make use of libgds's FREE(). The standard 
//   strdup() function can be safely used.
// ==================================================================


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/cli.h>
#include <libgds/cli_commands.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/memory.h>

#include <cli/common.h>

//#define DEBUG
#include <libgds/debug.h>

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>

#ifdef HAVE_RL_COMPLETION_MATCHES
// -----[ completion state ]-----------------------------------------
/**
 * The following variables are used to communicate information to the
 * completion generators. Due to the way GNU readline talks with
 * completion generators, they need to keep state in static variables.
 * This is not thread-safe and calling these functions from multiple
 * threads at the same time will result in unpredictable behaviour.
 *
 * '_compl_elem' is the basis CLI element where completion starts
 *    CLI_ELEM_CMD -> cmd
 *    CLI_ELEM_ARG -> arg
 */
static cli_elem_t _compl_elem;

/**
 * '_compl_cmds' is a list of possible subcommands matching the
 * current completion text.
 *
 * NOTE:
 *   '_compl_cmds' need to be initialized to NULL by this object's
 *   constructor (init function). It needs to be freed by this object's
 *   destructor (destroy function)
 */
static cli_cmds_t * _compl_cmds;
static cli_args_t * _compl_opts;

// -----[ _cli_display_completion ]----------------------------------
/**
 * This function displays the available completions in a non-default
 * way. In C-BGP, we show a vertical list of commands while in the
 * default readline behaviour, the matches are displayed on an
 * horizontal line.
 *
 * NOTE: the first item in 'matches' corresponds to the value being
 * completed, while the subsequent items are the available
 * matches. This is not explicitly explained in readline's
 * documentation.
 */
static void _cli_display_completion(char ** matches, int num_matches,
				    int max_length)
{
  unsigned int index;

  fprintf(stdout, "\n");
  for (index= 1; index <= num_matches; index++) {
    fprintf(stdout, "\t%s\n", matches[index]);
  }
#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
}

// -----[ _cli_compl_cmd_generator ]---------------------------------
/**
 * This function generates completion for CLI commands. It relies on
 * the list of sub-commands of the current command.
 *
 */
static char * _cli_compl_cmd_generator(const char * text, int state)
{
  static int index;
  cli_cmd_t * cmd;
  char * result= NULL;

  __debug("_cmd_gen\n");

  // Check that a command is to be completed.
  assert(_compl_elem.type == CLI_ELEM_CMD);

  cli_get_cmd_context(cli_get(), &cmd, NULL);

  // Initialize generator
  if (state == 0) {
    if (_compl_cmds != NULL)
      cli_cmds_destroy(&_compl_cmds);
    _compl_cmds= cli_matching_subcmds(cli_get(), _compl_elem.cmd, text,
				      _compl_elem.cmd == cmd);
    index= 0;
  }

  __debug(" +-- num_cmds:%u\n", cli_cmds_num(_compl_cmds));

  // Generate a single completion (if available)
  if ((_compl_cmds != NULL) && (index < cli_cmds_num(_compl_cmds))) {
    cmd= cli_cmds_at(_compl_cmds, index++);
    result= strdup(cmd->name);
  }

  rl_attempted_completion_over= (result == NULL);
  return result;
}

// -----[ _cli_compl_opt_generator ]---------------------------------
/**
 * Generates the list of options of the current command.
 */
static char * _cli_compl_opt_generator(const char * text, int state)
{
  static int index;
  cli_arg_t * opt;
  char * result= NULL;

  __debug("_opt_gen\n");

  // Check that a command is to be completed.
  assert(_compl_elem.type == CLI_ELEM_CMD);

  // Initialize generator
  if (state == 0) {
    if (_compl_opts != NULL)
      cli_args_destroy(&_compl_opts);
    _compl_opts= cli_matching_opts(_compl_elem.cmd, text);
    index= 0;
  }

  __debug(" +-- num_opts:%u\n", cli_args_num(_compl_opts));

  // Generate a single completion (if available)
  //  --opt
  //  --opt= (if option needs a value)
  if ((_compl_opts != NULL) && (index < cli_args_num(_compl_opts))) {
    opt= cli_args_at(_compl_opts, index++);
    result= (char *) malloc((strlen(opt->name)+3)*sizeof(char));
    strcpy(result, "--");
    strcpy(result+2, opt->name);
    if (opt->need_value)
      rl_completion_append_character= '=';
    else
      rl_completion_append_character= ' ';
  }

  rl_attempted_completion_over= (result == NULL);
  return result;
}

// -----[ _cli_compl_opt_value_generator ]---------------------------
static char * _cli_compl_opt_value_generator(const char * text, int state)
{
  char * result;

  __debug("_opt_value_gen\n");

  // Check that a valid option is to be completed.
  assert(_compl_elem.type == CLI_ELEM_ARG);

  // Advance to value part (after first "=" char)
  text= strchr(text, '=');
  assert(text != NULL);
  text++;
  
  __debug(" +-- state:%d\n", state);
  __debug(" +-- text :\"%s\"\n", text);

  if (_compl_elem.arg != NULL) {
    if (_compl_elem.arg->ops.enumerate != NULL) {
      result= _compl_elem.arg->ops.enumerate(text, state);
      if (result != NULL) {
	rl_attempted_completion_over= 0;
	return result;
      }
    } else {
      fprintf(stdout, "\n");
#ifdef HAVE_RL_ON_NEW_LINE
      rl_on_new_line();
#endif
      fprintf(stdout, "\t<option value>\n");
#ifdef HAVE_RL_ON_NEW_LINE
      rl_on_new_line();
#endif
    }
  }

  rl_attempted_completion_over= 1;
  return NULL;
}

// -----[ _cli_compl_arg_generator ]---------------------------------
/**
 * This function generates completion for CLI parameter values. The
 * generator first relies on a possible enumeration function [not
 * implemented yet], then on a parameter checking function.
 *
 * Note: use malloc() for the returned matches, not MALLOC().
 */
static char * _cli_compl_arg_generator(const char * text, int state)
{
  char * result= NULL;

  __debug("_arg_gen(%s, %d)\n", text, state);

  // Check that a valid parameter is to be completed.
  assert(_compl_elem.type == CLI_ELEM_ARG);

  __debug(" +-- state:%d\n", state);
  __debug(" +-- text :\"%s\"\n", text);

  if (_compl_elem.arg != NULL) {
    if (_compl_elem.arg->ops.enumerate != NULL) {
      result= _compl_elem.arg->ops.enumerate(text, state);
      __debug(" +-- match:\"%s\"\n", result);
      if (result != NULL) {
	rl_attempted_completion_over= 0;
	return result;
      }
    } else {
      fprintf(stdout, "\n");
#ifdef HAVE_RL_ON_NEW_LINE
      rl_on_new_line();
#endif
      fprintf(stdout, "\t<%s>\n", _compl_elem.arg->name);
#ifdef HAVE_RL_ON_NEW_LINE
      rl_on_new_line();
#endif
    }
  }

  rl_attempted_completion_over= 1;
  return NULL;
}

// -----[ _cli_compl ]-----------------------------------------------
/**
 *
 */
static char ** _cli_compl (const char * text, int start, int end)
{
  cli_t * cli= cli_get();
  char * start_cmd;
  char ** matches= NULL;
  int status= CLI_SUCCESS;

  // Authorize readline's completion.
  rl_attempted_completion_over= 0;
  rl_completion_append_character= ' ';

  // Find context command.
  _compl_elem.type= CLI_ELEM_CMD;
  cli_get_cmd_context(cli, &_compl_elem.cmd, NULL);
  
  // Match beginning of readline buffer.
  start_cmd= str_ncreate(rl_line_buffer, start);
  status= cli_complete(cli, start_cmd, text, &_compl_elem);
  str_destroy(&start_cmd);

  __debug("_cli_compl\n"
	  " +-- line  :\"%s\" ([%d-%d])\n"
	  " +-- text:\"%s\"\n"
	  " +-- status:%d\n"
	  " +-- type:%d\n",
	  rl_line_buffer, start, end, text, status, _compl_elem.type);

  switch (status) {
    /*case CLI_SUCCESS:
    fprintf(stdout, "DEFAULT cmd\n");
    matches= rl_completion_matches(text, _cli_compl_default_generator);
    break;*/

  case CLI_ERROR_UNKNOWN_CMD:
    __debug("enumerate cmd\n");
    matches= rl_completion_matches(text, _cli_compl_cmd_generator);
    break;

  case CLI_ERROR_MISSING_ARG:
    __debug("enumerate arg\n");
    matches= rl_completion_matches(text, _cli_compl_arg_generator);
    break;

  case CLI_ERROR_UNKNOWN_OPT:
    __debug("enumerate opt\n");
    matches= rl_completion_matches(text, _cli_compl_opt_generator);
    break;

  case CLI_ERROR_MISSING_OPT_VALUE:
    __debug("enumerate opt-value\n");
    matches= rl_completion_matches(text, _cli_compl_opt_value_generator);
    break;

  default:
    matches= NULL;
    rl_attempted_completion_over= 1;
  }

  return matches;
}
#endif /* HAVE_RL_COMPLETION_MATCHES */

#endif /* HAVE_READLINE_READLINE_H */
#endif /* HAVE_LIBREADLINE */


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _rl_completion_init ]--------------------------------------
/**
 * Initializes the auto-completion, in cooperation with GNU readline.
 * -> replaces the completion display function with ours.
 * -> replaces the completion function with ours if readline's
 *    rl_completion_matches() is available.
 */
void _rl_completion_init()
{
#ifdef HAVE_LIBREADLINE
#ifdef HAVE_READLINE_READLINE_H

#ifdef HAVE_RL_COMPLETION_MATCHES
  // Initialize static variables used by completion generators
  _compl_cmds= NULL;
  _compl_opts= NULL;

  // Setup alternate completion function.
  rl_attempted_completion_function= _cli_compl;
  // Setup alternate completion display function.
  rl_completion_display_matches_hook= _cli_display_completion;
#endif /* HAVE_RL_COMPLETION_MATCHES */

  // Break on space and tab
  rl_basic_word_break_characters= " \t";

#endif /* HAVE_READLINE_READLINE_H */
#endif /* HAVE_LIBREADLINE */
}

// -----[ _rl_completion_done ]--------------------------------------
/**
 * Frees the resources allocated for the auto-completion.
 */
void _rl_completion_done()
{
#ifdef HAVE_RL_COMPLETION_MATCHES
  // Destroy static variables used by completion generators
  cli_cmds_destroy(&_compl_cmds);
  cli_args_destroy(&_compl_opts);
#endif /* HAVE_RL_COMPLETION_MATCHES */
}
