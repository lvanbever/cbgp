// ==================================================================
// @(#)exceptions.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/12/2007
// $Id: exceptions.c,v 1.2 2009-03-24 15:59:55 bqu Exp $
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

#define _GNU_SOURCE
#include <stdio.h>

#include <assert.h>
#include <jni.h>
#include <stdlib.h>

#include <jni/jni_base.h>

#define CLASS_CBGPException \
  "be/ac/ucl/ingi/cbgp/exceptions/CBGPException"

#define CLASS_CBGPScriptException \
  "be/ac/ucl/ingi/cbgp/exceptions/CBGPScriptException"
#define CONSTR_CBGPScriptException \
  "(Ljava/lang/String;Ljava/lang/String;I)V"

#define CLASS_InvalidDestinationException \
  "be/ac/ucl/ingi/cbgp/exceptions/InvalidDestinationException"
#define CONSTR_InvalidDestinationException \
  "(Ljava/lang/String;)V"

// -----[ _get_throwable ]-------------------------------------------
static inline jthrowable _get_throwable(JNIEnv * jEnv,
					const char * pcClassName,
					const char * pcConstrSign,
					...)
{
  jclass jcExc;
  jmethodID jmExc;
  jthrowable jtExc;
  va_list ap;

  va_start(ap, pcConstrSign);

  jcExc= (*jEnv)->FindClass(jEnv, pcClassName);
  if (jcExc == NULL) {
    fprintf(stderr, "could not get class %s\n", pcClassName);
    (*jEnv)->ExceptionDescribe(jEnv);
    (*jEnv)->FatalError(jEnv, "could not get exception class.");
    return NULL;
  }

  jmExc= (*jEnv)->GetMethodID(jEnv, jcExc, "<init>", pcConstrSign);
  if (jmExc == NULL) {
    fprintf(stderr, "could not get method %s\n", pcConstrSign);
    (*jEnv)->ExceptionDescribe(jEnv);
    (*jEnv)->FatalError(jEnv, "could not get exception constructor.");
    return NULL;
  }

  jtExc= (*jEnv)->NewObjectV(jEnv, jcExc, jmExc, ap);
  va_end(ap);
  if (jtExc == NULL) {
    fprintf(stderr, "could not instanciate %s\n", pcClassName);
    (*jEnv)->ExceptionDescribe(jEnv);
    (*jEnv)->FatalError(jEnv, "could not instanciate exception.");
    return NULL;
  }

  return jtExc;
}


// -----[ throw_CBGPException ]--------------------------------------
/**
 * Throw a CBGPException.
 */
void throw_CBGPException(JNIEnv * jEnv, const char * pcMsg, ...)
{
  jclass jcException;
  char * pcFinalMsg;
  va_list ap;
  va_start(ap, pcMsg);

  // Allocate string dynamically, using malloc()
  vasprintf(&pcFinalMsg, pcMsg, ap);
  assert(pcFinalMsg != NULL);

  jcException= (*jEnv)->FindClass(jEnv, CLASS_CBGPException);
  if (jcException == NULL)
    (*jEnv)->FatalError(jEnv, "could not find class CBGPException");

  if ((*jEnv)->ThrowNew(jEnv, jcException, pcFinalMsg) != 0)
    (*jEnv)->FatalError(jEnv, "could not throw CBGPException");

  free(pcFinalMsg);
}

// -----[ throw_CBGPScriptException ]--------------------------------
/**
 * Throw a CBGPScriptException.
 */
void throw_CBGPScriptException(JNIEnv * jEnv,
			       const char * pcMsg,
			       const char * pcFileName,
			       int iLineNumber)
{
  jstring jsMsg= NULL;
  jstring jsFileName= NULL;
  jthrowable jtException;

  if (pcMsg != NULL) {
    jsMsg= cbgp_jni_new_String(jEnv, pcMsg);
    if (jsMsg == NULL)
      (*jEnv)->FatalError(jEnv, "could not create String");
  }

  if (pcFileName != NULL) {
    jsFileName= cbgp_jni_new_String(jEnv, pcFileName);
    if (jsFileName == NULL)
      (*jEnv)->FatalError(jEnv, "could not create String");
  }

  jtException= _get_throwable(jEnv,
			      CLASS_CBGPScriptException,
			      CONSTR_CBGPScriptException,
			      jsMsg, jsFileName, iLineNumber);

  if ((*jEnv)->Throw(jEnv, jtException) != 0)
    (*jEnv)->FatalError(jEnv, "could not throw "
			CLASS_CBGPScriptException);
}

// -----[ throw_InvalidDestination ]---------------------------------
/**
 * Throw an InvalidDestinationException.
 */
void throw_InvalidDestination(JNIEnv * jEnv, jstring jsDest)
{
  jthrowable jtException= _get_throwable(jEnv,
					 CLASS_InvalidDestinationException,
					 CONSTR_InvalidDestinationException,
					 jsDest);

  if ((*jEnv)->Throw(jEnv, jtException) != 0)
    (*jEnv)->FatalError(jEnv, "could not throw "
			CLASS_InvalidDestinationException);
}
