package net.jxta.impl.xindice.core;


/*
 * The Apache Software License, Version 1.1
 *
 *
 * Copyright (c) 1999 The Apache Software Foundation.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Xindice" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was
 * originally based on software copyright (c) 1999-2001, The dbXML
 * Group, L.L.C., http://www.dbxmlgroup.com.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 *

 */


/**
 * DBObject is the interface implemented by all Xindice database objects.
 * DBObjects are typically objects that can be managed using XML
 * configuration information, which is typically stored in the system
 * database.  XMLObjects are not considered DBObjects because of the
 * steps involved in having to generate them, which is usually
 * compilation of source code.
 */

public interface DBObject {

    /**
     * create creates a new DBObject and any associated resources for the new
     * DBObject, such as disk files, etc.
     *
     * @return Whether or not the DBObject was created
     * @throws DBException if a DB error occurs
     */
    boolean create() throws DBException;

    /**
     * open opens the DBObject
     *
     * @return Whether or not the DBObject was opened
     * @throws DBException if a DB error occurs
     */
    boolean open() throws DBException;

    /**
     * isOpened returns whether or not the DBObject is opened for business.
     *
     * @return The open status of the DBObject
     * @throws DBException if a DB error occurs
     */
    boolean isOpened() throws DBException;

    /**
     * exists returns whether or not a physical representation of this
     * DBObject actually exists.  In the case of a HashFiler, this would
     * check for the file, and in the case of an FTPFiler, it might
     * perform a connection check.
     *
     * @return Whether or not the physical resource exists
     * @throws DBException if a DB error occurs
     */
    boolean exists() throws DBException;

    /**
     * drop instructs the DBObjectimplementation to remove itself from
     * existence.  The DBObject's parent is responsible for removing any
     * references to the DBObject in its own context.
     *
     * @return Whether or not the DBObject was dropped
     * @throws DBException if a DB error occurs
     */
    boolean drop() throws DBException;

    /**
     * close closes the DBObject
     *
     * @return Whether or not the DBObject was closed
     * @throws DBException if a DB error occurs
     */
    boolean close() throws DBException;
}
