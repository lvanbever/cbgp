// ==================================================================
// @(#)jni_util.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 07/02/2005
// $Id: jni_util.c,v 1.24 2009-03-24 16:02:45 bqu Exp $
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

#include <jni/exceptions.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>

#include <bgp/as.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/path.h>
#include <bgp/domain.h>
#include <bgp/peer.h>
#include <bgp/route.h>
#include <net/error.h>
#include <net/igp_domain.h>
#include <net/link.h>
#include <net/network.h>
#include <net/node.h>
#include <net/util.h>
#include <jni.h>

#define CLASS_AbstractConsoleEventListener \
  "be/ac/ucl/ingi/cbgp/AbstractConsoleEventListener"
#define METHOD_AbstractConsoleEventListener_eventFired \
  "(Ljava/lang/String;)V"

#define CLASS_ASPath "be/ac/ucl/ingi/cbgp/bgp/ASPath"
#define CONSTR_ASPath "()V"
#define METHOD_ASPath_append "(Lbe/ac/ucl/ingi/cbgp/bgp/ASPathSegment;)V"

#define CLASS_ASPathSegment "be/ac/ucl/ingi/cbgp/bgp/ASPathSegment"
#define CONSTR_ASPathSegment "(I)V"

#define CLASS_ConsoleEvent "be/ac/ucl/ingi/cbgp/ConsoleEvent"
#define CONSTR_ConsoleEvent "(Ljava/lang/String;)V"

#define CLASS_IPPrefix "be/ac/ucl/ingi/cbgp/IPPrefix"
#define CONSTR_IPPrefix "(BBBBB)V"

#define CLASS_IPAddress "be/ac/ucl/ingi/cbgp/IPAddress"
#define CONSTR_IPAddress "(BBBB)V"

#define CLASS_RouteEntry "be/ac/ucl/ingi/cbgp/RouteEntry"
#define CONSTR_RouteEntry "(Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
                           "Lbe/ac/ucl/ingi/cbgp/IPAddress;)V"

#define CLASS_IPRoute "be/ac/ucl/ingi/cbgp/IPRoute"
#define CONSTR_IPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                       "[Lbe/ac/ucl/ingi/cbgp/RouteEntry;B)V"

#define CLASS_BGPRoute "be/ac/ucl/ingi/cbgp/bgp/Route"
#define CONSTR_BGPRoute "(Lbe/ac/ucl/ingi/cbgp/IPPrefix;" \
                        "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
                        "JJZZB" \
                        "Lbe/ac/ucl/ingi/cbgp/bgp/ASPath;Z" \
                        "Lbe/ac/ucl/ingi/cbgp/bgp/Communities;)V"

#define CLASS_MessageUpdate "be/ac/ucl/ingi/cbgp/bgp/MessageUpdate"
#define CONSTR_MessageUpdate \
  "(Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/bgp/Route;)V"

#define CLASS_MessageWithdraw "be/ac/ucl/ingi/cbgp/bgp/MessageWithdraw"
#define CONSTR_MessageWithdraw \
  "(Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/IPAddress;" \
  "Lbe/ac/ucl/ingi/cbgp/IPPrefix;)V"

#define CLASS_Communities "be/ac/ucl/ingi/cbgp/bgp/Communities"
#define CONSTR_Communities "()V"
#define METHOD_Communities_append "(I)V"

// -----[ cbgp_jni_new_Communities ]---------------------------------
/**
 * Build a new Communities object from a C-BGP Communities.
 */
jobject cbgp_jni_new_Communities(JNIEnv * env, bgp_comms_t * comms)
{
  jobject joCommunities;
  unsigned int index;

  if (comms == NULL)
    return NULL;

  /* Build new Communities object */
  if ((joCommunities= cbgp_jni_new(env, CLASS_Communities,
				   CONSTR_Communities)) == NULL)
    return NULL;

  /* Append all communities */
  for (index= 0; index < comms->num; index++) {
    if (cbgp_jni_Communities_append(env, joCommunities,
				    comms->values[index]) != 0)
      return NULL;
  }

  return joCommunities;
}

// -----[ cbgp_jni_Communities_append ]------------------------------
/**
 * Append a community to a Communities object.
 */
int cbgp_jni_Communities_append(JNIEnv * env, jobject joCommunities,
				bgp_comm_t comm)
{
  return cbgp_jni_call_void(env, joCommunities,
			    "append", METHOD_Communities_append,
			    (jint) comm);
}

// -----[ cbgp_jni_new_ASPath ]--------------------------------------
/**
 * Build a new ASPath object from a C-BGP AS-Path.
 */
jobject cbgp_jni_new_ASPath(JNIEnv * env, bgp_path_t * path)
{
  jobject joASPath;
  unsigned int index;
  bgp_path_seg_t* seg;

  /* Build new ASPath object */
  if ((joASPath= cbgp_jni_new(env, CLASS_ASPath, CONSTR_ASPath)) == NULL)
    return NULL;

  /* Append all AS-Path segments */
  for (index= 0; index < path_num_segments(path); index++) {
    seg= (bgp_path_seg_t*) path->data[index];
    if (cbgp_jni_ASPath_append(env, joASPath, seg) != 0)
      return NULL;
  }

  return joASPath;
}

// -----[ cbgp_jni_ASPath_append ]-----------------------------------
/**
 * Append an AS-Path segment to an ASPath object.
 */
int cbgp_jni_ASPath_append(JNIEnv * env, jobject joASPath,
			   bgp_path_seg_t* seg)
{
  jobject joASPathSeg;

  // Convert the AS-Path segment to an java object.
  if ((joASPathSeg= cbgp_jni_new_ASPathSegment(env, seg)) == NULL)
    return -1;

  return cbgp_jni_call_void(env, joASPath,
			    "append", METHOD_ASPath_append,
			    joASPathSeg);
}

// -----[ cbgp_jni_new_ASPathSegment ]-------------------------------
jobject cbgp_jni_new_ASPathSegment(JNIEnv * env, bgp_path_seg_t * seg)
{
  jobject joASPathSeg;
  unsigned int index;

  if ((joASPathSeg= cbgp_jni_new(env, CLASS_ASPathSegment,
				 CONSTR_ASPathSegment,
				 (jint) seg->type)) == NULL)
    return NULL;

  // Append all AS-Path segment elements
  for (index= 0; index < seg->length; index++) {
    if (cbgp_jni_ASPathSegment_append(env, joASPathSeg, seg->asns[index]) != 0)
      return NULL;
  }
  return joASPathSeg;
}

// -----[ cbgp_jni_ASPathSegment_append ]----------------------------
/**
 *
 */
int cbgp_jni_ASPathSegment_append(JNIEnv * env, jobject joASPathSeg,
				  asn_t asn)
{
  jclass jcASPathSeg;
  jmethodID jmASPathSeg;

  if ((jcASPathSeg= (*env)->GetObjectClass(env, joASPathSeg)) == NULL)
    return -1;
  if ((jmASPathSeg= (*env)->GetMethodID(env, jcASPathSeg, "append",
					"(I)V")) == NULL)
    return -1;

  (*env)->CallVoidMethod(env, joASPathSeg, jmASPathSeg, (jint) asn);

  return 0;
}

// -----[ cbgp_jni_new_ConsoleEvent ]--------------------------------
/**
 * Build a new ConsoleEvent object.
 */
jobject cbgp_jni_new_ConsoleEvent(JNIEnv * jEnv, char * pcMessage)
{
  jobject joConsoleEvent= NULL;
  jstring jsMessage;

  /* Build a Java String */
  jsMessage= cbgp_jni_new_String(jEnv, pcMessage);

  // Build new ConsoleEvent object.
  if ((joConsoleEvent= cbgp_jni_new(jEnv, CLASS_ConsoleEvent,
				    CONSTR_ConsoleEvent,
				    jsMessage)) == NULL)
    return NULL;

  return joConsoleEvent;
}

// -----[ cbgp_jni_new_IPPrefix ]------------------------------------
/**
 * This function creates a new instance of the Java IPPrefix object
 * from a CBGP prefix.
 */
jobject cbgp_jni_new_IPPrefix(JNIEnv * env, ip_pfx_t prefix)
{
  jclass class_IPPrefix;
  jmethodID id_IPPrefix;
  jobject obj_IPPrefix;

  if ((class_IPPrefix= (*env)->FindClass(env, CLASS_IPPrefix)) == NULL)
    return NULL;

  if ((id_IPPrefix= (*env)->GetMethodID(env, class_IPPrefix, "<init>", CONSTR_IPPrefix)) == NULL)
    return NULL;

  if ((obj_IPPrefix= (*env)->NewObject(env, class_IPPrefix, id_IPPrefix,
				       (jbyte) ((prefix.network >> 24) & 255),
				       (jbyte) ((prefix.network >> 16) & 255),
				       (jbyte) ((prefix.network >> 8) & 255),
				       (jbyte) (prefix.network & 255),
				       (jbyte) prefix.mask)) == NULL)
    return NULL;

  return obj_IPPrefix;
}

// -----[ cbgp_jni_new_IPAddress ]-----------------------------------
/**
 * This function creates a new instance of the Java IPAddress object
 * from a CBGP address.
 */
jobject cbgp_jni_new_IPAddress(JNIEnv * env, net_addr_t addr)
{
  jclass class_IPAddress;
  jmethodID id_IPAddress;
  jobject obj_IPAddress;

  if ((class_IPAddress= (*env)->FindClass(env, CLASS_IPAddress)) == NULL)
    return NULL;

  if ((id_IPAddress= (*env)->GetMethodID(env, class_IPAddress, "<init>",
				   CONSTR_IPAddress)) == NULL)
    return NULL;

  if ((obj_IPAddress= (*env)->NewObject(env, class_IPAddress, id_IPAddress,
					(jbyte) ((addr >> 24) & 255),
					(jbyte) ((addr >> 16) & 255),
					(jbyte) ((addr >> 8) & 255),
					(jbyte) (addr & 255))) == NULL)
    return NULL;

  return obj_IPAddress;
}

// -----[ cbgp_jni_new_RouteEntry ]----------------------------------
jobject cbgp_jni_new_RouteEntry(JNIEnv * jEnv, rt_entry_t * entry)
{
  jobject obj_oif;
  jobject obj_gateway;
  net_iface_id_t oif_id;

  /* Create Java oif */
  if (entry->oif != NULL) {
    oif_id= net_iface_id(entry->oif);
    obj_oif= cbgp_jni_new_IPAddress(jEnv, oif_id.network);
    if (obj_oif == NULL)
      return NULL;
  } else {
    obj_oif= NULL;
  }

  /* Create Java gateway */
  obj_gateway= cbgp_jni_new_IPAddress(jEnv, entry->gateway);
  if (obj_gateway == NULL)
    return NULL;

  /* Create Java entry */
  return cbgp_jni_new(jEnv, CLASS_RouteEntry, CONSTR_RouteEntry,
		      obj_oif, obj_gateway);
}

// -----[ cbgp_jni_new_RouteEntries ]--------------------------------
jobjectArray cbgp_jni_new_RouteEntries(JNIEnv * jEnv, rt_entries_t * entries)
{
  jobjectArray obj_entries;
  jobject obj_entry;
  unsigned int index;
  rt_entry_t * rtentry;
  unsigned int size= rt_entries_size(entries);

  obj_entries= jni_new_ObjectArray(jEnv, size, CLASS_RouteEntry);
  if (obj_entries == NULL)
    return NULL;

  for (index= 0; index < size; index++) {
    rtentry= rt_entries_get_at(entries, index);
    obj_entry= cbgp_jni_new_RouteEntry(jEnv, rtentry);
    if (obj_entry == NULL)
	return NULL;
    (*jEnv)->SetObjectArrayElement(jEnv, obj_entries, index, obj_entry);
    if ((*jEnv)->ExceptionOccurred(jEnv))
      return NULL;
  }
  return obj_entries;
}

// -----[ cbgp_jni_new_IPRoute ]-------------------------------------
/**
 * This function creates a new instance of the IPRoute object from a
 * CBGP route.
 */
jobject cbgp_jni_new_IPRoute(JNIEnv * jEnv, ip_pfx_t prefix,
			     rt_info_t * rtinfo)
{
  jclass class_IPRoute;
  jmethodID id_IPRoute;
  jobject joRoute;
  jobject joPrefix;
  jobjectArray obj_entries;

  /* Convert route attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, prefix);
  if (joPrefix == NULL)
    return NULL;

  /* Build array of outgoing interfaces and gateways (entries) */
  obj_entries= cbgp_jni_new_RouteEntries(jEnv, rtinfo->entries);

  /* Create new IPRoute object */
  if ((class_IPRoute= (*jEnv)->FindClass(jEnv, CLASS_IPRoute)) == NULL)
    return NULL;

  if ((id_IPRoute= (*jEnv)->GetMethodID(jEnv, class_IPRoute, "<init>",
				   CONSTR_IPRoute)) == NULL)
    return NULL;

  if ((joRoute= (*jEnv)->NewObject(jEnv, class_IPRoute, id_IPRoute,
				   joPrefix, obj_entries,
				   rtinfo->type)) == NULL)
    return NULL;

  return joRoute;
}

// -----[ cbgp_jni_new_LinkMetrics ]---------------------------------
jobject cbgp_jni_new_LinkMetrics(JNIEnv * jEnv,
				 net_link_delay_t tDelay,
				 net_link_delay_t tWeight)
{
  return NULL;
}

// -----[ cbgp_jni_new_BGPRoute ]------------------------------------
/**
 * This function creates a new instance of the BGPRoute object from a
 * CBGP route.
 */
jobject cbgp_jni_new_BGPRoute(JNIEnv * jEnv, bgp_route_t * route,
			      jobject joHashtable)
{
  jobject joPrefix;
  jobject joNextHop;
  jobject joASPath= NULL;
  jobject joCommunities= NULL;
  jobject joKey;
  jobject joRoute;

  /* Convert route attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, route->prefix);
  joNextHop= cbgp_jni_new_IPAddress(jEnv, route_get_nexthop(route));

  /* Check that the conversion was successful */
  if ((joPrefix == NULL) || (joNextHop == NULL))
    return NULL;

  /* Convert AS-Path or retrieve from hashtable */
  if (route->attr->path_ref != NULL) {
    if (joHashtable != NULL) {
#ifdef __LP64__
      joKey= cbgp_jni_new_Long(jEnv, (jlong) route->attr->path_ref);
#else
      joKey= cbgp_jni_new_Integer(jEnv, (jint) route->attr->path_ref);
#endif
      joASPath= cbgp_jni_Hashtable_get(jEnv, joHashtable, joKey);
      if (joASPath == NULL) {
	joASPath= cbgp_jni_new_ASPath(jEnv, route->attr->path_ref);
	if (joASPath == NULL)
	  return NULL;
	cbgp_jni_Hashtable_put(jEnv, joHashtable, joKey, joASPath);
      }
    } else {
      joASPath= cbgp_jni_new_ASPath(jEnv, route->attr->path_ref);
      if (joASPath == NULL)
	return NULL;
    }
  }

  /* Convert Communities or retrieve from hashtable */
  if (route->attr->comms != NULL) {
    if (joHashtable != NULL) {
#ifdef __LP64__
      joKey= cbgp_jni_new_Long(jEnv, (jlong) route->attr->comms);
#else
      joKey= cbgp_jni_new_Integer(jEnv, (jint) route->attr->comms);
#endif
      joCommunities= cbgp_jni_Hashtable_get(jEnv, joHashtable, joKey);
      if (joCommunities == NULL) {
	joCommunities=
	  cbgp_jni_new_Communities(jEnv, route->attr->comms);
	if (joCommunities == NULL)
	  return NULL;
	cbgp_jni_Hashtable_put(jEnv, joHashtable, joKey, joCommunities);
      }
    } else {
      joCommunities=
	cbgp_jni_new_Communities(jEnv, route->attr->comms);
      if (joCommunities == NULL)
	return NULL;
    }
  }

  /* Create new BGPRoute object */
  joRoute= cbgp_jni_new(jEnv, CLASS_BGPRoute, CONSTR_BGPRoute,
			joPrefix,
			joNextHop,
			(jlong) route_localpref_get(route),
			(jlong) route_med_get(route),
			(route_flag_get(route, ROUTE_FLAG_BEST))?JNI_TRUE:JNI_FALSE,
			(route_flag_get(route, ROUTE_FLAG_FEASIBLE))?JNI_TRUE:JNI_FALSE,
			route_get_origin(route),
			joASPath,
			(route_flag_get(route, ROUTE_FLAG_INTERNAL))?JNI_TRUE:JNI_FALSE,
			joCommunities);

  if (route_flag_get(route, ROUTE_FLAG_BEST)) {
    if (cbgp_jni_call_void(jEnv, joRoute, "setRank", "(I)V", route->rank) < 0)
      return NULL;
  }

  return joRoute;
}

// -----[ _new_BGPUpdateMsg ]----------------------------------------
static inline jobject _new_BGPUpdateMsg(JNIEnv * jEnv,
					jobject from,
					jobject to,
					bgp_msg_update_t * msg)
{
  jobject joRoute;

  if ((joRoute= cbgp_jni_new_BGPRoute(jEnv, msg->route, NULL)) == NULL)
      return NULL;
  return cbgp_jni_new(jEnv, CLASS_MessageUpdate,
		      CONSTR_MessageUpdate,
		      from, to, joRoute);
}

// -----[ _new_BGP>ithdrawMsg ]--------------------------------------
static inline jobject _new_BGPWithdrawMsg(JNIEnv * jEnv,
					  jobject from,
					  jobject to,
					  bgp_msg_withdraw_t * msg)
{
  jobject joPrefix;

  if ((joPrefix= cbgp_jni_new_IPPrefix(jEnv, msg->prefix)) == NULL)
    return NULL;
  return  cbgp_jni_new(jEnv, CLASS_MessageWithdraw,
		       CONSTR_MessageWithdraw,
		       from, to, joPrefix);
}

// -----[ cbgp_jni_new_BGPMessage ]----------------------------------
/**
 * This function creates a new instance of the bgp.Message object
 * from a CBGP message.
 */
jobject cbgp_jni_new_BGPMessage(JNIEnv * jEnv, net_msg_t * msg)
{
  jobject from= cbgp_jni_new_IPAddress(jEnv, msg->src_addr);
  jobject to= cbgp_jni_new_IPAddress(jEnv, msg->dst_addr);
  bgp_msg_t * bgp_msg= (bgp_msg_t *) msg->payload;
  
  /* Check that the conversion of addresses was successful */
  if ((from == NULL) || (to == NULL))
    return NULL;

  /* Create BGP message instance */
  switch (bgp_msg->type) {
  case BGP_MSG_TYPE_UPDATE:
    return _new_BGPUpdateMsg(jEnv, from, to,
			     (bgp_msg_update_t *) bgp_msg);
  case BGP_MSG_TYPE_WITHDRAW:
    return _new_BGPWithdrawMsg(jEnv, from, to,
			       (bgp_msg_withdraw_t *) bgp_msg);
  default:
    return NULL;
  }
}

// -----[ ip_jstring_to_address ]------------------------------------
/**
 * This function returns a CBGP address from a Java string.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
int ip_jstring_to_address(JNIEnv * env, jstring jsAddr,
			  net_addr_t * addr_ref)
{
  const char * addr_str;
  int result= 0;

  if (jsAddr == NULL)
    return -1;

  addr_str= (*env)->GetStringUTFChars(env, jsAddr, NULL);

  if ((str2address(addr_str, addr_ref) != 0))
    result= -1;
  (*env)->ReleaseStringUTFChars(env, jsAddr, addr_str);

  // If the conversion could not be performed, throw an Exception
  if (result != 0)
    throw_CBGPException(env, "Invalid IP address");

  return result;
}

// -----[ ip_jstring_to_prefix ]-------------------------------------
/**
 * This function returns a CBGP prefix from a Java string.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
int ip_jstring_to_prefix(JNIEnv * env, jstring jsPrefix,
			 ip_pfx_t * pfx_ref)
{
  const char * pfx_str;
  int result= 0;

  if (jsPrefix == NULL)
    return -1;

  pfx_str= (*env)->GetStringUTFChars(env, jsPrefix, NULL);
  if (str2prefix(pfx_str, pfx_ref)) 
    result= -1;
  (*env)->ReleaseStringUTFChars(env, jsPrefix, pfx_str);

  // If the conversion could not be performed, throw an Exception
  if (result != 0)
    throw_CBGPException(env, "Invalid IP prefix");

  return result;
}

// -----[ ip_jstring_to_dest ]---------------------------------------
/**
 * Convert the destination to a SNetDest structure.
 */
int ip_jstring_to_dest(JNIEnv * jEnv, jstring jsDest, ip_dest_t * dest)
{
  const char * cDest;
  int result= 0;

  if (jsDest == NULL)
    return -1;

  cDest= (*jEnv)->GetStringUTFChars(jEnv, jsDest, NULL);
  if (ip_string_to_dest((char *)cDest, dest) != 0) 
    result= -1;

  (*jEnv)->ReleaseStringUTFChars(jEnv, jsDest, cDest);

  /* If the conversion could not be performed, throw an Exception */
  if (result != 0)
    throw_InvalidDestination(jEnv, jsDest);

  return result;
}


// -----[ cbgp_jni_net_node_from_string ]----------------------------
/**
 * Get the C-BGP node identified by the String that contains its IP
 * address.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
net_node_t * cbgp_jni_net_node_from_string(JNIEnv * env, jstring jsAddr)
{
  net_node_t * pNode;
  net_addr_t tNetAddr;

  if (ip_jstring_to_address(env, jsAddr, &tNetAddr) != 0)
    return NULL;

  if ((pNode= network_find_node(network_get_default(), tNetAddr)) == NULL) {
    throw_CBGPException(env, "could not find node");
    return NULL;
  }
    
  return pNode;
}

// -----[ cbgp_jni_bgp_router_from_string ]--------------------------
/**
 * Get the C-BGP router identified by the String that contains its
 * IP address.
 *
 * @return NULL is the router was not found
 *
 * @throw CBGPException
 */
bgp_router_t * cbgp_jni_bgp_router_from_string(JNIEnv * env, jstring jsAddr)
{
  net_node_t * node;
  net_protocol_t * protocol;

  if ((node= cbgp_jni_net_node_from_string(env, jsAddr)) == NULL)
    return NULL;

  if (((protocol= protocols_get(node->protocols, NET_PROTOCOL_BGP)) == NULL) ||
      (protocol->handler == NULL)) {
    throw_CBGPException(env, "node does not support BGP");
    return NULL;
  }

  return protocol->handler;
}

// -----[ cbgp_jni_bgp_peer_from_string ]----------------------------
/**
 * Get the BGP peer identified by the Strings that contain the router
 * and peer's IP addresses.
 *
 * @return NULL if the peer was not found
 *
 * @throw CBGPException
 */
bgp_peer_t * cbgp_jni_bgp_peer_from_string(JNIEnv * env, jstring jsRouterAddr,
					 jstring jsPeerAddr)
{
  bgp_router_t * router;
  net_addr_t tPeerAddr;
  bgp_peer_t * pPeer;

  if ((router= cbgp_jni_bgp_router_from_string(env, jsRouterAddr)) == NULL)
    return NULL;
  if (ip_jstring_to_address(env, jsPeerAddr, &tPeerAddr) != 0)
    return NULL;

  pPeer= bgp_router_find_peer(router, tPeerAddr);
  if (pPeer == NULL) {
    throw_CBGPException(env, "peer not found");
    return NULL;
  }

  return pPeer;
}

// -----[ cbgp_jni_net_domain_from_int ]--------------------------
/**
 * Get the C-BGP domain identified by the String that contains its ID.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
igp_domain_t * cbgp_jni_net_domain_from_int(JNIEnv * env, jint id)
{
  igp_domain_t * domain;

  // Check that domain ID is in the range [0, 65535]
  if ((id < 0) || (id > 65535)) {
    throw_CBGPException(env, "invalid domain id (%d)", id);
    return NULL;
  }

  // Check that the domain exists
  domain= network_find_igp_domain(network_get_default(), (uint16_t) id);
  if (domain == NULL) {
    throw_CBGPException(env, "could not find domain (%d)", id);
    return NULL;
  }

  return domain;
}

// -----[ cbgp_jni_bgp_domain_from_int ]--------------------------
/**
 * Get the C-BGP domain identified by the String that contains its ID.
 *
 * @return 0 if the conversion was ok, -1 otherwise.
 *
 * @throw CBGPException
 */
bgp_domain_t * cbgp_jni_bgp_domain_from_int(JNIEnv * env, jint iNumber)
{
  // Check that domain ID is in the range [0, 65535]
  if ((iNumber < 0) || (iNumber > 65535)) {
    throw_CBGPException(env, "value out of bound");
    return NULL;
  }

  // Check that the domain exists
  if (!exists_bgp_domain((uint16_t) iNumber)) {
    throw_CBGPException(env, "could not find domain");
    return NULL;
  }

  return get_bgp_domain((uint16_t) iNumber);
}

// -----[ cbgp_jni_net_error_str ]-----------------------------------
jstring cbgp_jni_net_error_str(JNIEnv * jEnv, int iErrorCode)
{
  return cbgp_jni_new_String(jEnv, network_strerror(iErrorCode));
}


