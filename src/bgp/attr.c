// ==================================================================
// @(#)attr.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/11/2005
// $Id: attr.c,v 1.9 2009-03-24 14:25:33 bqu Exp $
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

#include <libgds/memory.h>

#include <bgp/attr.h>
#include <bgp/attr/comm_hash.h>
#include <bgp/attr/path_hash.h>

// -----[ Forward prototypes declaration ]---------------------------
/* Note: functions starting with underscore (_) are intended to be
 * used inside this file only (private). These functions should be
 * static and should not appear in the .h file.
 */
static inline void _bgp_attr_path_destroy(bgp_attr_t * attr);
static inline void _bgp_attr_comm_destroy(bgp_attr_t ** pattr);
static inline void _bgp_attr_ecomm_destroy(bgp_attr_t * attr);

// -----[ bgp_attr_set_nexthop ]-------------------------------------
/**
 *
 */
void bgp_attr_set_nexthop(bgp_attr_t ** attr_ref,
			  net_addr_t next_hop)
{
  (*attr_ref)->next_hop= next_hop;
}

// -----[ bgp_attr_get_nexthop ]-------------------------------------
/**
 *
 */
net_addr_t bgp_attr_get_nexthop(bgp_attr_t * attr)
{
  return attr->next_hop;
}

// -----[ bgp_attr_set_origin ]--------------------------------------
/**
 *
 */
void bgp_attr_set_origin(bgp_attr_t ** attr_ref,
			 bgp_origin_t origin)
{
  (*attr_ref)->origin= origin;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE AS-PATH
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_path_copy ]--------------------------------------
/*
 * Copy the given AS-Path and update the references into the global
 * path repository.
 *
 * Pre-condition:
 *   the source AS-path must be intern, that is it must already be in
 *   the global AS-Path repository.
 */
static inline void _bgp_attr_path_copy(bgp_attr_t * attr, bgp_path_t * path)
{
  /*log_printf(pLogErr, "-->PATH_COPY [%p]\n", pPath);*/

  attr->path_ref= path_hash_add(path);

  // Check that the referenced path is equal to the source path
  // (if not, that means that the source path was not intern)
  assert(attr->path_ref == path);

  /*log_printf(pLogErr, "<--PATH_COPY [%p]\n", attr->pASPathRef);*/
}

// -----[ _bgp_attr_path_destroy ]-----------------------------------
/**
 * Destroy the AS-Path and removes the global reference.
 */
static inline void _bgp_attr_path_destroy(bgp_attr_t * attr)
{
  /*log_printf(pLogErr, "-->PATH_DESTROY [%p]\n", attr->pASPathRef);*/
  if (attr->path_ref != NULL) {
    /*log_printf(pLogErr, "attr-remove (%p)\n", attr->pASPathRef);*/
    path_hash_remove(attr->path_ref);
    attr->path_ref= NULL;
  }
  /*log_printf(pLogErr, "<--PATH_DESTROY [---]\n");*/
}

// -----[ bgp_attr_set_path ]----------------------------------------
/**
 * Takes an external path and associates it to the route. If the path
 * already exists in the repository, the external path is destroyed.
 * Otherwize, it is internalized (put in the repository).
 */
void bgp_attr_set_path(bgp_attr_t ** attr_ref, bgp_path_t * path)
{
  /*log_printf(pLogErr, "-->PATH_SET [%p]\n", pPath);*/
  _bgp_attr_path_destroy(*attr_ref);
  if (path != NULL) {
    (*attr_ref)->path_ref= path_hash_add(path);
    assert((*attr_ref)->path_ref != NULL);
    if ((*attr_ref)->path_ref != path)
      path_destroy(&path);
  }

  /*log_printf(pLogErr, "<--PATH_SET [%p]\n", (*pattr)->pASPathRef);*/
}

// -----[ bgp_attr_path_prepend ]------------------------------------
/*
 * Copy the given AS-Path and update the references into the global
 * path repository.
 */
int bgp_attr_path_prepend(bgp_attr_t ** attr_ref,
			  asn_t asn, uint8_t amount)
{
  bgp_path_t * path;

  /*log_printf(pLogErr, "-->PATH_PREPEND [%p]\n", (*pattr)->pASPathRef);*/

  // Create extern AS-Path copy
  if ((*attr_ref)->path_ref == NULL)
    path= path_create();
  else
    path= path_copy((*attr_ref)->path_ref);

  // Prepend
  while (amount-- > 0) {
    if (path_append(&path, asn) < 0)
      return -1;
  }

  // Intern path
  bgp_attr_set_path(attr_ref, path);

  /*log_printf(pLogErr, "<--PATH_PREPEND [%p]\n", (*pattr)->pASPathRef);*/
  return 0;
}

// -----[ bgp_attr_path_rem_private ]--------------------------------
int bgp_attr_path_rem_private(bgp_attr_t ** attr_ref)
{
  bgp_path_t * path;

  /*log_printf(pLogErr, "-->PATH_REM_PRIVATE [%p]\n", (*pattr)->pASPathRef);*/

  // Create extern AS-Path copy
  if ((*attr_ref)->path_ref == NULL)
    path= path_create();
  else
    path= path_copy((*attr_ref)->path_ref);

  path_remove_private(path);

  // Intern path
  bgp_attr_set_path(attr_ref, path);

  /*log_printf(pLogErr, "<--PATH_REM_PRIVATE [%p]\n", (*pattr)->pASPathRef);*/
  return 0;
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_comm_copy ]--------------------------------------
/**
 * Copy the Communities attribute and update the references into the
 * global path repository.
 *
 * Pre-condition:
 *   the source Communities must be intern, that is it must already be in
 *   the global Communities repository.
 */
static inline void _bgp_attr_comm_copy(bgp_attr_t * attr,
				       bgp_comms_t * comms)
{
  attr->comms= comm_hash_add(comms);

  // Check that the referenced Communities is equal to the source
  // Communities (if not, that means that the source Communities was
  // not intern)
  assert(attr->comms == comms);
}

// -----[ _bgp_attr_comm_destroy ]-----------------------------------
/**
 * Destroy the Communities attribute and removes the global
 * reference.
 */
static inline void _bgp_attr_comm_destroy(bgp_attr_t ** attr_ref)
{
  if ((*attr_ref)->comms != NULL) {
    comm_hash_remove((*attr_ref)->comms);
    (*attr_ref)->comms= NULL;
  }
}

// -----[ bgp_attr_set_comm ]----------------------------------------
/**
 *
 */
void bgp_attr_set_comm(bgp_attr_t ** attr_ref,
		       bgp_comms_t * comms)
{
  _bgp_attr_comm_destroy(attr_ref);
  if (comms != NULL) {
    (*attr_ref)->comms= comm_hash_add(comms);
    assert((*attr_ref)->comms != NULL);
    if ((*attr_ref)->comms != comms)
      comms_destroy(&comms);
  }
}

// -----[ bgp_attr_comm_append ]-------------------------------------
/**
 *
 */
int bgp_attr_comm_append(bgp_attr_t ** attr_ref, bgp_comm_t comm)
{
  bgp_comms_t * comms;

  // Create extern Communities copy
  if ((*attr_ref)->comms == NULL)
    comms= comms_create();
  else
    comms= comms_dup((*attr_ref)->comms);

  // Add new community value
  if (comms_add(&comms, comm)) {
    comms_destroy(&comms);
    return -1;
  }

  // Intern Communities
  bgp_attr_set_comm(attr_ref, comms);
  return 0;
}

// -----[ bgp_attr_comm_remove ]-------------------------------------
/**
 *
 */
void bgp_attr_comm_remove(bgp_attr_t ** attr_ref, bgp_comm_t comm)
{
  bgp_comms_t * comms;

  if ((*attr_ref)->comms == NULL)
    return;

  // Create extern Communities copy
  comms= comms_dup((*attr_ref)->comms);

  // Remove given community (this will destroy the Communities if
  // needed)
  comms_remove(&comms, comm);

  // Intern Communities
  bgp_attr_set_comm(attr_ref, comms);
}

// -----[ bgp_attr_comm_strip ]--------------------------------------
/**
 *
 */
void bgp_attr_comm_strip(bgp_attr_t ** attr_ref)
{
  _bgp_attr_comm_destroy(attr_ref);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE EXTENDED-COMMUNITIES
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_ecomm_copy ]-------------------------------------
static inline void _bgp_attr_ecomm_copy(bgp_attr_t * attr,
					bgp_ecomms_t * ecomms)
{
  _bgp_attr_ecomm_destroy(attr);
  if (ecomms != NULL)
    attr->ecomms= ecomms_dup(ecomms);
}

// -----[ _bgp_attr_ecomm_destroy ]----------------------------------
/**
 * Destroy the extended-communities attribute.
 */
static inline void _bgp_attr_ecomm_destroy(bgp_attr_t * attr)
{
  ecomms_destroy(&attr->ecomms);
}

// -----[ bgp_attr_ecomm_append ]------------------------------------
/**
 *
 */
int bgp_attr_ecomm_append(bgp_attr_t ** attr_ref, bgp_ecomm_t * ecomm)
{
  if ((*attr_ref)->ecomms == NULL)
    (*attr_ref)->ecomms= ecomms_create();
  return ecomms_add(&(*attr_ref)->ecomms, ecomm);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE ORIGINATOR-ID
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_originator_copy ]--------------------------------
/**
 *
 */
static inline void _bgp_attr_originator_copy(bgp_attr_t * attr,
					     bgp_originator_t * originator)
{
  if (originator == NULL)
    attr->originator= NULL;
  else {
    attr->originator= (bgp_originator_t *) MALLOC(sizeof(bgp_originator_t));
    *attr->originator= *originator;
  }
}

// -----[ bgp_attr_originator_destroy ]------------------------------
/**
 *
 */
void bgp_attr_originator_destroy(bgp_attr_t * attr)
{
  if (attr->originator != NULL) {
    FREE(attr->originator);
    attr->originator= NULL;
  }
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTE CLUSTER-ID-LIST
//
/////////////////////////////////////////////////////////////////////

// -----[ _bgp_attr_cluster_list_copy ]-------------------------------
/**
 *
 */
static inline void _bgp_attr_cluster_list_copy(bgp_attr_t * attr,
					       bgp_cluster_list_t * cl)
{
  if (cl == NULL)
    attr->cluster_list= NULL;
  else
    attr->cluster_list= cluster_list_copy(cl);
}

// -----[ bgp_attr_cluster_list_destroy ]----------------------------
/**
 *
 */
void bgp_attr_cluster_list_destroy(bgp_attr_t * attr)
{
  cluster_list_destroy(&attr->cluster_list);
}


/////////////////////////////////////////////////////////////////////
//
// ATTRIBUTES
//
/////////////////////////////////////////////////////////////////////

// -----[ bgp_attr_create ]------------------------------------------
/**
 *
 */
bgp_attr_t * bgp_attr_create(net_addr_t next_hop,
			     bgp_origin_t origin,
			     uint32_t local_pref,
			     uint32_t med)
{
  bgp_attr_t * attr= (bgp_attr_t *) MALLOC(sizeof(bgp_attr_t));
  attr->next_hop= next_hop;
  attr->origin= origin;
  attr->local_pref= local_pref;
  attr->med= med;
  attr->path_ref= NULL;
  attr->comms= NULL;
  attr->ecomms= NULL;

  /* Route-reflection related fields */
  attr->originator= NULL;
  attr->cluster_list= NULL;

#ifdef __ROUTER_LIST_ENABLE__
  attr->router_list= cluster_list_create();
#endif

  return attr;
}

// -----[ bgp_attr_destroy ]-----------------------------------------
/**
 *
 */
void bgp_attr_destroy(bgp_attr_t ** attr_ref)
{
  if (*attr_ref != NULL) {
    _bgp_attr_path_destroy(*attr_ref);
    _bgp_attr_comm_destroy(attr_ref);
    _bgp_attr_ecomm_destroy(*attr_ref);

    /* Route-reflection */
    bgp_attr_originator_destroy(*attr_ref);
    bgp_attr_cluster_list_destroy(*attr_ref);

#ifdef __ROUTER_LIST_ENABLE__
    cluster_list_destroy(&(*attr_ref)->router_list);
#endif

    FREE(*attr_ref);
  }
}

// -----[ bgp_attr_copy ]--------------------------------------------
/**
 *
 */
bgp_attr_t * bgp_attr_copy(bgp_attr_t * attr)
{
  bgp_attr_t * attr_copy= bgp_attr_create(attr->next_hop,
					  attr->origin,
					  attr->local_pref,
					  attr->med);
  _bgp_attr_path_copy(attr_copy, attr->path_ref);
  _bgp_attr_comm_copy(attr_copy, attr->comms);
  _bgp_attr_ecomm_copy(attr_copy, attr->ecomms);

  /* Route-Reflection attributes */
  _bgp_attr_originator_copy(attr_copy, attr->originator);
  _bgp_attr_cluster_list_copy(attr_copy, attr->cluster_list);

#ifdef __ROUTER_LIST_ENABLE__
  if (attr->router_list == NULL)
    attr_copy->router_list= NULL;
  else
    attr_copy->router_list= cluster_list_copy(attr->router_list);
#endif

  return attr_copy;
}

// -----[ bgp_attr_cmp ]---------------------------------------------
/**
 * This function compares two sets of BGP attributes. The function
 * only tests if the values of both sets are equal or not.
 *
 * Return value:
 *   1  if attributes are equal
 *   0  otherwise
 */
int bgp_attr_cmp(bgp_attr_t * attr1, bgp_attr_t * attr2)
{
  // If both pointers are equal, the content must also be equal.
  if (attr1 == attr2)
    return 1;

  // NEXT-HOP attributes must be equal
  if (attr1->next_hop != attr2->next_hop) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different NEXT-HOP\n");
    return 0;
  }

  // LOCAL-PREF attributes must be equal
  if (attr1->local_pref != attr2->local_pref) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different LOCAL-PREFs\n");
    return 0;
  }

  // AS-PATH attributes must be equal
  if (!path_equals(attr1->path_ref, attr2->path_ref)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different AS-PATHs\n");
    return 0;
  }

  // ORIGIN attributes must be equal
  if (attr1->origin != attr2->origin) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different ORIGINs\n");
    return 0;
  }

  // MED attributes must be equal
  if (attr1->med != attr2->med) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different MEDs\n");
    return 0;
  }

  // COMMUNITIES attributes must be equal
  if (!comms_equals(attr1->comms, attr2->comms)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different COMMUNITIES\n");
    return 0;
  }

  // EXTENDED-COMMUNITIES attributes must be equal
  if (!ecomm_equals(attr1->ecomms, attr2->ecomms)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different EXTENDED-COMMUNITIES\n");
    return 0;
  }

  // ORIGINATOR-ID attributes must be equal
  if (!originator_equals(attr1->originator, attr2->originator)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different ORIGINATOR-ID\n");
    return 0;
  }
 
  // CLUSTER-ID-LIST attributes must be equal
  if (!cluster_list_equals(attr1->cluster_list, attr2->cluster_list)) {
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "different CLUSTER-ID-LIST\n");
    return 0;
  }
  
  return 1;
}

