// ==================================================================
// @(#)bgp_Peer.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/04/2006
// $Id: bgp_Peer.c,v 1.12 2009-03-25 07:49:45 bqu Exp $
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
#include <jni/impl/bgp_Filter.h>
#include <jni/impl/bgp_Peer.h>
#include <jni/impl/bgp_Router.h>

#include <bgp/mrtd.h>
#include <bgp/peer.h>

#define CLASS_BGPPeer "be/ac/ucl/ingi/cbgp/bgp/Peer"
#define CONSTR_BGPPeer "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPAddress;I)V"

// -----[ cbgp_jni_new_bgp_Peer ]------------------------------------
/**
 * This function creates a new instance of the bgp.Peer object from a
 * CBGP peer.
 */
jobject cbgp_jni_new_bgp_Peer(JNIEnv * jEnv, jobject joCBGP,
			      bgp_peer_t * peer)
{
  jobject joIPAddress;
  jobject joPeer;

  /* Java proxy object already existing ? */
  joPeer= jni_proxy_get(jEnv, peer);
  if (joPeer != NULL)
    return joPeer;

  /* Convert peer attributes to Java objects */
  if ((joIPAddress= cbgp_jni_new_IPAddress(jEnv, peer->addr)) == NULL)
    return NULL;

  /* Create new BGPPeer object */
  if ((joPeer= cbgp_jni_new(jEnv, CLASS_BGPPeer, CONSTR_BGPPeer,
			    joCBGP,
		            joIPAddress,
		            (jint) peer->asn)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joPeer, peer);

  return joPeer;
}

// -----[ getRouter ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getRouter
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/bgp/Router;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getRouter
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jobject joRouter;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return NULL;

  joRouter= cbgp_jni_new_bgp_Router(jEnv,
				    NULL/*jni_proxy_get_CBGP(jEnv, joPeer)*/,
				    peer->router);

  return_jni_unlock(jEnv, joRouter);
}

// -----[ getRouterID ]----------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getRouterID
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/IPAddress;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getRouterID
  (JNIEnv * jEnv, jobject joObject)
{
  bgp_peer_t * peer;

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joObject);
  if (peer == NULL)
    return NULL;

  return cbgp_jni_new_IPAddress(jEnv, peer->router_id);
}

// -----[ getSessionState ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getSessionState
 * Signature: ()B
 */
JNIEXPORT jbyte JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getSessionState
  (JNIEnv * jEnv, jobject joObject)
{
  bgp_peer_t * peer;
  
  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joObject);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  return_jni_unlock(jEnv, peer->session_state);
}

// -----[ openSession ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    openSession
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_openSession
  (JNIEnv * jEnv, jobject joObject)
{
  bgp_peer_t * peer;

  jni_lock(jEnv);
  
  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joObject);
  if (peer == NULL)
    return_jni_unlock2(jEnv);
 
  bgp_peer_open_session(peer);

  jni_unlock(jEnv);
}

// -----[ closeSession ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    closeSession
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_closeSession
  (JNIEnv * jEnv, jobject joObject)
{
  bgp_peer_t * peer;

  jni_lock(jEnv);
  
  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joObject);
  if (peer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_close_session(peer);

  jni_unlock(jEnv);
}

// -----[ recv ]-----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    recv
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_recv
  (JNIEnv * jEnv, jobject joPeer, jstring jsMesg)
{
  bgp_peer_t * peer;
  const char * cMesg;
  bgp_msg_t * msg;
  int result;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock2(jEnv);

  /* Check that the peer is virtual */
  if (!bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL)) {
    throw_CBGPException(jEnv, "only virtual peers can do that");
    return_jni_unlock2(jEnv);
  }

  /* Build a message from the MRT-record */
  cMesg= (*jEnv)->GetStringUTFChars(jEnv, jsMesg, NULL);
  result= mrtd_msg_from_line(peer->router, peer, cMesg,
			     &msg);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsMesg, cMesg);

  if (result >= 0) {
    if (bgp_peer_handle_message(peer, msg) != 0)
      throw_CBGPException(jEnv, "could not handle message");
  } else
    throw_CBGPException(jEnv, "could not understand MRT message (%s)",
			mrtd_strerror(result));

  jni_unlock(jEnv);
}

// -----[ isInternal ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    isInternal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_isInternal
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jboolean jResult;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (peer->asn == peer->router->asn)?JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}

// -----[ is ReflectorClient ]---------------------------------------
/**
 * Class:     bgp.Peer
 * Method:    isReflectorClient
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_isReflectorClient
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jboolean jResult;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (bgp_peer_flag_get(peer, PEER_FLAG_RR_CLIENT) != 0)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}									       

// -----[ setReflectorClient ]---------------------------------------
/*
 * Class:     bgp.Peer
 * Method:    setReflectorClient
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_setReflectorClient
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock2(jEnv);

  peer->router->reflector= 1;
  bgp_peer_flag_set(peer, PEER_FLAG_RR_CLIENT, 1);

  jni_unlock(jEnv);
}

// -----[ isVirtual ]------------------------------------------------
/**
 * Class:     bgp.Peer
 * Method:    isVirtual
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_isVirtual
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jboolean jResult;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (bgp_peer_flag_get(peer, PEER_FLAG_VIRTUAL) != 0)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}

// -----[ setVirtual ]-----------------------------------------------
/*
 * Class:     bgp.Peer
 * Method:    setVirtual
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_setVirtual
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_flag_set(peer, PEER_FLAG_VIRTUAL, 1);
  bgp_peer_flag_set(peer, PEER_FLAG_SOFT_RESTART, 1);

  jni_unlock(jEnv);
}

// -----[ getNextHopSelf ]-------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getNextHopSelf
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getNextHopSelf
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jboolean jbNextHopSelf;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jbNextHopSelf= bgp_peer_flag_get(peer, PEER_FLAG_NEXT_HOP_SELF)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jbNextHopSelf);
}

// -----[ setNextHopSelf ]-------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    setNextHopSelf
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_setNextHopSelf
  (JNIEnv * jEnv, jobject joPeer, jboolean state)
{
  bgp_peer_t * peer;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock2(jEnv);

  bgp_peer_flag_set(peer, PEER_FLAG_NEXT_HOP_SELF,
		    (state==JNI_TRUE)?1:0);

  jni_unlock(jEnv);
}

// -----[ hasSoftRestart ]-------------------------------------------
/**
 * Class:     bgp.Peer
 * Method:    hasSoftRestart
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_hasSoftRestart
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jboolean jResult;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (bgp_peer_flag_get(peer, PEER_FLAG_SOFT_RESTART) != 0)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}

// -----[ isAutoConfigured ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    isAutoConfigured
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_isAutoConfigured
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jboolean jResult;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  jResult= (bgp_peer_flag_get(peer, PEER_FLAG_AUTOCONF) != 0)?
    JNI_TRUE:JNI_FALSE;

  return_jni_unlock(jEnv, jResult);
}

// -----[ getNextHop ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getNextHop
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/IPAddress;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getNextHop
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jobject joAddress;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  joAddress= cbgp_jni_new_IPAddress(jEnv, peer->next_hop);

  return_jni_unlock(jEnv, joAddress);
}

// -----[ getUpdateSource ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getUpdateSource
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/IPAddress;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getUpdateSource
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  jobject joAddress;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  joAddress= cbgp_jni_new_IPAddress(jEnv, peer->src_addr);

  return_jni_unlock(jEnv, joAddress);
}

// -----[ getInputfilter ]-------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getInputFilter
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/bgp/Filter;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getInputFilter
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  bgp_filter_t * pFilter;
  jobject joFilter;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((pFilter= bgp_peer_filter_get(peer, FILTER_IN)) == NULL)
    return_jni_unlock(jEnv, NULL);

  joFilter= cbgp_jni_new_bgp_Filter(jEnv,
				    NULL/*jni_proxy_get_CBGP(jEnv, joPeer)*/,
				    pFilter);

  return_jni_unlock(jEnv, joFilter);
}

// -----[ getOutputFilter ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Peer
 * Method:    getOutputFilter
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/bgp/Filter;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Peer_getOutputFilter
  (JNIEnv * jEnv, jobject joPeer)
{
  bgp_peer_t * peer;
  bgp_filter_t * pFilter;
  jobject joFilter;

  jni_lock(jEnv);

  peer= (bgp_peer_t *) jni_proxy_lookup(jEnv, joPeer);
  if (peer == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((pFilter= bgp_peer_filter_get(peer, FILTER_OUT)) == NULL)
    return_jni_unlock(jEnv, NULL);

  joFilter= cbgp_jni_new_bgp_Filter(jEnv,
				    NULL/*jni_proxy_get_CBGP(jEnv, joPeer)*/,
				    pFilter);

  return_jni_unlock(jEnv, joFilter);
}
