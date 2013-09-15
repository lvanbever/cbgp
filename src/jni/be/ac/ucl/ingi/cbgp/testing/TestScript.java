// ==================================================================
// @(#)TestScript.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestScript.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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
import static org.junit.Assert.fail;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPScriptException;

public class TestScript {

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
	
	private void writeScript(String filename, String script)
		throws IOException
	{
        BufferedWriter out= new BufferedWriter(new FileWriter(filename));
        out.write(script);
        out.close();
	}

	@Test
	public void testSingle() throws CBGPException {
		cbgp.runCmd("net add node 1.0.0.1");
	}
	
	@Test
	public void testBasic() throws IOException {
		final String filename= "/tmp/cbgp-jni-junit-script.cli";
        writeScript(filename,
        		"net add node 1.0.0.1\n"+
        		"net add node 1.0.0.1\n");
        try {
        	cbgp.runScript(filename);
        } catch (CBGPScriptException e) {
        	assertEquals(2, e.getLine());
        	// assertEquals(filename, e.getFileName());
        }
	}
	
	@Test
	public void testInclude() throws IOException {
		final String filename1= "/tmp/cbgp-jni-junit-script1.cli";
		final String filename2= "/tmp/cbgp-jni-junit-script2.cli";
        writeScript(filename1,
        		"net add node 1.0.0.1\n"+
        		"net add node 1.0.0.1\n");
        writeScript(filename2,
        		"include \""+filename1+"\"\n");
        try {
        	cbgp.runScript(filename2);
        } catch (CBGPScriptException e) {
        	assertEquals(1, e.getLine());
        	//assertEquals(filename1, e.getFileName());
        }
	}
	
}
