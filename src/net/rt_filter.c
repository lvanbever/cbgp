// ==================================================================
// @(#)rt_filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/06/2008
// $Id: rt_filter.c,v 1.2 2009-03-24 16:26:53 bqu Exp $
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

#include <net/routing.h>
#include <net/rt_filter.h>

// -----[ rt_filter_create ]-----------------------------------------
rt_filter_t * rt_filter_create()
{
  rt_filter_t * filter=
    (rt_filter_t *) MALLOC(sizeof(rt_filter_t));
  //filter->prefix= ;
  filter->oif= NULL;
  filter->gateway= NULL;
  filter->type= NET_ROUTE_ANY;

  // For internal use
  filter->matches= 0;
  filter->del_list= NULL;

  return filter;
}

// -----[ rt_filter_destroy ]----------------------------------------
void rt_filter_destroy(rt_filter_t ** filter_ptr)
{
  if (*filter_ptr != NULL) {
    FREE(*filter_ptr);
    *filter_ptr= NULL;
  }
}

// -----[ rt_filter_matches ]----------------------------------------
/**
 * Check if the given route-info matches the given filter. The
 * following fields are checked:
 * - next-hop interface
 * - gateway address
 * - type (routing protocol)
 *
 * Return:
 *   0 if the route does not match filter
 *   1 if the route matches the filter
 */
int rt_filter_matches(rt_info_t * rtinfo, rt_filter_t * filter)
{
  const rt_entry_t * rtentry= rt_entries_get_at(rtinfo->entries, 0);

  if ((filter->oif != NULL) &&
      (filter->oif != rtentry->oif))
    return 0;

  if ((filter->gateway != NULL) &&
      (*filter->gateway != rtentry->gateway))
    return 0;

  if ((filter->type != NET_ROUTE_ANY) &&
      (filter->type != rtinfo->type))
    return 0;

  filter->matches++;

  return 1;
}

