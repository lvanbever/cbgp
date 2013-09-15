// ==================================================================
// @(#)lrp.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/12/2008
// $Id: lrp.c,v 1.1 2009-08-31 09:57:03 bqu Exp $
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

#define _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <libgds/memory.h>
#include <libgds/tokenizer.h>

#include <util/lrp.h>
#include <util/reader.h>

// -----[ lrp_t ]----------------------------------------------------
struct lrp_t {
  gds_tokenizer_t    * tokenizer;
  const gds_tokens_t * tokens;
  const char         * delimiters;
  int                  error;
  unsigned int         line_number;
  char               * line;
  size_t               max_line_len;
  unsigned int         token_index;
  reader_t           * reader;
  char               * user_error;
};

// -----[ lrp_create ]-----------------------------------------------
lrp_t * lrp_create(size_t max_line_len, const char * delimiters)
{
  lrp_t * parser= MALLOC(sizeof(lrp_t));
  parser->tokenizer= NULL;
  parser->tokens= NULL;
  parser->delimiters= delimiters;
  parser->error= LRP_SUCCESS;
  parser->line_number= 0;
  parser->line= NULL;
  parser->max_line_len= max_line_len;
  parser->token_index= 0;
  parser->reader= NULL;
  parser->user_error= NULL;
  return parser;
}

// -----[ lrp_destroy ]----------------------------------------------
void lrp_destroy(lrp_t ** parser_ref)
{
  lrp_t * parser= *parser_ref;
  if (parser == NULL)
    return;
  if (parser->tokenizer != NULL)
    tokenizer_destroy(&parser->tokenizer);
  if (parser->reader != NULL)
    reader_destroy(&parser->reader);
  if (parser->line != NULL)
    str_destroy(&parser->line);
  // Free memory allocated by vasprintf()
  if (parser->user_error != NULL)
    free(parser->user_error);
  FREE(parser);
  *parser_ref= NULL;
}

// -----[ lrp_reset ]------------------------------------------------
void lrp_reset(lrp_t * parser)
{
  parser->line_number= 0;
  parser->error= LRP_SUCCESS;
}

// -----[ lrp_open ]-------------------------------------------------
int lrp_open(lrp_t * parser, const char * filename)
{
  lrp_reset(parser);
  if (parser->reader != NULL)
    reader_destroy(&parser->reader);
  parser->reader= file_reader(filename);
  if (parser->reader == NULL) {
    parser->error= LRP_ERROR_OPEN;
    return parser->error;
  }
  return LRP_SUCCESS;
}

// -----[ lrp_close ]------------------------------------------------
void lrp_close(lrp_t * parser)
{
  if (parser->reader != NULL)
    reader_close(parser->reader);
}

// -----[ lrp_set_stream ]-----------------------------------------
int lrp_set_stream(lrp_t * parser, FILE * stream)
{
  lrp_reset(parser);
  if (parser->reader != NULL)
    reader_destroy(&parser->reader);
  parser->reader= stream_reader(stream);
  return LRP_SUCCESS;
}

// -----[ lrp_get_next_line ]----------------------------------------
int lrp_get_next_line(lrp_t * parser)
{
  assert(parser->reader != NULL);

  // Need to allocate line buffer ?
  if (parser->line == NULL)
    parser->line= str_lcreate(parser->max_line_len);

  while (!reader_eof(parser->reader)) {
    if (reader_gets(parser->reader,  parser->line, parser->max_line_len)
	== NULL)
      return 0;

    parser->line_number++;

    // Skip comments (lines starting with '#')
    if (parser->line[0] == '#')
      continue;

    // Check if a whole line was read (or if it was too long)
    // Recommendation (see STR35-C)
    if ((strchr(parser->line, '\n') == NULL) && !reader_eof(parser->reader)) {
      // Newline not found, flush stream to end of line
      parser->error= LRP_ERROR_TOO_LONG;
      // Flush remaining of line
      reader_flush(parser->reader);
    }
    return 1;
  }
  return 0;
}

// -----[ lrp_get_num_fields ]---------------------------------------
int lrp_get_num_fields(lrp_t * parser,
		       unsigned int * num_fields_ref)
{
  if (parser->tokenizer == NULL)
    parser->tokenizer= tokenizer_create(parser->delimiters, NULL, NULL);
  if (tokenizer_run(parser->tokenizer, parser->line)) {
    parser->error= LRP_ERROR_SYNTAX;
    return parser->error;
  }
  parser->tokens= tokenizer_get_tokens(parser->tokenizer);
  *num_fields_ref= tokens_get_num(parser->tokens);
  parser->token_index= 0;
  return 0;
}

// -----[ lrp_get_field ]--------------------------------------------
const char * lrp_get_field(lrp_t * parser, unsigned int index)
{
  return tokens_get_string_at(parser->tokens, index);
}

// -----[ lrp_error ]------------------------------------------------
int lrp_error(lrp_t * parser)
{
  return parser->error;
}

// -----[ lrp_set_user_error ]---------------------------------------
void lrp_set_user_error(lrp_t * parser, const char * format, ...)
{
  va_list ap;

  if (parser->user_error != NULL)
    free(parser->user_error);

  va_start(ap, format);
  parser->error= LRP_ERROR_USER;
  assert(vasprintf(&parser->user_error, format, ap) >= 0);
  assert(parser->user_error != NULL);
  va_end(ap);
}

// -----[ lrp_strerror ]---------------------------------------------
const char * lrp_strerror(lrp_t * parser)
{
  switch (parser->error) {
  case LRP_SUCCESS:
    return "success";
  case LRP_ERROR_OPEN:
    return "unable to open file";
  case LRP_ERROR_SYNTAX:
    return "syntax error";
  case LRP_ERROR_TOO_LONG:
    return "line is too long";
  default:
    return parser->user_error;
  }
}

/** Static error location string.
 * Contains strings of the form: ", at line <line>" where
 * <line> is smaller than 2^32, i.e. 10 characters.
 */
#define ERROR_LOC_STR_LEN 21
static char _error_loc_str[ERROR_LOC_STR_LEN];

// -----[ lrp_strerrorloc ]------------------------------------------
const char * lrp_strerrorloc(lrp_t * parser)
{
  if (parser->line_number <= 0)
    return "";
  assert(snprintf(_error_loc_str, ERROR_LOC_STR_LEN, ", at line %d",
		  parser->line_number) >= 0);
  return _error_loc_str;
}

// -----[ lrp_perror ]-----------------------------------------------
void lrp_perror(gds_stream_t * stream, lrp_t * parser)
{
  const char * msg= lrp_strerror(parser);
  if (msg != NULL)
    stream_printf(stream, "%s", msg);
  else
    stream_printf(stream, "unknown error (%d)", parser->error);
  if (parser->line_number > 0)
    stream_printf(stream, ", at line %d", parser->line_number);
}
