// ==================================================================
// @(#)TestNode.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestNode.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Node;

public class TestNode {

	CBGP cbgp;
	
	@Before
	public void setUp() {
		cbgp= new CBGP();
		try {
			cbgp.init(null);
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@After
	public void tearDown() {
		try {
			cbgp.destroy();
			cbgp= null;
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@Test
	public void testNormal() {
		try {
			IGPDomain domain= cbgp.netAddDomain(123);
			assertNotNull(domain);
			Node node= domain.addNode("1.0.0.1");
			assertNotNull(node);
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@Test
	public void testName() {
		try {
			IGPDomain domain= cbgp.netAddDomain(123);
			assertNotNull(domain);
			Node node= domain.addNode("1.0.0.1");
			assertNotNull(node);
			// Set name
			final String NODE_NAME= "This is my node"; 
			node.setName(NODE_NAME);
			// Check name
			String name= node.getName();
			assertNotNull(name);
			assertEquals(NODE_NAME, name);
			// Clear name
			node.setName(null);
			// Check name
			name= node.getName();
			assertNull(name);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
	
	@Test
	public void testProtocols() {
		try {
			IGPDomain domain= cbgp.netAddDomain(0);
			assertNotNull(domain);
			Node node= domain.addNode("1.0.0.1");
			assertNotNull(node);
			assertTrue(node.hasProtocol(Node.PROTOCOL_ICMP));
			assertTrue(node.getProtocols().contains(Node.PROTOCOL_ICMP));
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
	
	@Test
	public void testCoordinates() {
		try {
			IGPDomain domain= cbgp.netAddDomain(0);
			assertNotNull(domain);
			Node node= domain.addNode("1.0.0.1");
			assertNotNull(node);
			assertEquals((float) 0, node.getLatitude(), 0.00001);
			assertEquals((float) 0, node.getLongitude(), 0.00001);
			final float LLN_LATITUDE= (float) 50.68333;
			final float LLN_LONGITUDE= (float) 4.61667;
			node.setLatitude(LLN_LATITUDE);
			node.setLongitude(LLN_LONGITUDE);
			assertEquals(LLN_LATITUDE, node.getLatitude(), 0.00001);
			assertEquals(LLN_LONGITUDE, node.getLongitude(), 0.00001);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}
}
