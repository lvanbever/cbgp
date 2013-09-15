// ==================================================================
// @(#)jni_proxies.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 27/03/2006
// $Id: jni_proxies.c,v 1.19 2009-03-26 13:23:18 bqu Exp $
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

#include <sys/types.h>
#include <unistd.h>

#include <assert.h>
#include <jni.h>
#include <string.h>

#include <libgds/hash.h>
#include <libgds/memory.h>

#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>

jobject get_Global_CBGP();

//#define DEBUG_LOCK
//#define DEBUG_PROXIES

#define JNI_PROXY_HASH_SIZE 1000

#define CLASS_ProxyObject "be/ac/ucl/ingi/cbgp/ProxyObject"
#define METHOD_ProxyObject_getCBGP "()Lbe/ac/ucl/ingi/cbgp/CBGP;"

static gds_hash_set_t * _c2java_map= NULL;
static gds_hash_set_t * _java2c_map= NULL;
static unsigned long    _next_id   = 0;
static JNIEnv         * jCurrentEnv= NULL;

// -----[ _proxy_t ]-----
typedef struct {
  void              * cobj; // Identifier for CObj
  jobject             jobj; // Identifier for JObj (weak global reference)
  unsigned long int   id;   // Global identifier

#if defined(DEBUG_PROXIES)
  char              * class_name;
#endif /* DEBUG_PROXIES */

} _proxy_t;

// -----[ _get_class_name ]------------------------------------------
#if defined(DEBUG_PROXIES)
static inline char * _get_class_name(JNIEnv * env, jobject jobj)
{
  jclass jcClass;
  jstring jsName;
  const char * str;
  char * class_name;

  jcClass= (*env)->GetObjectClass(env, jobj);
  assert(jcClass != NULL);
  
  jsName= (jstring) cbgp_jni_call_Object(env, jcClass, "getName",
					 "()Ljava/lang/String;");
  assert(jsName != NULL);

  str= (*env)->GetStringUTFChars(env, jsName, NULL);
  if (str == NULL)
    return NULL; /* OutOfMemoryError already thrown */

  class_name= strdup(str);
  (*jEnv)->ReleaseStringUTFChars(jEnv, jsName, str);
  return class_name;
}
#endif /* DEBUG_PROXIES */

// -----[ _jni_proxy_Object_get_id ]---------------------------------
static inline
jlong _jni_proxy_Object_get_id(JNIEnv * env, jobject jobj)
{
  jclass jcClass;
  jfieldID jfFieldID;

  jcClass= (*env)->GetObjectClass(env, jobj);
  if (jcClass == NULL)
    jni_abort(env, "couldn't get object's class (proxy-get-id)");

  jfFieldID= (*env)->GetFieldID(env, jcClass, "objectId", "J");
  if (jfFieldID == NULL)
    jni_abort(env, "couldn't get class's field ID (proxy-get-id)");

  return (*env)->GetLongField(env, jobj, jfFieldID);
}

// -----[ _jni_proxy_Object_set_id ]---------------------------------
static inline 
void _jni_proxy_Object_set_id(JNIEnv * env, jobject jobj, jlong jlId)
{
  jclass jcClass;
  jfieldID jfFieldID;

  jcClass= (*env)->GetObjectClass(env, jobj);
  if (jcClass == NULL)
    jni_abort(env, "couldn't get object's class (proxy-set-id)");

  jfFieldID= (*env)->GetFieldID(env, jcClass, "objectId", "J");
  if (jfFieldID == NULL)
    jni_abort(env, "couldn't get class's field ID (proxy-set-id)");

  (*env)->SetLongField(env, jobj, jfFieldID, jlId);
}

// -----[ _jni_proxy_create ]----------------------------------------
static inline
_proxy_t * _jni_proxy_create(JNIEnv * env, void * cobj, jobject jobj)
{
  _proxy_t * proxy= MALLOC(sizeof(_proxy_t));
  proxy->cobj= cobj;
  proxy->jobj= (*env)->NewGlobalRef(env, jobj);
  proxy->id= ++_next_id;

  // NOTE: We can't use Java's hashcode since it is allowed to return
  // an equal value for two different objects (sadly unicity isn't
  // guaranteed for Java hashcodes). Therefore, to provide the
  // mapping from the JObj to the CObj, we store a sequence number
  // which is then used as a key to retrieve the CObj's pointer.
  _jni_proxy_Object_set_id(env, jobj, proxy->id);

#if defined(DEBUG_PROXIES)
  proxy->class_name= _get_class_name(env, jobj);
#endif /* DEBUG_PROXIES */

  return proxy;
}

// -----[ _jni_proxy_dump ]------------------------------------------
static void _jni_proxy_dump(_proxy_t * proxy)
{
  fprintf(stderr, "[c-obj:%p/j-obj:%p/id:%lu",
	  proxy->cobj, proxy->jobj, proxy->id);

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "/class:%s", proxy->class_name);
#endif /* DEBUG_PROXIES */

  fprintf(stderr, "]");
}

// -----[ _jni_proxy_c2j_cmp ]---------------------------------------
static int _jni_proxy_c2j_cmp(const void * item1,
			      const void * item2,
			      unsigned int item_size)
{
  _proxy_t * proxy1= (_proxy_t *) item1;
  _proxy_t * proxy2= (_proxy_t *) item2;

  if (proxy1->cobj > proxy2->cobj)
    return 1;
  else if (proxy1->cobj < proxy2->cobj)
    return -1;
  else
    return 0;
}

// -----[ _jni_proxy_j2c_cmp ]---------------------------------------
static int _jni_proxy_j2c_cmp(const void * item1,
			      const void * item2,
			      unsigned int item_size)
{
  _proxy_t * proxy1= (_proxy_t *) item1;
  _proxy_t * proxy2= (_proxy_t *) item2;

  if (proxy1->id > proxy2->id)
    return 1;
  else if (proxy1->id < proxy2->id)
    return -1;
  else
    return 0;
}

// -----[ _jni_proxy_c2j_destroy ]-----------------------------------
static void _jni_proxy_c2j_destroy(void * item)
{
  _proxy_t * proxy= (_proxy_t *) item;

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "debug: invalidate ");
  _jni_proxy_dump(proxy);
  fprintf(stderr, "\n");
#endif /* DEBUG_PROXIES */

  (*jCurrentEnv)->DeleteGlobalRef(jCurrentEnv, proxy->jobj);
  proxy->cobj= NULL;
}

// -----[ _jni_proxy_j2c_destroy ]-----------------------------------
static void _jni_proxy_j2c_destroy(void * item)
{
  _proxy_t * proxy= (_proxy_t *) item;

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "debug: destroy ");
  _jni_proxy_dump(proxy);
  fprintf(stderr, "\n");
#endif /* DEBUG_PROXIES */

  FREE(proxy);
}

// -----[ _jni_proxy_c2j_compute ]-----------------------------------
static uint32_t _jni_proxy_c2j_compute(const void * item,
				       unsigned int hash_size)
{
  _proxy_t * proxy= (_proxy_t *) item;

  return ((unsigned long int) proxy->cobj) % hash_size;
}

// -----[ _jni_proxy_j2c_compute ]-----------------------------------
static uint32_t _jni_proxy_j2c_compute(const void * item,
				       unsigned int hash_size)
{
  _proxy_t * proxy= (_proxy_t *) item;

  return proxy->id % hash_size;
}

// -----[ jni_proxy_add ]--------------------------------------------
void jni_proxy_add(JNIEnv * env, jobject jobj, void * cobj)
{
  _proxy_t * proxy= _jni_proxy_create(env, cobj, jobj);

#if defined(DEBUG_PROXIES)
  fprintf(stderr, "debug: proxy add ");
  _jni_proxy_dump(proxy);
  fprintf(stderr, "\n");
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  if (hash_set_add(_c2java_map, proxy) != proxy) {
    fprintf(stderr, "*** WARNING: same object already referenced (C2J) ! ***\n");
    _jni_proxy_dump(proxy);
    fprintf(stderr, "\n");
    abort();
  }
  if (hash_set_add(_java2c_map, proxy) != proxy) {
    fprintf(stderr, "*** WARNING: same object already referenced (J2C) ! ***\n");
    _jni_proxy_dump(proxy);
    fprintf(stderr, "\n");
    abort();
  }

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy added\n");
  fflush(stderr);
#endif /* DEBUG_PROXIES */
}

// -----[ jni_proxy_remove ]-----------------------------------------
/**
 *
 */
void jni_proxy_remove(JNIEnv * env, jobject jobj)
{
  _proxy_t static_proxy, * proxy;
  //char * pcSearchedClassName;

  static_proxy.id= _jni_proxy_Object_get_id(env, jobj);

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy remove [j-obj:%p/key:%lu]\n",
	  jobj, static_proxy.id);
  fflush(stderr);
#endif /* DEBUG_PROXIES */

  proxy= hash_set_search(_java2c_map, &static_proxy);
  if (proxy == NULL)
    jni_abort(env, "couldn't find a mapping (remove-proxy)");

  /*
    if (!(*jEnv)->IsSameObject(jEnv, joObject, pProxy->joObject))
      jni_abort(jEnv, "not the same object (remove-proxy)");
  */

  jCurrentEnv= env;
  if (_c2java_map != NULL)
    hash_set_remove(_c2java_map, proxy);
  hash_set_remove(_java2c_map, proxy);
  jCurrentEnv= NULL;

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy removed\n");
  fflush(stderr);
#endif /* DEBUG_PROXIES */
}

// -----[ jni_proxy_lookup ]-----------------------------------------
/**
 *
 */
void * jni_proxy_lookup2(JNIEnv * env, jobject jobj,
			 const char * filename, int line)
{
#if defined(DEBUG_PROXIES)
  char * pcSearchedClassName;
#endif /* DEBUG_PROXIES */
  unsigned long int id= _jni_proxy_Object_get_id(env, jobj);
  _proxy_t static_proxy;
  _proxy_t * proxy;

  // If it is a lookup on a NULL pointer, generate a NullPointerException
  if (jni_check_null(env, jobj))
    return NULL;

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy lookup [j-obj:%p/key:%lu]\n", jobj, id);
#endif

#if defined(DEBUG_PROXIES)
  pcSearchedClassName= _get_class_name(env, jobj);
  fprintf(stderr, "  --> class-name: \"%s\"\n", pcSearchedClassName);
  fflush(stderr);
  free(pcSearchedClassName);
#endif /* DEBUG_PROXIES */

  if (_java2c_map == NULL) {
    fprintf(stderr, "Error: proxy lookup while C-BGP not initialized.\n");
    abort();
  }

  static_proxy.id= id;
  proxy= hash_set_search(_java2c_map, &static_proxy);
  if (proxy == NULL) {
    fprintf(stderr, "Error: during lookup, reported ID has no mapping "
	    "(%s,%d)\n", filename, line);
    abort();
  }

  return proxy->cobj;

}

// -----[ jni_proxy_get ]--------------------------------------------
/**
 *
 */
jobject jni_proxy_get2(JNIEnv * env, void * cobj,
		       const char * filename, int line)
{
  _proxy_t static_proxy;
  _proxy_t * proxy;

#ifdef DEBUG_PROXIES
  fprintf(stderr, "debug: proxy get [c-obj:%p]\n", cobj);
#endif

  static_proxy.cobj= cobj;
  proxy= hash_set_search(_c2java_map, &static_proxy);
  if (proxy == NULL) {
#ifdef DEBUG_PROXIES
    fprintf(stderr, "debug: proxy get failed\n");
    fflush(stderr);
#endif /* DEBUG_PROXIES */
    return NULL;
  }

#ifdef DEBUG_PROXIES
  fprintf(stderr, "  --> proxy: ");
  _jni_proxy_dump(proxy);
  fprintf(stderr, "\n");
#endif /* DEBUG_PROXIES */

  /*#if defined(DEBUG_PROXIES)
  if (strcmp(pcSearchedClassName, pHashObject->class_name) != 0) {
    fprintf(stderr, "debug: \033[33;1merror, class-name mismatch\033[0m\n");
    fprintf(stderr, "       --> searched: \"\033[33;1m%s\033[0m\" (key:\033[33;1m%p\033[0m)\n", pcSearchedClassName, sHashObject.pObject);
    fprintf(stderr, "       --> found: \"\033[33;1m%s\033[0m\" (key:\033[33;1m%p\033[0m)\n", pObject->class_name, pHashObject->pObject);
  }
  #endif*/ /* DEBUG_PROXIES */

  /** TODO: need to check if weak reference has not been
   * garbage-collected ! This can be done by relying on the
   * "IsSameObject" method...
   *
   * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
   */
  if ((*env)->IsSameObject(env, proxy->jobj, NULL)) {
    fprintf(stderr, "Error: object was garbage-collected c:%p/j:%p (%s, %d).\n",
	    cobj, proxy->jobj, filename, line);
    abort();
  }

  return proxy->jobj;
}

// -----[ getCBGP ]--------------------------------------------------
/*
 * Class:     be_ac_ucl_ingi_cbgp_ProxyObject
 * Method:    getCBGP
 * Signature: ()Lbe/ac/ucl/ingi/cbgp/CBGP;
 */
JNIEXPORT jobject JNICALL Java_be_ac_ucl_ingi_cbgp_ProxyObject_getCBGP
  (JNIEnv * env, jobject jobj)
{
  return get_Global_CBGP();
}

// -----[ _jni_unregister ]------------------------------------------
/**
 * This is called when a Java proxy object is garbage-collected.
 *
 * This must only happen when the _java2c_map is allocated and the
 * jni_lock_init() has been called. The CBGP.init() function takes
 * care of this.
 */
JNIEXPORT void JNICALL Java_be_ac_ucl_ingi_cbgp_ProxyObject__1jni_1unregister
  (JNIEnv * env, jobject jobj)
{
  jni_lock(env);
  jni_proxy_remove(env, jobj);
  jni_unlock(env);
}


/////////////////////////////////////////////////////////////////////
//
// INITIALIZATION / FINALIZATION OF THE C->JAVA AND JAVA->C MAPPINGS
//
/////////////////////////////////////////////////////////////////////

// -----[ _jni_proxies_init ]----------------------------------------
/**
 * This function initializes the C->Java and Java->C mappings. It can
 * safely be used upon a CBGP JNI session creating or upon the JNI
 * library loading. The CBGP.init() JNI function currently takes care
 * of this call.
 */
void _jni_proxies_init()
{
  /** Note: this hash should be persistent accross multiple C-BGP
   * classes initialization/finalization since proxy objects can be
   * garbage-collected by the Java VM after the C-BGP object has been
   * removed.
   */
  if (_c2java_map == NULL) {
    _c2java_map= hash_set_create(JNI_PROXY_HASH_SIZE,
			      0,
			      _jni_proxy_c2j_cmp,
			      _jni_proxy_c2j_destroy,
			      _jni_proxy_c2j_compute);
  }
  if (_java2c_map == NULL) {
    _java2c_map= hash_set_create(JNI_PROXY_HASH_SIZE,
			      0,
			      _jni_proxy_j2c_cmp,
			      _jni_proxy_j2c_destroy,
			      _jni_proxy_j2c_compute);
  }
}

// -----[ _jni_proxies_invalidate ]----------------------------------
/**
 * This function invalidates all the C->Java mappings. This function
 * must be called at the end of a C-BGP session. The CBGP.destroy()
 * JNI function takes care of this call.
 */
void _jni_proxies_invalidate(JNIEnv * env)
{
  jCurrentEnv= env;
  hash_set_destroy(&_c2java_map);
  jCurrentEnv= NULL;
}

// -----[ _jni_proxies_destroy ]-------------------------------------
/**
 * This function frees all the C->Java and Java->C mappings. This
 * function must only be called when the JNI library is unloaded. The
 * JNI_OnUnload function takes care of this call.
 */
void _jni_proxies_destroy(JNIEnv * env)
{
  jCurrentEnv= env;
  hash_set_destroy(&_c2java_map);
  hash_set_destroy(&_java2c_map);
  jCurrentEnv= NULL;
}


/////////////////////////////////////////////////////////////////////
//
// JNI CRITICAL SECTION MANAGEMENT FUNCTIONS
//
/////////////////////////////////////////////////////////////////////

static jobject joLockObj= NULL;

// -----[ _jni_lock_init ]-------------------------------------------
/**
 * This function initializes the C-BGP JNI locking system. It is
 * responsible for creating a globally reference object which will be
 * used for locking by all the JNI functions. This function can safely
 * be used upon a C-BGP JNI session creation of upon a JNI library
 * loading. The JNI_OnLoad function currently takes care of this call.
 */
void _jni_lock_init(JNIEnv * env)
{
  jclass jcClass;

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: init monitor\n");
  fflush(stderr);
#endif /* DEBUG_LOCK */

  if (joLockObj == NULL) {
    jcClass= (*env)->FindClass(env, "java/lang/Object");
    if (jcClass == NULL)
      jni_abort(env, "could not get class for java/lang/Object (lock-init)");

    joLockObj= (*env)->AllocObject(env, jcClass);
    if (joLockObj == NULL)
      jni_abort(env, "could not allocate Object (lock-init)");

    joLockObj= (*env)->NewGlobalRef(env, joLockObj);
    if (joLockObj == NULL)
      jni_abort(env, "could not create global ref to Object (lock-init)");

#ifdef DEBUG_LOCK
    fprintf(stderr, "debug: global lock created [%p]\n", joLockObj);
    fflush(stderr);
#endif /* DEBUG_LOCK */

  }
}

// -----[ _jni_lock_destroy ]----------------------------------------
/**
 * This function terminates the C-BGP JNI locking system. It removes
 * the global reference towards the dummy monitored object. The
 * JNI_OnUnload function takes care of this call.
 */
void _jni_lock_destroy(JNIEnv * env)
{
  (*env)->DeleteGlobalRef(env, joLockObj);
  joLockObj= NULL;
}

// -----[ jni_lock ]-------------------------------------------------
/**
 * Enters the global JNI monitor.
 *
 * Note: If there is a pending exception, it is cleared before the
 * call to MonitorEnter, then re-raised after the call. If the call
 * to MonitorEnter fails, the program is aborted.
 */
void jni_lock(JNIEnv * env)
{
  jthrowable jtException= NULL;

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor enter [%p] [pid=%d]\n", joLockObj, getpid());
  fflush(stderr);
#endif /* DEBUG_LOCK */

  if (joLockObj == NULL)
    jni_abort(env, "not initialized (lock)");

  // Check if there is a pending exception => clear
  // Note: ExceptionOccurred creates a local ref. to the exception
  jtException= (*env)->ExceptionOccurred(env);
  if (jtException != NULL)
    (*env)->ExceptionClear(env);

  // Enter monitor (failure => abort)
  if ((*env)->MonitorEnter(env, joLockObj) != JNI_OK)
    jni_abort(env, "could not enter JNI monitor (lock)");

  // Re-raise pending exception
  if (jtException != NULL)
    if ((*env)->Throw(env, jtException) < 0)
      jni_abort(env, "could not re-raise pending exception (lock)");

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor entered [pid=%d]\n", getpid());
  fflush(stderr);
#endif /* DEBUG_LOCK */
}

// -----[ jni_unlock ]-----------------------------------------------
/**
 * Exits the global JNI monitor.
 *
 * Note: If there is a pending exception, it is cleared before the
 * call to MonitorEnter, then re-raised after the call. If the call
 * to MonitorEnter fails, the program is aborted.
 */
void jni_unlock(JNIEnv * env)
{
  jthrowable jtException= NULL;

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor exit [%p]\n", joLockObj);
  fflush(stderr);
#endif /* DEBUG_LOCK */

  if (joLockObj == NULL)
    jni_abort(env, "not initialized (unlock)");

  // Check if there is a pending exception => clear
  // Note: ExceptionOccurred creates a local ref. to the exception
  jtException= (*env)->ExceptionOccurred(env);
  if (jtException != NULL)
    (*env)->ExceptionClear(env);

  // Exit monitor (failure => abort)
  if ((*env)->MonitorExit(env, joLockObj) != JNI_OK)
    jni_abort(env, "could not exit JNI monitor (unlock)");

  // Re-raise pending exception
  if (jtException != NULL)
    if ((*env)->Throw(env, jtException) < 0)
      jni_abort(env, "could not re-raise pending exception (unlock)");

#ifdef DEBUG_LOCK
  fprintf(stderr, "debug: monitor exited\n");
  fflush(stderr);
#endif /* DEBUG_LOCK */
}
