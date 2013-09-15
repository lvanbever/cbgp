// ==================================================================
// @(#)exceptions.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/12/2007
// $Id: exceptions.h,v 1.2 2009-03-24 15:59:55 bqu Exp $
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

#ifndef __JNI_EXCEPTIONS_H__
#define __JNI_EXCEPTIONS_H__

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ throw_CBGPException ]------------------------------------
  void throw_CBGPException(JNIEnv * jEnv, const char * pcMsg, ...);
  // -----[ throw_CBGPScriptException ]------------------------------
  void throw_CBGPScriptException(JNIEnv * jEnv,
				 const char * pcMsg,
				 const char * pcFileName,
				 int iLineNumber);
  // -----[ throw_InvalidDestination ]-------------------------------
  void throw_InvalidDestination(JNIEnv * jEnv, jstring jsDest);

#ifdef __cplusplus
}
#endif

#endif /* __JNI_EXCEPTIONS_H__ */
