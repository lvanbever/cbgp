// ==================================================================
// @(#)route_map.c
//
// @author Sebastien Tandel (sta@info.ucl.ac.be)
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/12/2004
// $Id: route_map.c,v 1.13 2009-03-24 15:52:01 bqu Exp $
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
#include <string.h>

#include <libgds/array.h>
#include <libgds/hash.h>
#include <libgds/hash_utils.h>
#include <libgds/memory.h>
#include <libgds/str_util.h>

#include <bgp/filter/filter.h>
#include <bgp/route_map.h>

static gds_hash_set_t * _route_maps= NULL;
static unsigned int _route_maps_size= 64;

// -----[ _route_map_compare ]---------------------------------------
static int _route_map_compare(const void * item1,
			      const void * item2, 
			      unsigned int item_size)
{
  _route_map_t * rm1= (_route_map_t *) item1;
  _route_map_t * rm2= (_route_map_t *) item2;
  return strcmp(rm1->name, rm2->name);
}

// ------[ _route_map_destroy ]--------------------------------------
static void _route_map_destroy(void * item)
{
  _route_map_t * rm= (_route_map_t *) item;

  if (rm != NULL) {
    if (rm->name != NULL)
      FREE(rm->name);
    if (rm->filter != NULL)
      filter_destroy(&rm->filter);
    FREE(rm);
  } 
}

// -----[ _route_map_hash_compute ]----------------------------------
static uint32_t _route_map_hash_compute(const void * item,
					unsigned int hash_size)
{
  _route_map_t * rm= (_route_map_t *) item;
  return hash_utils_key_compute_string(rm->name, hash_size);
}

// -----[ route_map_add ]--------------------------------------------
/**
 *
 *  returns the key of the hash if inserted, and 
 *  -1 if the Route Map already exists or the key is too large
 *
 */
int route_map_add(const char * name, bgp_filter_t * filter)
{
  _route_map_t static_rm= { .name= (char *) name };
  _route_map_t * rm;

  if (hash_set_search(_route_maps, &static_rm) != NULL)
    return -1;

  rm= MALLOC(sizeof(_route_map_t));
  rm->name= str_create(name);
  rm->filter= filter;
  assert(hash_set_add(_route_maps, rm) == rm);

  return 0;
}

// -----[ route_map_remove ]-----------------------------------------
int route_map_remove(const char * name)
{
  _route_map_t static_rm= { .name= (char *) name };
  int result;

  result= hash_set_remove(_route_maps, &static_rm);

  if ((result != HASH_SUCCESS) &&
      (result != HASH_SUCCESS_UNREF))
    return -1;
  return 0;
}

// -----[ route_map_get ]--------------------------------------------
bgp_filter_t * route_map_get(const char * name)
{
  _route_map_t static_rm= { .name= (char *) name };
  _route_map_t * rm;

  rm= hash_set_search(_route_maps, &static_rm);
  if (rm == NULL)
    return NULL;
  return rm->filter;
}

// -----[ route_map_enum ]-------------------------------------------
gds_enum_t * route_map_enum()
{
  return hash_set_get_enum(_route_maps);
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _route_maps_init ]-----------------------------------------
void _route_maps_init()
{
  _route_maps= hash_set_create(_route_maps_size,
			       0,
			       _route_map_compare,
			       _route_map_destroy,
			       _route_map_hash_compute);
}

// -----[ _route_maps_destroy ]--------------------------------------
void _route_maps_destroy()
{
  hash_set_destroy(&_route_maps);
}
