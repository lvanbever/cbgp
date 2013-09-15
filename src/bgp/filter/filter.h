// ==================================================================
// @(#)filter.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Pradeep Bangera (pradeep.bangera@imdea.org)
// @date 27/11/2002
// $Id: filter.h,v 1.1 2009-03-24 13:42:45 bqu Exp $
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

#ifndef __BGP_FILTER_H__
#define __BGP_FILTER_H__

#include <stdio.h>

#include <libgds/array.h>
#include <libgds/hash.h>
#include <libgds/stream.h>

#include <bgp/types.h>
#include <bgp/filter/types.h>

#include <util/regex.h>

#define FTM_AND(fm1, fm2) filter_match_and(fm1, fm2)
#define FTM_OR(fm1, fm2) filter_match_or(fm1, fm2)
#define FTM_NOT(fm) filter_match_not(fm)

#define FTA_ACCEPT filter_action_accept()
#define FTA_DENY filter_action_deny()

typedef struct {
  char * pcPattern;
  SRegEx * pRegEx;
  uint32_t uArrayPos;
} SPathMatch;

#ifdef __cplusplus
extern "C" {
#endif

  // ----- filter_rule_create ---------------------------------------
  bgp_ft_rule_t * filter_rule_create(bgp_ft_matcher_t * matcher,
				   bgp_ft_action_t * action);
  // ----- filter_create --------------------------------------------
  bgp_filter_t * filter_create();
  // ----- filter_destroy -------------------------------------------
  void filter_destroy(bgp_filter_t ** pfilter);
  // ----- filter_matcher_destroy -----------------------------------
  void filter_matcher_destroy(bgp_ft_matcher_t ** pmatcher);
  // ----- filter_action_destroy ------------------------------------
  void filter_action_destroy(bgp_ft_action_t ** paction);
  // ----- filter_add_rule ------------------------------------------
  int filter_add_rule(bgp_filter_t * filter,
		      bgp_ft_matcher_t * matcher,
		      bgp_ft_action_t * action);
  // ----- filter_add_rule2 -----------------------------------------
  int filter_add_rule2(bgp_filter_t * filter,
		       bgp_ft_rule_t * rule);
  // ----- filter_insert_rule ---------------------------------------
  int filter_insert_rule(bgp_filter_t * filter, unsigned int uIndex,
			 bgp_ft_rule_t * rule);
  // ----- filter_remove_rule ---------------------------------------
  int filter_remove_rule(bgp_filter_t * filter, unsigned int uIndex);
  // ----- filter_apply ---------------------------------------------
  int filter_apply(bgp_filter_t * filter, bgp_router_t * pRouter,
		   bgp_route_t * pRoute);
  // ----- filter_matcher_apply -------------------------------------
  int filter_matcher_apply(bgp_ft_matcher_t * matcher,
			   bgp_router_t * pRouter,
			   bgp_route_t * pRoute);
  // ----- filter_action_apply --------------------------------------
  int filter_action_apply(bgp_ft_action_t * action,
			  bgp_router_t * pRouter,
			  bgp_route_t * pRoute);
  // ----- filter_match_and -----------------------------------------
  bgp_ft_matcher_t * filter_match_and(bgp_ft_matcher_t * matcher1,
				    bgp_ft_matcher_t * matcher2);
  // ----- filter_match_or ------------------------------------------
  bgp_ft_matcher_t * filter_match_or(bgp_ft_matcher_t * matcher1,
				   bgp_ft_matcher_t * matcher2);
  // ----- filter_match_not -----------------------------------------
  bgp_ft_matcher_t * filter_match_not(bgp_ft_matcher_t * matcher);
  // ----- filter_match_comm_contains -------------------------------
  bgp_ft_matcher_t * filter_match_comm_contains(bgp_comm_t comm);
  // ----- filter_match_nexthop_equals ------------------------------
  bgp_ft_matcher_t * filter_match_nexthop_equals(net_addr_t nexthop);
  // ----- filter_match_nexthop_in ----------------------------------
  bgp_ft_matcher_t * filter_match_nexthop_in(ip_pfx_t prefix);
  // ----- filter_match_prefix_equals -------------------------------
  bgp_ft_matcher_t * filter_match_prefix_equals(ip_pfx_t prefix);
  // ----- filter_match_prefix_in -----------------------------------
  bgp_ft_matcher_t * filter_match_prefix_in(ip_pfx_t prefix);
  // ----- filter_match_prefix_ge -----------------------------------
  bgp_ft_matcher_t * filter_match_prefix_ge(ip_pfx_t prefix,
					  uint8_t uMaskLen);
  // ----- filter_match_prefix_le -----------------------------------
  bgp_ft_matcher_t * filter_match_prefix_le(ip_pfx_t prefix,
					  uint8_t uMaskLen);
  // ----- filter_math_path -----------------------------------------
  bgp_ft_matcher_t * filter_match_path(int iArrayPathRegExPos);
  // ----- filter_action_accept -------------------------------------
  bgp_ft_action_t * filter_action_accept();
  // ----- filter_action_deny ---------------------------------------
  bgp_ft_action_t * filter_action_deny();
  // ----- filter_action_jump ---------------------------------------
  bgp_ft_action_t * filter_action_jump(bgp_filter_t * filter);
  // ----- filter_action_call ---------------------------------------
  bgp_ft_action_t * filter_action_call(bgp_filter_t * filter);
  // ----- filter_action_pref_set -----------------------------------
  bgp_ft_action_t * filter_action_pref_set(uint32_t pref);
  // ----- filter_action_metric_set ---------------------------------
  bgp_ft_action_t * filter_action_metric_set(uint32_t metric);
  // ----- filter_action_metric_internal ----------------------------
  bgp_ft_action_t * filter_action_metric_internal();
  // ----- filter_action_comm_append --------------------------------
  bgp_ft_action_t * filter_action_comm_append(bgp_comm_t comm);
  // ----- filter_action_comm_remove --------------------------------
  bgp_ft_action_t * filter_action_comm_remove(bgp_comm_t comm);
  // ----- filter_action_comm_strip ---------------------------------
  bgp_ft_action_t * filter_action_comm_strip();
  // ----- filter_action_ecomm_append -------------------------------
  bgp_ft_action_t * filter_action_ecomm_append(bgp_ecomm_t * comm);
  // ----- filter_action_path_prepend -------------------------------
  bgp_ft_action_t * filter_action_path_prepend(uint8_t amount);
  // ----- filter_action_path_insert -------------------------------
  bgp_ft_action_t * filter_action_path_insert(asn_t asn, uint8_t amount);
  // ----- filter_action_path_rem_private ---------------------------
  bgp_ft_action_t * filter_action_path_rem_private();
  // ----- filter_dump ----------------------------------------------
  void filter_dump(gds_stream_t * stream, bgp_filter_t * filter);
  // ----- filter_rule_dump -------------------------------------------
  void filter_rule_dump(gds_stream_t * stream, bgp_ft_rule_t * rule);
  // ----- filter_matcher_dump --------------------------------------
  void filter_matcher_dump(gds_stream_t * stream,
			   bgp_ft_matcher_t * matcher);
  // ----- filter_action_dump ---------------------------------------
  void filter_action_dump(gds_stream_t * stream,
			  bgp_ft_action_t * action);
  
  // ----- filter_path_regex_init -----------------------------------
  void _filter_path_regex_init();
  // ----- filter_path_regex_destroy --------------------------------
  void _filter_path_regex_destroy();

#ifdef __cplusplus
}
#endif

#endif /* __BGP_FILTER_H__ */
