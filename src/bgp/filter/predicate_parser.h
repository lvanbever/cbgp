// ==================================================================
// @(#)predicate_parser.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/02/2004
// $Id: predicate_parser.h,v 1.1 2009-03-24 13:42:45 bqu Exp $
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

#ifndef __BGP_FILTER_PREDICATE_PARSER_H__
#define __BGP_FILTER_PREDICATE_PARSER_H__

#include <libgds/stream.h>
#include <bgp/filter/types.h>

#define PREDICATE_PARSER_END                     1
#define PREDICATE_PARSER_SUCCESS                 0
#define PREDICATE_PARSER_ERROR_UNEXPECTED        -1
#define PREDICATE_PARSER_ERROR_UNFINISHED_STRING -2
#define PREDICATE_PARSER_ERROR_ATOM              -3
#define PREDICATE_PARSER_ERROR_UNEXPECTED_EOF    -4
#define PREDICATE_PARSER_ERROR_LEFT_EXPR         -5
#define PREDICATE_PARSER_ERROR_PAR_MISMATCH      -6
#define PREDICATE_PARSER_ERROR_STATE_TOO_LARGE   -7
#define PREDICATE_PARSER_ERROR_UNARY_OP          -8

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ predicate_parser ]---------------------------------------
  int predicate_parser(const char * expr, bgp_ft_matcher_t ** matcher_ref);
  // -----[ predicate_parser_perror ]--------------------------------
  void predicate_parser_perror(gds_stream_t * stream, int error);
  // -----[ predicate_parser_strerror ]------------------------------
  char * predicate_parser_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_FILTER_PREDICATE_PARSER_H__ */
