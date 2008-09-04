/*
 * Copyright (c) 2004-2007 Sun Microsystems, Inc.  All rights reserved.
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;
import java.util.WeakHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;

import net.jxta.logging.Logging;
import net.jxta.util.SimpleSelectable.IdentityReference;

/**
 * This a tool to implement selectable objects. It may be composed or extended.
 * <p/>
 * {@code SimpleSelectable} objects that are not {@code SimpleSelector} objects
 * only report changes to their listeners.
 * <p/>
 * The listeners of a {@code SimpleSelectable} may be {@code SimpleSelector}
 * objects or other {@code SimpleSelectable} objects. However the method to
 * register non-selectors is and must remain protected since it would allow the
 * connection of arbitrary listeners.
 *
 * @see SimpleSelector
 */

public abstract class AbstractSimpleSelectable implements SimpleSelectable {
    
    /**
     *  Logger
     */
    private final static transient Logger LOG = Logger.getLogger(AbstractSimpleSelectable.class.getName());
    
    /**
     * The identity reference for this selectable.
     *
     * @see SimpleSelectable.IdentityReference
     */
    public final IdentityReference identityReference = new IdentityReference(this);
    
    /**
     * The object that is to reported to listeners as having changed.  When this
     * class is composed rather than extended, "this" is probably not the right
     * choice.
     */
    private final SimpleSelectable srcObject;
    
    /**
     * Registered Change Listeners.
     * <p/>
     * We use a weakHashMap as a Set. The values are never used.
     */
    private final Map<SimpleSelectable, Object> myListeners = new WeakHashMap<SimpleSelectable, Object>(2);
    
    /**
     * Standard constructor for cases where the selectable object is this
     * object.
     */
    public AbstractSimpleSelectable() {
        this.srcObject = this;
    }
    
    /**
     * Standard constructor for cases where the selectable object is some other
     * object.
     *
     * @param srcObject the source object
     */
    public AbstractSimpleSelectable(SimpleSelectable srcObject) {
        this.srcObject = srcObject;
    }
    
    /**
     * {@inheritDoc}
     */
    public IdentityReference getIdentityReference() {
        return identityReference;
    }
    
    /**
     * Tells whether there are registered selectors right now, or not.  A
     * SimpleSelectable that also registers with something else may want to
     * unregister (with the obvious consistency precautions) if it no longer has
     * selectors of its own.
     *
     * @return true if there are listeners.
     */
    protected boolean haveListeners() {
        synchronized(myListeners) {
            return !myListeners.isEmpty();
        }
    }
    
    /**
     * This method takes any listener, not just a SimpleSelector.
     *
     * @param selectable The SimpleSelectable to register
     */
    protected void registerListener(SimpleSelectable selectable) {
        synchronized(myListeners) {
            myListeners.put(selectable, null);
        }
    }
    
    /**
     * This method takes any listener, not just a SimpleSelector.
     *
     * @param selectable The SimpleSelectable to unregister
     */
    protected void unregisterListener(SimpleSelectable selectable) {
        synchronized(myListeners) {
            myListeners.remove(selectable);
        }
    }
    
    /**
     * {@inheritDoc}
     */
    public void register(SimpleSelector simpleSelector) {
        registerListener(simpleSelector);
        simpleSelector.itemChanged(this);
    }
    
    /**
     * {@inheritDoc}
     */
    public void unregister(SimpleSelector simpleSelector) {
        unregisterListener(simpleSelector);
    }
    
    /**
     * This method tells us that something changed and so we need to notify our
     * selectors by invoking their {@code itemChanged()} method. This is
     * normally invoked internally by the implementation. One of the reasons for
     * the implementation to invoke this method is that a SimpleSelectable
     * object that this one is registered with has changed and so has invoked
     * the itemChanged method.  However, the correlation between the two is left
     * up to the implementation.
     * <p/>
     * No external synchronization needed, nor desirable.
     *
     * @return false if there are no selectors left (that's a suggestion for the
     * implementation to use haveListeners and possibly unregister itself).
     */
    protected final boolean notifyChange() {
        Collection<SimpleSelectable> listeners;
        
        synchronized(myListeners) {
            listeners = new ArrayList<SimpleSelectable>(myListeners.keySet());
        }
        
        for (SimpleSelectable listener : listeners) {
            try {
                listener.itemChanged(srcObject);
            } catch(Throwable all) {
                if (Logging.SHOW_SEVERE && LOG.isLoggable(Level.SEVERE)) {
                    LOG.log(Level.SEVERE, "Uncaught Throwable in listener " + listener, all);
                }
            }
        }
        
        return !listeners.isEmpty();
    }
}
