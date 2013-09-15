// ==================================================================
// @(#)rl.h
//
// Provides an interactive CLI, based on GNU readline. If GNU
// readline isn't available, provides a basic replacement CLI.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/02/2004
// $Id: rl.h,v 1.7 2009-08-31 09:48:39 bqu Exp $
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

/**
 * \file
 * Provide data structures and functions to interface with the GNU
 * readline library.
 */

#ifndef __UI_RL_H__
#define __UI_RL_H__

#include <string.h>

// -----[ rl_on_new_line ]-----
/* This function is used to enumerate files during the command-line
 * auto-completion.
 */
#undef _FILENAME_COMPLETION_FUNCTION
#if defined(HAVE_LIBREADLINE) && defined(HAVE_READLINE_READLINE_H)
# include <readline/readline.h>
# ifdef HAVE_RL_FILENAME_COMPLETION_FUNCTION
#  define _FILENAME_COMPLETION_FUNCTION rl_filename_completion_function
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

  // ----[ rl_gets ]-------------------------------------------------
  /**
   * Read a line from the user.
   *
   * \retval a line of characters,
   *   or NULL if the end-of-file (Ctrl-D) has been sent to the
   *   input stream.
   */
  char * rl_gets();

  // -----[ _rl_init ]-----------------------------------------------
  /**
   * \internal
   * Initialize the GNU readline interface. This function should be
   * called by the library initialization function.
   */
  void _rl_init();

  // -----[ _rl_destroy ]--------------------------------------------
  /**
   * \internal
   * Finalize the GNU readline interface. This function should be
   * called by the library finalization function.
   */
  void _rl_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __UI_RL_H__ */
