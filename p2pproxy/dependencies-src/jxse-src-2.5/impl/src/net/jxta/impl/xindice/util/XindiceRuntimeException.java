/*
 * Copyright 1999-2004 The Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * CVS $Id$
 * CVS XindiceRuntimeException.java,v 1.5 2004/02/08 02:59:39 vgritsenko Exp $
 */

package net.jxta.impl.xindice.util;


import java.io.PrintStream;
import java.io.PrintWriter;


/**
 * A XindiceRuntimeException is the base class for all Xindice related RuntimeExceptions.
 *
 * @version CVS $Revision$, $Date$
 */
public class XindiceRuntimeException extends RuntimeException {
    protected Throwable cause;

    public XindiceRuntimeException() {}

    public XindiceRuntimeException(String message) {
        super(message);
    }

    public XindiceRuntimeException(Throwable cause) {
        super();
        this.cause = cause;
    }

    public XindiceRuntimeException(String message, Throwable cause) {
        super(message);
        this.cause = cause;
    }

    @Override
    public void printStackTrace() {
        printStackTrace(System.err);
    }

    @Override
    public void printStackTrace(PrintStream s) {
        super.printStackTrace(s);
        if (this.cause != null) {
            s.print("Caused by: ");
            this.cause.printStackTrace(s);
        }
    }

    @Override
    public void printStackTrace(PrintWriter s) {
        super.printStackTrace(s);
        if (this.cause != null) {
            s.print("Caused by: ");
            this.cause.printStackTrace(s);
        }
    }

    @Override
    public Throwable getCause() {
        return cause;
    }
}
