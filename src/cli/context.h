// ==================================================================
// @(#)context.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be),
// @date 15/12/2008
// $Id$
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
 * Provide utility functions for getting specific objects from
 * the CLI context stack.
 */

#ifndef __BGP_CLI_CONTEXT_H__
#define __BGP_CLI_CONTEXT_H__

#include <libgds/cli_ctx.h>
#include <bgp/types.h>

// -----[ _cli_context_top ]-----------------------------------------
static inline void * _cli_context_top(const cli_ctx_t * ctx) {
  void * user_data= cli_context_top_data(ctx);
  assert(user_data != NULL);
  return user_data;
}

// -----[ _link_from_context ]---------------------------------------
static inline net_iface_t * _link_from_context(const cli_ctx_t * ctx) {
  return (net_iface_t *) _cli_context_top(ctx);
}

// -----[ _node_from_context ]---------------------------------------
static inline net_node_t * _node_from_context(cli_ctx_t * ctx) {
  return (net_node_t *) _cli_context_top(ctx);
}

// -----[ _subnet_from_context ]-------------------------------------
static inline net_subnet_t * _subnet_from_context(cli_ctx_t * ctx) {
  return (net_subnet_t *) _cli_context_top(ctx);
}

// -----[ _iface_from_context ]--------------------------------------
static inline net_iface_t *_iface_from_context(cli_ctx_t * ctx) {
  return (net_iface_t *) _cli_context_top(ctx);
}

// -----[ _igp_domain_from_context ]---------------------------------
static inline igp_domain_t * _igp_domain_from_context(cli_ctx_t * ctx) {
  return (igp_domain_t *) _cli_context_top(ctx);
}

// -----[ _domain_from_context ]-------------------------------------
static inline bgp_domain_t * _domain_from_context(const cli_ctx_t * ctx) {
  return (bgp_domain_t *) _cli_context_top(ctx);
}

// -----[ _router_from_context ]-------------------------------------
static inline bgp_router_t * _router_from_context(const cli_ctx_t * ctx) {
  return (bgp_router_t *) _cli_context_top(ctx);
}

// -----[ _routes_from_context ]-------------------------------------
static inline bgp_routes_t * _routes_from_context(const cli_ctx_t * ctx) {
  return (bgp_routes_t *) _cli_context_top(ctx);
}

// -----[ _peer_from_context ]---------------------------------------
static inline bgp_peer_t * _peer_from_context(const cli_ctx_t * ctx) {
  return (bgp_peer_t *) _cli_context_top(ctx);
}

// -----[ _rule_from_context ]---------------------------------------
static inline bgp_ft_rule_t * _rule_from_context(const cli_ctx_t * ctx) {
  return (bgp_ft_rule_t *) _cli_context_top(ctx);
}

// -----[ _filter_from_context ]-------------------------------------
static inline bgp_filter_t ** _filter_from_context(cli_ctx_t * ctx) {
  return (bgp_filter_t **) _cli_context_top(ctx);
}

#endif /*  __BGP_CLI_CONTEXT_H__ */
