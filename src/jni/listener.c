// ==================================================================
// @(#)listener.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/06/2007
// $Id: listener.c,v 1.6 2009-03-26 13:24:14 bqu Exp $
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
#include <jni/listener.h>
#include <jni/jni_base.h>
#include <jni/jni_util.h>

// -----[ jni_listener_init ]----------------------------------------
void jni_listener_init(jni_listener_t * listener)
{
  listener->jVM= NULL;
  listener->joListener= NULL;
}

// -----[ jni_listener_set ]-----------------------------------------
void jni_listener_set(jni_listener_t * listener, JNIEnv * env,
		      jobject java_listener)
{
  jni_listener_unset(listener, env);

  if (java_listener == NULL)
    return;

  // Get reference to Java Virtual Machine (required by the JNI callback)
  if (listener->jVM == NULL)
    if ((*env)->GetJavaVM(env, &listener->jVM) != JNI_OK) {
      throw_CBGPException(env, "could not get reference to Java VM");
      return;
    }

  // Add a global reference to the listener object (returns NULL if
  // system has run out of memory)
  listener->joListener= (*env)->NewGlobalRef(env, java_listener);
  if (listener->joListener == NULL)
    jni_abort(env, "Could not obtain global reference in jni_set_listener()");
}

// -----[ jni_listener_unset ]---------------------------------------
void jni_listener_unset(jni_listener_t * listener, JNIEnv * env)
{
  // Remove global reference to former listener (if applicable)
  if (listener->joListener != NULL)
    (*env)->DeleteGlobalRef(env, listener->joListener);
  listener->joListener= NULL;
}

// -----[ jni_listener_get_env ]-----------------------------------
void jni_listener_get_env(jni_listener_t * listener, JNIEnv ** env_ref)
{
  JavaVM * java_vm= listener->jVM;
  void ** env_ref_as_void= (void **) &(*env_ref);
  
  /* Attach the current thread to the Java VM (and get a pointer to
   * the JNI environment). This is required since the calling thread
   * might be different from the one that initialized the related
   * listener object. */
  if ((*java_vm)->AttachCurrentThread(java_vm, env_ref_as_void, NULL) != 0)
    jni_abort(*env_ref, "AttachCurrentThread failed in JNI\n");
}
