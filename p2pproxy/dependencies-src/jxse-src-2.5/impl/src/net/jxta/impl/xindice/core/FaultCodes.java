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
package net.jxta.impl.xindice.core;

import java.util.HashMap;
import java.util.Map;

/**
 * FaultCodes defines the Xindice specific fault codes and associated error
 * messages.
 */
public abstract class FaultCodes {

    // the constants below have been pasted from
    // org.apache.xindice.client.corba.db.FaultCodes

    //
    // Constant value
    //
    public static final int GEN = (int) (0l);

    //
    // Constant value
    //
    public static final int OBJ = (int) (100l);

    //
    // Constant value
    //
    public static final int COL = (int) (200l);

    //
    // Constant value
    //
    public static final int IDX = (int) (300l);

    //
    // Constant value
    //
    public static final int TRX = (int) (400l);

    //
    // Constant value
    //
    public static final int DBE = (int) (500l);

    //
    // Constant value
    //
    public static final int QRY = (int) (600l);

    //
    // Constant value
    //
    public static final int SEC = (int) (700l);

    //
    // Constant value
    //
    public static final int URI = (int) (800l);

    //
    // Constant value
    //
    public static final int JAVA = (int) (2000l);

    //
    // Constant value
    //
    public static final int GEN_UNKNOWN = (int) (0l);

    //
    // Constant value
    //
    public static final int GEN_GENERAL_ERROR = (int) (40l);

    //
    // Constant value
    //
    public static final int GEN_CRITICAL_ERROR = (int) (70l);

    //
    // Constant value
    //
    public static final int GEN_FATAL_ERROR = (int) (90l);

    //
    // Constant value
    //
    public static final int OBJ_OBJECT_NOT_FOUND = (int) (100l);

    //
    // Constant value
    //
    public static final int OBJ_METHOD_NOT_FOUND = (int) (101l);

    //
    // Constant value
    //
    public static final int OBJ_NULL_RESULT = (int) (140l);

    //
    // Constant value
    //
    public static final int OBJ_INVALID_RESULT = (int) (141l);

    //
    // Constant value
    //
    public static final int OBJ_DUPLICATE_OBJECT = (int) (142l);

    //
    // Constant value
    //
    public static final int OBJ_RUNTIME_EXCEPTION = (int) (170l);

    //
    // Constant value
    //
    public static final int OBJ_CLASS_FORMAT_ERROR = (int) (171l);

    //
    // Constant value
    //
    public static final int OBJ_INVALID_CONTEXT = (int) (172l);

    //
    // Constant value
    //
    public static final int OBJ_CANNOT_CREATE = (int) (173l);

    //
    // Constant value
    //
    public static final int COL_COLLECTION_NOT_FOUND = (int) (200l);

    //
    // Constant value
    //
    public static final int COL_DOCUMENT_NOT_FOUND = (int) (201l);

    //
    // Constant value
    //
    public static final int COL_DUPLICATE_COLLECTION = (int) (240l);

    //
    // Constant value
    //
    public static final int COL_NULL_RESULT = (int) (241l);

    //
    // Constant value
    //
    public static final int COL_NO_FILER = (int) (242l);

    //
    // Constant value
    //
    public static final int COL_NO_INDEXMANAGER = (int) (242l);

    //
    // Constant value
    //
    public static final int COL_DOCUMENT_MALFORMED = (int) (243l);

    //
    // Constant value
    //
    public static final int COL_CANNOT_STORE = (int) (244l);

    //
    // Constant value
    //
    public static final int COL_CANNOT_RETRIEVE = (int) (245l);

    //
    // Constant value
    //
    public static final int COL_COLLECTION_READ_ONLY = (int) (246l);

    //
    // Constant value
    //
    public static final int COL_COLLECTION_CLOSED = (int) (247l);

    //
    // Constant value
    //
    public static final int COL_CANNOT_CREATE = (int) (270l);

    //
    // Constant value
    //
    public static final int COL_CANNOT_DROP = (int) (271l);

    //
    // Constant value
    //
    public static final int IDX_VALUE_NOT_FOUND = (int) (300l);

    //
    // Constant value
    //
    public static final int IDX_INDEX_NOT_FOUND = (int) (301l);

    //
    // Constant value
    //
    public static final int IDX_MATCHES_NOT_FOUND = (int) (340l);

    //
    // Constant value
    //
    public static final int IDX_DUPLICATE_INDEX = (int) (341l);

    //
    // Constant value
    //
    public static final int IDX_NOT_SUPPORTED = (int) (370l);

    //
    // Constant value
    //
    public static final int IDX_STYLE_NOT_FOUND = (int) (371l);

    //
    // Constant value
    //
    public static final int IDX_CORRUPTED = (int) (372l);

    //
    // Constant value
    //
    public static final int IDX_CANNOT_CREATE = (int) (373l);

    //
    // Constant value
    //
    public static final int TRX_DOC_LOCKED = (int) (400l);

    //
    // Constant value
    //
    public static final int TRX_NO_CONTEXT = (int) (440l);

    //
    // Constant value
    //
    public static final int TRX_NOT_ACTIVE = (int) (441l);

    //
    // Constant value
    //
    public static final int TRX_NOT_SUPPORTED = (int) (470l);

    //
    // Constant value
    //
    public static final int DBE_NO_PARENT = (int) (500l);

    //
    // Constant value
    //
    public static final int DBE_CANNOT_DROP = (int) (570l);

    //
    // Constant value
    //
    public static final int DBE_CANNOT_CREATE = (int) (571l);

    //
    // Constant value
    //
    public static final int QRY_NULL_RESULT = (int) (600l);

    //
    // Constant value
    //
    public static final int QRY_COMPILATION_ERROR = (int) (640l);

    //
    // Constant value
    //
    public static final int QRY_PROCESSING_ERROR = (int) (641l);

    //
    // Constant value
    //
    public static final int QRY_NOT_SUPPORTED = (int) (670l);

    //
    // Constant value
    //
    public static final int QRY_STYLE_NOT_FOUND = (int) (671l);

    //
    // Constant value
    //
    public static final int SEC_INVALID_USER = (int) (770l);

    //
    // Constant value
    //
    public static final int SEC_INVALID_GROUP = (int) (771l);

    //
    // Constant value
    //
    public static final int SEC_INVALID_ACCESS = (int) (772l);

    //
    // Constant value
    //
    public static final int SEC_INVALID_CREDENTIALS = (int) (773l);

    //
    // Constant value
    //
    public static final int URI_EMPTY = (int) (800l);

    //
    // Constant value
    //
    public static final int URI_NULL = (int) (801l);

    //
    // Constant value
    //
    public static final int URI_PARSE_ERROR = (int) (820l);

    //
    // Constant value
    //
    public static final int JAVA_RUNTIME_ERROR = (int) (2070l);

    private static final Map<Integer, String> FaultMsg = new HashMap<Integer, String>();

    private FaultCodes() {}

    static {
        // General errors 0 series
        putCodeMessage(GEN_UNKNOWN, "Unknown");
        putCodeMessage(GEN_GENERAL_ERROR, "General Error");
        putCodeMessage(GEN_CRITICAL_ERROR, "Critical Error");
        putCodeMessage(GEN_FATAL_ERROR, "Fatal Error");

        // XMLObject invocation errors 100 series
        putCodeMessage(OBJ_OBJECT_NOT_FOUND, "XMLObject Not Found");
        putCodeMessage(OBJ_METHOD_NOT_FOUND, "XMLObject Method Not Found");
        putCodeMessage(OBJ_NULL_RESULT, "XMLObject Null Result");
        putCodeMessage(OBJ_INVALID_RESULT, "XMLObject Invalid Result");
        putCodeMessage(OBJ_DUPLICATE_OBJECT, "XMLObject Duplicate Object");
        putCodeMessage(OBJ_RUNTIME_EXCEPTION, "XMLObject Runtime Exception");
        putCodeMessage(OBJ_CLASS_FORMAT_ERROR, "XMLObject Class Format Error");
        putCodeMessage(OBJ_INVALID_CONTEXT, "XMLObject Invalid Context");
        putCodeMessage(OBJ_CANNOT_CREATE, "XMLObject Cannot Create");
      
        // Collection-related errors 200 series
        putCodeMessage(COL_COLLECTION_NOT_FOUND, "Collection Not Found");
        putCodeMessage(COL_DOCUMENT_NOT_FOUND, "Collection Document Not Found");
        putCodeMessage(COL_DUPLICATE_COLLECTION, "Collection Duplicated");
        putCodeMessage(COL_NULL_RESULT, "Collection Null Result");
        putCodeMessage(COL_NO_FILER, "Collection No Filer");
        putCodeMessage(COL_NO_INDEXMANAGER, "Collection No IndexManager");
        putCodeMessage(COL_DOCUMENT_MALFORMED, "Collection Document Malformed");
        putCodeMessage(COL_CANNOT_STORE, "Collection Cannot Store");
        putCodeMessage(COL_CANNOT_RETRIEVE, "Collection Cannot Retrieve");
        putCodeMessage(COL_COLLECTION_READ_ONLY, "Collection Read-only");
        putCodeMessage(COL_COLLECTION_CLOSED, "Collection Closed");
        putCodeMessage(COL_CANNOT_CREATE, "Collection Cannot Create");
        putCodeMessage(COL_CANNOT_DROP, "Collection Cannot Drop");

        // Index-related errors 300 series
        putCodeMessage(IDX_VALUE_NOT_FOUND, "Index Value Not Found");
        putCodeMessage(IDX_INDEX_NOT_FOUND, "Index Not Found");
        putCodeMessage(IDX_MATCHES_NOT_FOUND, "Index Matches Not Found");
        putCodeMessage(IDX_DUPLICATE_INDEX, "Index Duplicate Index");
        putCodeMessage(IDX_NOT_SUPPORTED, "Index Not Supported");
        putCodeMessage(IDX_STYLE_NOT_FOUND, "Index Style Not Found");
        putCodeMessage(IDX_CORRUPTED, "Index Corrupted");
        putCodeMessage(IDX_CANNOT_CREATE, "Index Cannot Create");

        // Transaction-related errors 400 series
        putCodeMessage(TRX_DOC_LOCKED, "Transaction Document Locked");
        putCodeMessage(TRX_NO_CONTEXT, "Transaction No Context");
        putCodeMessage(TRX_NOT_ACTIVE, "Transaction Not Active");
        putCodeMessage(TRX_NOT_SUPPORTED, "Transaction Not Supported");

        // Database-related errors 500 series
        putCodeMessage(DBE_NO_PARENT, "Database No Parent");
        putCodeMessage(DBE_CANNOT_DROP, "Database Cannot Drop");
        putCodeMessage(DBE_CANNOT_CREATE, "Database Cannot Create");

        // Query-related errors 600 series
        putCodeMessage(QRY_NULL_RESULT, "Query Null Result");
        putCodeMessage(QRY_COMPILATION_ERROR, "Query Compilation Error");
        putCodeMessage(QRY_PROCESSING_ERROR, "Query Processing Error");
        putCodeMessage(QRY_NOT_SUPPORTED, "Query Not Supported");
        putCodeMessage(QRY_STYLE_NOT_FOUND, "Query Style Not Found");

        // Security-related errors 700 series
        putCodeMessage(SEC_INVALID_USER, "Security Invalid User");
        putCodeMessage(SEC_INVALID_GROUP, "Security Invalid Group");
        putCodeMessage(SEC_INVALID_ACCESS, "Security Invalid Access");
        putCodeMessage(SEC_INVALID_CREDENTIALS, "Security Invalid Credentials");
      
        // URI-related errors 800 series
        putCodeMessage(URI_EMPTY, "URI Empty");
        putCodeMessage(URI_NULL, "URI Null");
        putCodeMessage(URI_PARSE_ERROR, "URI Parse Error");
      
        // Java-related errors 2000 series
        putCodeMessage(JAVA_RUNTIME_ERROR, "Java Runtime Error");
    }

    private static void putCodeMessage(int code, String message) {
        FaultMsg.put(code, message);
    }

    /**
     * getMessage returns a textual form for the specified fault code.
     *
     * @param code The Fault Code
     * @return It's textual form
     */
    public static String getMessage(int code) {
        String msg = FaultMsg.get(code);

        return msg != null ? msg : "";
    }

    /**
     * getFaultCodeType examines the provided exception to determine
     * the general fault code that is associated with it.  General
     * fault codes are reduced from actual fault codes to be one of
     * the GEN_ prefixed fault code values.
     *
     * @param e The Exception to examine
     * @return The General Fault Code
     */
    public static int getFaultCodeType(Exception e) {
        int code = 0;

        if (e instanceof DBException) {
            code = ((DBException) e).faultCode;
        }
        // Strip it to the General series
        code = code % 100;
        // Narrow to a General value
        code = code - (code % 10);
        return code;
    }

    /**
     * getFaultCodeSeries examines the provided exception to
     * determine the fault code series that is associated with it.
     * Series are reduced from actual fault codes to be one of
     * the fault code prefixes (ex: COL, DB, SEC).
     *
     * @param e The Exception to examine
     * @return The Fault Code Series
     */
    public static int getFaultCodeSeries(Exception e) {
        int code = 0;

        if (e instanceof DBException) {
            code = ((DBException) e).faultCode;
        }
        // Strip it to the series
        code = code - (code % 100);
        return code;
    }

    /**
     * getFaultCode examines the provided exception to determine
     * the fault code that is associated with it.
     *
     * @param e The Exception to examine
     * @return The Fault Code
     */
    public static int getFaultCode(Exception e) {
        if (e instanceof DBException) {
            return ((DBException) e).faultCode;
        } else {
            return 0;
        }
    }

    /**
     * getFaultMessage examines the provided exception to determine
     * the fault message that is associated with it.
     *
     * @param e The Exception to examine
     * @return The Fault Message
     */
    public static String getFaultMessage(Exception e) {
        return getMessage(getFaultCode(e));
    }
}

