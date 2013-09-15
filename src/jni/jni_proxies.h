// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/03/2006
// $Id: jni_proxies.h,v 1.11 2009-03-26 13:23:18 bqu Exp $
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

/**
 * \file
 * Provide data structure and functions to handle the mapping between
 * Java and C objects.
 */

#ifndef __JNI_PROXIES_H__
#define __JNI_PROXIES_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ jni_proxy_add ]------------------------------------------
  /**
   * Add a mapping between a Java object and a C object.
   *
   * \param env  is the JNI environment.
   * \param jobj is the Java object.
   * \param cobj is the C object (pointer).
   */
  void jni_proxy_add(JNIEnv * env, jobject jobj, void * cobj);

  // -----[ jni_proxy_remove ]---------------------------------------
  /**
   * Remove the mapping between a Java object and a C object.
   *
   * \param env  is the JNI environment.
   * \param jobj is the Java object.
   */
  void jni_proxy_remove(JNIEnv * env, jobject jobj);

  // -----[ jni_proxy_lookup ]---------------------------------------
  /**
   * Get the C object mapped to a Java object.
   *
   * \param env      is the JNI environment.
   * \param jobj     is the Java object.
   * \param filename is the source file name where the request was
   *                 issued (for debugging purposes).
   * \param line     is the source line where the request was
   *                 issued (for debugging purposes).
   * \retval the corresponding C object, or NULL if there is no such
   *         mapping.
   */
  void * jni_proxy_lookup2(JNIEnv * env, jobject jobj,
			   const char * filename, int line);

  // -----[ jni_proxy_get ]------------------------------------------
  /**
   * Get the Java object mapped to a C object.
   *
   * \param env      is the JNI environment.
   * \param cobj     is the C object (pointer).
   * \param filename is the source file name where the request was
   *                 issued (for debugging purposes).
   * \param line     is the source line where the request was
   *                 issued (for debugging purposes).
   * \retval the corresponding Java object, or NULL if there is no
   *         mapping.
   */
  jobject jni_proxy_get2(JNIEnv * env, void * cobj,
			const char * filename, int line);

  // -----[ jni_proxy_get_CBGP ]-------------------------------------
  //jobject jni_proxy_get_CBGP(JNIEnv * jEnv, jobject joObject);

  // -----[ jni_lock ]-----------------------------------------------
  void jni_lock(JNIEnv * env);
  // -----[ jni_unlock ]---------------------------------------------
  void jni_unlock(JNIEnv * env);

  // -----[ _jni_proxies_init ]--------------------------------------
  void _jni_proxies_init();
  // -----[ _jni_proxies_destroy ]-----------------------------------
  void _jni_proxies_destroy(JNIEnv * env);
  // -----[ _jni_proxies_invalidate ]--------------------------------
  void _jni_proxies_invalidate(JNIEnv * env);

  // -----[ _jni_lock_init ]------------------------------------------
  void _jni_lock_init(JNIEnv * env);
  // -----[ _jni_lock_destroy ]---------------------------------------
  void _jni_lock_destroy(JNIEnv * env);

#define return_jni_unlock(E, R) if (1) { jni_unlock(E); return R; }
#define return_jni_unlock2(E) if (1) { jni_unlock(E); return; }
#define jni_proxy_lookup(E,O) jni_proxy_lookup2(E,O,__FILE__,__LINE__)
#define jni_proxy_get(E,O) jni_proxy_get2(E,O,__FILE__,__LINE__)

#ifdef __cplusplus
}
#endif

#endif /* __JNI_PROXIES_H__ */
