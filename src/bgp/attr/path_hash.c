// ==================================================================
// @(#)path_hash.c
//
// This is the global AS-Path repository.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 10/10/2005
// $Id: path_hash.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
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
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <libgds/hash.h>
#include <libgds/hash_utils.h>
#include <libgds/stream.h>

#include <bgp/attr/path.h>
#include <bgp/attr/path_hash.h>

// ---| Function prototypes |---
static uint32_t _path_hash_item_compute(const void * item,
					unsigned int hash_size);

// ---| Private parameters |---
static struct {
  gds_hash_set_t     * hash;
  unsigned int         size;
  uint8_t              method;
  gds_hash_compute_f   compute;
} _global_ref= {
  .hash   = NULL,
  .size   = 25000,
  .method = PATH_HASH_METHOD_STRING,
  .compute= _path_hash_item_compute,
};

// -----[ _path_hash_item_compute ]----------------------------------
/**
 * This is a helper function that computes the hash key of an
 * AS-Path. The function relies on a static buffer in order to avoid
 * frequent memory allocation.
 */
#define AS_PATH_STR_SIZE 1024
static uint32_t _path_hash_item_compute(const void * item,
					unsigned int hash_size)
{
  char acPathStr1[AS_PATH_STR_SIZE];
  assert(path_to_string((bgp_path_t *) item, 1, acPathStr1, AS_PATH_STR_SIZE) >= 0);
  return hash_utils_key_compute_string(acPathStr1, hash_size);
}

// -----[ path_hash_item_compare ]-----------------------------------
static int _path_hash_item_compare(const void * item1,
				   const void * item2,
				   unsigned int elt_size)
{
  return path_cmp(item1, item2);
}

// -----[ path_hash_item_destroy ]-----------------------------------
static void _path_hash_item_destroy(void * item)
{
  bgp_path_t * path= (bgp_path_t *) item;
  path_destroy(&path);
}

// -----[ path_hash_add ]--------------------------------------------
/**
 *
 */
void * path_hash_add(bgp_path_t * path)
{
  _path_hash_init();
  return hash_set_add(_global_ref.hash, path);
}

// -----[ path_hash_get ]--------------------------------------------
/**
 *
 */
bgp_path_t * path_hash_get(bgp_path_t * path)
{
  _path_hash_init();
  return (bgp_path_t *) hash_set_search(_global_ref.hash, path);
}

// -----[ path_hash_remove ]-----------------------------------------
/**
 *
 */
int path_hash_remove(bgp_path_t * path)
{
  int result;
  _path_hash_init();
  assert((result= hash_set_remove(_global_ref.hash, path))
	 != HASH_ERROR_NO_MATCH);
  return result;
}

// -----[ path_hash_get_size ]---------------------------------------
/**
 * Return the AS-Path hash size.
 */
unsigned int path_hash_get_size()
{
  return _global_ref.size;
}

// -----[ path_hash_set_size ]---------------------------------------
/**
 * Change the AS-Path hash size. This is only allowed before the hash
 * is instanciated.
 */
int path_hash_set_size(unsigned int size)
{
  if (_global_ref.hash != NULL)
    return -1;
  _global_ref.size= size;
  return 0;
}

// -----[ path_hash_get_method ]-------------------------------------
uint8_t path_hash_get_method()
{
  return _global_ref.method;
}

// -----[ path_hash_set_method ]-------------------------------------
int path_hash_set_method(uint8_t method)
{
  if (_global_ref.hash != NULL)
    return -1;
  if ((method != PATH_HASH_METHOD_STRING) && 
      (method != PATH_HASH_METHOD_ZEBRA))
    return -1;
  _global_ref.method= method;
  switch (_global_ref.method) {
  case PATH_HASH_METHOD_STRING:
    _global_ref.compute= _path_hash_item_compute;
    break;
  case PATH_HASH_METHOD_ZEBRA:
    _global_ref.compute= path_hash_zebra;
    break;
  case PATH_HASH_METHOD_OAT:
    _global_ref.compute= path_hash_OAT;
    break;
  default:
    abort();
  }
  return 0;
}

// -----[ path_hash_content_for_each ]-------------------------------
/**
 *
 */
int _path_hash_content_for_each(void * item, void * ctx)
{
  gds_stream_t * stream= (gds_stream_t *) ctx;
  bgp_path_t * path= (bgp_path_t *) item;
  unsigned int refcnt= 0;

  refcnt= hash_set_get_refcnt(_global_ref.hash, path);

  stream_printf(stream, "%u\t", refcnt);
  path_dump(stream, path, 1);
  stream_printf(stream, " (%p)", path);
  stream_printf(stream, "\n");
  return 0;
}

// -----[ path_hash_content ]----------------------------------------
/**
 *
 */
void path_hash_content(gds_stream_t * stream)
{
  time_t now= time(NULL);

  _path_hash_init();
  stream_printf(stream, "# C-BGP Global AS-Path repository content\n");
  stream_printf(stream, "# generated on %s", ctime(&now));
  stream_printf(stream, "# hash-size is %u\n", _global_ref.size);
  assert(!hash_set_for_each(_global_ref.hash,
			    _path_hash_content_for_each,
			    stream));
}

// -----[ path_hash_statistics_for_each ]----------------------------
/**
 *
 */
int _path_hash_statistics_for_each(void * item, void * ctx)
{
  gds_stream_t * stream= (gds_stream_t *) ctx;
  ptr_array_t * array= (ptr_array_t *) item;
  unsigned int count= 0;

  if (array != NULL)
    count= ptr_array_length(array);
  stream_printf(stream, "%u\n", count);
  return 0;
}

// -----[ path_hash_statistics ]-------------------------------------
/**
 *
 */
void path_hash_statistics(gds_stream_t * stream)
{
  time_t now= time(NULL);

  _path_hash_init();
  stream_printf(stream, "# C-BGP Global AS-Path repository statistics\n");
  stream_printf(stream, "# generated on %s", ctime(&now));
  stream_printf(stream, "# hash-size is %u\n", _global_ref.size);
  assert(!hash_set_for_each_key(_global_ref.hash,
				_path_hash_statistics_for_each,
				stream));
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _path_hash_init ]-------------------------------------------
void _path_hash_init()
{
  if (_global_ref.hash == NULL) {
    _global_ref.hash= hash_set_create(_global_ref.size,
				      0,
				      _path_hash_item_compare,
				      _path_hash_item_destroy,
				      _global_ref.compute);
    assert(_global_ref.hash != NULL);
  }
}

// -----[ _path_hash_destroy ]---------------------------------------
void _path_hash_destroy()
{
  if (_global_ref.hash != NULL)
    hash_set_destroy(&_global_ref.hash);
}
