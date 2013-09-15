// ==================================================================
// @(#)rl.c
//
// Provides an interactive CLI, based on GNU readline. If GNU
// readline isn't available, provides a basic replacement CLI.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: rl.c,v 1.12 2009-03-24 16:29:41 bqu Exp $
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

#include <libgds/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>

#include <cli/common.h>
#include <ui/rl.h>
#include <ui/completion.h>
#include <ui/help.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#define CBGP_HIST_FILE      ".cbgp_history"
#define CBGP_HIST_FILE_SIZE 500
#define MAX_LINE_READ       1024

// A static variable for holding the line.
static char * _line= NULL;

// Maximum number of lines to hold in readline's history.
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
static int _hist_file_size= CBGP_HIST_FILE_SIZE;
#endif /* HAVE_LIBREADLINE */


// ---- rl_gets -----------------------------------------------------
/**
 * Reads a string, and return a pointer to it. This function is
 * inspired from an example in the GNU readline manual.
 *
 * Returns NULL on EOF.
 */
char * rl_gets()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)

  // If the buffer has already been allocated,
  // return the memory to the free pool.
  if (_line) {
    free(_line);
    _line= (char *) NULL;
  }

  // Show the prompt and get a line from the user.
  _line= readline(cli_context_to_string(cli_get()->ctx, "cbgp"));

  // If the line has any text in it, save it on the history.
  if (_line && *_line) {
    add_history(_line);
    stifle_history((int) _hist_file_size);
  }

#else

  // If the buffer has not yet been allocated, allocate it.
  // The buffer will be freed by the destructor function.
  if (_line == NULL)
    _line= (char *) malloc(MAX_LINE_READ*sizeof(char));

  // Print the prompt
  fprintf(stdout, "%s", cli_context_to_string(cli_get()->ctx, "cbgp"));

  // Get at most MAX_LINE_READ-1 characters from stdin
  if (fgets(_line, MAX_LINE_READ, stdin) == NULL)
    return NULL;

#endif

  return (_line);
}


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- _rl_init ---------------------------------------------------
/**
 * Initializes the interactive CLI, using GNU readline. This function
 * loads the CLI history if the environment variable CBGP_HIST_FILE is
 * defined.
 */
void _rl_init()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_HISTORY_H)
  char * env_hist_file;
  char * env_hist_file_size;
  char * home;
  char * endptr;
  long int tmp;

  env_hist_file= getenv("CBGP_HIST_FILE");
  if (env_hist_file != NULL) {
    if (strlen(env_hist_file) > 0) {
      read_history(env_hist_file);
    } else {
      home= getenv("HOME");
      if (home != NULL) { /* use ~/.cbgp_history file */
	env_hist_file= (char *) malloc(sizeof(char)*(2+strlen(CBGP_HIST_FILE)+
						     strlen(home)));
	sprintf(env_hist_file, "%s/%s", home, CBGP_HIST_FILE);
	read_history(env_hist_file);
	free(env_hist_file);
      } else {
	read_history(NULL); /* use default ~/.history file */
      }
    }
  }

  env_hist_file_size= getenv("CBGP_HIST_FILESIZE");
  if (env_hist_file_size != NULL) {
    tmp= strtol(env_hist_file_size, &endptr, 0);
    if ((*endptr == '\0') && (tmp < INT_MAX))
      _hist_file_size= (int) tmp;
  }

  _rl_completion_init();

#endif
}

// ----- _rl_destroy ------------------------------------------------
/**
 * Frees the resources used by the interactive CLI. This function
 * also stores the CLI history in a file if the environment variable
 * CBGP_HIST_FILE is defined.
 */
void _rl_destroy()
{
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_HISTORY_H)
  char * env_hist_file;
  char * home;

  _rl_completion_done();

  env_hist_file= getenv("CBGP_HIST_FILE");
  if (env_hist_file != NULL) {
    if (strlen(env_hist_file) > 0) {
      write_history(env_hist_file);
    } else {
      home= getenv("HOME");
      if (home != NULL) { /* use ~/.cbgp_history file */
	env_hist_file= (char *) malloc(sizeof(char)*(2+strlen(CBGP_HIST_FILE)+
						     strlen(home)));
	sprintf(env_hist_file, "%s/%s", home, CBGP_HIST_FILE);
	write_history(env_hist_file);
	free(env_hist_file);
      } else {
	write_history(NULL); /* use default ~/.history file */
      }
    }
  }
#endif

  if (_line != NULL) {
    free(_line);
    _line= NULL;
  }
}
