// ==================================================================
// @(#)jni_base.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 21/03/2006
// $Id: jni_base.h,v 1.7 2009-03-24 16:00:41 bqu Exp $
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

#ifndef __JNI_BASE_H__
#define __JNI_BASE_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ jni_abort ]----------------------------------------------
  void jni_abort(JNIEnv * jEnv, char * pcMsg, ...);

  ///////////////////////////////////////////////////////////////////
  // Helper functions for Java objects creation and method calls
  ///////////////////////////////////////////////////////////////////

  // -----[ jni_check_null ]-----------------------------------------
  int jni_check_null(JNIEnv * jEnv, jobject joObject);
  // -----[ cbgp_jni_new ]-------------------------------------------
  jobject cbgp_jni_new(JNIEnv * env, const char * pcClass,
		       const char * pcConstr, ...);
  // -----[ cbgp_jni_call_void ]-------------------------------------
  int cbgp_jni_call_void(JNIEnv * env, jobject joObject,
			 const char * pcMethod,
			 const char * pcSignature, ...);
  // -----[ cbgp_jni_call_boolean ]----------------------------------
  int cbgp_jni_call_boolean(JNIEnv * env, jobject joObject,
			    const char * pcMethod,
			    const char * pcSignature, ...);
  // -----[ cbgp_jni_call_Object ]-----------------------------------
  jobject cbgp_jni_call_Object(JNIEnv * env, jobject joObject,
			       const char * pcMethod,
			       const char * pcSignature, ...);
  
  
  ///////////////////////////////////////////////////////////////////
  // Management of standard Java API objects
  ///////////////////////////////////////////////////////////////////
  
  // -----[ cbgp_jni_new_Short ]-------------------------------------
  jobject cbgp_jni_new_Short(JNIEnv * jEnv, jshort jsValue);
  // -----[ cbgp_jni_new_Integer ]-----------------------------------
  jobject cbgp_jni_new_Integer(JNIEnv * jEnv, jint jiValue);
  // -----[ cbgp_jni_new_Long ]--------------------------------------
  jobject cbgp_jni_new_Long(JNIEnv * jEnv, jlong jlValue);
  // -----[ cbgp_jni_new_String ]------------------------------------
  jstring cbgp_jni_new_String(JNIEnv * jEnv, const char * pcString);
  // -----[ cbgp_jni_new_Vector ]------------------------------------
  jobject cbgp_jni_new_Vector(JNIEnv * jEnv);
  // -----[ cbgp_jni_Vector_add ]------------------------------------
  int cbgp_jni_Vector_add(JNIEnv * jEnv, jobject joVector,
			  jobject joObject);
  /*
  // -----[ cbgp_jni_new_HashMap ]-----------------------------------
  jobject cbgp_jni_new_HashMap(JNIEnv * jEnv);
  // -----[ cbgp_jni_HashMap_put ]-----------------------------------
  jobject cbgp_jni_HashMap_put(JNIEnv * jEnv, jobject joHashMap,
  jobject joKey, jobject joValue);
  */
  // -----[ cbgp_jni_new_Hashtable ]---------------------------------
  jobject cbgp_jni_new_Hashtable(JNIEnv * jEnv);
  // -----[ cbgp_jni_Hashtable_get ]---------------------------------
  jobject cbgp_jni_Hashtable_get(JNIEnv * jEnv, jobject joHashMap,
				 jobject joKey);
  // -----[ cbgp_jni_Hashtable_put ]---------------------------------
  jobject cbgp_jni_Hashtable_put(JNIEnv * jEnv, jobject joHashMap,
				 jobject joKey, jobject joValue);
  // -----[ cbgp_jni_new_ArrayList ]---------------------------------
  jobject cbgp_jni_new_ArrayList(JNIEnv * jEnv);
  // -----[ cbgp_jni_ArrayList_add ]---------------------------------
  int cbgp_jni_ArrayList_add(JNIEnv * jEnv, jobject joArrayList,
			     jobject joItem);
  // -----[ jni_new_ObjectArray ]------------------------------------
  jobjectArray jni_new_ObjectArray(JNIEnv * jEnv, unsigned int size,
				   char * object_class);
  
#ifdef __cplusplus
}
#endif

#endif /* __JNI_BASE_H__ */
