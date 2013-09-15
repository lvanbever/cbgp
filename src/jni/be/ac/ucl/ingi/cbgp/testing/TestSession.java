// ==================================================================
// @(#)TestSession.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestSession.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;

public class TestSession {

	@Test
	public void testNormal() {
		CBGP cbgp= new CBGP();
		try {
			cbgp.init(null);
			assertTrue(cbgp.getVersion().startsWith("2.0"));
			cbgp.destroy();
		} catch (CBGPException e) {
			fail();
		} 
	}

	@Test
	public void testDuplicateInit() {
		CBGP cbgp= new CBGP();
		try { cbgp.init(null);
		} catch (CBGPException e) { fail(); }
		try {
			cbgp.init(null);
			fail();
		} catch (CBGPException e) {	}
		try { cbgp.destroy();
		} catch (CBGPException e) {	fail(); }
	}
	
	@Test
	public void testMultiple()	{
		CBGP cbgp= new CBGP();
		try {
			cbgp.init(null);
			cbgp.destroy();
			cbgp.init(null);
			cbgp.destroy();
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@Test(expected=NullPointerException.class)
	public void TestNull() {
		CBGP cbgp= null;
		try {
			cbgp.init(null);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
	
}
