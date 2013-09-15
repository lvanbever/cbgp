// ==================================================================
// @(#)jni_interface.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 27/10/2004
// $Id: jni_interface.c,v 1.53 2009-04-02 19:16:15 bqu Exp $
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
// TODO :
//   cannot be used with Walton [ to be fixed by STA ]
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libgds/cli_ctx.h>
#include <libgds/gds.h>
#include <libgds/stream.h>
#include <libgds/str_util.h>

/*** jni_md.h ***
 * Include machine-dependent typedefs (is this portable ?)
 * This header file is located in linux/jni_md.h under Linux while it
 * is located in solaris/jni_md.h under Solaris. I haven't found a
 * specification for this. So we might get portability problems at a
 * point...
 */
#include <jni_md.h>

#include <jni.h>
#include <string.h>
#include <assert.h>

#include <jni/exceptions.h>
#include <jni/headers/be_ac_ucl_ingi_cbgp_CBGP.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <jni/listener.h>
#include <jni/impl/bgp_Domain.h>
#include <jni/impl/bgp_Peer.h>
#include <jni/impl/bgp_Router.h>
#include <jni/impl/net_IGPDomain.h>
#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/net_Subnet.h>

#include <net/error.h>
#include <net/igp.h>
#include <net/link-list.h>
#include <net/network.h>
#include <net/node.h>
#include <net/prefix.h>
#include <net/protocol.h>
#include <net/routing.h>
#include <net/subnet.h>

#include <bgp/types.h>
#include <bgp/as.h>
#include <bgp/domain.h>
#include <bgp/dp_rules.h>
#include <bgp/mrtd.h>
#include <bgp/peer.h>
#include <bgp/route_map.h>

#include <sim/simulator.h>

#include <cli/common.h>
#include <api.h>

// -----[ CBGP listeners ]-------------------------------------------
static jni_listener_t _out_listener;
static jni_listener_t _err_listener;
static jni_listener_t _bgp_msg_listener;

static jobject joGlobalCBGP= NULL;

static gds_stream_t * _saved_err= NULL;
static gds_stream_t * _saved_out= NULL;


/////////////////////////////////////////////////////////////////////
// C-BGP Java Native Interface Initialization/finalization
/////////////////////////////////////////////////////////////////////

// -----[ init ]-----------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    init
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_init
  (JNIEnv * jEnv, jobject joCBGP, jstring file_log)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  // Test if a C-BGP JNI session has already been created.
  if (joGlobalCBGP != NULL) {
    throw_CBGPException(jEnv, "CBGP class already initialized");
    return_jni_unlock2(jEnv);
  }

  // Create global reference
  joGlobalCBGP= (*jEnv)->NewGlobalRef(jEnv, joCBGP);

  // Initialize C-BGP library (but not the GDS library)
  libcbgp_init2();
  _jni_proxies_init();

  // Initialize console listeners contexts
  jni_listener_init(&_out_listener);
  jni_listener_init(&_err_listener);
  jni_listener_init(&_bgp_msg_listener);

  jni_unlock(jEnv);
}

// -----[ destroy ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    destroy
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_destroy
  (JNIEnv * jEnv, jobject joCBGP)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  // Test if a C-BGP JNI session has already been created.
  if (joGlobalCBGP == NULL) {
    throw_CBGPException(jEnv, "CBGP class not initialized");
    return_jni_unlock2(jEnv);
  }

  // Delete global references to console listeners
  bgp_router_set_msg_listener(NULL, NULL);
  stream_destroy(&gdsout);
  stream_destroy(&gdserr);
  gdsout= stream_create(stdout);
  gdserr= stream_create(stderr);
  jni_listener_unset(&_out_listener, jEnv);
  jni_listener_unset(&_err_listener, jEnv);
  jni_listener_unset(&_bgp_msg_listener, jEnv);

  // Invalidates all the C->Java mappings
  _jni_proxies_invalidate(jEnv);

  // Free simulator data structures (but do not terminate GDS).
  libcbgp_done2();

  // Remove global reference
  (*jEnv)->DeleteGlobalRef(jEnv, joGlobalCBGP);
  joGlobalCBGP= NULL;

  jni_unlock(jEnv);
}

// -----[ get_Global_CBGP ]------------------------------------------
jobject get_Global_CBGP()
{
  return joGlobalCBGP;
}

// -----[ _console_listener ]----------------------------------------
static int _console_listener(void * ctx, char * pcBuffer)
{
  jni_listener_t * listener= (jni_listener_t *) ctx;
  JNIEnv * env= NULL;
  jobject joEvent= NULL;
  
  jni_listener_get_env(listener, &env);

  // Create new ConsoleEvent object.
  if ((joEvent= cbgp_jni_new_ConsoleEvent(env, pcBuffer)) == NULL)
    return -1;

  // Call the listener "eventFired" method.
  cbgp_jni_call_void(env, listener->joListener,
		     "eventFired",
		     "(Lbe/ac/ucl/ingi/cbgp/ConsoleEvent;)V",
		     joEvent);
  return strlen(pcBuffer);
}

// -----[ consoleSetOutListener ]------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    consoleSetOutListener
 * Signature: (Lbe/ac/ucl/ingi/cbgp/ConsoleEventListener;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_consoleSetOutListener
  (JNIEnv * jEnv, jobject joCBGP, jobject joListener)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  if (joListener != NULL) {
    _saved_out= gdsout;
    jni_listener_set(&_out_listener, jEnv, joListener);
    gdsout= stream_create_callback(_console_listener, &_out_listener);
  } else {
    jni_listener_unset(&_out_listener, jEnv);
    gdsout= _saved_out;
  }

  jni_unlock(jEnv);
}

// -----[ consoleSetErrListener ]------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    consoleSetErrListener
 * Signature: (Lbe/ac/ucl/ingi/cbgp/ConsoleEventListener;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_consoleSetErrListener
  (JNIEnv * jEnv, jobject joCBGP, jobject joListener)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);
  
  if (joListener != NULL) {
    _saved_err= gdserr;
    jni_listener_set(&_err_listener, jEnv, joListener);
    gdserr= stream_create_callback(_console_listener, &_err_listener);
  } else {
    jni_listener_unset(&_err_listener, jEnv);
    gdserr= _saved_err;
  }

  jni_unlock(jEnv);
}

// -----[ consoleSetLevel ]------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    consoleSetLevel
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_consoleSetLevel
  (JNIEnv * jEnv, jobject joCBGP, jint iLevel)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);
  
  if ((iLevel < 0) || (iLevel > 255)) {
    throw_CBGPException(jEnv, "invalid log-level");
    return_jni_unlock2(jEnv);
  }

  libcbgp_set_debug_level(iLevel);
  //log_set_level(pMainLog, (uint8_t) iLevel);

  jni_unlock(jEnv);
}


/////////////////////////////////////////////////////////////////////
// IGP Domains Management
/////////////////////////////////////////////////////////////////////

// -----[ netAddDomain ]---------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netAddDomain
 * Signature: (I)Lbe/ac/ucl/ingi/cbgp/net/IGPDomain;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddDomain
(JNIEnv * jEnv, jobject joCBGP, jint jiDomain)
{
  igp_domain_t * domain= NULL;
  jobject joDomain;
  net_error_t result;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  if ((jiDomain < 0) || (jiDomain > 65535)) {
    throw_CBGPException(jEnv, "invalid domain number %i", jiDomain);
    return_jni_unlock(jEnv, NULL);
  }

  domain= igp_domain_create((uint16_t) jiDomain, IGP_DOMAIN_IGP);
  result= network_add_igp_domain(network_get_default(), domain);
  if (result != ESUCCESS) {
    igp_domain_destroy(&domain);
    throw_CBGPException(jEnv, "could not create domain (%s)",
			network_strerror(result));
    return_jni_unlock(jEnv, NULL);
  }

  joDomain= cbgp_jni_new_net_IGPDomain(jEnv, joCBGP, domain);

  return_jni_unlock(jEnv, joDomain);
}

// -----[ netGetDomain ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetDomain
 * Signature: (I)Lbe/ac/ucl/ingi/cbgp/net/IGPDomain;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetDomain
  (JNIEnv * jEnv, jobject joCBGP, jint jiDomain)
{
  igp_domain_t * domain;
  jobject joDomain;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  if ((jiDomain < 0) || (jiDomain > 65535)) {
    throw_CBGPException(jEnv, "invalid domain number %i", jiDomain);
    return_jni_unlock(jEnv, NULL);
  }

  domain= network_find_igp_domain(network_get_default(),
				  (uint16_t) jiDomain);
  if (domain == NULL)
    return NULL;

  joDomain= cbgp_jni_new_net_IGPDomain(jEnv, joCBGP, domain);
  return_jni_unlock(jEnv, joDomain);
}

// -----[ _netGetDomains ]-------------------------------------------
static int _netGetDomains(igp_domain_t * domain, void * ctx)
{
  jni_ctx_t * pCtx= (jni_ctx_t *) ctx;
  jobject joDomain;

  // Create IGPDomain instance
  joDomain= cbgp_jni_new_net_IGPDomain(pCtx->jEnv,
				       pCtx->joCBGP,
				       domain);
  if (joDomain == NULL)
    return -1;

  // Add to Vector
  if (cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joDomain) != 0)
    return -1;
  
  return 0;
}

// -----[ netGetDomains ]--------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetDomains
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetDomains
  (JNIEnv * jEnv, jobject joCBGP)
{
  jni_ctx_t sCtx;
  jobject joVector;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joVector= joVector;
  sCtx.joCBGP= joCBGP;

  if (igp_domains_for_each(network_get_default()->domains,
			   _netGetDomains, &sCtx) != 0) {
    throw_CBGPException(jEnv, "could not get list of domains");
    return_jni_unlock(jEnv, NULL);
  }

  return_jni_unlock(jEnv, joVector);
}

// -----[ netAddSubnet ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netAddSubnet
 * Signature: (Ljava/lang/String;I)Lbe/ac/ucl/ingi/cbgp/net/Subnet;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddSubnet
  (JNIEnv * jEnv, jobject joCBGP, jstring jsPrefix, jint jiType)
{
  net_subnet_t * subnet;
  jobject joSubnet= NULL;
  ip_pfx_t prefix;
  int result;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  // Convert prefix
  if (ip_jstring_to_prefix(jEnv, jsPrefix, &prefix) != 0)
    return_jni_unlock(jEnv, NULL);

  // Create new subnet
  subnet= subnet_create(prefix.network, prefix.mask, 0);
  if (subnet == NULL) {
    throw_CBGPException(jEnv, "subnet cound notbe created");
    return_jni_unlock(jEnv, NULL);
  }

  // Add subnet to network
  result= network_add_subnet(network_get_default(), subnet);
  if (result != ESUCCESS) {
    subnet_destroy(&subnet);
    throw_CBGPException(jEnv, "subnet already exists");
    return_jni_unlock(jEnv, NULL);
  }

  // Create Java Subnet object
  joSubnet= cbgp_jni_new_net_Subnet(jEnv, joCBGP, subnet);

  return_jni_unlock(jEnv, joSubnet);
}

//////////////////////////////////////////////////////////////////////
//
// Nodes management
//
//////////////////////////////////////////////////////////////////////

// -----[ netAddNode ]-----------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netAddNode
 * Signature: (I)Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddNode
(JNIEnv * jEnv, jobject joCBGP, jstring jsAddr, jint iDomain)
{
  igp_domain_t * domain= NULL;
  net_node_t * node; 
  net_addr_t addr;
  jobject joNode;
  net_error_t error;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  // Check that the domain is valid
  if (iDomain >= 0) {
    domain= cbgp_jni_net_domain_from_int(jEnv, iDomain);
    if (domain == NULL)
      return_jni_unlock(jEnv, NULL);
  }

  if (ip_jstring_to_address(jEnv, jsAddr, &addr) != 0)
    return_jni_unlock(jEnv, NULL);

  error= node_create(addr, &node, NODE_OPTIONS_LOOPBACK);
  if (error != ESUCCESS) {
    throw_CBGPException(jEnv, "node could not be created (%s)",
			network_strerror(error));
    return_jni_unlock(jEnv, NULL);
  }

  error= network_add_node(network_get_default(), node);
  if (error != ESUCCESS) {
    throw_CBGPException(jEnv, "node already exists (%s)",
			network_strerror(error));
    return_jni_unlock(jEnv, NULL);
  }

  if (domain != NULL)
    igp_domain_add_router(domain, node);

  joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, node);
  return_jni_unlock(jEnv, joNode);
}

// -----[ netGetNodes ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetNodes
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetNodes
  (JNIEnv * jEnv, jobject joCBGP)
{
  gds_enum_t * nodes;
  net_node_t * node;
  jobject joVector;
  jobject joNode;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  nodes= trie_get_enum(network_get_default()->nodes);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));

    // Create Node object
    if ((joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, node)) == NULL) {
      joVector= NULL;
      break;
    }

    // Add to Vector
    cbgp_jni_Vector_add(jEnv, joVector, joNode);
  }
  enum_destroy(&nodes);

  return_jni_unlock(jEnv, joVector);
}

// -----[ netGetNode ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetNode
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/net/Node;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetNode
(JNIEnv * jEnv, jobject joCBGP, jstring jsAddr)
{
  net_node_t * node;
  jobject joNode;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  // Find net_node_t object
  if ((node= cbgp_jni_net_node_from_string(jEnv, jsAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  // Create Node object
  if ((joNode= cbgp_jni_new_net_Node(jEnv, joCBGP, node)) == NULL)
    return_jni_unlock(jEnv, NULL);

  return_jni_unlock(jEnv, joNode);
}


/////////////////////////////////////////////////////////////////////
// Links management
/////////////////////////////////////////////////////////////////////

// -----[ netAddLink ]-----------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    nodeLinkAdd
 * Signature: (II)I
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netAddLink
  (JNIEnv * jEnv, jobject joCBGP, jstring jsSrcAddr, jstring jsDstAddr,
   jint jiWeight)
{
  net_node_t * nodeSrc, * nodeDst;
  net_iface_t * pIface;
  jobject joIface;
  int iResult;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  if ((nodeSrc= cbgp_jni_net_node_from_string(jEnv, jsSrcAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  if ((nodeDst= cbgp_jni_net_node_from_string(jEnv, jsDstAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  iResult= net_link_create_rtr(nodeSrc, nodeDst, BIDIR, &pIface);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not create link (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }

  iResult= net_iface_set_metric(pIface, 0, jiWeight, BIDIR);
  if (iResult != ESUCCESS) {
    throw_CBGPException(jEnv, "could not set link metric (%s)",
			network_strerror(iResult));
    return_jni_unlock(jEnv, NULL);
  }
  
  joIface= cbgp_jni_new_net_Interface(jEnv, joCBGP, pIface);

  return_jni_unlock(jEnv, joIface);
}


////////////////////////////////b/////////////////////////////////////
// BGP domains management
/////////////////////////////////////////////////////////////////////

// -----[ _bgpGetDomains ]-------------------------------------------
static int _bgpGetDomains(bgp_domain_t * domain, void * ctx)
{
  jni_ctx_t * pCtx= (jni_ctx_t *) ctx;
  jobject joDomain;

  // Create BGPDomain instance
  joDomain= cbgp_jni_new_bgp_Domain(pCtx->jEnv,
				    pCtx->joCBGP,
				    domain);

  if (joDomain == NULL)
    return -1;

  // Add to Vector
  if (cbgp_jni_Vector_add(pCtx->jEnv, pCtx->joVector, joDomain) != 0)
    return -1;
  
  return 0;
}

// -----[ bgpGetDomains ]--------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpGetDomains
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpGetDomains
  (JNIEnv * jEnv, jobject joCBGP)
{
  jni_ctx_t sCtx;
  jobject joVector= NULL;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  /* Create new Vector */
  if ((joVector= cbgp_jni_new_Vector(jEnv)) == NULL)
    return_jni_unlock(jEnv, NULL);

  sCtx.jEnv= jEnv;
  sCtx.joCBGP= joCBGP;
  sCtx.joVector= joVector;

  if (bgp_domains_for_each(_bgpGetDomains, &sCtx) != 0) {
    throw_CBGPException(jEnv, "could not get list of domains");
    return_jni_unlock(jEnv, NULL);
  }

  return_jni_unlock(jEnv, joVector);
}


/////////////////////////////////////////////////////////////////////
// BGP router management
/////////////////////////////////////////////////////////////////////

// -----[ bgpAddRouter ]---------------------------------------------
/**
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    bgpAddRouter
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)Lbe/ac/ucl/ingi/cbgp/bgp/Router;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_bgpAddRouter
  (JNIEnv * jEnv, jobject joCBGP, jstring jsName, jstring jsAddr,
   jint jiASNumber)
{
  net_node_t * node;
  bgp_router_t * router;
  jobject joRouter;
  net_error_t error;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  /* Try to find related router. */
  if ((node= cbgp_jni_net_node_from_string(jEnv, jsAddr)) == NULL)
    return_jni_unlock(jEnv, NULL);

  /* Create BGP router, and register. */
  error= bgp_add_router(jiASNumber, node, &router);
  if (error != ESUCCESS) {
    throw_CBGPException(jEnv, "Could not create BGP router (%s)",
			network_strerror(error));
    return_jni_unlock(jEnv, NULL);
  }

  /*
  if (jsName != NULL) {
    pcName = (*env)->GetStringUTFChars(env, jsName, NULL);
    bgp_router_set_name(router, str_create((char *) pcName));
    (*env)->ReleaseStringUTFChars(env, jsName, pcName);
    }*/

  joRouter= cbgp_jni_new_bgp_Router(jEnv, joCBGP, router);

  return_jni_unlock(jEnv, joRouter);
}


/////////////////////////////////////////////////////////////////////
// Simulation queue management
/////////////////////////////////////////////////////////////////////

// -----[ simCancel ]------------------------------------------------
/**
 *
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simCancel
(JNIEnv * jEnv, jobject joCBGP)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  sim_cancel(network_get_simulator(network_get_default()));
}

// -----[ simRun ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simRun
 * Signature: ()I
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simRun
  (JNIEnv * jEnv, jobject joCBGP)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  if (sim_run(network_get_simulator(network_get_default())) != 0)
    throw_CBGPException(jEnv, "simulation error");

  jni_unlock(jEnv);
}

// -----[ simStep ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simStep
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simStep
  (JNIEnv * jEnv, jobject joCBGP, jint jiNumSteps)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  if (sim_step(network_get_simulator(network_get_default()),
	       jiNumSteps) != 0)
    throw_CBGPException(jEnv, "simulation error");

  jni_unlock(jEnv);
}

// -----[ simClear ]-------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simClear
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simClear
  (JNIEnv * jEnv, jobject joCBGP)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  sim_clear(network_get_simulator(network_get_default()));

  jni_unlock(jEnv);
}

// -----[ simGetEventCount ]-----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simGetEventCount
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simGetEventCount
  (JNIEnv * jEnv, jobject joCBGP)
{
  jlong jlResult;

  if (jni_check_null(jEnv, joCBGP))
    return -1;

  //jni_lock(jEnv);

  jlResult= (jlong) sim_get_num_events(network_get_simulator(network_get_default()));

  //return_jni_unlock(jEnv, jlResult); 
  return jlResult;
}

// -----[ simGetEvent ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    simGetEvent
 * Signature: (I)Lbe/ac/ucl/ingi/cbgp/net/Message
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_simGetEvent
(JNIEnv * jEnv, jobject joCBGP, jint i)
{
  net_send_ctx_t * ctx;
  simulator_t * sim;
  jobject joMessage;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  // Note: we assume that only network events (net_send_ctx_t) are
  // stored in the pending event set.
  sim= network_get_simulator(network_get_default());
  ctx= (net_send_ctx_t *) sim_get_event(sim, (unsigned int) i);
  
  // Create new Message instance.
  if ((joMessage= cbgp_jni_new_BGPMessage(jEnv, ctx->msg)) == NULL)
    return_jni_unlock(jEnv, NULL);
  
  return_jni_unlock(jEnv, joMessage);
}


/////////////////////////////////////////////////////////////////////
// Miscellaneous methods
/////////////////////////////////////////////////////////////////////

// -----[ runCmd ]---------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    runCmd
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_runCmd
  (JNIEnv * jEnv, jobject joCBGP, jstring jsCommand)
{
  char * cCommand;
  cli_error_t cli_error;
  const char * msg;

  if (jni_check_null(jEnv, joCBGP))
    return;

  if (jsCommand == NULL)
    return;

  jni_lock(jEnv);

  cCommand= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsCommand, NULL);
  if (libcbgp_exec_cmd(cCommand) != CLI_SUCCESS) {
    cli_get_error_details(cli_get(), &cli_error);

    // If there is a detailled user error message, then generate
    // an exception with this message. Otherwise, throw an exception
    // with the default CLI error message.
    if (cli_error.user_msg != NULL)
      msg= cli_error.user_msg;
    else
      msg= cli_strerror(cli_error.error);
    throw_CBGPScriptException(jEnv, msg, NULL,
			      cli_error.line_number);
  }
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsCommand, cCommand);

  jni_unlock(jEnv);
}

// -----[ runScript ]------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    runScript
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_runScript
  (JNIEnv * jEnv, jobject joCBGP, jstring jsFileName)
{
  char * filename;
  int iResult;
  cli_error_t cli_error;
  const char * msg;

  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  filename= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);
  if ((iResult= libcbgp_exec_file(filename)) != CLI_SUCCESS) {
    cli_get_error_details(cli_get(), &cli_error);

    // If there is a detailled user error message, then generate the
    // an exception with this message. Otherwise, throw an exception
    // with the default CLI error message.
    if (cli_error.user_msg != NULL)
      msg= cli_error.user_msg;
    else
      msg= cli_strerror(cli_error.error);
    throw_CBGPScriptException(jEnv, msg, NULL,
			      cli_error.line_number);
  }
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, filename);

  jni_unlock(jEnv);
}

// -----[ cliGetPrompt ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    cliGetPrompt
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_cliGetPrompt
  (JNIEnv *jEnv, jobject joCBGP)
{
  char * pcPrompt;
  jstring jsPrompt;

  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  jni_lock(jEnv);

  pcPrompt= cli_context_to_string(cli_get()->ctx, "cbgp");
  jsPrompt= cbgp_jni_new_String(jEnv, pcPrompt);

  return_jni_unlock(jEnv, jsPrompt);
}


/////////////////////////////////////////////////////////////////////
// Experimental features
/////////////////////////////////////////////////////////////////////

// -----[ _cbgp_jni_load_mrt_handler ]-------------------------------
static int _cbgp_jni_load_mrt_handler(int status,
				      bgp_route_t * route,
				      net_addr_t peer_addr,
				      asn_t peer_asn,
				      void * ctx)
{
  jni_ctx_t * jni_ctx= (jni_ctx_t *) ctx;
  jobject joRoute;
  int result;

  // Check that there is a route to handle (according to API)
  if (status != BGP_INPUT_STATUS_OK)
    return BGP_INPUT_SUCCESS;

  // Create Java object for route
  if ((joRoute= cbgp_jni_new_BGPRoute(jni_ctx->jEnv, route,
				      jni_ctx->joHashtable)) == NULL)
    return -1;

  // Destroy converted route
  route_destroy(&route);

  // Add the route object to the list
  result= cbgp_jni_Vector_add(jni_ctx->jEnv, jni_ctx->joVector, joRoute);

  // if there is a large number of objects, we should remove the
  // reference to the route as soon as it is added to the Vector
  // object. If we keep a large number of references to Java
  // instances, the performance will quickly decrease!
  (*(jni_ctx->jEnv))->DeleteLocalRef(jni_ctx->jEnv, joRoute);

  return result;
}

// -----[ loadMRT ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    loadMRT
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_loadMRT
(JNIEnv * jEnv , jobject joCBGP, jstring jsFileName, jstring jsFormat)
{
  jobject joVector= NULL;
  const char * filename, * format_str;
  bgp_input_type_t format= BGP_ROUTES_INPUT_MRT_ASC;
  jni_ctx_t jni_ctx;
  int result;
  jobject joPathHash;

  /* Check the input format */
  format_str= (char *) (*jEnv)->GetStringUTFChars(jEnv, jsFormat, NULL);
  result= bgp_routes_str2format(format_str, &format);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFormat, format_str);
  if (result != 0)
    throw_CBGPException(jEnv, "invalid format");

  /* Check the CBGP instance */
  if (jni_check_null(jEnv, joCBGP))
    return NULL;

  /* Lock instance */
  jni_lock(jEnv);

  joPathHash= cbgp_jni_new_Hashtable(jEnv);

  filename= (*jEnv)->GetStringUTFChars(jEnv, jsFileName, NULL);

  joVector= cbgp_jni_new_Vector(jEnv);
  jni_ctx.jEnv= jEnv;
  jni_ctx.joVector= joVector;
  jni_ctx.joHashtable= joPathHash;

  // Load routes
  result= bgp_routes_load(filename, format,
			  _cbgp_jni_load_mrt_handler,
			  &jni_ctx);
  if (result != BGP_INPUT_SUCCESS) {
    joVector= NULL;
    throw_CBGPException(jEnv, "Could not load BGP routes (%s)",
			bgp_input_strerror(result));
  }

  (*jEnv)->ReleaseStringUTFChars(jEnv, jsFileName, filename);

  return_jni_unlock(jEnv, joVector);
}

// -----[ _bgp_msg_listener_cb ]-------------------------------------
static void _bgp_msg_listener_cb(net_msg_t * msg, void * ctx)
{
  jni_listener_t * listener= (jni_listener_t *) ctx;
  JNIEnv * env= NULL;
  jobject joMessage= NULL;

  jni_listener_get_env(listener, &env);

  // Create new Message instance.
  if ((joMessage= cbgp_jni_new_BGPMessage(env, msg)) == NULL)
    return;

  // Call the listener's method.
  cbgp_jni_call_void(env, listener->joListener,
		     "handleMessage",
		     "(Lbe/ac/ucl/ingi/cbgp/net/Message;)V",
		     joMessage);
}

// -----[ setBGPMsgListener ]----------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    setBGPMsgListener
 * Signature: (Lbe/ac/ucl/ingi/cbgp/BGPMsgListener;)V
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_setBGPMsgListener
  (JNIEnv * jEnv , jobject joCBGP, jobject joListener)
{
  if (jni_check_null(jEnv, joCBGP))
    return;

  jni_lock(jEnv);

  if (joListener != NULL) {
    jni_listener_set(&_bgp_msg_listener, jEnv, joListener);
    bgp_router_set_msg_listener(_bgp_msg_listener_cb, &_bgp_msg_listener);
  } else {
    jni_listener_unset(&_bgp_msg_listener, jEnv);
    bgp_router_set_msg_listener(NULL, NULL);
  }

  jni_unlock(jEnv);
}

// -----[ netGetLinks ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    netGetLinks
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_netGetLinks
  (JNIEnv * env, jobject joCBGP)
{
  jobject joVector, joLink;
  gds_enum_t * nodes= NULL, * links= NULL;
  net_node_t * node;
  net_iface_t * link;

  jni_lock(env);

  joVector= cbgp_jni_new_Vector(env);
    
  nodes= trie_get_enum(network_get_default()->nodes);
  while (enum_has_next(nodes)) {
    node= *((net_node_t **) enum_get_next(nodes));
    
    links= net_links_get_enum(node->ifaces);
    while (enum_has_next(links)) {
      link= *((net_iface_t **) enum_get_next(links));

      if ((link->type == NET_IFACE_LOOPBACK) ||
	  (link->type == NET_IFACE_VIRTUAL))
	continue;

      joLink= cbgp_jni_new_net_Interface(env, joCBGP, link);
      if (joLink == NULL)
	return_jni_unlock(env, NULL);
      if (cbgp_jni_Vector_add(env, joVector, joLink))
	return_jni_unlock(env, NULL);
      //(*env)->DeleteLocalRef(env, joLink);
    }
    enum_destroy(&links);
  }
  enum_destroy(&nodes);

  return_jni_unlock(env, joVector);
}


/////////////////////////////////////////////////////////////////////
//
// CBGP STATIC METHODS
//
/////////////////////////////////////////////////////////////////////

// -----[ getVersion ]-----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    getVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_getVersion
  (JNIEnv * env, jclass cls)
{
  return cbgp_jni_new_String(env, PACKAGE_VERSION);
}

// -----[ getErrorMsg ]----------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_CBGP
 * Method:    getErrorMsg
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_CBGP_getErrorMsg
  (JNIEnv * env, jclass cls, jint error)
{
  return cbgp_jni_net_error_str(env, error);
}

// -----[ rankToString ]---------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_bgp_Route
 * Method:    rankToString
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_be_ac_ucl_ingi_cbgp_bgp_Route_rankToString
(JNIEnv * jEnv, jclass jcRoute, jint jiRank) {
  if (jiRank == 0) {
    return cbgp_jni_new_String(jEnv, "Single Choice");
  } else if (jiRank < DP_NUM_RULES) {
    return cbgp_jni_new_String(jEnv, DP_RULES[jiRank-1].name);
  }
  return NULL;
}


/////////////////////////////////////////////////////////////////////
//
// JNI Library Load/UnLoad
//
/////////////////////////////////////////////////////////////////////

/**
 * The VM calls JNI_OnLoad when the native library is loaded (for
 * example, through System.loadLibrary). JNI_OnLoad must return the
 * JNI version needed by the native library. In order to use any of
 * the new JNI functions, a native library must export a JNI_OnLoad
 * function that returns JNI_VERSION_1_2. If the native library does
 * not export a JNI_OnLoad function, the VM assumes that the library
 * only requires JNI version JNI_VERSION_1_1. If the VM does not
 * recognize the version number returned by JNI_OnLoad, the native
 * library cannot be loaded.
 */

// -----[ JNI_OnLoad ]-----------------------------------------------
/**
 * This function is called when the JNI library is loaded.
 */
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM * jVM, void *reserved)
{
  JNIEnv * jEnv= NULL;
  void * ppEnv= &jEnv;

  /*fprintf(stdout, "C-BGP JNI library loaded.\n");
    fflush(stdout);*/

#ifdef __MEMORY_DEBUG__
  gds_init(GDS_OPTION_MEMORY_DEBUG);
#else
  gds_init(0);
#endif

  // Get JNI environment
  if ((*jVM)->GetEnv(jVM, ppEnv, JNI_VERSION_1_2) != JNI_OK)
    abort();

  // Init locking system
  _jni_lock_init(jEnv);

  return JNI_VERSION_1_2;
}

// -----[ JNI_OnUnload ]---------------------------------------------
/**
 * This function is called when the JNI library is unloaded.
 */
JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM * jVM, void *reserved)
{
  JNIEnv * jEnv= NULL;
  void * ppEnv= &jEnv;

  /*fprintf(stdout, "C-BGP JNI library unloaded.\n");
    fflush(stdout);*/

  // Get JNI environment
  if ((*jVM)->GetEnv(jVM, ppEnv, JNI_VERSION_1_2) != JNI_OK)
    abort();

  // Terminate C<->Java mappings and locking system
  _jni_proxies_destroy(jEnv);
  _jni_lock_destroy(jEnv);

  gds_destroy();
}
