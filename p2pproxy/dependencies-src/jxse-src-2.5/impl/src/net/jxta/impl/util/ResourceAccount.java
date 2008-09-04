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
 *  A descriptor for a resource consumser. The resource consumer's resource
 *  allocation and dynamic usage is tracked.
 */
public interface ResourceAccount {
    
    /**
     * Tear down this account.
     * Releases all reserved resources.
     */
    public void close();
    
    /**
     * Try and grant a new item to this account. If it cannot be done,
     * the account may be eligible for the next available extra item.
     * The account is automatically set to be in need, as if inNeed(true)
     * has been invoked.
     *
     * @return boolean true if an item was granted, false otherwise.
     */
    public boolean obtainItem();
    
    /**
     * Try and grant a certain quantity.
     *
     * <p/>It is useful to manage the allocation of variable sized aggregates
     * when what matters is the cummulated quantity rather than an item
     * count. Quantity could be a number of bytes needed to store
     * something for example. The advantage of using this method rather
     * than obtainItem repeatedly is that it is obvisouly faster if
     * quantity is more than one or two, and also that it is atomic;
     * the entire quantity is either granted or denied.
     * Using this routine is by definition incompatible with the round-robin
     * mode, which could only re-assign quantities of 1.
     *
     * <p/>It is legal to use this routine along with round-robin mode if the
     * same dispatcher is used to manage quantities of 1 in this manner,
     * but an account that has failed to obtain its desired quantity is
     * not queued for later re-assignment. And items released with
     * releaseQuantity() are not re-assigned, so overall it is
     * probably best to not mix the two.
     *
     * @param quantity The number of units wanted. The unit is arbitrary
     * It is only meaningfull to the code that uses this dispatcher.
     * @return boolean whether the requested quantity is authorized.
     */
    public boolean obtainQuantity(long quantity);
    
    /**
     * This will release an item and return the most eligible account to
     * re-use this item for. The account that is returned has been granted
     * the item and thus the invoker is expected to do with this account
     * whatever an invoker of obtainItem() would do in case of success.
     * If the items that are managed are threads, the invoker is
     * likely to be one these threads and it should therefore process
     * the returned account as it did the one for which it was calling
     * releaseItem, however be very carefull not to process the new account
     * in the context of the old one; that would rapidly lead to stack
     * overflow. In other words, be carefull of not making a construct
     * equivalent to:
     *
     * <p/><pre>
     * process() {
     *   doStuff();
     *   myAccount.releaseItem().getUserObject().process();
     * }
     * </pre>
     *
     * That won't work. Instead do:
     *
     * <p/><pre>
     * work() {
     *  while (myAccount != null) {
     *   myAccount.getUserObject().doStuff();
     *   myAccount = myAccount.releaseItem();
     *  }
     * }
     * </pre>
     *
     * <p/>Or similar; always go back to base stack level.
     * It is mandatory to handle accounts returned by {@code releaseItem()}.
     * If handling leads to releaseItem, then it has to be done in a
     * forever loop. That is typical if the items are threads.
     * That is normally not happening if the items are only memory.
     *
     * @return ResourceAccount the account to which the released item
     * has been re-assigned. null if the released item was not re-assigned.
     */
    public ResourceAccount releaseItem();
    
    /**
     * This will release a number of items at once rather than
     * once. To be used in conjunctino with obtainItems(). See that
     * method.
     *
     * @param quantity the number of items to be released.
     */
    public void releaseQuantity(long quantity);
    
    /**
     * Call this with true as soon as this account needs a new item.
     * Call this with false as soon as this account has all that it needs.
     * For proper operation, this must be done.
     *
     * @param needs Whether the account needs a new item or not.
     */
    public void inNeed(boolean needs);
    
    /**
     * @return Object The userObject that was supplied when creating the
     * account.
     */
    public Object getUserObject();
    
    /**
     * Set the userObject associated with that account.
     */
    public void setUserObject(Object obj);
    
    /**
     * Returns the number of reserved items that can still be obtained by
     * this account.
     *
     * <p/>If that number is negative or zero, it means that all reserved
     * items are currently in use. Still more items might still be obtained
     * from the extra items pool.
     *
     * @return long The number of reserved items.
     */
    public long getNbReserved();
    
    /**
     * Tells if this account is idle (that is, none of the resources
     * that it controls are currently in use). This means it can be closed
     * safely.
     */
    public boolean isIdle();
}

