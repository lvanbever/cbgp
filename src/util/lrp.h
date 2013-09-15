// ==================================================================
// @(#)lrp.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/12/2008
// $Id: lrp.h,v 1.1 2009-08-31 09:57:03 bqu Exp $
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

#ifndef __UTIL_LRP_H__
#define __UTIL_LRP_H__

// -----[ lrp_error_code_t ]-----------------------------------------
typedef enum {
  LRP_SUCCESS       = 0,
  LRP_ERROR_OPEN    = -1,
  LRP_ERROR_SYNTAX  = -2,
  LRP_ERROR_TOO_LONG= -3,
  LRP_ERROR_USER    = -4,
} lrp_error_code_t;

struct lrp_t;
typedef struct lrp_t lrp_t;

#ifdef _cplusplus
extern "C" {
#endif

  // -----[ lrp_create ]---------------------------------------------
  /**
   * Create a line records parser.
   */
  lrp_t * lrp_create(size_t max_line_len, const char * delimiters);

  // -----[ lrp_destroy ]--------------------------------------------
  /**
   * Destroy a line records parser.
   */
  void lrp_destroy(lrp_t ** parser_ref);

  // -----[ lrp_reset ]----------------------------------------------
  void lrp_reset(lrp_t * parser);

  // -----[ lrp_open ]-----------------------------------------------
  int lrp_open(lrp_t * parser, const char * filename);

  // -----[ lrp_close ]----------------------------------------------
  void lrp_close(lrp_t * parser);

  // -----[ lrp_set_stream ]-----------------------------------------
  int lrp_set_stream(lrp_t * parser, FILE * stream);

  // -----[ lrp_get_next_line ]--------------------------------------
  int lrp_get_next_line(lrp_t * parser);

  // -----[ lrp_get_num_fields ]-------------------------------------
  int lrp_get_num_fields(lrp_t * parser,
			 unsigned int * num_fields_ref);

  // -----[ lrp_get_field ]------------------------------------------
  const char * lrp_get_field(lrp_t * parser, unsigned int index);

  // -----[ lrp_error ]----------------------------------------------
  /**
   * Get the latest parser error code.
   *
   * \param parser is the parser.
   */
  int lrp_error(lrp_t * parser);

  // -----[ lrp_set_user_error ]-------------------------------------
  /**
   * Set a user error message.
   *
   * This changes the parser user error message and sets the error
   * code to LRP_ERROR_USER.
   *
   * \param parser is the parser.
   * \param format is the user error message and format specifier.
   * \param ...    is a variable list of arguments (specified by
   *   \a format).
   */
  void lrp_set_user_error(lrp_t * parser, const char * format, ...);

  // -----[ lrp_get_user_error ]-------------------------------------
  /**
   * Get the latest user error message.
   *
   * \param parser is the parser.
   * \retval a user error if the latest error was LRP_ERROR_USER,
   *   or NULL otherwise.
   */
  //const char * lrp_get_user_error(lrp_t * parser);

  // -----[ lrp_strerror ]-------------------------------------------
  const char * lrp_strerror(lrp_t * parser);

  // -----[ lrp_strerrorloc ]----------------------------------------
  const char * lrp_strerrorloc(lrp_t * parser);

  // -----[ lrp_perror ]---------------------------------------------
  /**
   * Print an error message.
   *
   * \param stream is the output stream.
   * \param parser is the parser.
   */
  void lrp_perror(gds_stream_t * stream, lrp_t * parser);

#ifdef _cplusplus
}
#endif

#endif /* __UTIL_LRP_H__ */
