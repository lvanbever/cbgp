// ==================================================================
// @(#)TestIGPDomain.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestIGPDomain.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

import static org.junit.Assert.*;

import java.util.Vector;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Node;

public class TestIGPDomain {

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
	public void testCreateDomain() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		assertNotNull(domain);
		assertEquals(123, domain.getID());
	}
	
	@Test
	public void testGetDomain() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		assertNotNull(domain);
		assertEquals(123, domain.getID());
		assertEquals(domain, cbgp.netGetDomain(123));
		assertNull(cbgp.netGetDomain(1));
	}
	
	@Test
	public void testGetDomains() throws CBGPException {
		IGPDomain domain1= cbgp.netAddDomain(123);
		IGPDomain domain2= cbgp.netAddDomain(246);
		Vector<IGPDomain> domains= cbgp.netGetDomains();
		assertEquals(domains.size(), (int) 2);
		assert(domains.contains(domain1));
		assert(domains.contains(domain2));
	}
	
	@Test(expected=CBGPException.class)
	public void testCreateDuplicate() throws CBGPException {
		cbgp.netAddDomain(123);
		cbgp.netAddDomain(123);
	}
	
	@Test
	public void testAddNode() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(123);
		Node node1= domain.addNode("1.0.0.0");
		assertNotNull(node1);
		assertEquals("1.0.0.0", node1.getAddress().toString());
		Node node2= domain.addNode("1.0.0.1");
		assertNotNull(node2);
		assertEquals("1.0.0.1", node2.getAddress().toString());
		Vector nodes= domain.getNodes();
		assertNotNull(nodes);
		assertEquals(2, nodes.size());
		assertTrue(nodes.contains(node1));
		assertTrue(nodes.contains(node2));
	}
	
	@Test(expected=CBGPException.class)
	public void testAddNodeDuplicate() throws CBGPException {
		IGPDomain domain= null;
		Node node1= null;
		@SuppressWarnings("unused")
		Node node2= null;
		try {
			domain= cbgp.netAddDomain(123);
			node1= domain.addNode("1.0.0.0");
			assertNotNull(node1);
			assertEquals("1.0.0.0", node1.getAddress().toString());
		} catch (CBGPException e) {
			fail();
		}
		node2= domain.addNode("1.0.0.0");
	}
}
