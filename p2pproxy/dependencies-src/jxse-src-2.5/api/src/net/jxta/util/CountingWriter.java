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

package net.jxta.util;


import java.io.Writer;
import java.io.FilterWriter;

import java.io.IOException;


/**
 *  A filter writer which counts the characters sent to the writer. A filter
 *  so that you don't have to count seperately from writing to the output.
 **/
public class CountingWriter extends FilterWriter {
    
    /**
     *  For recursion detection to prevent overcounting.
     **/
    private transient boolean alreadycounting;
    
    /**
     *  number of chars which have been written on this stream
     **/
    private long charsWritten;
    
    /**
     * Creates a new instance of CountingWriter
     **/
    public CountingWriter(Writer out) {
        super(out);
        charsWritten = 0;
        alreadycounting = false;
    }
    
    /**
     * {@inheritDoc}
     *
     *  <p/>Debugging toString.
     **/
    @Override
    public String toString() {
        if (null == out) {
            return "closed/" + super.toString();
        } else {
            return out.toString() + "/" + super.toString();
        }
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>Calls the super version of the same method.
     **/
    @Override
    public synchronized void write(int b) throws IOException {
        boolean wascounting = alreadycounting;

        alreadycounting = true;
        super.write(b);
        alreadycounting = wascounting;
        if (!alreadycounting) {
            charsWritten++;
        }
    }
    
    /**
     * {@inheritDoc}
     *
     * <p/>Calls the super version of the same method.
     **/
    @Override
    public synchronized void write(char[] b, int off, int len) throws IOException {
        boolean wascounting = alreadycounting;

        alreadycounting = true;
        super.write(b, off, len);
        alreadycounting = wascounting;
        if (!alreadycounting) {
            charsWritten += len;
        }
    }
    
    /**
     *  Returns the number of chars written to the stream thus far. This and all
     *  the methods in this class are synchronized because bytesWritten cannot be
     *  volatile.
     *
     *  @return long containing the number of bytes written.
     **/
    public synchronized long getCharsWritten() {
        return charsWritten;
    }
}
