// ==================================================================
// @(#)api.h
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/10/2006
// $Id: api.h,v 1.12 2009-03-10 13:54:23 bqu Exp $
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
 * \mainpage
 * This is the documentation for the CBGP library. The library API
 * as well as the internal functions and data structures are
 * documented here.
 *
 * \see api.h for the library API documentation.
 *
 * \authors
 * Bruno Quoitin (bruno.quoitin@uclouvain.be)
 */

/**
 * \file
 * This CBGP's main API.
 *
 * A typical utilization of the CBGP library is demonstrated below:
 * \code
 * static char * script[]= {
 *   "net add node 1.0.0.0",
 *   "net node 1.0.0.0 domain 1",
 *   "net add node 1.0.0.1",
 *   "net node 1.0.0.1 domain 1",
 *   "net add link 1.0.0.0 1.0.0.1",
 *   "net domain 1 compute",
 *   "net node 1.0.0.0 show rt *",
 * };
 *
 * int main(int argc, char * argv[])
 * {
 *   unsigned int index;
 *   int result;
 *
 *   libcbgp_init(argc, argv);
 *   for (index= 0; index < sizeof(script)/sizeof(script[0]); index++) {
 *     result= libcbgp_exec_cmd(script[index]);
 *     if (result != ESUCCESS) {
 *       fprintf(stderr, "ERROR: could not execute \"%s\"\n", script[index]);
 *       fflush(stderr);
 *       break;
 *     }
 *   }
 *   libcbgp_done();
 * }
 * \endcode
 */

#ifndef __CBGP_API_H__
#define __CBGP_API_H__

#include <libgds/libgds-config.h>
#include <libgds/params.h>
#include <libgds/stream.h>

#ifdef CYGWIN
# define CBGP_EXP_DECL __declspec(dllexport)
#else
# define CBGP_EXP_DECL
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /*/////////////////////////////////////////////////////////////////
  //
  // Initialization and configuration of the library.
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_init ]-------------------------------------------
  /**
   * Initialize the CBGP library.
   *
   * This function must be called prior to using any of the library
   * functions.
   *
   * \param argc is the number of arguments on the command-line.
   * \param argv is the list of arguments on the command-line.
   */
  CBGP_EXP_DECL void libcbgp_init(int argc, char * argv[]);

  // -----[ libcbgp_done ]-------------------------------------------
  /**
   * Finalize the CBGP library.
   *
   * This function should be called when none of the library
   * functions and data structures are needed anymore.
   */
  CBGP_EXP_DECL void libcbgp_done();

  // -----[ libcbgp_banner ]-----------------------------------------
  CBGP_EXP_DECL void libcbgp_banner();

  // -----[ libcbgp_set_debug_callback ]-----------------------------
  CBGP_EXP_DECL void libcbgp_set_debug_callback(gds_stream_cb_f cb,
						void * ctx);

  // -----[ libcbgp_set_err_callback ]-------------------------------
  CBGP_EXP_DECL void libcbgp_set_err_callback(gds_stream_cb_f cb,
					      void * ctx);

  // -----[ libcbgp_set_out_callback ]-------------------------------
  CBGP_EXP_DECL void libcbgp_set_out_callback(gds_stream_cb_f cb,
					      void * ctx);

  // -----[ libcbgp_set_debug_file ]---------------------------------
  CBGP_EXP_DECL void libcbgp_set_debug_file(const char * filename);

  // -----[ libcbgp_set_debug_level ]--------------------------------
  CBGP_EXP_DECL void libcbgp_set_debug_level(stream_level_t level);

  // -----[ libcbgp_set_err_level ]----------------------------------
  CBGP_EXP_DECL void libcbgp_set_err_level(stream_level_t level);
  

  /*/////////////////////////////////////////////////////////////////
  //
  // API
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_set_param ]--------------------------------------
  CBGP_EXP_DECL void libcbgp_set_param(const char * name,
				       const char * value);
  // -----[ libcbgp_get_param ]--------------------------------------
  CBGP_EXP_DECL const char * libcbgp_get_param(const char * name);
  // -----[ libcbgp_has_param ]--------------------------------------
  CBGP_EXP_DECL int libcbgp_has_param(const char * name);
  // -----[ libcbgp_get_param_lookup ]-------------------------------
  CBGP_EXP_DECL param_lookup_t libcbgp_get_param_lookup();
  // -----[ libcbgp_exec_cmd ]---------------------------------------
  CBGP_EXP_DECL int libcbgp_exec_cmd(const char * cmd);
  // -----[ libcbgp_exec_file ]--------------------------------------
  CBGP_EXP_DECL int libcbgp_exec_file(const char * file_name);
  // -----[ libcbgp_exec_stream ]------------------------------------
  CBGP_EXP_DECL int libcbgp_exec_stream(FILE * stream);
  // -----[ libcbgp_interactive ]------------------------------------
  CBGP_EXP_DECL int libcbgp_interactive();


  
  /*/////////////////////////////////////////////////////////////////
  //
  // Other functions (use with care)
  //
  /////////////////////////////////////////////////////////////////*/

  // -----[ libcbgp_init2 ]------------------------------------------
  /**
   * \internal
   * Initialize the CBGP library, but not the GDS library. This
   * function is used by programs or libraries that run multiple C-BGP
   * sessions and manage the GDS library by themselve (see the JNI
   * library for an example).
   *
   * \attention
   * You should normaly not use this function. Consider using the
   * \c libcbgp_init function instead.
   */
  void libcbgp_init2();

  // -----[ libcbgp_done2 ]------------------------------------------
  /**
   * \internal
   * Free the resources allocated by the CBGP library, but do not
   * terminate the GDS library. See the \c libcbgp_init2 function for
   * more details.
   *
   * \attention
   * You should normaly not use this function. Consider using the
   * \c libcbgp_done function instead.
   */
  void libcbgp_done2();

#ifdef __cplusplus
}
#endif

#endif /* __CBGP_API_H__ */
