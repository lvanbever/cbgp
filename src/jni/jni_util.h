// ==================================================================
// @(#)jni_util.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 07/02/2005
// $Id: jni_util.h,v 1.17 2009-03-24 16:02:45 bqu Exp $
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

#ifndef __JNI_UTIL_H__
#define __JNI_UTIL_H__

#include <net/net_types.h>
#include <net/link.h>
#include <net/network.h>
#include <net/prefix.h>
#include <net/routing.h>
#include <bgp/types.h>
#include <bgp/attr/types.h>
#include <bgp/peer.h>
#include <bgp/route.h>

#include <jni.h>

// -----[ context-structure for building vectors of links/routes ]---
typedef struct {
  union {
    net_node_t   * node;
    bgp_router_t * router;
    jobject        joHashtable;
  };
  jobject   joVector;
  jobject   joCBGP;
  JNIEnv  * jEnv;
} jni_ctx_t;

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cbgp_jni_new_ConsoleEvent ]------------------------------
  jobject cbgp_jni_new_ConsoleEvent(JNIEnv * env, char * msg);
  // -----[ cbgp_jni_Communities_append ]----------------------------
  int cbgp_jni_Communities_append(JNIEnv * env, jobject joCommunities,
				  bgp_comm_t comm);
  // -----[ cbgp_jni_new_ASPath ]------------------------------------
  jobject cbgp_jni_new_ASPath(JNIEnv * env, bgp_path_t * path);
  // -----[ cbgp_jni_ASPath_append ]---------------------------------
  int cbgp_jni_ASPath_append(JNIEnv * env, jobject joASPath,
			     bgp_path_seg_t * segment);
  // -----[ cbgp_jni_new_ASPathSegment ]-----------------------------
  jobject cbgp_jni_new_ASPathSegment(JNIEnv * env,
				     bgp_path_seg_t * segment);
  // -----[ cbgp_jni_ASPathSegment_append ]--------------------------
  int cbgp_jni_ASPathSegment_append(JNIEnv * env, jobject joASPathSeg,
				    asn_t asn);
  // -----[ cbgp_jni_new_IPPrefix ]----------------------------------
  jobject cbgp_jni_new_IPPrefix(JNIEnv * env, ip_pfx_t pfx);
  // -----[ cbgp_jni_new_IPAddress ]---------------------------------
  jobject cbgp_jni_new_IPAddress(JNIEnv * env, net_addr_t addr);
  // -----[ cbgp_jni_new_IPRoute ]-----------------------------------
  jobject cbgp_jni_new_IPRoute(JNIEnv * env, ip_pfx_t pfx,
			       rt_info_t * route);
  // -----[ cbgp_jni_new_BGPRoute ]----------------------------------
  jobject cbgp_jni_new_BGPRoute(JNIEnv * jEnv, bgp_route_t * route,
				jobject joHashtable);
  // -----[ cbgp_jni_new_BGPMessage ]----------------------------------
  jobject cbgp_jni_new_BGPMessage(JNIEnv * jEnv, net_msg_t * msg);
  // -----[ ip_jstring_to_address ]----------------------------------
  int ip_jstring_to_address(JNIEnv * env, jstring jsAddr,
			    net_addr_t * addr_ref);
  // -----[ ip_jstring_to_prefix ]-----------------------------------
  int ip_jstring_to_prefix(JNIEnv * env, jstring jsPrefix,
			   ip_pfx_t * pfx_ref);
  // -----[ ip_jstring_to_dest ]-------------------------------------
  int ip_jstring_to_dest(JNIEnv * jEnv, jstring jsDest, ip_dest_t * dest);
  // -----[ cbgp_jni_net_node_from_string ]--------------------------
  net_node_t * cbgp_jni_net_node_from_string(JNIEnv * env, jstring jsAddr);
  // -----[ cbgp_jni_bgp_router_from_string ]------------------------
  bgp_router_t * cbgp_jni_bgp_router_from_string(JNIEnv * env, jstring jsAddr);
  // -----[ cbgp_jni_bgp_peer_from_string ]--------------------------
  bgp_peer_t * cbgp_jni_bgp_peer_from_string(JNIEnv * env,
					   jstring jsRouterAddr,
					   jstring jsPeerAddr);
  // -----[ cbgp_jni_net_domain_from_int ]---------------------------
  igp_domain_t * cbgp_jni_net_domain_from_int(JNIEnv * env, jint iNumber);
  // -----[ cbgp_jni_bgp_domain_from_int ]---------------------------
  bgp_domain_t * cbgp_jni_bgp_domain_from_int(JNIEnv * env, jint iNumber);

  // -----[ cbgp_jni_net_error_str ]---------------------------------
  jstring cbgp_jni_net_error_str(JNIEnv * jEnv, int error);

#ifdef __cplusplus
}
#endif

#endif /* __JNI_UTIL_H__ */
