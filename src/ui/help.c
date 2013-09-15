// ==================================================================
// @(#)help.c
//
// Provides functions to obtain help from the CLI in interactive mode.
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 22/11/2002
// $Id: help.c,v 1.7 2009-03-24 16:29:41 bqu Exp $
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
// Use cases
// ---------
//
//   U1: user wants to know what are the available commands in the
//   current context. Example: context is "net". User types "help"
//   without argument or invokes the help key ("?") without a command
//   typed. Help system should list available commands for this
//   context. In this case, a list is provided that contains "add",
//   "node", ... (list of subcommands for "net")
//
//   U2a: user wants to know how to use a specific command in the
//   current context. Example context is "net". User types
//   "help add node". Help system identifies it is a valid command,
//   locates documentation and call pager.
//
//   U2b: same case as U2a except user types "help add". Help system
//   identifies it is a group. A group only has subcommands, hence
//   fallback to U1 and display list of sub-commands.
//
//   U2c: same case as U2a except user types "help node". Help system
//   identifies it is a context command. A context command can
//   require arguments. Help system locates documentation and call
//   pager.
//
//   U3: user is in the middle of typing a command and wants to
//   obtain help about the current argument/option. User invokes the
//   help key ("?") with a command typed. Help system try to
//   identifies the command and provide contextual help. Behaviour
//   not yet well defined here. Example: context is "net", user types
//   "net add node " then invokes "?". Help system should provide
//   help for the "net add node" command and especially for the
//   <addr> argument which is expected at the place where "?" was
//   typed.
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/cli_commands.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/memory.h>
#include <libgds/str_util.h>

#include <api.h>
#include <cli/common.h>
#include <ui/pager.h>

#ifdef HAVE_LIBREADLINE
# ifdef HAVE_READLINE_READLINE_H
#  include <readline/readline.h>
# endif
# include <ui/rl.h>
#endif

GDS_ARRAY_TEMPLATE(str_array, char *, 0, NULL, NULL, NULL)
static str_array_t * _help_paths= NULL;

// -----[ _cli_cmd_get_path ]----------------------------------------
/**
 * Return the complete command path for the given command.
 */
static char * _cli_cmd_get_path(cli_cmd_t * cmd)
{
  char * path= NULL;

  // Concatenate all command names. Stop before root command i.e.
  // when parent is NULL
  while ((cmd != NULL) && (cmd->parent != NULL)) {
    if (strcmp(cmd->name, "")) {
      if (path == NULL)
	path= str_create(cmd->name);
      else
	path= str_prepend(str_prepend(path, "_"), cmd->name);
    }
    cmd= cmd->parent;
  }
  return path;
}

// -----[ _cli_help_cmd_str ]----------------------------------------
static int _cli_help_cmd_str(const char * str)
{
  char * cmd_filename= str_create(str);
  char * filename;
  int result= -1;
  unsigned int index;
  char * r_filename;

  str_translate(cmd_filename, " ", "_");
  cmd_filename= str_append(cmd_filename, ".txt");

  // Try every known documentation path until the corresponding file
  // is found.
  for (index= 0; index < str_array_size(_help_paths); index++) {
    filename= str_create(_help_paths->data[index]);
    filename= str_append(filename, cmd_filename);
    result= params_replace(filename, libcbgp_get_param_lookup(),
			   &r_filename, 0);
    // Only use paths with all variables resolved
    // (do not accept unknown variables, result != 0)
    if (result == 0) {
      result= pager_run(r_filename);
      FREE(r_filename);
    }
    str_destroy(&filename);
    if (result == PAGER_SUCCESS)
      break;
  }

  // If the help file could not be found in any of the search paths
  // display short error message with list of search paths. If paths
  // contain variables, resolve them. Display an error if a path
  // contains unknown variables.
  if (result != PAGER_SUCCESS) {
    stream_printf(gdserr, "Help file \"\%s\" not found.\n", cmd_filename);
    stream_printf(gdserr, "The following locations were searched :\n");
    for (index= 0; index < str_array_size(_help_paths); index++) {
      filename= str_create(_help_paths->data[index]);
      stream_printf(gdserr, "* %s", filename);
      result= params_replace(filename, libcbgp_get_param_lookup(),
			     &r_filename, 0);
      if (result == 0) {
	stream_printf(gdserr, " (%s)", r_filename);
	FREE(r_filename);
      } else
	stream_printf(gdserr, " (undefined variable(s))");
      stream_printf(gdserr, "\n");
      str_destroy(&filename);
    }
  }

  str_destroy(&cmd_filename);
  return result;
}

// -----[ _cli_help_cmd ]--------------------------------------------
/**
 * Display help for a context: gives the available sub-commands and
 * their parameters.
 */
static void _cli_help_cmd(cli_cmd_t * cmd)
{
  char * cmd_filename= _cli_cmd_get_path(cmd);

  fprintf(stdout, "\n");
  _cli_help_cmd_str(cmd_filename);
  str_destroy(&cmd_filename);
#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif  
  return;
}

// -----[ _cli_help_option ]-----------------------------------------
static void _cli_help_option(cli_arg_t * option)
{
  fprintf(stdout, "\n");
  /*if (option->info == NULL)*/
    fprintf(stdout, "No help available for option \"--%s\"\n",
	    option->name);
    /*else
      fprintf(stdout, "%s\n", option->info);*/

#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
}

// -----[ _cli_help_param ]------------------------------------------
/*static void _cli_help_param(cli_arg_t * arg)
{
#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif
  fprintf(stdout, "\n");
  //if (arg->info == NULL)
  fprintf(stdout, "No help available for parameter \"%s\"\n",
	  arg->name);
  //else
  //  fprintf(stdout, "%s\n", arg->info);
}*/

// -----[ cli_help ]-------------------------------------------------
void cli_help(const char * str)
{
  //fprintf(stdout, "CLI_HELP [%s]\n", str);

  unsigned int index;
  cli_t * cli= cli_get();
  cli_cmd_t * ctx;
  cli_cmd_t * cmd;

  // Get the current command context.
  cli_get_cmd_context(cli, &ctx, NULL);

  // There are two main cases to consider:
  // -------------------------------------
  // 1). Help is requested for the current command ('str' argument is
  //     equal to NULL. In this case, we provide a list of available
  //     commands in the current context.
  // 2). Help is requested for a specific command (provided in 'str'
  //     argument). In this case, we try to identify what the user
  //     wants and provide help accordingly.
  if (str == NULL) {

    gds_stream_t * os= stream_create_proc("less");
    assert(os != NULL);

    cli_cmds_t * cmds= cli_matching_subcmds(cli, ctx, "", 1);
    if (cmds == NULL) {
      stream_printf(os, 
		    "\n\n"
		    "Sorry, I fear I can't do much to help you.\n"
		    "Seems you're kinda stuck in a dead end.\n"
		    "How can it be ? Dunno, ask the author...\n\n");
    } else {
      stream_printf(os,
		    "\n\n"
		    "The following commands are available in this context.\n"
		    "Try \"help <cmd-name>\" to learn more about a specific"
		    " command.\n\n");
      for (index= 0; index < cli_cmds_num(cmds); index++) {
	cmd= cli_cmds_at(cmds, index);
	stream_printf(os, "\t%s\n", cmd->name);
      }
      stream_printf(os, "\n");
      cli_cmds_destroy(&cmds);
    }

    stream_destroy(&os);

  } else {

    cli_elem_t el;

    // Note: consider breaking input according to
    //       'rl_basic_word_break_characters' before calling the
    //       'cli_complete' function...

    int status= cli_complete(cli, str, "", &el);

    switch (status) {
    case CLI_ERROR_UNKNOWN_CMD:
      _cli_help_cmd(el.cmd);
      break;

    case CLI_ERROR_MISSING_ARG:
      // Works but we should find command instead of argument
      // (would it be possible to get cmd from arg ? currently not)
      //_cli_help_param(el.arg);
      _cli_help_cmd(el.arg->parent);
      break;

    case CLI_ERROR_UNKNOWN_OPT:
      // Does not work as we need to split 'rl_line_buffer' before
      // calling 'cli_complete' (in particular the final "--" should
      // be the "completing" text).
      _cli_help_cmd(el.cmd);
      break;

    case CLI_ERROR_MISSING_OPT_VALUE:
      // Does not work as we need to split 'rl_line_buffer' before
      // calling 'cli_complete' (in particular the final "=" should
      // be the completing text).
      _cli_help_option(el.arg);
      break;

    default:
#ifdef HAVE_RL_ON_NEW_LINE
      rl_on_new_line();
#endif
      stream_printf(gdsout, "\nNo help available\n", str);
      
    }
    
  }

  return;
}

// -----[ _rl_help ]-------------------------------------------------
/**
 * This function provides help through the 'readline' subsystem.
 * The help is linked to key '?'. The help is provided at the point
 * where the '?' is typed. For example,
 *
 * "bgp router?"
 *   will provide help on the "bgp router" command
 *
 * "bgp router ?"
 *   will provide help on the first parameter of the "bgp router"
 *   command
 *
 * "bgp add? router"
 *   will provide help on the "bgp add" command, not on the full
 *   "bgp add router" command.
 */
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
static int _rl_help(int count, int key)
{
  char * cmd= NULL;
  int pos= rl_point;

  // Retrieve the content of the readline buffer, from the first
  // character to the first occurence of '?'. On this basis, try to
  // match the command for which help was requested.
  if (pos > 0) {

    // This is the command line prefix
    cmd= (char *) malloc((pos+1)*sizeof(char));
    strncpy(cmd, rl_line_buffer, pos);
    cmd[pos]= '\0';

    cli_help(cmd);

    free(cmd);
  } else {
    cli_help(NULL);
  }

#ifdef HAVE_RL_ON_NEW_LINE
  //rl_on_new_line();
#endif

  return 0;
}
#endif /* HAVE_LIBREADLINE */

// -----[ _cli_help ]------------------------------------------------
/**
 * This function provides help through the 'cli' subsystem, i.e. when
 * the "help" command is issued by the user.
 */
static int _cli_help(cli_ctx_t * ctx, struct cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  cli_t * cli= cli_get();
  cli_cmd_t * help_cmd_ctx;

  //cli_help(arg);

  // Get the current command context.
  cli_get_cmd_context(cli, &help_cmd_ctx, NULL);

  // Identify command
  cli_cmd_t * help_cmd= cli_cmd_find_subcmd(help_cmd_ctx, arg);
  if (help_cmd == NULL) {
    printf("Sorry, this command could not be found.\n");
  } else {
    _cli_help_cmd(help_cmd);
  }

  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _help_init ]-----------------------------------------------
void _help_init()
{
  cli_cmd_t * cmd;

  // Initialize possible help paths
  _help_paths= str_array_create(0);
  str_array_add(_help_paths, "$DOC_PATH/txt/");
  str_array_add(_help_paths, "$EXEC_PATH/../share/doc/cbgp/txt/");
  str_array_add(_help_paths, str_append(str_create(DOCDIR), "/txt/"));

  // Initialize omnipresent CLI "help" command
  cmd= cli_add_cmd(cli_get_omni_cmd(cli_get()),
		   cli_cmd("help", _cli_help));
  cli_add_arg(cmd, cli_arg("cmd", NULL));

  // Initialize 'readline' help key (if available)
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
  if (rl_bind_key('?', _rl_help) != 0)
    fprintf(stderr, "Error: could not bind '?' key to help.\n");
  else
    fprintf(stderr, "help is bound to '?' key\n");
#ifdef HAVE_RL_ON_NEW_LINE
  rl_on_new_line();
#endif /* HAVE_RL_ON_NEW_LINE */
#endif /* HAVE_LIBREADLINE */
}

// -----[ _help_destroy ]--------------------------------------------
void _help_destroy()
{
  str_array_destroy(&_help_paths);
}
