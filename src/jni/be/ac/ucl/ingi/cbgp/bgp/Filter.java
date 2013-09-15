// ==================================================================
// @(#)Filter.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/04/2006
// $Id: Filter.java,v 1.4 2009-08-31 09:42:43 bqu Exp $
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

package be.ac.ucl.ingi.cbgp.bgp;

import java.util.Vector;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.ProxyObject;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

public class Filter extends ProxyObject {

	// -----[ constructor ]-----------------------------------------
	protected Filter(CBGP cbgp)	{
		super(cbgp);
	}
	
	// -----[ getRules ]--------------------------------------------
	public native Vector<FilterRule> getRules()
		throws CBGPException;
	
	// -----[ toString ]--------------------------------------------
	public String toString() {
		return "Filter";
	}
	
}
