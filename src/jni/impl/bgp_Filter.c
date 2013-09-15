// ==================================================================
// @(#)bgp_Filter.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/04/2006
// $Id: bgp_Filter.c,v 1.7 2009-03-25 07:49:45 bqu Exp $
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

#include <jni_md.h>
#include <jni.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/bgp_Peer.h>
#include <jni/impl/bgp_FilterRule.h>

#include <bgp/mrtd.h>
#include <bgp/peer.h>

#define CLASS_BGPFilter "be/ac/ucl/ingi/cbgp/bgp/Filter"
#define CONSTR_BGPFilter "(Lbe/ac/ucl/ingi/cbgp/CBGP;)V"

// -----[ cbgp_jni_new_bgp_Filter ]----------------------------------
/**
 * This function creates a new instance of the bgp.Filter object from
 * a CBGP filter.
 */
jobject cbgp_jni_new_bgp_Filter(JNIEnv * jEnv, jobject joCBGP,
				bgp_filter_t * filter)
{
  jobject joFilter;

  /* Java proxy object already existing ? */
  joFilter= jni_proxy_get(jEnv, filter);
  if (joFilter != NULL)
    return joFilter;

  /* Create new Filter object */
  if ((joFilter= cbgp_jni_new(jEnv, CLASS_BGPFilter, CONSTR_BGPFilter,
			      joCBGP)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joFilter, filter);

  return joFilter;
}

// -----[ getRules ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Filter
 * Method:    getRules
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Filter_getRules
  (JNIEnv * jEnv, jobject joFilter)
{
  bgp_filter_t * filter;
  bgp_ft_rule_t * rule;
  unsigned int index;
  jobject joRule;
  jobject joVector;

  jni_lock(jEnv);

  filter= (bgp_filter_t *) jni_proxy_lookup(jEnv, joFilter);
  if (filter == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  for (index= 0; index < filter->rules->size; index++) {
    rule= (bgp_ft_rule_t *) filter->rules->items[index];
    joRule= cbgp_jni_new_bgp_FilterRule(jEnv, NULL, rule);
    if (joRule == NULL)
      return_jni_unlock(jEnv, NULL);
    if (cbgp_jni_Vector_add(jEnv, joVector, joRule) != 0)
      return_jni_unlock(jEnv, NULL);
  }
  
  return_jni_unlock(jEnv, joVector);
}
