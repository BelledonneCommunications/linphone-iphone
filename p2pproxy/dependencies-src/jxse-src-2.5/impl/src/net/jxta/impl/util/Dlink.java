/*
 * Copyright (c) 2002-2007 Sun Microsystems, Inc.  All rights reserved.
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

package net.jxta.impl.util;


/**
 * A dlinkable base class.
 * It is far less general than java's LinkedList but permits much better
 * removal performance from the middle of the list because a contained
 * element and the corresponding chaining object are one and the same.
 *
 * The major inconvenient of this class is that it is...a class, not an
 * interface. Making it an interface does not make sense since one would have
 * to re-implement it entirely. There is almost no value added in a List class
 * in addition to a Link class. A list of these Dlink is just a stand-alone
 * Dlink with just a couple of additional convenience methods.
 */

public class Dlink {

    /**
     * Previous element in list.
     */
    private Dlink prev;

    /**
     * Next element in list.
     */
    private Dlink next;

    private void setNext(Dlink n) {
        next = n;
    }

    private void setPrev(Dlink p) {
        prev = p;
    }

    public Dlink next() {
        return next;
    }

    public Dlink prev() {
        return prev;
    }

    public void unlink() {
        next.setPrev(prev);
        prev.setNext(next);
        next = prev = this;
    }

    public boolean isLinked() {
        return next != this;
    }

    public void linkNewNext(Dlink ne) {
        ne.unlink();
        ne.setPrev(this);
        ne.setNext(next);
        next.setPrev(ne);
        next = ne;
    }

    public void linkNewPrev(Dlink ne) {
        ne.unlink();
        ne.setNext(this);
        ne.setPrev(prev);
        prev.setNext(ne);
        prev = ne;
    }

    public Dlink() {
        next = prev = this;
    }
}
