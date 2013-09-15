// ==================================================================
// @(#)parser.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/08/2003
// $Id: parser.h,v 1.1 2009-03-24 13:42:45 bqu Exp $
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

#ifndef __BGP_FILTER_PARSER_H__
#define __BGP_FILTER_PARSER_H__

#include <bgp/filter/types.h>

#define FILTER_PARSER_SUCCESS           0
#define FILTER_PARSER_ERROR_UNEXPECTED -1
#define FILTER_PARSER_ERROR_END        -2

#ifdef _cplusplus
extern "C" {
#endif

  // ----- filter_parser_rule ---------------------------------------
  int filter_parser_rule(const char * rule, bgp_ft_rule_t ** rule_ref);
  // ----- filter_parser_action -------------------------------------
  int filter_parser_action(const char * action,
			   bgp_ft_action_t ** action_ref);

#ifdef __cplusplus
}
#endif

#endif /* __BGP_FILTER_PARSER_H__ */
