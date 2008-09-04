/*
 * Copyright (c) 2001-2007 Sun Microsystems, Inc.  All rights reserved.
 *  
 *  The Sun Project JXTA(TM) Software License
 *  
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions are met:
 *  
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation 
 *     and/or other materials provided with the distribution.
 *  
 *  3. The end-user documentation included with the redistribution, if any, must 
 *     include the following acknowledgment: "This product includes software 
 *     developed by Sun Microsystems, Inc. for JXTA(TM) technology." 
 *     Alternately, this acknowledgment may appear in the software itself, if 
 *     and wherever such third-party acknowledgments normally appear.
 *  
 *  4. The names "Sun", "Sun Microsystems, Inc.", "JXTA" and "Project JXTA" must 
 *     not be used to endorse or promote products derived from this software 
 *     without prior written permission. For written permission, please contact 
 *     Project JXTA at http://www.jxta.org.
 *  
 *  5. Products derived from this software may not be called "JXTA", nor may 
 *     "JXTA" appear in their name, without prior written permission of Sun.
 *  
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 *  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SUN 
 *  MICROSYSTEMS OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 *  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 *  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 *  JXTA is a registered trademark of Sun Microsystems, Inc. in the United 
 *  States and other countries.
 *  
 *  Please see the license information page at :
 *  <http://www.jxta.org/project/www/license.html> for instructions on use of 
 *  the license in source files.
 *  
 *  ====================================================================
 *  
 *  This software consists of voluntary contributions made by many individuals 
 *  on behalf of Project JXTA. For more information on Project JXTA, please see 
 *  http://www.jxta.org.
 *  
 *  This license is based on the BSD license adopted by the Apache Foundation. 
 */

package net.jxta.impl.id.unknown;


import net.jxta.id.IDFactory;
import java.net.URI;
import java.net.URL;

import java.net.MalformedURLException;

import java.util.logging.Logger;
import java.util.logging.Level;
import net.jxta.logging.Logging;


/**
 *  IDs are used to uniquely identify peers, peer groups, pipes and other
 *  types of objects manipulated by the JXTA APIs.
 *
 *  @see net.jxta.id.IDFactory
 *  @see net.jxta.codat.CodatID
 *  @see net.jxta.peer.PeerID
 *  @see net.jxta.peergroup.PeerGroupID
 *  @see net.jxta.pipe.PipeID
 *  @see net.jxta.platform.ModuleClassID
 *  @see net.jxta.platform.ModuleSpecID
 *
 **/
public final class ID extends net.jxta.id.ID {
    
    /**
     *  Log4J Logger
     **/
    private static final transient Logger LOG = Logger.getLogger(ID.class.getName());
    
    String  unqiueValue;
    
    /**
     *  Constructor for IDs.
     **/
    ID(String value) {
        unqiueValue = value;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public boolean equals(Object target) {
        if (this == target) {
            return true;
        }
        
        if (target instanceof ID) {
            return getUniqueValue().toString().equals(((ID) target).getUniqueValue().toString());
        } else {
            return false;
        }
    }
    ;
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public int hashCode() {
        return getUniqueValue().hashCode();
    }
    ;
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public String getIDFormat() {
        return unqiueValue.substring(0, unqiueValue.indexOf('-'));
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public Object getUniqueValue() {
        return unqiueValue;
    }
    
    /**
     *  {@inheritDoc}
     **/
    @Override
    public URL getURL() {
        return getURL((String) getUniqueValue());
    }
    
    /**
     *  Public member which returns a URI (URL in Java nomenclature) of the ID.
     *
     *  @param	uniqueValue the unique portion of the ID
     *  @return	URL Object containing the URI
     **/
    static URL getURL(String uniqueValue) {
        URL result = null;
        
        // Encode the unique value so that the resulting URN is valid.
        String encoded = sun.net.www.protocol.urn.Handler.encodeURN(uniqueValue);
        
        try {
            result = IDFactory.jxtaURL(ID.URIEncodingName, "", ID.URNNamespace + ":" + encoded);
        } catch (MalformedURLException failed) {
            LOG.log(Level.SEVERE, "Failed to construct URL", failed);
        }
        
        return result;
    }

    /**
     *  {@inheritDoc}
     **/
    @Override
    public URI toURI() {
        return URI.create(ID.URIEncodingName + ":" + ID.URNNamespace + ":" + getUniqueValue());
    }
}
