// ==================================================================
// @(#)bgp_Peer.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 11/04/2006
// $Id: bgp_Peer.h,v 1.3 2009-03-25 07:49:45 bqu Exp $
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

#ifndef __JNI_BGP_PEER_H__
#define __JNI_BGP_PEER_H__

#include <jni_md.h>
#include <jni.h>

#include <jni/headers/be_ac_ucl_ingi_cbgp_bgp_Peer.h>

#include <bgp/types.h>

// -----[ cbgp_jni_new_bgp_Peer ]------------------------------------
jobject cbgp_jni_new_bgp_Peer(JNIEnv * jEnv, jobject joCBGP,
			      bgp_peer_t * peer);

#endif /* __JNI_BGP_PEER_H__ */
