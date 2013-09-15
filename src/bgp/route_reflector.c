// ==================================================================
// @(#)route_reflector.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/12/2003
// $Id: route_reflector.c,v 1.8 2009-03-24 15:52:37 bqu Exp $
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

#include <libgds/stream.h>
#include <bgp/route_reflector.h>

#include <net/prefix.h>

// -----[ cluster_list_append ]--------------------------------------
int cluster_list_append(bgp_cluster_list_t * cl,
			bgp_cluster_id_t cluster_id)
{
  return uint32_array_append(cl, cluster_id);
}

// ----- cluster_list_destroy ---------------------------------------
void cluster_list_destroy(bgp_cluster_list_t ** pcl)
{
  uint32_array_destroy(pcl);
}

// ----- cluster_list_dump ------------------------------------------
/**
 * This function dumps the cluster-IDs contained in the given list to
 * the given stream.
 */
void cluster_list_dump(gds_stream_t * stream, bgp_cluster_list_t * cl)
{
  unsigned int index;

  for (index= 0; index < uint32_array_size(cl); index++) {
    if (index > 0)
      stream_printf(stream, " ");
    ip_address_dump(stream, cl->data[index]);
  }
}

// ----- cluster_list_equals -------------------------------------------
/**
 * This function compares 2 cluster-ID-lists.
 *
 * Return value:
 *    1  if they are equal
 *    0  otherwise
 */
int cluster_list_equals(bgp_cluster_list_t * cl1,
			bgp_cluster_list_t * cl2)
{
  unsigned int index;

  if (cl1 == cl2)
    return 1;
  if ((cl1 == NULL) || (cl2 == NULL))
    return 0;
  if (uint32_array_size(cl1) !=
      uint32_array_size(cl2))
    return 0;
  for (index= 0; index < uint32_array_size(cl1); index++)
    if (cl1->data[index] != cl2->data[index])
      return 0;
  return 1;
}

// ----- cluster_list_contains --------------------------------------
/**
 *
 */
int cluster_list_contains(bgp_cluster_list_t * cl,
			  bgp_cluster_id_t cluster_id)
{
  unsigned int index;

  for (index= 0; index < uint32_array_size(cl); index++) {
    if (cl->data[index] == cluster_id)
      return 1;
  }
  return 0;
}

// -----[ originator_equals ]----------------------------------------
/**
 * Test if two Originator-IDs are equal.
 *
 * Return value:
 *   1  if they are equal
 *   0  otherwise
 */
int originator_equals(bgp_originator_t * orig1, bgp_originator_t * orig2)
{  
  if (orig1 == orig2)
    return 1;
  if ((orig1 == NULL) ||
      (orig2 == NULL) ||
      (*orig1 != *orig2))
      return 0;
  return 1;
}
