// ==================================================================
// @(#)net_IGPDomain.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/04/2006
// $Id: net_IGPDomain.c,v 1.12 2009-03-25 07:51:59 bqu Exp $
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
#include <jni/exceptions.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/net_IGPDomain.h>
#include <jni/impl/net_Node.h>

#define CLASS_IGPDomain "be/ac/ucl/ingi/cbgp/net/IGPDomain"
#define CONSTR_IGPDomain "(Lbe/ac/ucl/ingi/cbgp/CBGP;II)V"

// -----[ cbgp_jni_new_net_IGPDomain ]-------------------------------
/**
 * This function creates a new instance of the net.IGPDomain object
 * from an IGP domain.
 */
jobject cbgp_jni_new_net_IGPDomain(JNIEnv * jEnv, jobject joCBGP,
				   igp_domain_t * domain)
{
  jobject joDomain;

  /* Java proxy object already existing ? */
  joDomain= jni_proxy_get(jEnv, domain);
  if (joDomain != NULL)
    return joDomain;

  /* Create new IGPDomain object */
  if ((joDomain= cbgp_jni_new(jEnv, CLASS_IGPDomain,
			      CONSTR_IGPDomain,
			      joCBGP,
			      domain->id,
			      domain->type)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joDomain, domain);

  return joDomain;
}

// -----[ addNode ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_IGPDomain
 * Method:    addNode
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_IGPDomain_addNode
  (JNIEnv * jEnv, jobject joDomain, jstring jsAddr)
{
  igp_domain_t * domain;
  net_node_t * node; 
  net_addr_t tNetAddr;
  jobject joNode;
  net_error_t error;

  jni_lock(jEnv);

  /* Get the domain */
  domain= (igp_domain_t *) jni_proxy_lookup(jEnv, joDomain);
  if (domain == NULL)
    return_jni_unlock(jEnv, NULL);

  if (ip_jstring_to_address(jEnv, jsAddr, &tNetAddr) != 0)
    return_jni_unlock(jEnv, NULL);

  error= node_create(tNetAddr, &node, NODE_OPTIONS_LOOPBACK);
  if (error != ESUCCESS) {
    throw_CBGPException(jEnv, "node could not be created (%s)",
			network_strerror(error));
    return_jni_unlock(jEnv, NULL);
  }

  error= network_add_node(network_get_default(), node);
  if (error != ESUCCESS) {
    node_destroy(&node);
    throw_CBGPException(jEnv, "node already exists (%s)",
			network_strerror(error));
    return_jni_unlock(jEnv, NULL);
  }

  igp_domain_add_router(domain, node);

  joNode= cbgp_jni_new_net_Node(jEnv,
				NULL/*jni_proxy_get_CBGP(jEnv, joDomain)*/,
				node); 

  return_jni_unlock(jEnv, joNode);
}

// -----[ _netDomainGetNodes ]---------------------------------------
static int _netDomainGetNodes(uint32_t key, uint8_t key_len,
			      void * item, void * ctx)
{
  jni_ctx_t * pCtx= (jni_ctx_t *) ctx;
  net_node_t * node= (net_node_t * ) item;
  jobject joNode;

  if ((joNode= cbgp_jni_new_net_Node(pCtx->jEnv,
				     pCtx->joCBGP,
				     node)) == NULL)
    return -1;
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joNode);
}

// -----[ getNodes ]-------------------------------------------------
/**
 * Class:  be_ac_ucl_ingi_cbgp_net_IGPDomain
 * Method: getNodes
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_IGPDomain_getNodes
(JNIEnv * jEnv, jobject joDomain)
{
  jobject joVector;
  jni_ctx_t sCtx;
  igp_domain_t * domain= NULL;

  jni_lock(jEnv);

  domain= (igp_domain_t *) jni_proxy_lookup(jEnv, joDomain);
  if (domain == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joDomain)*/;

  if (igp_domain_routers_for_each(domain, _netDomainGetNodes, &sCtx))
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ compute ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_IGPDomain
 * Method:    compute
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_IGPDomain_compute
  (JNIEnv * jEnv, jobject joDomain)
{
  igp_domain_t * domain= NULL;

  jni_lock(jEnv);

  domain= (igp_domain_t *) jni_proxy_lookup(jEnv, joDomain);
  if (domain == NULL)
    return_jni_unlock2(jEnv);

  if (igp_domain_compute(domain, 0) != 0) {
    throw_CBGPException(jEnv, "could not compute IGP paths");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}
