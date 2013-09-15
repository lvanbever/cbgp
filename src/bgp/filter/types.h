// ==================================================================
// @(#)types.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/11/2002
// $Id: types.h,v 1.1 2009-03-24 13:42:45 bqu Exp $
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
 * Provide main BGP filter data structures.
 */

#ifndef __BGP_FILTER_TYPES_H__
#define __BGP_FILTER_TYPES_H__

#include <libgds/sequence.h>

// -----[ bgp_filter_dir_t ]-----------------------------------------
/** Direction of BGP filters (input/output). */
typedef enum {
  /** Input filter. */
  FILTER_IN,
  /** Output filter. */
  FILTER_OUT,
  FILTER_MAX
} bgp_filter_dir_t;


// -----[ bgp_ft_matcher_code_t ]------------------------------------
/** BGP filter predicate codes. */
typedef enum {
  /** Match anything. */
  FT_MATCH_ANY,
  /** Match the conjunction of two predicates. */
  FT_MATCH_OP_AND,
  /** Match the union of two predicates. */
  FT_MATCH_OP_OR,
  /** Match the opposite of a predicate. */
  FT_MATCH_OP_NOT,
  /** Match if Communities attribute contains a value. */
  FT_MATCH_COMM_CONTAINS,
  /** Match if AS-Path attribute corresponds to regular expression. */
  FT_MATCH_PATH_MATCHES,
  /** Match if next-hop is equal to address. */
  FT_MATCH_NEXTHOP_IS,
  /** Match if next-hop is contained in prefix. */
  FT_MATCH_NEXTHOP_IN,
  /** Match if prefix is equal to prefix. */
  FT_MATCH_PREFIX_IS,
  /** Match if prefix is contained in prefix. */
  FT_MATCH_PREFIX_IN,
  FT_MATCH_PREFIX_GE,
  FT_MATCH_PREFIX_LE,
} bgp_ft_matcher_code_t;


// -----[ bgp_ft_action_code_t ]-------------------------------------
/** BGP filter action codes. */
typedef enum {
  /** Do nothing. */
  FT_ACTION_NOP,
  /** Accept route. */
  FT_ACTION_ACCEPT,
  /** Deny route. */
  FT_ACTION_DENY,
  /** Append community value. */
  FT_ACTION_COMM_APPEND,
  /** Clear communities attribute. */
  FT_ACTION_COMM_STRIP,
  /** Remove community value. */
  FT_ACTION_COMM_REMOVE,
  /** Append ASN to AS-Path. */
  FT_ACTION_PATH_INSERT,
  /** Prepend ASN to AS-Path. */
  FT_ACTION_PATH_PREPEND,
  /** Remove private AS-Paths. */
  FT_ACTION_PATH_REM_PRIVATE,
  /** Set local-preference. */
  FT_ACTION_PREF_SET,
  /** Append extended-community value. */
  FT_ACTION_ECOMM_APPEND,
  /** Set MED to value. */
  FT_ACTION_METRIC_SET,
  /** Set MED to IGP weight. */
  FT_ACTION_METRIC_INTERNAL,
  /** Jump to rule. */
  FT_ACTION_JUMP,
  /** Call rule. */
  FT_ACTION_CALL,
} bgp_ft_action_code_t;


/** Maximum size of the parameter field of a filter predicate */
#define BGP_FT_MATCHER_SIZE_MAX 255


// -----[ bgp_ft_matcher_t ]-----------------------------------------
/**
 * Definition of a BGP filter predicate.
 *
 * \attention
 * This structure must be "packed" as we might access their fields
 * in a non-aligned manner. We use the GCC specific construct:
 *    __attribute__((packed)) construct.
 */
struct bgp_ft_matcher_t {
  bgp_ft_matcher_code_t code;
  unsigned char         size;
  unsigned char         params[0];
} __attribute__((packed));
typedef struct bgp_ft_matcher_t bgp_ft_matcher_t;


// -----[ bgp_ft_action_t ]------------------------------------------
/**
 * Definition of a BGP filter action.
 *
 * \attention
 * This structure must be "packed" as we might access their fields
 * in a non-aligned manner. We use the GCC specific construct:
 *    __attribute__((packed)) construct.
 */
struct bgp_ft_action_t {
  bgp_ft_action_code_t     code;
  unsigned char            size;
  struct bgp_ft_action_t * next_action;
  unsigned char            params[0];
} __attribute__((packed));
typedef struct bgp_ft_action_t bgp_ft_action_t;

// -----[ bgp_ft_rule_t ]--------------------------------------------
/**
 * Definition of a BGP filter rule.
 *
 * A rule is a couple composed of one predicate and one action.
*/
typedef struct {
  /** Predicate. */
  bgp_ft_matcher_t * matcher;
  /** Action. */
  bgp_ft_action_t  * action;
} bgp_ft_rule_t;


// -----[ bgp_filter_t ]---------------------------------------------
/**
 * Definition of a BGP filter.
 *
 * A filter is a sequence of rules.
 */
typedef struct bgp_filter_t {
  /** Sequence of rules. */
  gds_seq_t * rules;
} bgp_filter_t;

#endif /** __BGP_FILTER_TYPES_H__ */
