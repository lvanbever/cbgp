// ==================================================================
// @(#)net_Link.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/03/2006
// $Id: net_Link.c,v 1.15 2009-03-25 07:51:59 bqu Exp $
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
#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/net_Subnet.h>

#include <net/link.h>

#define CLASS_Link "be/ac/ucl/ingi/cbgp/net/Interface"
#define CONSTR_Link "(Lbe/ac/ucl/ingi/cbgp/CBGP;"\
                    "Lbe/ac/ucl/ingi/cbgp/IPPrefix;J)V"

// -----[ cbgp_jni_new_net_Interface ]-------------------------------
/**
 * This function creates a new instance of the Link object from a CBGP
 * link.
 */
jobject cbgp_jni_new_net_Interface(JNIEnv * jEnv, jobject joCBGP,
			      net_iface_t * iface)
{
  jobject joLink;
  jobject joPrefix;

  /* Java proxy object already existing ? */
  joLink= jni_proxy_get(jEnv, iface);
  if (joLink != NULL)
    return joLink;

  /* Convert link attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, net_iface_dst_prefix(iface));
  if (joPrefix == NULL)
    return NULL;

  /* Create new Link object */
  if ((joLink= cbgp_jni_new(jEnv, CLASS_Link, CONSTR_Link,
			    joCBGP, joPrefix,
			    (jlong) net_iface_get_delay(iface))) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joLink, iface);

  return joLink;
}

// -----[ getType ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Interface
 * Method:    getType
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getType
(JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * iface;
  jstring jsType;

  jni_lock(jEnv);

  /* Get interface */
  iface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (iface == NULL)
    return_jni_unlock(jEnv, NULL);

  jsType= cbgp_jni_new_String(jEnv, net_iface_type2str(iface->type));

  return_jni_unlock(jEnv, jsType);
}


// -----[ getCapacity ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    getCapacity
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getCapacity
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jlong jlCapacity= 0;

  jni_lock(jEnv);

  /* Get interface */
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, -1);

  jlCapacity= net_iface_get_capacity(pIface);

  return_jni_unlock(jEnv, jlCapacity);
}

// -----[ setCapacity ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    setCapacity
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_setCapacity
  (JNIEnv * jEnv, jobject joIface, jlong jlCapacity)
{
  net_iface_t * pIface;
  int iResult;

  jni_lock(jEnv);

  /* Get interface */
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock2(jEnv);

  iResult= net_iface_set_capacity(pIface, jlCapacity, 0);
  if (iResult != ESUCCESS)
    throw_CBGPException(jEnv, "could not set capacity (%s)",
			network_strerror(iResult));

  jni_unlock(jEnv);
}

// -----[ getDelay ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Interface
 * Method:    getDelay
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getDelay
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * iface;
  jlong jlDelay= 0;

  jni_lock(jEnv);

  /* Get interface */
  iface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (iface == NULL)
    return_jni_unlock(jEnv, -1);

  jlDelay= net_iface_get_delay(iface);

  return_jni_unlock(jEnv, jlDelay);
}

// -----[ setDelay ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Interface
 * Method:    setDelay
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_setDelay
  (JNIEnv * jEnv, jobject joIface, jlong jlDelay)
{
  net_iface_t * iface;
  int result;

  jni_lock(jEnv);

  /* Get interface */
  iface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (iface == NULL)
    return_jni_unlock2(jEnv);

  result= net_iface_set_delay(iface, jlDelay, 0);
  if (result != ESUCCESS)
    throw_CBGPException(jEnv, "could not set delay (%s)",
			network_strerror(result));

  jni_unlock(jEnv);

}

// -----[ getLoad ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    getLoad
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getLoad
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jlong jlLoad= 0;

  jni_lock(jEnv);

  /* Get interface */
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, -1);

  jlLoad= net_iface_get_load(pIface);

  return_jni_unlock(jEnv, jlLoad);
}

// -----[ getState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    getState
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getState
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jboolean jbState;

  jni_lock(jEnv);
  
  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jbState= net_iface_is_enabled(pIface)?JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jbState);
}

// -----[ setState ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    setState
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_setState
  (JNIEnv * jEnv, jobject joIface, jboolean bState)
{
  net_iface_t * pIface;

  jni_lock(jEnv);

  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock2(jEnv);
    
  net_iface_set_enabled(pIface, (bState == JNI_TRUE)?1:0);

  jni_unlock(jEnv);
}

// -----[ getWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    getWeight
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getWeight
  (JNIEnv * jEnv, jobject joIface)
{
  net_iface_t * pIface;
  jlong jlResult;

  jni_lock(jEnv);

  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock(jEnv, 0);

  jlResult= (jlong) net_iface_get_metric(pIface, 0);

  return_jni_unlock(jEnv, jlResult);
}

// -----[ setWeight ]--------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    setWeight
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_setWeight
  (JNIEnv * jEnv, jobject joIface, jlong jlWeight)
{
  net_iface_t * pIface;
  int iResult;

  jni_lock(jEnv);

  pIface= (net_iface_t *) jni_proxy_lookup(jEnv, joIface);
  if (pIface == NULL)
    return_jni_unlock2(jEnv);

  iResult= net_iface_set_metric(pIface, 0, (uint32_t) jlWeight, 0);
  if (iResult != ESUCCESS)
    throw_CBGPException(jEnv, "could not set weight (%s)",
			network_strerror(iResult));

  jni_unlock(jEnv);
}

// -----[ getHead ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    getHead
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getHead
  (JNIEnv * jEnv, jobject joLink)
{
  net_iface_t * iface;
  jobject joNode;

  jni_lock(jEnv);

  iface= (net_iface_t *) jni_proxy_lookup(jEnv, joLink);
  if (iface == NULL)
    return_jni_unlock(jEnv, NULL);

  joNode= cbgp_jni_new_net_Node(jEnv,
				NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				iface->owner);

  return_jni_unlock(jEnv, joNode);
}

// -----[ getTail ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    getTail
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/Element;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Interface_getTail
  (JNIEnv * jEnv, jobject joLink)
{
  net_iface_t * iface;
  jobject joElement= NULL;

  jni_lock(jEnv);

  iface= (net_iface_t *) jni_proxy_lookup(jEnv, joLink);
  if (iface == NULL)
    return_jni_unlock(jEnv, NULL);

  switch (iface->type) {
  case NET_IFACE_LOOPBACK:
    joElement= NULL;
    break;

  case NET_IFACE_RTR:
    joElement= cbgp_jni_new_net_Node(jEnv,
				     NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				     iface->dest.iface->owner);
    break;

  case NET_IFACE_PTP:
    joElement= cbgp_jni_new_net_Node(jEnv,
				     NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				     iface->dest.iface->owner);
    break;

  case NET_IFACE_PTMP:
    joElement= cbgp_jni_new_net_Subnet(jEnv,
				       NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				       iface->dest.subnet);
    break;

  case NET_IFACE_VIRTUAL:
    joElement= NULL;
    break;

  default:
    cbgp_fatal("invalid link type in getTail()");
  }

  return_jni_unlock(jEnv, joElement);
}

// -----[ getTailInterface ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Iface
 * Method:    getTail
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/Element;
 */
JNIEXPORT jobject JNICALL
Java_be_ac_ucl_ingi_cbgp_net_Interface_getTailInterface
(JNIEnv * jEnv, jobject joLink)
{
  net_iface_t * iface;
  jobject joElement= NULL;

  jni_lock(jEnv);

  iface= (net_iface_t *) jni_proxy_lookup(jEnv, joLink);
  if (iface == NULL)
    return_jni_unlock(jEnv, NULL);

  switch (iface->type) {
  case NET_IFACE_RTR:
    joElement=
      cbgp_jni_new_net_Interface(jEnv,
				 NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				 iface->dest.iface);
    break;

  case NET_IFACE_PTP:
    joElement=
      cbgp_jni_new_net_Interface(jEnv,
				 NULL/*jni_proxy_get_CBGP(jEnv, joLink)*/,
				 iface->dest.iface);
    break;

  case NET_IFACE_LOOPBACK:
  case NET_IFACE_PTMP:
  case NET_IFACE_VIRTUAL:
    break;

  default:
    cbgp_fatal("invalid link type in getTailInterface()");
  }

  return_jni_unlock(jEnv, joElement);
}


