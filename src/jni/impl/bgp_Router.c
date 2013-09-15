// ==================================================================
// @(#)bgp_Router.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 14/04/2006
// $Id: bgp_Router.c,v 1.15 2009-04-02 19:17:04 bqu Exp $
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
#include <jni/impl/bgp_Peer.h>
#include <jni/impl/bgp_Router.h>
#include <jni/impl/net_Node.h>

#include <bgp/as.h>
#include <bgp/rib.h>
#include <bgp/route.h>
#include <bgp/route-input.h>

#define CLASS_BGPRouter "be/ac/ucl/ingi/cbgp/bgp/Router"
#define CONSTR_BGPRouter "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                         "Lbe/ac/ucl/ingi/cbgp/net/Node;" \
                         "SLbe/ac/ucl/ingi/cbgp/IPAddress;)V"

// -----[ cbgp_jni_new_bgp_Router ]-----------------------------------
/**
 * This function creates a new instance of the bgp.Router object from a
 * BGP router.
 */
jobject cbgp_jni_new_bgp_Router(JNIEnv * jEnv, jobject joCBGP,
				bgp_router_t * router)
{
  jobject joNode;
  jobject joRouterID;
  jobject joRouter;

  /* Java proxy object already existing ? */
  joRouter= jni_proxy_get(jEnv, router);
  if (joRouter != NULL)
    return joRouter;

  /* Get underlying node */
  joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, router->node);
  if (joNode == NULL)
    return NULL;

  /* Convert router attributes to Java objects */
  if ((joRouterID= cbgp_jni_new_IPAddress(jEnv, router->rid)) == NULL)
    return NULL;

  /* Create new BGPRouter object */
  if ((joRouter= cbgp_jni_new(jEnv, CLASS_BGPRouter, CONSTR_BGPRouter,
			      joCBGP,
			      joNode,
			      (jshort) router->asn,
			      joRouterID)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joRouter, router);

  return joRouter;
}

// -----[ isRouteReflector ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    isRouteReflector
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_isRouteReflector
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * router;

  jni_lock(jEnv);

  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  return_jni_unlock(jEnv, (router->reflector != 0)?JNI_TRUE:JNI_FALSE);
}

// -----[ addNetwork ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    addNetwork
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/IPPrefix;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_addNetwork
  (JNIEnv * jEnv, jobject joRouter, jstring jsPrefix)
{
  bgp_router_t * router;
  ip_pfx_t prefix;
  jobject joPrefix;
  int result;

  jni_lock(jEnv);

  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock(jEnv, NULL);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &prefix) != 0)
    return_jni_unlock(jEnv, NULL);

  result= bgp_router_add_network(router, prefix);
  if (result != ESUCCESS) {
    throw_CBGPException(jEnv, "could not add network (%s)",
			network_strerror(result));
    return_jni_unlock(jEnv, NULL);
  }

  joPrefix= cbgp_jni_new_IPPrefix(jEnv, prefix);

  return_jni_unlock(jEnv, joPrefix);
}

// -----[ delNetwork ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    delNetwork
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_delNetwork
  (JNIEnv * jEnv, jobject joRouter, jstring jsPrefix)
{
  bgp_router_t * router;
  ip_pfx_t prefix;

  jni_lock(jEnv);

  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &prefix) != 0)
    return_jni_unlock2(jEnv);

  if (bgp_router_del_network(router, prefix) != 0) {
    throw_CBGPException(jEnv, "coud not remove network");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}

// -----[ addPeer ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    addPeer
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_addPeer
  (JNIEnv * jEnv, jobject joRouter, jstring jsPeerAddr, jint jiASNumber)
{
  bgp_router_t * router;
  bgp_peer_t * peer;
  net_addr_t tPeerAddr;
  jobject joPeer;

  jni_lock(jEnv);

  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock(jEnv, NULL);

  if (ip_jstring_to_address(jEnv, jsPeerAddr, &tPeerAddr) != 0)
    return_jni_unlock(jEnv, NULL);

  if (bgp_router_add_peer(router, jiASNumber, tPeerAddr, &peer) != 0) {
    throw_CBGPException(jEnv, "could not add peer");
    return_jni_unlock(jEnv, NULL);
  }

  joPeer= cbgp_jni_new_bgp_Peer(jEnv,
				NULL/*jni_proxy_get_CBGP(jEnv, joRouter)*/,
				peer);

  return_jni_unlock(jEnv, joPeer);
}

// -----[ _cbgp_jni_get_peer ]----------------------------------------
static int _cbgp_jni_get_peer(const void * item,
			      const void * ctx)
{
  jni_ctx_t * jni_ctx= (jni_ctx_t *) ctx;
  jobject joPeer;

  if ((joPeer= cbgp_jni_new_bgp_Peer(jni_ctx->jEnv,
				     jni_ctx->joCBGP,
				     *((bgp_peer_t **) item))) == NULL)
    return -1;

  return cbgp_jni_Vector_add(jni_ctx->jEnv, jni_ctx->joVector, joPeer);
}

// -----[ getPeers ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    getPeers
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getPeers
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * router;
  jobject joVector;
  jni_ctx_t sCtx;

  jni_lock(jEnv);

  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joRouter)*/;
  sCtx.jEnv= jEnv;
  if (bgp_router_peers_for_each(router, _cbgp_jni_get_peer, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_rib_route ]----------------------------------
static int _cbgp_jni_get_rib_route(uint32_t uKey, uint8_t uKeyLen,
				   void * pItem, void * pContext)
{
   jni_ctx_t * pCtx= (jni_ctx_t *) pContext;
  jobject joRoute;

  joRoute= cbgp_jni_new_BGPRoute(pCtx->jEnv, (bgp_route_t *) pItem, NULL);
  if (joRoute == NULL)
    return -1;

  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ getRIB ]---------------------------------------------------
/**
 * Class    : bgp.Router
 * Method   : getRIB
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 *
 * This function creates an array of BGPRoute objects corresponding
 * with the content of the given router's RIB.
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getRIB
  (JNIEnv * jEnv, jobject joRouter, jstring jsDest)
{
  bgp_router_t * router;
  jobject joVector;
  bgp_route_t * route;
  jni_ctx_t sCtx;
  ip_dest_t dest;

  jni_lock(jEnv);

  /* Get the router instance */
  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create a Vector object that will hold the BGP routes to be
     returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the destination specifier (*|address|prefix) */
  if (jsDest != NULL) {
    if (ip_jstring_to_dest(jEnv, jsDest, &dest) < 0)
      return_jni_unlock(jEnv, NULL);
  } else
    dest.type= NET_ADDR_ANY;

  /* Prepare the JNI Context environment */
  sCtx.router= router;
  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;

  switch (dest.type) {
  case NET_DEST_ANY:

    /* For each route in the RIB, create a new BGPRoute object and add
       it to the Vector object */
    if (rib_for_each(router->loc_rib, _cbgp_jni_get_rib_route, &sCtx) != 0)
      return_jni_unlock(jEnv, NULL);
    break;

  case NET_DEST_ADDRESS:
#ifndef __EXPERIMENTAL_WALTON__ 
    dest.prefix.network= dest.addr;
    dest.prefix.mask= 32;
    route= rib_find_best(router->loc_rib, dest.prefix);
    if (route != NULL)
      if (_cbgp_jni_get_rib_route(dest.prefix.network,
				  32, route, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
#endif

    break;

  case NET_DEST_PREFIX:
#ifndef __EXPERIMENTAL_WALTON__ 
    route= rib_find_exact(router->loc_rib, dest.prefix);
    if (route != NULL)
      if (_cbgp_jni_get_rib_route(dest.prefix.network,
				  dest.prefix.mask,
				  route, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
#endif
    break;

  default:
    throw_CBGPException(jEnv, "invalid destination type for getRIB()");
  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_adj_rib_routes ]-----------------------------
static int _cbgp_jni_get_adj_rib_routes(bgp_peer_t * peer, ip_dest_t * dest,
					int iIn, jni_ctx_t * pCtx)
{
  bgp_route_t * route;
  bgp_rib_dir_t dir= (iIn?RIB_IN:RIB_OUT);

  switch (dest->type) {
  case NET_DEST_ANY:
    return rib_for_each(peer->adj_rib[dir],
			_cbgp_jni_get_rib_route, pCtx);

  case NET_DEST_ADDRESS:
    dest->prefix.network= dest->addr;
    dest->prefix.mask= 32;

    route= rib_find_best(peer->adj_rib[dir], dest->prefix);
    if (route != NULL)
      return _cbgp_jni_get_rib_route(dest->prefix.network,
				     32, route, pCtx);
    break;
    
  case NET_DEST_PREFIX:
    route= rib_find_exact(peer->adj_rib[dir], dest->prefix);
    if (route != NULL)
      return _cbgp_jni_get_rib_route(dest->prefix.network,
				     dest->prefix.mask,
				     route, pCtx);
    break;

  default:
    throw_CBGPException(pCtx->jEnv, "invalid destination type for getAdjRIB()");
  }

  return 0;
}

// -----[ getAdjRIB ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    getAdjRIB
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getAdjRIB
  (JNIEnv * jEnv, jobject joRouter, jstring jsPeerAddr,
   jstring jsDest, jboolean bIn)
{
  bgp_router_t * router;
  jobject joVector;
  net_addr_t tPeerAddr;
  unsigned int index;
  bgp_peer_t * peer= NULL;
  ip_dest_t dest;
  jni_ctx_t sCtx;

  jni_lock(jEnv);

  /* Get the router instance */
  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create a Vector object that will hold the BGP routes to be returned */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the peer address */
  if (jsPeerAddr != NULL) {
    if (ip_jstring_to_address(jEnv, jsPeerAddr, &tPeerAddr) != 0)
      return_jni_unlock(jEnv, NULL);
    if ((peer= bgp_router_find_peer(router, tPeerAddr)) == NULL) {
      throw_CBGPException(jEnv, "unknown peer");
      return_jni_unlock(jEnv, NULL);
    }
  }

  /* Convert the destination specifier (*|address|prefix) */
  if (jsDest != NULL) {
    if (ip_jstring_to_dest(jEnv, jsDest, &dest) < 0)
      return_jni_unlock(jEnv, NULL);
  } else
    dest.type= NET_ADDR_ANY;

  /* Prepare the JNI Context environment */
  sCtx.router= router;
  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;

  if (peer == NULL) {
    
    for (index= 0; index < ptr_array_length(router->peers); index++) {
      peer= (bgp_peer_t *) router->peers->data[index];
      if (_cbgp_jni_get_adj_rib_routes(peer, &dest,
				       (bIn==JNI_TRUE), &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
    }

  } else {

    if (_cbgp_jni_get_adj_rib_routes(peer, &dest,
				     (bIn==JNI_TRUE), &sCtx) < 0)
      return_jni_unlock(jEnv, NULL);

  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ _get_networks ]--------------------------------------------
static int _get_networks(const void * item, const void * ctx)
{
  jni_ctx_t * jni_ctx= (jni_ctx_t *) ctx;
  jobject joNetwork;
  bgp_route_t * route= *((bgp_route_t **) item);

  if ((joNetwork= cbgp_jni_new_IPPrefix(jni_ctx->jEnv,
					route->prefix)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(jni_ctx->jEnv, jni_ctx->joVector, joNetwork);
}

// -----[ getNetworks ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    getNetworks
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_getNetworks
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * router;
  jobject joVector;
  jni_ctx_t sCtx;

  jni_lock(jEnv);

  /* Get the router instance */
  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joRouter)*/;
  
  bgp_router_networks_for_each(router, _get_networks, &sCtx);

  return_jni_unlock(jEnv, joVector);
}

// -----[ loadRib ]--------------------------------------------------
/*
 * Class:     bgp.Router
 * Method:    loadRib
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_loadRib
(JNIEnv * jEnv, jobject joRouter, jstring jsFileName, jboolean jbForce,
 jstring jsFormat)
{
  const char * filename, * format_str;
  bgp_router_t * router;
  bgp_input_type_t format= BGP_ROUTES_INPUT_MRT_ASC;
  uint8_t options= 0;
  int result;

  format_str= (*jEnv)->GetStringUTFChars(jEnv, jsFormat, NULL);
  result= bgp_routes_str2format(format_str, &format);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFormat, format_str);
  if (result != 0)
    throw_CBGPException(jEnv, "invalid format");

  jni_lock(jEnv);

  /* Get the router instance */
  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock2(jEnv);

  if (jbForce == JNI_TRUE) {
    options|= BGP_ROUTER_LOAD_OPTIONS_FORCE;
    options|= BGP_ROUTER_LOAD_OPTIONS_AUTOCONF;
  }

  options|= BGP_ROUTER_LOAD_OPTIONS_SUMMARY;

  filename= (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);
  result= bgp_router_load_rib(router, filename,
			      format, options);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, filename);
  if (result != 0)
    throw_CBGPException(jEnv, "could not load RIB (%s)",
			bgp_input_strerror(result));

  jni_unlock(jEnv);
}

// -----[ rescan ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Router
 * Method:    rescan
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Router_rescan
  (JNIEnv * jEnv, jobject joRouter)
{
  bgp_router_t * router;

  jni_lock(jEnv);

  /* Get the router instance */
  router= (bgp_router_t *) jni_proxy_lookup(jEnv, joRouter);
  if (router == NULL)
    return_jni_unlock2(jEnv);

  if (bgp_router_scan_rib(router) != 0)
    throw_CBGPException(jEnv, "could not rescan router");

  jni_unlock(jEnv);
}
