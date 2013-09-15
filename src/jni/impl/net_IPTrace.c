// ==================================================================
// @(#)net_IPTrace.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/09/2007
// $Id: net_IPTrace.c,v 1.2 2009-03-25 07:51:59 bqu Exp $
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
#  include <config.h>
#endif

#include <jni/impl/net_IPTrace.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>

#include <jni/impl/net_Link.h>
#include <jni/impl/net_Node.h>
#include <jni/impl/net_Subnet.h>

#define BASE "be/ac/ucl/ingi/cbgp/";

#define CLASS_IPTrace "be/ac/ucl/ingi/cbgp/IPTrace"
#define CONSTR_IPTrace "(IJJ)V"
#define METHOD_IPTrace_append "(Lbe/ac/ucl/ingi/cbgp/IPTraceElement;)V"

#define CLASS_IPTraceElement "be/ac/ucl/ingi/cbgp/IPTraceElement"
#define CLASS_IPTraceElementNode "be/ac/ucl/ingi/cbgp/IPTraceNode"
#define CONSTR_IPTraceElementNode "(Lbe/ac/ucl/ingi/cbgp/net/Node;" \
  "Lbe/ac/ucl/ingi/cbgp/net/Interface;" \
  "Lbe/ac/ucl/ingi/cbgp/net/Interface;)V"

#define CLASS_IPTraceElementSubnet "be/ac/ucl/ingi/cbgp/IPTraceSubnet"
#define CONSTR_IPTraceElementSubnet "(Lbe/ac/ucl/ingi/cbgp/net/Subnet;)V"

#define CLASS_IPTraceElementTrace "be/ac/ucl/ingi/cbgp/IPTraceTrace"
#define CONSTR_IPTraceElementTrace "(Lbe/ac/ucl/ingi/cbgp/IPTrace;)V"

// -----[ _new_IPTraceElementNode ]----------------------------------
static inline jobject _new_IPTraceElementNode(JNIEnv * env,
					      net_node_t * node,
					      net_iface_t * iif,
					      net_iface_t * oif)
{
  jobject joNode= NULL, joInIface= NULL, joOutIface= NULL;

  if (node != NULL) {
    joNode= cbgp_jni_new_net_Node(env, NULL, node);
    if (joNode == NULL)
      return NULL;
    if (iif != NULL) {
      joInIface= cbgp_jni_new_net_Interface(env, NULL, iif);
      if (joInIface == NULL)
	return NULL;
    }
    if (oif != NULL) {
      joOutIface= cbgp_jni_new_net_Interface(env, NULL, oif);
      if (joOutIface == NULL)
	return NULL;
    }
  }

  return cbgp_jni_new(env, CLASS_IPTraceElementNode,
		      CONSTR_IPTraceElementNode, joNode,
		      joInIface, joOutIface);
}

// -----[ _new_IPTraceElementSubnet ]--------------------------------
static inline jobject _new_IPTraceElementSubnet(JNIEnv * env,
						net_subnet_t * subnet)
{
  jobject joSubnet= cbgp_jni_new_net_Subnet(env, NULL, subnet);
  if (joSubnet == NULL)
    return NULL;

  return cbgp_jni_new(env, CLASS_IPTraceElementSubnet,
		      CONSTR_IPTraceElementSubnet, joSubnet);
}

// -----[ _new_IPTraceElementTrace ]---------------------------------
static inline jobject _new_IPTraceElementTrace(JNIEnv * env,
					       ip_trace_t * trace)
{
  jobject joTrace= cbgp_jni_new_IPTrace(env, trace);
  if (joTrace == NULL)
    return NULL;
  return cbgp_jni_new(env, CLASS_IPTraceElementTrace,
		      CONSTR_IPTraceElementTrace, joTrace);
}

// -----[ _new_IPTraceElement ]--------------------------------------
static inline jobject _new_IPTraceElement(JNIEnv * env,
					  ip_trace_item_t * item,
					  int * hop_count)
{
  jobject element= NULL;
  jclass class;
  jfieldID field;
  int hop_count_val= *hop_count;

  switch (item->elt.type) {
  case NODE:
    element= _new_IPTraceElementNode(env,
				     item->elt.node,
				     item->iif,
				     item->oif);
    (*hop_count)++;
    break;

  case SUBNET:
    element= _new_IPTraceElementSubnet(env, item->elt.subnet);
    break;

  case TRACE:
    element= _new_IPTraceElementTrace(env, (ip_trace_t *) item->elt.trace);
    break;

  default:
    cbgp_fatal("invalid element type in IP Trace");
  }
  if ((*env)->ExceptionOccurred(env))
    return NULL;

  /* Initialize "hop_count" field value (direct access) */
  class= (*env)->FindClass(env, CLASS_IPTraceElement);
  if ((class == NULL) || (*env)->ExceptionOccurred(env))
    jni_abort(env, "Could not get id of class \"IPTraceElement\"");

  field= (*env)->GetFieldID(env, class, "hop_count", "I");
  if ((field == NULL) || (*env)->ExceptionOccurred(env))
    jni_abort(env, "Could not get id of field \"hop_count\" in class "
	      "\"IPTraceElement\"");
  
  (*env)->SetIntField(env, element, field, hop_count_val);
  if ((*env)->ExceptionOccurred(env))
    jni_abort(env, "Could not get id of field \"hop_count\" in class "
	      "\"IPTraceElement\"");

  return element;
}

// -----[ _IPTrace_append ]------------------------------------------
static inline int _IPTrace_append(JNIEnv * env, jobject joTrace,
				  jobject joElement)
{
  return cbgp_jni_call_void(env, joTrace, "append",
			    METHOD_IPTrace_append,
			    joElement);
}

// -----[ cbgp_jni_new_IPTrace ]-------------------------------------
/**
 *
 */
jobject cbgp_jni_new_IPTrace(JNIEnv * env,
			     ip_trace_t * trace)
{
  jobject joTrace;
  jobject joElement;
  unsigned int index;
  ip_trace_item_t * trace_item;
  int hop_count= 0;

  // Create new IPTrace object
  if ((joTrace= cbgp_jni_new(env, CLASS_IPTrace, CONSTR_IPTrace,
			     (jint) trace->status,
			     trace->delay,
			     trace->capacity))
      == NULL)
    return NULL;

  // Add hops
  for (index= 0; index < ip_trace_length(trace); index++) {
    trace_item= ip_trace_item_at(trace, index);
    joElement= _new_IPTraceElement(env, trace_item, &hop_count);
    if (joElement == NULL)
      return NULL;
    if (_IPTrace_append(env, joTrace, joElement))
      return NULL;
  }

  return joTrace;
}

// -----[ cbgp_jni_new_IPTraces ]------------------------------------
jobjectArray cbgp_jni_new_IPTraces(JNIEnv * env,
				   ip_trace_t ** traces,
				   unsigned int num_traces)
{
  jobject trace;
  jobjectArray array;
  unsigned int index;

  array= jni_new_ObjectArray(env, num_traces, CLASS_IPTrace);
  if ((array == NULL) || (*env)->ExceptionOccurred(env))
    return NULL;

  for (index= 0; index < num_traces; index++) {

    // Convert to a Java IPTrace object
    trace= cbgp_jni_new_IPTrace(env, traces[index]);
    if (trace == NULL)
      return NULL;

    // Add to array
    (*env)->SetObjectArrayElement(env, array, 0, trace);
    if ((*env)->ExceptionOccurred(env))
      return NULL;

  }

  return array;
}
