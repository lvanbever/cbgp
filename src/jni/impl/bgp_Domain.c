// ==================================================================
// @(#)bgp_Domain.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/04/2006
// $Id: bgp_Domain.c,v 1.8 2009-03-25 07:49:45 bqu Exp $
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
#include <jni/impl/bgp_Domain.h>
#include <jni/impl/bgp_Router.h>

#include <bgp/as.h>
#include <bgp/domain.h>

#define CLASS_BGPDomain "be/ac/ucl/ingi/cbgp/bgp/Domain"
#define CONSTR_BGPDomain "(Lbe/ac/ucl/ingi/cbgp/CBGP;I)V"

// -----[ cbgp_jni_new_bgp_Domain ]-----------------------------------
/**
 * This function creates a new instance of the bgp.Domain object from a
 * BGP domain.
 */
jobject cbgp_jni_new_bgp_Domain(JNIEnv * jEnv, jobject joCBGP,
				bgp_domain_t * domain)
{
  jobject joDomain;

  /* Java proxy object already existing ? */
  joDomain= jni_proxy_get(jEnv, domain);
  if (joDomain != NULL)
    return joDomain;

  /* Create new BGPDomain object */
  if ((joDomain= cbgp_jni_new(jEnv, CLASS_BGPDomain, CONSTR_BGPDomain,
			      joCBGP,
			      (jint) domain->asn)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joDomain, domain);

  return joDomain;
}

// -----[ _bgpDomainGetRouters ]-------------------------------------
static int _bgpDomainGetRouters(uint32_t key, uint8_t key_len,
				void * item, void * ctx)
{
  jni_ctx_t * pCtx= (jni_ctx_t *) ctx;
  bgp_router_t * router= (bgp_router_t * ) item;
  jobject joRouter;

  if ((joRouter= cbgp_jni_new_bgp_Router(pCtx->jEnv,
					 pCtx->joCBGP,
					 router)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRouter);
}

// -----[ getRouters ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Domain
 * Method:    getRouters
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Domain_getRouters
  (JNIEnv * jEnv, jobject joDomain)
{
  jobject joVector= NULL;
  jni_ctx_t sCtx;
  bgp_domain_t * domain;

  jni_lock(jEnv);

  domain= (bgp_domain_t *) jni_proxy_lookup(jEnv, joDomain);
  if (domain == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joDomain)*/;

  if (bgp_domain_routers_for_each(domain, _bgpDomainGetRouters, &sCtx))
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ rescan ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Domain
 * Method:    rescan
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Domain_rescan
  (JNIEnv * jEnv, jobject joDomain)
{
  bgp_domain_t * domain;

  jni_lock(jEnv);

  domain= (bgp_domain_t *) jni_proxy_lookup(jEnv, joDomain);
  if (domain == NULL)
    return_jni_unlock2(jEnv);

  bgp_domain_rescan(domain);

  jni_unlock(jEnv);
}
