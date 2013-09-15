// ==================================================================
// @(#)net_Subnet.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 19/10/2007
// $Id: net_Subnet.c,v 1.3 2008-04-11 11:03:06 bqu Exp $
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

#include <jni_md.h>
#include <jni.h>
#include <jni/jni_base.h>
#include <jni/jni_proxies.h>
#include <jni/jni_util.h>
#include <net/subnet.h>

#define CLASS_Subnet "be/ac/ucl/ingi/cbgp/net/Subnet"
#define CONSTR_Subnet "(Lbe/ac/ucl/ingi/cbgp/CBGP;" \
                       "Lbe/ac/ucl/ingi/cbgp/IPPrefix;)V"

// -----[ cbgp_jni_new_net_Subnet ]----------------------------------
/**
 * This function creates a new instance of the Node object from a CBGP
 * node.
 */
jobject cbgp_jni_new_net_Subnet(JNIEnv * jEnv, jobject joCBGP,
				net_subnet_t * subnet)
{
  jobject joSubnet;
  jobject joPrefix;

  /* Java proxy object already existing ? */
  joSubnet= jni_proxy_get(jEnv, subnet);
  if (joSubnet != NULL)
    return joSubnet;

  /* Convert node attributes to Java objects */
  joPrefix= cbgp_jni_new_IPPrefix(jEnv, subnet->prefix);
  if (joPrefix == NULL)
    return NULL;

  /* Create new Subnet object */
  if ((joSubnet= cbgp_jni_new(jEnv, CLASS_Subnet, CONSTR_Subnet,
			      joCBGP,
			      joPrefix)) == NULL)
    return NULL;

  // Add reference into proxy repository
  jni_proxy_add(jEnv, joSubnet, subnet);

  return joSubnet;
}
