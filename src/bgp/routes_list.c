// ==================================================================
// @(#)routes_list.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 20/02/2004
// $Id: routes_list.c,v 1.9 2009-03-24 15:53:12 bqu Exp $
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

#include <bgp/route.h>
#include <bgp/routes_list.h>

// -----[ _routes_list_item_destroy ]--------------------------------
static void _routes_list_item_destroy(void * item, const void * ctx)
{
  route_destroy((bgp_route_t **) item);
}

// ----- routes_list_create -----------------------------------------
/**
 * Create an array of routes. The created array of routes support a
 * single option.
 *
 * Options:
 * - ROUTER_LIST_OPTION_REF: if this flag is set, the function creates
 *                           an array of references to existing
 *                           routes.
 */
bgp_routes_t * routes_list_create(uint8_t options)
{
  if (options & ROUTES_LIST_OPTION_REF) {
    return (bgp_routes_t *) ptr_array_create_ref(0);
  } else {
    return (bgp_routes_t *) ptr_array_create(0, NULL,
					     _routes_list_item_destroy,
					     NULL);
  }
}

// ----- routes_list_destroy ----------------------------------------
/**
 *
 */
void routes_list_destroy(bgp_routes_t ** routes_ref)
{
  ptr_array_destroy((ptr_array_t **) routes_ref);
}

// ----- routes_list_append -----------------------------------------
/**
 *
 */
void routes_list_append(bgp_routes_t * routes, bgp_route_t * route)
{
  assert(ptr_array_append((ptr_array_t *) routes, route) >= 0);
}

// ----- routes_list_remove_at --------------------------------------
/**
 *
 */
void routes_list_remove_at(bgp_routes_t * routes, unsigned int index)
{
  ptr_array_remove_at((ptr_array_t *) routes, index);
}

// ----- routes_list_dump -------------------------------------------
/**
 *
 */
void routes_list_dump(gds_stream_t * stream, bgp_routes_t * routes)
{
  unsigned int index;

  for (index= 0; index < bgp_routes_size(routes); index++) {
    route_dump(stream, bgp_routes_at(routes, index));
    stream_printf(stream, "\n");
  }
}

// -----[ routes_list_for_each ]-------------------------------------
/**
 * Call the given function for each route contained in the list.
 */
int routes_list_for_each(bgp_routes_t * routes, gds_array_foreach_f foreach,
			 void * ctx)
{
  return _array_for_each((array_t *) routes, foreach, ctx);
}
