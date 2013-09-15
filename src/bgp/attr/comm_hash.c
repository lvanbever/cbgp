// ==================================================================
// @(#)comm_hash.c
//
// This is the global Communities repository.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/10/2005
// $Id: comm_hash.c,v 1.1 2009-03-24 13:41:26 bqu Exp $
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

#include <libgds/array.h>
#include <libgds/hash.h>
#include <libgds/hash_utils.h>
#include <libgds/stream.h>

#include <bgp/attr/comm.h>
#include <bgp/attr/comm_hash.h>

static uint32_t _comm_hash_item_compute(const void * item,
					unsigned int hash_size);
// ---| Private parameters |---
static struct {
  gds_hash_set_t    * hash;
  unsigned int        size;
  uint8_t             method;
  gds_hash_compute_f  compute;
} _global_ref= {
  .hash   = NULL,
  .size   = 25000,
  .method = COMM_HASH_METHOD_STRING,
  .compute= _comm_hash_item_compute,
};

// -----[ _comm_hash_item_compute ]----------------------------------
/**
 * This is a helper function that computes the hash key of an
 * Communities attribute.
 */
#define AS_COMM_STR_SIZE 1024
static uint32_t _comm_hash_item_compute(const void * item,
					unsigned int hash_size)
{
  char acCommStr1[AS_COMM_STR_SIZE];
  assert(comm_to_string((bgp_comms_t *) item, acCommStr1,
			AS_COMM_STR_SIZE) >= 0);
  return hash_utils_key_compute_string(acCommStr1, hash_size);
}

// -----[ _comm_hash_item_compute_zebra ]----------------------------
/**
 * This is a helper function that computes the hash key of an
 * Communities attribute. The function is based on Zebra's Communities
 * attribute hashing function.
 */
static uint32_t _comm_hash_item_compute_zebra(const void * item,
					      unsigned int hash_size)
{
  bgp_comms_t * comms= (bgp_comms_t *) item;
  unsigned int key= 0;
  unsigned int index;
  bgp_comm_t comm;

  for (index= 0; index < comms->num; index++) {
    comm= comms->values[index];
    key+= comm & 255;
    comm= comm >> 8;
    key+= comm & 255;
    comm= comm >> 8;
    key+= comm & 255;
    comm= comm >> 8;
    key+= comm;
  }
  return key % hash_size;
}

// -----[ comm_hash_item_compare ]-----------------------------------
int _comm_hash_item_compare(const void * comm1, const void * comm2,
			    unsigned int elt_size)
{
  return comms_cmp((bgp_comms_t *) comm1,
		   (bgp_comms_t *) comm2);
}

// -----[ comm_hash_item_destroy ]-----------------------------------
void _comm_hash_item_destroy(void * item)
{
  bgp_comms_t * comms= (bgp_comms_t *) item;
  comms_destroy(&comms);
}

// -----[ comm_hash_add ]--------------------------------------------
/**
 *
 */
void * comm_hash_add(bgp_comms_t * comms)
{
  _comm_hash_init();
  return hash_set_add(_global_ref.hash, comms);
}

// -----[ comm_hash_get ]--------------------------------------------
/**
 *
 */
bgp_comms_t * comm_hash_get(bgp_comms_t * comms)
{
  _comm_hash_init();
  return (bgp_comms_t *) hash_set_search(_global_ref.hash, comms);
}

// -----[ comm_hash_remove ]-----------------------------------------
/**
 *
 */
int comm_hash_remove(bgp_comms_t * comms)
{
  _comm_hash_init();
  return hash_set_remove(_global_ref.hash, comms);
}

// -----[ comm_hash_refcnt ]-----------------------------------------
/**
 *
 */
unsigned int comm_hash_refcnt(bgp_comms_t * comms)
{
  _comm_hash_init();
  return hash_set_get_refcnt(_global_ref.hash, comms);
}

// -----[ comm_hash_get_size ]---------------------------------------
/**
 * Return the Communities hash size.
 */
unsigned int comm_hash_get_size()
{
  return _global_ref.size;
}

// -----[ comm_hash_set_size ]---------------------------------------
/**
 * Change the Communities hash size. This is only allowed before the
 * hash is instanciated.
 */
int comm_hash_set_size(unsigned int size)
{
  if (_global_ref.hash != NULL)
    return -1;
  _global_ref.size= size;
  return 0;
}

// -----[ comm_hash_get_method ]-------------------------------------
uint8_t comm_hash_get_method()
{
  return _global_ref.method;
}

// -----[ comm_hash_set_method ]-------------------------------------
int comm_hash_set_method(uint8_t method)
{
  if (_global_ref.hash != NULL)
    return -1;
  if ((method != COMM_HASH_METHOD_STRING) && 
      (method != COMM_HASH_METHOD_ZEBRA))
    return -1;
  _global_ref.method= method;
  switch (_global_ref.method) {
  case COMM_HASH_METHOD_STRING:
    _global_ref.compute= _comm_hash_item_compute;
    break;
  case COMM_HASH_METHOD_ZEBRA:
    _global_ref.compute= _comm_hash_item_compute_zebra;
    break;
  default:
    abort();
  }
  return 0;
}

// -----[ _comm_hash_content_for_each ]------------------------------
/**
 *
 */
static int _comm_hash_content_for_each(void * item, void * ctx)
{
  gds_stream_t * stream= (gds_stream_t *) ctx;
  bgp_comms_t * comms= (bgp_comms_t *) item;

  stream_printf(stream, "%u\t[", hash_set_get_refcnt(_global_ref.hash, comms));
  comm_dump(stream, comms, 1);
  stream_printf(stream, "]\n");
  return 0;
}

// -----[ comm_hash_content ]----------------------------------------
/**
 *
 */
void comm_hash_content(gds_stream_t * stream)
{
  time_t now= time(NULL);

  _comm_hash_init();
  stream_printf(stream, "# C-BGP Global Communities repository content\n");
  stream_printf(stream, "# generated on %s", ctime(&now));
  stream_printf(stream, "# hash-size is %u\n", _global_ref.size);
  assert(!hash_set_for_each(_global_ref.hash,
			    _comm_hash_content_for_each,
			    stream));
}

// -----[ _comm_hash_statistics_for_each ]---------------------------
/**
 *
 */
static int _comm_hash_statistics_for_each(void * item, void * ctx)
{
  gds_stream_t * stream= (gds_stream_t *) ctx;
  ptr_array_t * array= (ptr_array_t *) item;
  unsigned int count= 0;

  if (array != NULL)
    count= ptr_array_length(array);
  stream_printf(stream, "%u\n", count);
  return 0;
}

// -----[ comm_hash_statistics ]-------------------------------------
/**
 *
 */
void comm_hash_statistics(gds_stream_t * stream)
{
  time_t now= time(NULL);

  _comm_hash_init();
  stream_printf(stream, "# C-BGP Global Communities repository statistics\n");
  stream_printf(stream, "# generated on %s", ctime(&now));
  stream_printf(stream, "# hash-size is %u\n", _global_ref.size);
  assert(!hash_set_for_each_key(_global_ref.hash,
				_comm_hash_statistics_for_each,
				stream));
}

/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION AND FINALIZATION SECTION
//
/////////////////////////////////////////////////////////////////////

// -----[ _comm_hash_init ]-------------------------------------------
void _comm_hash_init()
{
  if (_global_ref.hash == NULL) {
    _global_ref.hash= hash_set_create(_global_ref.size,
				      0,
				      _comm_hash_item_compare,
				      _comm_hash_item_destroy,
				      _global_ref.compute);
    assert(_global_ref.hash != NULL);
  }
}

// -----[ _comm_hash_destroy ]---------------------------------------
void _comm_hash_destroy()
{
  if (_global_ref.hash != NULL)
    hash_set_destroy(&_global_ref.hash);
}
