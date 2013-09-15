// ==================================================================
// @(#)bgp_Filter.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/04/2006
// $Id: bgp_Filter.h,v 1.4 2009-03-25 07:49:45 bqu Exp $
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

#ifndef __JNI_BGP_FILTER_H__
#define __JNI_BGP_FILTER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Filter.h>
#include <bgp/filter/types.h>

// -----[ cbgp_jni_new_bgp_Filter ]------------------------------------
jobject cbgp_jni_new_bgp_Filter(JNIEnv * jEnv, jobject joCBGP,
				bgp_filter_t * filter);

#endif /* __JNI_BGP_FILTER_H__ */
