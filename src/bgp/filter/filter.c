// ==================================================================
// @(#)filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @author Pradeep Bangera (pradeep.bangera@imdea.org)
// @date 27/11/2002
// $Id: filter.c,v 1.1 2009/03/24 13:42:45 bqu Exp $
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
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <libgds/memory.h>
#include <libgds/types.h>
#include <libgds/hash_utils.h>

#include <net/network.h>
#include <net/node.h>

#include <bgp/attr/comm.h>
#include <bgp/attr/ecomm.h>
#include <bgp/attr/path.h>
#include <bgp/filter/filter.h>
#include <bgp/route.h>

ptr_array_t * paPathExpr = NULL;
gds_hash_set_t * pHashPathExpr = NULL;
static unsigned int  uHashPathRegExSize= 64;

// -----[ _filter_path_regex_hash_compare ]--------------------------
static int _filter_path_regex_hash_compare(const void * item1,
					   const void * item2, 
					   unsigned int elt_size)
{
  SPathMatch * pRegEx1= (SPathMatch *) item1;
  SPathMatch * pRegEx2= (SPathMatch *) item2;

  return strcmp(pRegEx1->pcPattern, pRegEx2->pcPattern);
}

// -----[ _filter_path_regex_hash_destroy ]--------------------------
static void _filter_path_regex_hash_destroy(void * item)
{
  SPathMatch * pRegEx= (SPathMatch *) item;

  if ( pRegEx != NULL) {
    if (pRegEx->pcPattern != NULL)
      FREE(pRegEx->pcPattern);

    if (pRegEx->pRegEx != NULL)
      regex_finalize(&(pRegEx->pRegEx));
    FREE(item);
  }
}

// -----[ _filter_path_regex_hash_compute ]--------------------------
/**
 * Universal hash function for string keys (discussed in Sedgewick's
 * "Algorithms in C, 3rd edition") and adapted.
 */
static uint32_t _filter_path_regex_hash_compute(const void * item,
						unsigned int hash_size)
{
  SPathMatch * pRegEx= (SPathMatch *) item;
  
  if (pRegEx == NULL)
    return 0;

  return hash_utils_key_compute_string(pRegEx->pcPattern,
				       uHashPathRegExSize) % hash_size;
}

// -----[ _ft_matcher_create ]---------------------------------------
/**
 *
 */
static inline bgp_ft_matcher_t *
_ft_matcher_create(bgp_ft_matcher_code_t code, unsigned char size)
{
  bgp_ft_matcher_t * matcher=
    (bgp_ft_matcher_t *) MALLOC(sizeof(bgp_ft_matcher_t)+size);
  matcher->code= code;
  matcher->size= size;
  return matcher;
}

// ----- filter_matcher_destroy -------------------------------------
/**
 *
 */
void filter_matcher_destroy(bgp_ft_matcher_t ** pmatcher)
{
  if (*pmatcher != NULL) {
    FREE(*pmatcher);
    *pmatcher= NULL;
  }
}

// -----[ _ft_action_create ]----------------------------------------
/**
 *
 */
static inline bgp_ft_action_t *
_ft_action_create(bgp_ft_action_code_t code, unsigned char size)
{
  bgp_ft_action_t * action=
    (bgp_ft_action_t *) MALLOC(sizeof(bgp_ft_action_t)+size);
  action->code= code;
  action->size= size;
  action->next_action= NULL;
  return action;
}

// ----- filter_action_destroy --------------------------------------
/**
 *
 */
void filter_action_destroy(bgp_ft_action_t ** action_ref)
{
  bgp_ft_action_t * action;
  bgp_ft_action_t * old_action;

  if (*action_ref != NULL) {
    action= *action_ref;
    while (action != NULL) {
      old_action= action;
      action= action->next_action;
      FREE(old_action);
    }
    *action_ref= NULL;
  }
}

// ----- filter_rule_create -----------------------------------------
/**
 *
 */
bgp_ft_rule_t * filter_rule_create(bgp_ft_matcher_t * matcher,
				   bgp_ft_action_t * action)
{
  bgp_ft_rule_t * rule= (bgp_ft_rule_t *) MALLOC(sizeof(bgp_ft_rule_t));
  rule->matcher= matcher;
  rule->action= action;
  return rule;
}

// ----- filter_rule_destroy ----------------------------------------
/**
 *
 */
void filter_rule_destroy(bgp_ft_rule_t ** prule)
{
  if (*prule != NULL) {
    filter_matcher_destroy(&(*prule)->matcher);
    filter_action_destroy(&(*prule)->action);
    FREE(*prule);
    *prule= NULL;
  }
}

// ----- filter_rule_destroy ----------------------------------------
/**
 *
 */
void filter_rule_seq_destroy(void ** ppItem)
{
  bgp_ft_rule_t ** prule= (bgp_ft_rule_t **) ppItem;

  filter_rule_destroy(prule);
}

// ----- filter_create ----------------------------------------------
/**
 *
 */
bgp_filter_t * filter_create()
{
  bgp_filter_t * filter= (bgp_filter_t *) MALLOC(sizeof(bgp_filter_t));
  filter->rules= sequence_create(NULL, filter_rule_seq_destroy);
  return filter;
}

// ----- filter_destroy ---------------------------------------------
/**
 *
 */
void filter_destroy(bgp_filter_t ** filter_ref)
{
  if (*filter_ref != NULL) {
    sequence_destroy(&(*filter_ref)->rules);
    FREE(*filter_ref);
    *filter_ref= NULL;
  }
}

// ----- filter_rule_apply ------------------------------------------
/**
 * result:
 * 0 => DENY
 * 1 => ACCEPT
 * 2 => continue with next rule
 */
int filter_rule_apply(bgp_ft_rule_t * rule, bgp_router_t * router,
		      bgp_route_t * route)
{
  if (!filter_matcher_apply(rule->matcher, router, route))
    return 2;
  return filter_action_apply(rule->action, router, route);
}

// ----- filter_apply -----------------------------------------------
/**
 *
 */
int filter_apply(bgp_filter_t * filter, bgp_router_t * router, bgp_route_t * route)
{
  unsigned int index;
  int result;

  if (filter != NULL) {
    for (index= 0; index < filter->rules->size; index++) {
      result= filter_rule_apply((bgp_ft_rule_t *)
				 filter->rules->items[index],
				 router, route);
      if ((result == 0) || (result == 1))
	return result;
    }
  }
  return 1; // ACCEPT
}


// ----- filter_action_jump ------------------------------------------
/**
 *
 *
 */
int filter_jump(bgp_filter_t * filter, bgp_router_t * router, bgp_route_t * route)
{
  return filter_apply(filter, router, route);
}

// ----- filter_action_call ------------------------------------------
/**
 *
 *
 */
int filter_call(bgp_filter_t * filter, bgp_router_t * router, bgp_route_t * route)
{
  unsigned int index; 
  int result;

  if (filter != NULL) {
    for (index= 0; index < filter->rules->size; index++) {
      result= filter_rule_apply((bgp_ft_rule_t *)
				 filter->rules->items[index],
				 router, route);
      if ((result == 0) || (result == 1))
	return result;
    }
  }
  return 2; // CONTINUE with next rule
}


// ----- filter_matcher_apply ---------------------------------------
/**
 * result:
 * 0 => route does not match the filter
 * 1 => route matches the filter
 */
int filter_matcher_apply(bgp_ft_matcher_t * matcher, bgp_router_t * router,
			 bgp_route_t * route)
{
  bgp_comm_t comm;
  SPathMatch * pPathMatcher;
  bgp_ft_matcher_t * matcher1;
  bgp_ft_matcher_t * matcher2;

  if (matcher != NULL) {
    switch (matcher->code) {
    case FT_MATCH_ANY:
      return 1;
    case FT_MATCH_OP_AND:
      matcher1= (bgp_ft_matcher_t *) matcher->params;
      matcher2= (bgp_ft_matcher_t *) &matcher->params[sizeof(bgp_ft_matcher_t)+
							  matcher1->size];
      return (filter_matcher_apply(matcher1, router, route) &&
	      filter_matcher_apply(matcher2, router, route));
    case FT_MATCH_OP_OR:
      matcher1= (bgp_ft_matcher_t *) matcher->params;
      matcher2= (bgp_ft_matcher_t *) &matcher->params[sizeof(bgp_ft_matcher_t)+
							  matcher1->size];
	return (filter_matcher_apply(matcher1, router, route) ||
		filter_matcher_apply(matcher2, router, route));
    case FT_MATCH_OP_NOT:
      return !filter_matcher_apply((bgp_ft_matcher_t *) matcher->params,
				   router, route);
    case FT_MATCH_COMM_CONTAINS:
      memcpy(&comm, matcher->params, sizeof(comm));
      return route_comm_contains(route, comm)?1:0;
    case FT_MATCH_NEXTHOP_IS:
      return (route->attr->next_hop == *((net_addr_t *) matcher->params))?1:0;
    case FT_MATCH_NEXTHOP_IN:
      return ip_address_in_prefix(route->attr->next_hop,
				  *((ip_pfx_t*) matcher->params))?1:0;
    case FT_MATCH_PREFIX_IS:
      return ip_prefix_cmp(&route->prefix,
			    ((ip_pfx_t*) matcher->params))?0:1;
    case FT_MATCH_PREFIX_IN:
      return ip_prefix_in_prefix(route->prefix,
				 *((ip_pfx_t*) matcher->params))?1:0;
    case FT_MATCH_PREFIX_GE:
      return ip_prefix_ge_prefix(route->prefix,
				 *((ip_pfx_t*) matcher->params),
				 *((uint8_t *) (matcher->params+
						sizeof(ip_pfx_t))))?1:0;
    case FT_MATCH_PREFIX_LE:
      return ip_prefix_le_prefix(route->prefix,
				 *((ip_pfx_t*) matcher->params),
				 *((uint8_t *) (matcher->params+
						sizeof(ip_pfx_t))))?1:0;
    case FT_MATCH_PATH_MATCHES:
      assert(ptr_array_get_at(paPathExpr, *((int *) matcher->params),
			      &pPathMatcher) >= 0);
      return path_match(route_get_path(route), pPathMatcher->pRegEx)?1:0;
    default:
      cbgp_fatal("invalid filter matcher byte code (%u)\n", matcher->code);
    }
  }
  return 1;
}

// ----- filter_action_apply ----------------------------------------
/**
 * result:
 * 0 => DENY route
 * 1 => ACCEPT route
 * 2 => continue with next rule
 */
int filter_action_apply(bgp_ft_action_t * action, bgp_router_t * router,
			bgp_route_t * route)
{
  rt_info_t * rtinfo;
  while (action != NULL) {
    switch (action->code) {
    case FT_ACTION_ACCEPT:
      return 1;
    case FT_ACTION_DENY:
      return 0;
    case FT_ACTION_COMM_APPEND:
      route_comm_append(route, *((uint32_t *) action->params));
      break;
    case FT_ACTION_COMM_STRIP:
      route_comm_strip(route);
      break;
    case FT_ACTION_COMM_REMOVE:
      route_comm_remove(route, *((uint32_t *) action->params));
      break;
    case FT_ACTION_PATH_PREPEND:
      route_path_prepend(route, router->asn, *((uint8_t *) action->params));
      break;
//-------Added by pradeep bangera----------------------------------
    case FT_ACTION_PATH_INSERT:
      route_path_prepend(route, *((asn_t *) action->params),
			 *((uint8_t *) (action->params+sizeof(asn_t))));
      break;
//-----------------------------------------------------------------
    case FT_ACTION_PATH_REM_PRIVATE:
      route_path_rem_private(route);
      break;
    case FT_ACTION_PREF_SET:
      route_localpref_set(route, *((uint32_t *) action->params));
      break;
    case FT_ACTION_METRIC_SET:
      route_med_set(route, *((uint32_t *) action->params));
      break;
    case FT_ACTION_METRIC_INTERNAL:
      if (!node_has_address(router->node, route->attr->next_hop)) {
	rtinfo= rt_find_best(router->node->rt, route->attr->next_hop,
			     NET_ROUTE_ANY);
	assert(rtinfo != NULL);
	route_med_set(route, rtinfo->metric);
      } else {
	route_med_set(route, 0);
      }
      break;
    case FT_ACTION_ECOMM_APPEND:
      route_ecomm_append(route, ecomm_val_copy((bgp_ecomm_t *)
						action->params));
      break;
    case FT_ACTION_JUMP:
      return filter_jump((bgp_filter_t *)action->params, router, route);
      break;
    case FT_ACTION_CALL:
      return filter_call((bgp_filter_t *)action->params, router, route);
    default:
      cbgp_fatal("invalid filter action byte code (%u)\n", action->code);
    }
    action= action->next_action;
  }
  return 2;
}

// ----- filter_add_rule --------------------------------------------
/**
 *
 */
int filter_add_rule(bgp_filter_t * filter, bgp_ft_matcher_t * matcher,
		    bgp_ft_action_t * action)
{
  sequence_add(filter->rules,
	       filter_rule_create(matcher, action));
  return 0;
}

// ----- filter_add_rule2 -------------------------------------------
/**
 *
 */
int filter_add_rule2(bgp_filter_t * filter, bgp_ft_rule_t * rule)
{
  sequence_add(filter->rules, rule);
  return 0;
}

// ----- filter_insert_rule -----------------------------------------
int filter_insert_rule(bgp_filter_t * filter, unsigned int index,
		       bgp_ft_rule_t * rule)
{
  if (index > filter->rules->size)
    return -1;
  sequence_insert_at(filter->rules, index, rule);
  return 0;
}

// ----- filter_remove_rule -----------------------------------------
int filter_remove_rule(bgp_filter_t * filter, unsigned int index)
{
  if (index >= filter->rules->size)
    return -1;
  if (index < filter->rules->size)
    filter_rule_destroy((bgp_ft_rule_t **) &filter->rules->items[index]);
  sequence_remove_at(filter->rules, index);
  return 0;
}

// -----[ _ft_match_compound ]---------------------------------------
/**
 * Create a new matcher whose parameters are other matchers. 
 * The function checks if there is enough space in the target matcher
 * to store the children.
 */
static inline bgp_ft_matcher_t *
_ft_match_compound(bgp_ft_matcher_code_t code,
		   bgp_ft_matcher_t * matcher1,
		   bgp_ft_matcher_t * matcher2)
{
  bgp_ft_matcher_t * matcher;
  unsigned int size1, size2;

  size1= matcher1->size + sizeof(bgp_ft_matcher_t);
  if (matcher2 != NULL)
    size2= matcher2->size + sizeof(bgp_ft_matcher_t);
  else
    size2= 0;

  assert(size1 + size2 <= BGP_FT_MATCHER_SIZE_MAX);
  matcher= _ft_matcher_create(code, size1 + size2);
  memcpy(matcher->params, matcher1, size1);
  filter_matcher_destroy(&matcher1);
  if (matcher2 != NULL) {
    memcpy(&matcher->params[size1], matcher2, size2);
    filter_matcher_destroy(&matcher2);
  }
  return matcher;
}

// ----- filter_match_and -------------------------------------------
bgp_ft_matcher_t * filter_match_and(bgp_ft_matcher_t * matcher1,
				    bgp_ft_matcher_t * matcher2)
{
  // Simplify:
  //   AND(ANY, predicate) = predicate
  //   AND(predicate, ANY) = predicate
  if ((matcher1 == NULL) || (matcher1->code == FT_MATCH_ANY)) {
    filter_matcher_destroy(&matcher1);
    return matcher2;
  } else if ((matcher2 == NULL) || (matcher2->code == FT_MATCH_ANY)) {
    filter_matcher_destroy(&matcher2);
    return matcher1;
  }

  // Build AND(predicate1, predicate2)
  return _ft_match_compound(FT_MATCH_OP_AND, matcher1, matcher2);
}

// ----- filter_match_or --------------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_or(bgp_ft_matcher_t * matcher1,
				   bgp_ft_matcher_t * matcher2)
{
  // Simplify expression
  //   OR(ANY, ANY) == ANY
  //   OR(ANY, *) = ANY
  //   OR(*, ANY) = ANY
  if ((matcher1 == NULL) || (matcher1->code == FT_MATCH_ANY) ||
      (matcher2 == NULL) || (matcher2->code == FT_MATCH_ANY)) {
    filter_matcher_destroy(&matcher2);
    filter_matcher_destroy(&matcher1);
    return NULL;
  }

  // Build OR(predicate1, predicate2)
  return _ft_match_compound(FT_MATCH_OP_OR, matcher1, matcher2);
}

// ----- filter_match_not -------------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_not(bgp_ft_matcher_t * matcher)
{
  bgp_ft_matcher_t * new_matcher;

  // Simplify operation: in the case of "not(not(expr))", return "expr"
  if ((matcher != NULL) && (matcher->code == FT_MATCH_OP_NOT)) {

    new_matcher=
      _ft_matcher_create(((bgp_ft_matcher_t *) matcher->params)->code,
			    ((bgp_ft_matcher_t *) matcher->params)->size);
    memcpy(new_matcher->params,
	   ((bgp_ft_matcher_t *) matcher->params)->params,
	   ((bgp_ft_matcher_t *) matcher->params)->size);
    filter_matcher_destroy(&matcher);
    return new_matcher;
  }

  return _ft_match_compound(FT_MATCH_OP_NOT, matcher, NULL);
}

// ----- filter_match_nexthop_equals --------------------------------
bgp_ft_matcher_t * filter_match_nexthop_equals(net_addr_t next_hop)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_NEXTHOP_IS,
		       sizeof(next_hop));
  memcpy(matcher->params, &next_hop, sizeof(next_hop));
  return matcher;
}

// ----- filter_match_nexthop_in ------------------------------------
bgp_ft_matcher_t * filter_match_nexthop_in(ip_pfx_t prefix)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_NEXTHOP_IN,
		       sizeof(ip_pfx_t));
  memcpy(matcher->params, &prefix, sizeof(ip_pfx_t));
  return matcher;
}

// ----- filter_match_prefix_equals ---------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_equals(ip_pfx_t prefix)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_PREFIX_IS,
			  sizeof(ip_pfx_t));
  memcpy(matcher->params, &prefix, sizeof(ip_pfx_t));
  return matcher;
}

// ----- filter_match_prefix_in -------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_in(ip_pfx_t prefix)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_PREFIX_IN,
			  sizeof(ip_pfx_t));
  memcpy(matcher->params, &prefix, sizeof(ip_pfx_t));
  return matcher;
}

// ----- filter_match_prefix_ge -------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_ge(ip_pfx_t prefix, uint8_t mask_len)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_PREFIX_GE,
			  sizeof(ip_pfx_t)+sizeof(uint8_t));
  memcpy(matcher->params, &prefix, sizeof(ip_pfx_t));
  memcpy(matcher->params+sizeof(ip_pfx_t), &mask_len, sizeof(uint8_t));
  return matcher;
}

// ----- filter_match_prefix_le -------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_prefix_le(ip_pfx_t prefix, uint8_t mask_len)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_PREFIX_LE,
			  sizeof(ip_pfx_t)+sizeof(uint8_t));
  memcpy(matcher->params, &prefix, sizeof(ip_pfx_t));
  memcpy(matcher->params+sizeof(ip_pfx_t), &mask_len, sizeof(uint8_t));
  return matcher;
}

// ----- filter_match_comm_contains ---------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_comm_contains(bgp_comm_t comm)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_COMM_CONTAINS,
		       sizeof(bgp_comm_t));
  memcpy(matcher->params, &comm, sizeof(comm));
  return matcher;
}

// ----- filter_match_path -------------------------------------------
/**
 *
 */
bgp_ft_matcher_t * filter_match_path(int iArrayPathRegExPos)
{
  bgp_ft_matcher_t * matcher=
    _ft_matcher_create(FT_MATCH_PATH_MATCHES,
			  sizeof(int));
  
  memcpy(matcher->params, &iArrayPathRegExPos, sizeof(iArrayPathRegExPos));
  return matcher;
}

// ----- filter_action_accept -----------------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_accept()
{
  return _ft_action_create(FT_ACTION_ACCEPT, 0);
}

// ----- filter_action_deny -----------------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_deny()
{
  
  return _ft_action_create(FT_ACTION_DENY, 0);
}

// ----- filter_action_jump ------------------------------------------
/**
 *
 *
 */
bgp_ft_action_t * filter_action_jump(bgp_filter_t * filter)
{
  bgp_ft_action_t * action = _ft_action_create(FT_ACTION_JUMP,
						  sizeof(bgp_filter_t *));
  memcpy(action->params, filter, sizeof(bgp_filter_t *));
  return action;
}

// ----- filter_action_call ------------------------------------------
/**
 *
 *
 */
bgp_ft_action_t * filter_action_call(bgp_filter_t * filter)
{
  bgp_ft_action_t * action = _ft_action_create(FT_ACTION_CALL,
						  sizeof(bgp_filter_t *));
  memcpy(action->params, filter, sizeof(bgp_filter_t *));
  return action;
}

// ----- filter_action_pref_set -------------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_pref_set(uint32_t pref)
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_PREF_SET,
					      sizeof(pref));
  memcpy(action->params, &pref, sizeof(pref));
  return action;
}

// ----- filter_action_metric_set -----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_metric_set(uint32_t metric)
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_METRIC_SET,
						sizeof(metric));
  memcpy(action->params, &metric, sizeof(metric));
  return action;
}

// ----- filter_action_metric_internal ------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_metric_internal()
{
  return _ft_action_create(FT_ACTION_METRIC_INTERNAL, 0);
}


// ----- filter_action_comm_append ----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_comm_append(bgp_comm_t comm)
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_COMM_APPEND,
						sizeof(bgp_comm_t));
  memcpy(action->params, &comm, sizeof(comm));
  return action;
}

// ----- filter_action_comm_remove ----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_comm_remove(bgp_comm_t comm)
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_COMM_REMOVE,
					      sizeof(bgp_comm_t));
  memcpy(action->params, &comm, sizeof(comm));
  return action;
}

// ----- filter_action_comm_strip -----------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_comm_strip()
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_COMM_STRIP, 0);
  return action;
}

// ----- filter_action_ecomm_append ---------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_ecomm_append(bgp_ecomm_t * comm)
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_ECOMM_APPEND,
						sizeof(bgp_ecomm_t));
  memcpy(action->params, comm, sizeof(bgp_ecomm_t));
  ecomm_val_destroy(&comm);
  return action;
}

// ----- filter_action_path_prepend ---------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_path_prepend(uint8_t amount)
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_PATH_PREPEND,
					      sizeof(uint8_t));
  memcpy(action->params, &amount, sizeof(uint8_t));
  return action;
}

//-----------------Added by pradeep bangera-------------------------
// ----- filter_action_path_insert ---------------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_path_insert(asn_t asn, uint8_t amount)
{
  bgp_ft_action_t * action= _ft_action_create(FT_ACTION_PATH_INSERT,
					      sizeof(asn)+sizeof(amount));
  memcpy(action->params, &asn, sizeof(asn));
  memcpy(action->params+sizeof(asn), &amount, sizeof(amount));
  return action;
}
//-------------------------------------------------------------------

// ----- filter_action_path_rem_private -----------------------------
/**
 *
 */
bgp_ft_action_t * filter_action_path_rem_private()
{
  return _ft_action_create(FT_ACTION_PATH_REM_PRIVATE, 0);
}

// ----- filter_matcher_dump ----------------------------------------
void filter_matcher_dump(gds_stream_t * stream, bgp_ft_matcher_t * matcher)
{
  bgp_comm_t comm;
  SPathMatch * pPathMatch;
  bgp_ft_matcher_t * matcher1, * matcher2;

  if (matcher != NULL) {
    switch (matcher->code) {
    case FT_MATCH_ANY:
      stream_printf(stream, "any");
      break;
    case FT_MATCH_OP_AND:
      matcher1= (bgp_ft_matcher_t *) matcher->params;
      matcher2= (bgp_ft_matcher_t *) &matcher->params[sizeof(bgp_ft_matcher_t)+
						      matcher1->size];
      stream_printf(stream, "(");
      filter_matcher_dump(stream, matcher1);
      stream_printf(stream, ") & (");
      filter_matcher_dump(stream, matcher2);
      stream_printf(stream, ")");
      break;
    case FT_MATCH_OP_OR:
      matcher1= (bgp_ft_matcher_t *) matcher->params;
      matcher2= (bgp_ft_matcher_t *) &matcher->params[sizeof(bgp_ft_matcher_t)+
						      matcher1->size];
      stream_printf(stream, "(");
      filter_matcher_dump(stream, matcher1);
      stream_printf(stream, ") | (");
      filter_matcher_dump(stream, matcher2);
      stream_printf(stream, ")");
      break;
    case FT_MATCH_OP_NOT:
      stream_printf(stream, "NOT(");
      filter_matcher_dump(stream, (bgp_ft_matcher_t *) matcher->params);
      stream_printf(stream, ")");
      break;
    case FT_MATCH_COMM_CONTAINS:
      memcpy(&comm, matcher->params, sizeof(comm));
      stream_printf(stream, "community is ");
      comm_value_dump(stream, comm, COMM_DUMP_TEXT);
      break;
    case FT_MATCH_NEXTHOP_IS:
      stream_printf(stream, "next-hop is ");
      ip_address_dump(stream, *((net_addr_t *) matcher->params));
      break;
    case FT_MATCH_NEXTHOP_IN:
      stream_printf(stream, "next-hop in ");
      ip_prefix_dump(stream, *((ip_pfx_t *) matcher->params));
      break;
    case FT_MATCH_PREFIX_IS:
      stream_printf(stream, "prefix is ");
      ip_prefix_dump(stream, *((ip_pfx_t *) matcher->params));
      break;
    case FT_MATCH_PREFIX_IN:
      stream_printf(stream, "prefix in ");
      ip_prefix_dump(stream, *((ip_pfx_t *) matcher->params));
      break;
    case FT_MATCH_PREFIX_GE:
      stream_printf(stream, "prefix ge ");
      ip_prefix_dump(stream, *((ip_pfx_t *) matcher->params));
      stream_printf(stream, " %u", matcher->params[sizeof(ip_pfx_t)]);
      break;
    case FT_MATCH_PREFIX_LE:
      stream_printf(stream, "prefix le ");
      ip_prefix_dump(stream, *((ip_pfx_t *) matcher->params));
      stream_printf(stream, " %u", matcher->params[sizeof(ip_pfx_t)]);
      break;
    case FT_MATCH_PATH_MATCHES:
      ptr_array_get_at(paPathExpr, *((int *) matcher->params), &pPathMatch);
      if (pPathMatch != NULL)
	stream_printf(stream, "path \\\"%s\\\"", pPathMatch->pcPattern);
      else
	abort();
      break;
    default:
      stream_printf(stream, "?");
    }
  } else
    stream_printf(stream, "any");
}

// ----- filter_action_dump -----------------------------------------
/**
 *
 */
void filter_action_dump(gds_stream_t * stream, bgp_ft_action_t * action)
{
  bgp_ecomm_t * ecomm;

  switch (action->code) {
  case FT_ACTION_ACCEPT:
    stream_printf(stream, "accept");
    break;
  case FT_ACTION_DENY:
    stream_printf(stream, "deny");
    break;
  case FT_ACTION_COMM_APPEND:
    stream_printf(stream, "community add ");
    comm_value_dump(stream, *((uint32_t *) action->params),
		    COMM_DUMP_TEXT);
    break;
  case FT_ACTION_COMM_STRIP:
    stream_printf(stream, "community strip");
    break;
  case FT_ACTION_COMM_REMOVE:
    stream_printf(stream, "community remove ");
    comm_value_dump(stream, *((uint32_t *) action->params),
		    COMM_DUMP_TEXT);
    break;
  case FT_ACTION_PATH_PREPEND:
    stream_printf(stream, "as-path prepend %u",
		  *((uint8_t *) action->params));
    break;
//-----------Added by pradeep bangera----------------------------
//***The command "as-path insert <asn>" will insert user specified 
//***AS number into the AS PATH before suffixing the its own AS ID
  case FT_ACTION_PATH_INSERT:
    stream_printf(stream, "as-path insert %u %u",
		  *((asn_t *) action->params),
		  *((uint8_t *) (action->params+sizeof(asn_t))));
    break;
    //---------------------------------------------------------------
  case FT_ACTION_PATH_REM_PRIVATE:
    stream_printf(stream, "as-path remove-private");
    break;
  case FT_ACTION_PREF_SET:
    stream_printf(stream, "local-pref %u", *((uint32_t *) action->params));
    break;
  case FT_ACTION_METRIC_SET:
    stream_printf(stream, "metric %u", *((uint32_t *) action->params));
    break;
  case FT_ACTION_METRIC_INTERNAL:
    stream_printf(stream, "metric internal");
    break;
  case FT_ACTION_ECOMM_APPEND:
    ecomm= (bgp_ecomm_t *) action->params;
    switch (ecomm->type_high) {
    case ECOMM_RED:
      stream_printf(stream, "red-community add ");
      ecomm_red_dump(stream, ecomm);
      break;
    default:
      stream_printf(stream, "ext-community");
    }
    break;
  case FT_ACTION_CALL:
    stream_printf(stream, "call \"%s\"", "---");
    break;
  case FT_ACTION_JUMP:
    stream_printf(stream, "jump \"%s\"", "---");
    break;
  default:
    stream_printf(stream, "?");
  }
}

// ----- filter_rule_dump -------------------------------------------
void filter_rule_dump(gds_stream_t * stream, bgp_ft_rule_t * rule)
{
  bgp_ft_action_t * action= rule->action;

  stream_printf(stream, "  predicate {\n    ");
  filter_matcher_dump(stream, rule->matcher);
  stream_printf(stream, "\n  }\n");
  stream_printf(stream, "  actions {\n");
  while (action != NULL) {
    stream_printf(stream, "    ");
    filter_action_dump(stream, action);
    action= action->next_action;
    stream_printf(stream, "\n");
  }
  stream_printf(stream, "  }\n");
}

// ----- filter_dump ------------------------------------------------
void filter_dump(gds_stream_t * stream, bgp_filter_t * filter)
{
  unsigned int index;

  if (filter != NULL)
    for (index= 0; index < filter->rules->size; index++) {
      stream_printf(stream, "rule %u {\n", index);
      filter_rule_dump(stream,
		       (bgp_ft_rule_t *) filter->rules->items[index]);
      stream_printf(stream, "}\n");
    }
  stream_printf(stream, "default. any --> ACCEPT\n");
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// ----- filter_path_regex_init --------------------------------------
/**
 *
 */
void _filter_path_regex_init()
{
  pHashPathExpr= hash_set_create(uHashPathRegExSize, 0, 
				 _filter_path_regex_hash_compare, 
				 _filter_path_regex_hash_destroy, 
				 _filter_path_regex_hash_compute);
  paPathExpr= ptr_array_create(0, NULL, NULL, NULL);
}

// ----- filter_path_regex_destroy ----------------------------------
/**
 *
 *
 */
void _filter_path_regex_destroy()
{
  ptr_array_destroy(&paPathExpr);
  hash_set_destroy(&pHashPathExpr);
}
