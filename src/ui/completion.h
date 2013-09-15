// ==================================================================
// @(#)completion.h
//
// Provides functions to auto-complete commands/parameters in the CLI
// in interactive mode.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 22/11/2002
// $Id: completion.h,v 1.2 2009-03-24 16:29:41 bqu Exp $
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
 * Provide data structures and function for handling auto-completion
 * in the command-line interface (CLI).
 */

#ifndef __UI_COMPLETION_H__
#define __UI_COMPLETION_H__

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ _rl_completion_init ]------------------------------------
  /**
   * \internal
   * Initialize the CLI auto-completion subsystem. This function
   * should be called by the library initialization function.
   */
  void _rl_completion_init();

  // -----[ _rl_completion_done ]------------------------------------
  /**
   * \internal
   * Finalize the CLI auto-completion subsystem. This function
   * should be called by the library finalization function.
   */
  void _rl_completion_done();

#ifdef __cplusplus
}
#endif

#endif /* __UI_COMPLETION_H__ */
