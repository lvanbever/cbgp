// ==================================================================
// @(#)net_Node.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/04/2006
// $Id: net_Node.c,v 1.21 2009-03-25 07:51:59 bqu Exp $
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

#include <string.h>
#include <jni_md.h>
#include <jni.h>
#include <jni/exceptions.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/impl/net_IGPDomain.h>
#include <jni/impl/net_IPTrace.h>
#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/bgp_Router.h>

#include <net/error.h>
#include <net/icmp.h>
#include <net/igp_domain.h>
#include <net/node.h>

#define CLASS_Node "be/ac/ucl/ingi/cbgp/net/Node"
#define CONSTR_Node "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                    "Lbe/ac/ucl/ingi/cbgp/IPAddress;)V"

// -----[ cbgp_jni_new_net_Node ]------------------------------------
/**
 * This function creates a new instance of the Node object from a CBGP
 * node.
 */
jobject cbgp_jni_new_net_Node(JNIEnv * jEnv, jobject joCBGP,
			      net_node_t * node)
{
  jobject joNode;
  jobject joAddress;

  /* Java proxy object already existing ? */
  joNode= jni_proxy_get(jEnv, node);
  if (joNode != NULL)
    return joNode;

  /* Convert node attributes to Java objects */
  joAddress= cbgp_jni_new_IPAddress(jEnv, node->rid);

  /* Check that the conversion was successful */
  if (joAddress == NULL)
    return NULL;

  /* Create new Node object */
  if ((joNode= cbgp_jni_new(jEnv, CLASS_Node, CONSTR_Node,
			    joCBGP,
			    joAddress)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joNode, node);

  return joNode;
}

// -----[ getDomain ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getDomain
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/IGPDomain;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getDomain
  (JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;
  igp_domain_t * domain= NULL;
  jobject joDomain= NULL;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the domain */
  if ((node->domains != NULL) &&
      (uint16_array_size(node->domains) > 0)) {
    domain= network_find_igp_domain(node->network,
				    node->domains->data[0]);
    joDomain= cbgp_jni_new_net_IGPDomain(jEnv, NULL, domain);
  }

  jni_unlock(jEnv);
  return joDomain;
}

// -----[ getName ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getName
  (JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;
  jstring jsName= NULL;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the name */
  if (node->name != NULL)
    jsName= cbgp_jni_new_String(jEnv, node->name);

  jni_unlock(jEnv);
  return jsName;
}

// -----[ setName ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    setName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setName
  (JNIEnv * jEnv, jobject joNode, jstring jsName)
{
  net_node_t * node;
  const char * name;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  /* Set the name */
  if (jsName != NULL) {
    name= (*jEnv)->GetStringUTFChars(jEnv, jsName, NULL);
    node_set_name(node, name);
    (*jEnv)->ReleaseStringUTFChars(jEnv, jsName, name);
  } else
    node_set_name(node, NULL);

  jni_unlock(jEnv);
}

// -----[ recordRoute ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    recordRoute
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/IPTrace;
 */
JNIEXPORT jobjectArray JNICALL
Java_be_ac_ucl_ingi_cbgp_net_Node_recordRoute
(
 JNIEnv * env,
 jobject joNode,
 jstring jsDestination
)
{
  net_node_t * node;
  jobject joVector;
  jobject joTrace;
  ip_dest_t dest;
  ip_trace_t * trace;
  gds_enum_t * traces;
  ip_opt_t * opts;

  jni_lock(env);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(env, joNode);
  if (node == NULL)
    return_jni_unlock(env, NULL);

  /* Get the destination */
  if (ip_jstring_to_dest(env, jsDestination, &dest) != 0)
    return_jni_unlock(env, NULL);

  /* "Any" destination not allowed here */
  if (dest.type == NET_DEST_ANY) {
    throw_CBGPException(env, "invalid destination (*)");
    return_jni_unlock(env, NULL);
  }

  /* Trace the IP-level route */
  opts= ip_options_create();
  ip_options_set(opts, IP_OPT_CAPACITY);
  ip_options_set(opts, IP_OPT_DELAY);
  ip_options_set(opts, IP_OPT_WEIGHT);
  ip_options_set(opts, IP_OPT_TUNNEL);
  ip_options_set(opts, IP_OPT_ECMP);
  if (dest.type == NET_DEST_PREFIX)
    ip_options_alt_dest(opts, dest.prefix);

  traces= icmp_trace_send(node, dest.addr, 255, opts);

  joVector= cbgp_jni_new_Vector(env);
  if (joVector == NULL)
    return_jni_unlock(env, NULL);

  while (enum_has_next(traces)) {
    trace= *((ip_trace_t **) enum_get_next(traces));

    joTrace= cbgp_jni_new_IPTrace(env, trace);
    if (joTrace == NULL)
      return_jni_unlock(env, NULL);

    if (cbgp_jni_Vector_add(env, joVector, joTrace) < 0)
      return_jni_unlock(env, NULL);
    
    ip_trace_destroy(&trace);
  }
  enum_destroy(&traces);

  return_jni_unlock(env, joVector);
}

// -----[ traceRoute ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    traceRoute
 * Signature: (Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/IPTrace;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_traceRoute
  (JNIEnv * jEnv, jobject joNode, jstring jsDestination)
{
  net_node_t * node;
  jobject joIPTrace;
  ip_dest_t dest;
  ip_trace_t * trace;
  net_error_t error;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the destination */
  if (ip_jstring_to_address(jEnv, jsDestination, &dest.addr) != 0)
    return_jni_unlock(jEnv, NULL);
  dest.type= NET_DEST_ADDRESS;

  /* Trace the IP-level route */
  error= icmp_trace_route(NULL, node, NET_ADDR_ANY,
			  dest.addr, 0, &trace);

  /* Convert to an IPTrace object */
  joIPTrace= cbgp_jni_new_IPTrace(jEnv, trace);

  ip_trace_destroy(&trace);

  return_jni_unlock(jEnv, joIPTrace);
}

// -----[ _getAddresses ]--------------------------------------------
/**
 *
 */
static int _getAddresses(const void * item, const void * ctx)
{
  net_addr_t * addr= (net_addr_t *) item;
  jni_ctx_t * jni_ctx= (jni_ctx_t *) ctx;
  jobject joAddress;

  if ((joAddress= cbgp_jni_new_IPAddress(jni_ctx->jEnv, *addr)) == NULL)
    return -1;
  
  return cbgp_jni_Vector_add(jni_ctx->jEnv, jni_ctx->joVector, joAddress);
}

// -----[ getAddresses ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getAddresses
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getAddresses
  (JNIEnv * jEnv, jobject joNode)
{
  net_node_t * pNode;
  jobject joVector= NULL;
  jni_ctx_t sCtx;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;

  if (node_addresses_for_each(pNode, _getAddresses, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joVector);
}

// -----[ addLTLLink ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addLTLLink
 * Signature: (Lbe/ac/ucl/ingi/cbgp/net/Node;Z)Lbe/ac/ucl/ingi/cbgp/net/Link;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addLTLLink
  (JNIEnv * jEnv, jobject joNode, jobject joDst, jboolean jbBidir)
{
  net_node_t * pNode, *pNodeDst;
  net_iface_t * pIface;
  jobject joLink;
  int iResult;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the destination */
  pNodeDst= (net_node_t*) jni_proxy_lookup(jEnv, joDst);
  if (pNodeDst == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Add link */
  iResult= net_link_create_rtr(pNode, pNodeDst,
			       (jbBidir==JNI_TRUE) ? BIDIR : UNIDIR,
			       &pIface);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not create link (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  /* Retrieve new link */
  joLink= cbgp_jni_new_net_Interface(jEnv,
				NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
				pIface);

  return_jni_unlock(jEnv, joLink);
}

// -----[ addPTPLink ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addPTPLink
 * Signature: (Lbe/ac/ucl/ingi/cbgp/net/Node;Ljava/lang/String;Ljava/lang/String;Z)Lbe/ac/ucl/ingi/cbgp/net/Link;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addPTPLink
  (JNIEnv * jEnv, jobject joNode, jobject joDst, jstring jsSrcIface,
   jstring jsDstIface, jboolean jbBidir)
{
  net_node_t * pNode;
  net_node_t * pNodeDst;
  int iResult;
  net_iface_t * pIface;
  net_iface_id_t tSrcIfaceID;
  net_iface_id_t tDstIfaceID;
  jobject joIface;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the destination */
  pNodeDst= (net_node_t*) jni_proxy_lookup(jEnv, joDst);
  if (pNodeDst == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the source's interface ID */
  if (ip_jstring_to_prefix(jEnv, jsSrcIface, &tSrcIfaceID) != 0)
    return_jni_unlock(jEnv, NULL);

  /* Get the destination's interface ID */
  if (ip_jstring_to_prefix(jEnv, jsDstIface, &tDstIfaceID) != 0)
    return_jni_unlock(jEnv, NULL);

  /* Add link */
  iResult= net_link_create_ptp(pNode, tSrcIfaceID,
			       pNodeDst, tDstIfaceID,
			       (jbBidir==JNI_TRUE) ? BIDIR : UNIDIR,
			       &pIface);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not create link (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  /* Retrieve new link */
  joIface= cbgp_jni_new_net_Interface(jEnv,
				 NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
				 pIface);

  return_jni_unlock(jEnv, joIface);
}

// -----[ addPTMPLink ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addPTMPLink
 * Signature: (Lbe/ac/ucl/ingi/cbgp/net/Subnet;Ljava/lang/String;)Lbe/ac/ucl/ingi/cbgp/net/Link;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addPTMPLink
  (JNIEnv * jEnv, jobject joNode, jobject joDst, jstring jsIface)
{
  net_node_t * pNode;
  net_subnet_t * pSubnet;
  net_iface_t * pIface;
  jobject joIface;
  int iResult;
  net_addr_t tIfaceID;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the subnet */
  pSubnet= (net_subnet_t *) jni_proxy_lookup(jEnv, joDst);
  if (pSubnet == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Get the interface ID */
  if (ip_jstring_to_address(jEnv, jsIface, &tIfaceID) != 0)
    return_jni_unlock(jEnv, NULL);  

  /* Add link */
  iResult= net_link_create_ptmp(pNode, pSubnet, tIfaceID, &pIface);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not create link (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  /* Retrieve new link */
  joIface= cbgp_jni_new_net_Interface(jEnv,
				      NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
				      pIface);
  return_jni_unlock(jEnv, joIface);
}

// -----[ _cbgp_jni_get_link ]---------------------------------------
static int _cbgp_jni_get_link(const void * item, const void * ctx)
{
  jni_ctx_t * jni_ctx= (jni_ctx_t *) ctx;
  net_iface_t * iface= *((net_iface_t **) item);
  jobject joLink;

  if (!net_iface_is_connected(iface))
    return 0;

  if ((joLink= cbgp_jni_new_net_Interface(jni_ctx->jEnv,
					  jni_ctx->joCBGP,
					  iface)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(jni_ctx->jEnv, jni_ctx->joVector, joLink);
}

// -----[ getLinks ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getLinks
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getLinks
  (JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;
  jobject joVector;
  jni_ctx_t sCtx;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/;
  if (node_links_for_each(node, _cbgp_jni_get_link, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_iface ]--------------------------------------
static int _cbgp_jni_get_iface(const void * item, const void * ctx)
{
  jni_ctx_t * jni_ctx= (jni_ctx_t *) ctx;
  net_iface_t * iface= *((net_iface_t **) item);
  jobject joLink;


  if ((joLink= cbgp_jni_new_net_Interface(jni_ctx->jEnv,
					  jni_ctx->joCBGP,
					  iface)) == NULL)
    return -1;

  return cbgp_jni_Vector_add(jni_ctx->jEnv, jni_ctx->joVector, joLink);
}

// -----[ getInterfaces ]--------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getInterfaces
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getInterfaces
  (JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;
  jobject joVector;
  jni_ctx_t sCtx;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL); 

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.joVector= joVector;
  sCtx.jEnv= jEnv;
  sCtx.joCBGP= NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/;
  if (node_links_for_each(node, _cbgp_jni_get_iface, &sCtx) != 0)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, joVector);
}

// -----[ _cbgp_jni_get_rt_route ]------------------------------------
static int _cbgp_jni_get_rt_route(uint32_t key, uint8_t key_len,
				  void * pItem, void * pContext)
{
  jni_ctx_t * pCtx= (jni_ctx_t *) pContext;
  rt_info_t * rtinfo= (rt_info_t *) pItem;
  ip_pfx_t prefix;
  jobject joRoute;

  prefix.network= key;
  prefix.mask= key_len;

  if ((joRoute= cbgp_jni_new_IPRoute(pCtx->jEnv, prefix, rtinfo)) == NULL)
    return -1;
  return cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joRoute);
}

// -----[ getRT ]----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getRT
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getRT
  (JNIEnv * jEnv, jobject joNode, jstring jsDest)
{
  net_node_t * node;
  jobject joVector;
  jni_ctx_t sCtx;
  rt_info_t * rtinfo;
  ip_dest_t dest;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Convert the destination specifier (*|address|prefix) */
  if (jsDest != NULL) {
    if (ip_jstring_to_dest(jEnv, jsDest, &dest) < 0)
      return_jni_unlock(jEnv, NULL);
  } else
    dest.type= NET_ADDR_ANY;

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Prepare JNI context */
  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;

  switch (dest.type) {
  case NET_DEST_ANY:
    if (rt_for_each(node->rt, _cbgp_jni_get_rt_route, &sCtx) != 0)
      return_jni_unlock(jEnv, NULL);
    break;

  case NET_DEST_ADDRESS:
    rtinfo= rt_find_best(node->rt, dest.addr, NET_ROUTE_ANY);

    if (rtinfo != NULL)
      if (_cbgp_jni_get_rt_route(dest.addr, 32, rtinfo, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
    break;

  case NET_DEST_PREFIX:
    rtinfo= rt_find_exact(node->rt, dest.prefix, NET_ROUTE_ANY);
    if (rtinfo != NULL)
      if (_cbgp_jni_get_rt_route(dest.prefix.network,
				 dest.prefix.mask,
				 rtinfo, &sCtx) < 0)
	return_jni_unlock(jEnv, NULL);
    break;
    
  case NET_DEST_INVALID:
    throw_CBGPException(jEnv, "invalid destination");
    break;

  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ hasProtocol ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    hasProtocol
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_hasProtocol
  (JNIEnv * jEnv, jobject joNode, jstring jsProtocol)
{
  net_node_t * pNode;
  const char * pcProtocol;
  net_protocol_id_t tProtoID;
  net_error_t error;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, JNI_FALSE);

  /* Find protocol index */
  pcProtocol= (*jEnv)->GetStringUTFChars(jEnv, jsProtocol, NULL);
  error= net_str2protocol(pcProtocol, &tProtoID);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsProtocol, pcProtocol);

  /* Protocol name unknown */
  if (error != ESUCCESS)
    return_jni_unlock(jEnv, JNI_FALSE);

  /* Check if protocol is supported */
  if (node_get_protocol(pNode, tProtoID) != NULL)
    return_jni_unlock(jEnv, JNI_TRUE);

  return_jni_unlock(jEnv, JNI_FALSE);
}

// -----[ getProtocols ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getProtocols
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getProtocols
  (JNIEnv * jEnv, jobject joNode)
{
  net_node_t * pNode;
  jobject joVector= NULL;
  net_protocol_id_t tProtoID;
  jstring jsProtocol;

  jni_lock(jEnv);

  /* Get the node */
  pNode= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (pNode == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create Vector object */
  joVector= cbgp_jni_new_Vector(jEnv);
  if (joVector == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Populate with identifier of supported protocols */
  for (tProtoID= 0; tProtoID < NET_PROTOCOL_MAX; tProtoID++) {
    if (node_get_protocol(pNode, tProtoID)) {
      jsProtocol= cbgp_jni_new_String(jEnv, net_protocol2str(tProtoID));
      if (jsProtocol == NULL)
	return_jni_unlock(jEnv, NULL);
      cbgp_jni_Vector_add(jEnv, joVector, jsProtocol);
    }
  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ getBGP ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    getBGP
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/bgp/Router;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getBGP
  (JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;
  bgp_router_t * router;
  net_protocol_t * protocol;
  jobject joRouter;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Get the BGP handler */
  if ((protocol= node_get_protocol(node, NET_PROTOCOL_BGP)) == NULL)
    return_jni_unlock(jEnv, NULL);
  router= (bgp_router_t *) protocol->handler;

  /* Create bgp.Router instance */
  if ((joRouter= cbgp_jni_new_bgp_Router(jEnv,
					 NULL/*jni_proxy_get_CBGP(jEnv, joNode)*/,
					 router)) == NULL)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, joRouter);
}

// -----[ addRoute ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    addRoute
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_addRoute
  (JNIEnv * jEnv, jobject joNode, jstring jsPrefix,
   jstring jsNexthop, jint jiWeight)
{
  net_node_t * node;
  ip_pfx_t prefix;
  net_addr_t next_hop;
  net_iface_id_t tIfaceID;
  int error;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_prefix(jEnv, jsPrefix, &prefix) != 0)
    return_jni_unlock2(jEnv);

  if (ip_jstring_to_address(jEnv, jsNexthop, &next_hop) != 0)
    return_jni_unlock2(jEnv);

  tIfaceID.network= next_hop;
  tIfaceID.mask= 32;
  error= node_rt_add_route(node, prefix, tIfaceID,
			   next_hop, jiWeight, NET_ROUTE_STATIC);
  if (error != ESUCCESS) {
    throw_CBGPException(jEnv, "could not add route (%s)",
			network_strerror(error));
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}

// -----[ getLatitude ]----------------------------------------------
/**
 *
 */
JNIEXPORT jfloat JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getLatitude
(JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, 0);

  return_jni_unlock(jEnv, node->coord.latitude);
}

// -----[ setLatitude ]----------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setLatitude
(JNIEnv * jEnv, jobject joNode, jfloat jfLatitude)
{
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  node->coord.latitude= jfLatitude;

  jni_unlock(jEnv);
}

// -----[ getLongitude ]---------------------------------------------
/**
 *
 */
JNIEXPORT jfloat JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_getLongitude
(JNIEnv * jEnv, jobject joNode)
{
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock(jEnv, 0);

  return_jni_unlock(jEnv, node->coord.longitude);
}

// -----[ setLongitude ]---------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_setLongitude
(JNIEnv * jEnv, jobject joNode, jfloat jfLongitude)
{
  net_node_t * node;

  jni_lock(jEnv);

  /* Get the node */
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  node->coord.longitude= jfLongitude;

  jni_unlock(jEnv);
}

// -----[ loadTraffic ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_net_Node
 * Method:    loadTraffic
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_net_Node_loadTraffic
  (JNIEnv * jEnv, jobject joNode, jstring jsFileName)
{
  net_node_t * node;
  const char * filename;
  uint8_t options= 0;
  int result;

  jni_lock(jEnv);

  // Get the node
  node= (net_node_t*) jni_proxy_lookup(jEnv, joNode);
  if (node == NULL)
    return_jni_unlock2(jEnv);

  // Load Netflow from file
  filename= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);
  result= node_load_netflow(node, filename, options, NULL);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, filename);
  if (result != ESUCCESS) {
    throw_CBGPException(jEnv, "could not load Netflow");
    return_jni_unlock2(jEnv);
  }

  jni_unlock(jEnv);
}
