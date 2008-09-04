package net.jxta.impl.xindice.core.indexer;


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

import net.jxta.impl.xindice.core.data.*;
import java.util.*;


/**
 * IndexQuery represents the most primitive form of index querying.
 * Instances of this object should be created by QueryResolvers and
 * cached in Query instances.
 */

public class IndexQuery {
    // No Operator
    public static final int ANY = 0; // Any And All Matches
   
    // Singleton Operators
    public static final int EQ = 1; // Equal To
    public static final int NEQ = -1; // Not Equal To
    public static final int GT = 2; // Greater Than
    public static final int LEQ = -2; // Less Than Or Equal To
    public static final int LT = 3; // Less Than
    public static final int GEQ = -3; // Greater Than Or Equal To
   
    // Range Operators
    public static final int BW = 4; // Between (Inclusive)
    public static final int NBW = -4; // Not Between (Inclusive)
    public static final int BWX = 5; // Between (Exclusive)
    public static final int NBWX = -5; // Not Between (Exclusive)
   
    // Set Operators
    public static final int IN = 6; // In The Set
    public static final int NIN = -6; // Not In The Set
   
    // Other operators
    public static final int SW = 7; // Starts-with
    public static final int NSW = -7; // Not Starts-with
   
    public static final int EW = 8; // Ends-with
    public static final int NEW = -8; // Not Ends-with

    protected int op;
    protected Value[] vals;
   
    public IndexQuery() {
        op = ANY;
    }
   
    public IndexQuery(int op, Value[] vals) {
        this.op = op;
        this.vals = vals;
    }
   
    public IndexQuery(Value[] vals) {
        this(IN, vals);
    }

    public IndexQuery(int op, Value val1) {
        this.op = op;
        if (op == SW || op == NSW) {
            byte[] b = new byte[val1.getLength() + 1];

            System.arraycopy(val1.getData(), 0, b, 0, b.length - 1);
            b[b.length - 1] = 127; // TODO: Must fix this
            Value val2 = new Value(b);

            vals = new Value[] { val1, val2 };
        } else {
            vals = new Value[] { val1 };
        }
    }
      
    public IndexQuery(Value val1) {
        this(EQ, val1);
    }
   
    public IndexQuery(int op, Value val1, Value val2) {
        this.op = op;
        vals = new Value[] { val1, val2 };
    }
   
    public IndexQuery(Value val1, Value val2) {
        this(IN, val1, val2);
    }
   
    public IndexQuery(int op, String val1) {
        this(op, new Value(val1));
    }
   
    public IndexQuery(String val1) {
        this(new Value(val1));
    }
   
    public IndexQuery(int op, String val1, String val2) {
        this(op, new Value(val1), new Value(val2));
    }
   
    public IndexQuery(String val1, String val2) {
        this(new Value(val1), new Value(val2));
    }
   
    /**
     * getOperator returns the operator associated with this query.
     *
     * @return The operator
     */
    public int getOperator() {
        return op;
    }
   
    /**
     * getValue returns one of the Values associated with this query.
     *
     * @param index The Value index
     * @return The request Value
     */
    public final Value getValue(int index) {
        return vals[index];
    }

    /**
     * getValues returns the Values associated with this query.
     *
     * @return The Value set
     */
    public Value[] getValues() {
        return vals;
    }
   
    /**
     * getLength returns the length of the Value set associated with
     * this query.
     *
     * @return The Value set length
     */
    public final int getLength() {
        return vals.length;
    }
   
    /**
     * testValue tests the specified value for validity against this
     * IndexQuery.  The helper classes in org.apache.xindice.core.indexer.helpers
     * should be used for optimized performance.
     *
     * @param value The Value to compare
     * @return Whether or not the value matches
     */
    public boolean testValue(Value value) {
        switch (op) {
        // No Comparison (Any)
        case ANY:
            return true;
            
        // Singleton Comparisons
        case EQ:
            return value.equals(vals[0]);

        case NEQ:
            return !value.equals(vals[0]);

        case GT:
            return value.compareTo(vals[0]) > 0;

        case LEQ:
            return value.compareTo(vals[0]) <= 0;

        case LT:
            return value.compareTo(vals[0]) < 0;

        case GEQ:
            return value.compareTo(vals[0]) >= 0;
            
        // Range Comparisons
        case BW:
            return value.compareTo(vals[0]) >= 0 && value.compareTo(vals[1]) <= 0;

        case NBW:
            return value.compareTo(vals[0]) <= 0 || value.compareTo(vals[1]) >= 0;

        case BWX:
            return value.compareTo(vals[0]) > 0 && value.compareTo(vals[1]) < 0;

        case NBWX:
            return value.compareTo(vals[0]) < 0 || value.compareTo(vals[1]) > 0;
            
        // Set Comparisons
        case IN:
        case NIN:
            return Arrays.binarySearch(vals, value) >= 0 ? op == IN : op == NIN;
            
        // Other comparisons
        case SW:
        case NSW:
            return value.startsWith(vals[0]) ? op == SW : op == NSW;
         
        case EW:
        case NEW:
            return value.endsWith(vals[0]) ? op == EW : op == NEW;
         
        }
        return false;
    }

    /**
     * testValue tests the specified value for validity against this
     * IndexQuery.  The helper classes in org.apache.xindice.core.indexer.helpers
     * should be used for optimized performance.
     *
     * @param value The Value to compare
     * @return Whether or not the value matches
     */
    public final boolean testValue(String value) {
        return testValue(new Value(value));
    }
}

