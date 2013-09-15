// ==================================================================
// @(#)net_IPTrace.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 05/09/2007
// $Id: net_IPTrace.h,v 1.2 2009-03-25 07:51:59 bqu Exp $
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

#ifndef __JNI_NET_IPTRACE_H__
#define __JNI_NET_IPTRACE_H__

#include <jni_md.h>
#include <jni.h>

#include <net/prefix.h>
#include <net/ip_trace.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ cbgp_jni_new_IPTrace ]-----------------------------------
  jobject cbgp_jni_new_IPTrace(JNIEnv * env, ip_trace_t * trace);
  // -----[ cbgp_jni_new_IPTraces ]------------------------------------
  jobjectArray cbgp_jni_new_IPTraces(JNIEnv * env,
				     ip_trace_t ** traces,
				     unsigned int num_traces);
  
#ifdef __cplusplus
}
#endif

#endif /* __JNI_NET_IPTRACE_H__ */
