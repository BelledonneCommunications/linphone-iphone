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


import net.jxta.discovery.DiscoveryService;
import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredTextDocument;
import net.jxta.document.TextElement;
import net.jxta.exception.JxtaException;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;
import net.jxta.peergroup.PeerGroup;
import net.jxta.pipe.PipeID;
import net.jxta.platform.ModuleClassID;
import net.jxta.platform.ModuleSpecID;
import net.jxta.protocol.ModuleClassAdvertisement;
import net.jxta.protocol.ModuleImplAdvertisement;
import net.jxta.protocol.ModuleSpecAdvertisement;
import net.jxta.protocol.PeerAdvertisement;
import net.jxta.protocol.PipeAdvertisement;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.StringReader;
import java.io.StringWriter;
import java.net.URI;
import java.net.URISyntaxException;


/**
 * @deprecated Will be deprecated soon. Do not use these methods. They contain
 *             a number of incorrect assumption that cannot be corrected while maintaining
 *             backwards compatibility with programs which already use them.
 *             THIS CLASS IS SCHEDULED TO BE REMOVED AFTER 2.5
 */
@Deprecated
public final class AdvertisementUtilities {

    /**
     * Logger
     */
    // private static final transient Logger LOG = Logger.getLogger(AdvertisementUtilities.class.getName());

    public static final StructuredTextDocument STANDARD_COMPATABILITY = (StructuredTextDocument) StructuredDocumentFactory.newStructuredDocument(
            MimeMediaType.XMLUTF8, "Comp");
    public static final String STANDARD_URI = "http://www.jxta.org/download/jxta.jar";
    public static final String STANDARD_PROVIDER = "jxta.org";

    static {
        Element element = STANDARD_COMPATABILITY.createElement("Efmt", "JDK1.4.1");

        STANDARD_COMPATABILITY.appendChild(element);
        element = STANDARD_COMPATABILITY.createElement("Bind", "V2.0 Ref Impl");
        STANDARD_COMPATABILITY.appendChild(element);
    }

    private AdvertisementUtilities() {}

    /**
     * Read a JXTA Advertisement from a File
     *
     * @param fileName The file containing the Advertisement
     * @return An polymorphic Advertisement object
     * @throws JxtaException if Unable to parse the Advertisement
     */
    public static Advertisement readAdvertisementFromFile(String fileName) throws JxtaException {
        return readAdvertisementFromFile(new File(fileName));
    }

    /**
     * Read a JXTA Advertisement from a File
     *
     * @param file The file containing the Advertisement
     * @return An polymorphic Advertisement object
     * @throws JxtaException if Unable to parse the Advertisement
     */
    public static Advertisement readAdvertisementFromFile(File file) throws JxtaException {
        FileInputStream in = null;

        try {
            in = new FileInputStream(file);

            return AdvertisementFactory.newAdvertisement(MimeMediaType.XML_DEFAULTENCODING, in);
        } catch (IOException e) {
            throw new JxtaException("Advertisement Load Failed: " + file, e);
        } catch (Exception e) {
            throw new JxtaException("Advertisement Load Failed: " + file, e);
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException ignored) {// ignored
                }
            }
        }
    }

    /**
     * Save a JXTA Advertisement as an XML Document to a File
     *
     * @param adv      The Advertisement to be saved
     * @param fileName The file to store the Advertisement
     * @throws JxtaException if Unable to parse the Advertisement
     */
    public static void saveAdvertisementToFile(Advertisement adv, String fileName) throws JxtaException {
        saveAdvertisementToFile(adv, new File(fileName));
    }

    /**
     * Save a JXTA Advertisement as an XML Document to a File
     *
     * @param adv  The Advertisement to be saved
     * @param file The file to store the Advertisement
     * @throws JxtaException if Unable to parse the Advertisement
     */
    public static void saveAdvertisementToFile(Advertisement adv, File file) throws JxtaException {
        OutputStream out = null;

        try {
            out = new FileOutputStream(file);
            Document document = adv.getDocument(MimeMediaType.XML_DEFAULTENCODING);

            document.sendToStream(out);
        } catch (IOException e) {} finally {
            if (out != null) {
                try {
                    out.close();
                } catch (IOException ignored) {// ignored
                }
            }
        }
    }

    /**
     * Save a JXTA Advertisement as an XML Document to an array of bytes
     *
     * @param advertisement The Advertisement to be saved
     * @throws JxtaException if Unable to parse the Advertisement
     * @deprecated This method should not be used because it produces a result
     *             who's encoding is not predictable and may (will) differ from JVM to JVM.
     */
    @Deprecated
    public static byte[] advertisementToBytes(Advertisement advertisement) throws JxtaException {
        try {
            Document document = advertisement.getDocument(MimeMediaType.XML_DEFAULTENCODING);
            ByteArrayOutputStream bo = new ByteArrayOutputStream();

            document.sendToStream(bo);

            return bo.toByteArray();
        } catch (IOException e) {
            throw new JxtaException("Error converting a document to bytes", e);
        }
    }

    /**
     * Convert an array of bytes containing an XML encoded String to an JXTA Advertisement
     *
     * @param buf The source of the advertisement
     * @return The Advertisement
     * @deprecated This method should not be used because it interprets the
     *             input using the local default encoding which is not predictable and
     *             may (will) differ from JVM to JVM.
     */
    @Deprecated
    public static Advertisement bytesToAdvertisement(byte buf[]) {
        try {
            InputStream in = new ByteArrayInputStream(buf);
            Advertisement advertisement = AdvertisementFactory.newAdvertisement(MimeMediaType.XML_DEFAULTENCODING, in);

            return advertisement;
        } catch (IOException e) {
            return null;
        } // This will never be thrown
    }

    /**
     * Save a JXTA Advertisement to a String
     *
     * @param advertisement The Advertisement to be converted
     * @param mimeType      Type of document to be created
     */
    public static String advertisementToText(Advertisement advertisement, String mimeType) {
        try {
            StructuredTextDocument doc = (StructuredTextDocument) advertisement.getDocument(new MimeMediaType(mimeType));
            StringWriter stringWriter = new StringWriter();

            doc.sendToWriter(stringWriter);

            return stringWriter.toString();
        } catch (IOException e) {
            return null;
        } // This will never be thrown
    }

    /**
     * Save a JXTA Advertisement to a Plain Text String
     *
     * @param advertisement The Advertisement to be converted
     */
    public static String advertisementToPlainText(Advertisement advertisement) {
        return advertisementToText(advertisement, MimeMediaType.TEXT_DEFAULTENCODING.toString());
    }

    /**
     * Save a JXTA Advertisement to an XML String
     *
     * @param advertisement The Advertisement to be converted
     * @deprecated Equivalent to Advertisement.toString()
     */
    @Deprecated
    public static String advertisementToXmlText(Advertisement advertisement) {
        return advertisementToText(advertisement, MimeMediaType.XMLUTF8.toString());
    }

    /**
     * Convert an array of bytes containing an XML encoded String to an JXTA Advertisement
     *
     * @param xmlTextAsBytes The source of the advertisement
     * @return The Advertisement
     * @throws JxtaException if Unable to parse the Advertisement
     * @deprecated This method should not be used because it produces a result
     *             who's encoding is not predictable and may (will) differ from JVM to JVM.
     */
    @Deprecated
    public static Advertisement newAdvertisementFromXml(byte xmlTextAsBytes[]) throws JxtaException {
        try {
            return AdvertisementFactory.newAdvertisement(MimeMediaType.XML_DEFAULTENCODING
                    ,
                    new ByteArrayInputStream(xmlTextAsBytes));
        } catch (Exception e) {
            throw new JxtaException("Unable to create Advertisement from the provided XML", e);
        }
    }

    /**
     * Convert a String containing an XML encoded String to an JXTA Advertisement
     *
     * @param xmlText The source of the advertisement
     * @return The Advertisement
     * @throws JxtaException if Unable to parse the Advertisement
     * @deprecated This method should not be used because it interprets the
     *             input using the local default encoding which is not precidcatable and
     *             may (will) differ from JVM to JVM.
     */
    @Deprecated
    public static Advertisement newAdvertisementFromXml(String xmlText) throws JxtaException {
        try {
            return AdvertisementFactory.newAdvertisement(MimeMediaType.XML_DEFAULTENCODING, new StringReader(xmlText));
        } catch (Exception e) {
            throw new JxtaException("Unable to create Advertisement from the provided XML", e);
        }
    }

    /**
     * Create a Pipe Advertisement
     *
     * @return A new Pipe Advertisement
     */
    public static PipeAdvertisement createPipeAdvertisement() {
        return (PipeAdvertisement) AdvertisementFactory.newAdvertisement(PipeAdvertisement.getAdvertisementType());
    }

    /**
     * Create a Pipe Advertisement
     *
     * @param pipeId   The pipe ID
     * @param pipeType The type of the Pipe
     * @return A new Pipe Advertisement
     */
    public static PipeAdvertisement createPipeAdvertisement(PipeID pipeId, String pipeType) {
        PipeAdvertisement pipeAdvertisement = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(
                PipeAdvertisement.getAdvertisementType());

        pipeAdvertisement.setPipeID(pipeId);
        pipeAdvertisement.setType(pipeType);
        return pipeAdvertisement;
    }

    /**
     * Create a Pipe Advertisement
     *
     * @param pipeIdText The pipe ID
     * @param pipeType   The type of the Pipe
     * @return A new Pipe Advertisement
     */
    public static PipeAdvertisement createPipeAdvertisement(String pipeIdText, String pipeType) throws JxtaException {
        PipeID pipeId;

        try {
            pipeId = (PipeID) IDFactory.fromURI(new URI(pipeIdText));
        } catch (URISyntaxException failed) {
            IllegalArgumentException failure = new IllegalArgumentException("Bad pipe id");

            failure.initCause(failed);

            throw failure;
        }
        PipeAdvertisement pipeAdvertisement = (PipeAdvertisement) AdvertisementFactory.newAdvertisement(
                PipeAdvertisement.getAdvertisementType());

        pipeAdvertisement.setPipeID(pipeId);
        pipeAdvertisement.setType(pipeType);
        return pipeAdvertisement;
    }

    /**
     * Create a Pipe Advertisement
     *
     * @param root Element containing a Pipe Advertisement
     * @return A new Pipe Advertisement
     */
    public static PipeAdvertisement createPipeAdvertisement(Element root) {
        TextElement pipeAdvElement = (TextElement) DocumentUtilities.getChild(root, PipeAdvertisement.getAdvertisementType());

        if (pipeAdvElement == null) {
            return null;
        }

        return (PipeAdvertisement) AdvertisementFactory.newAdvertisement(pipeAdvElement);
    }

    /**
     * Create a Pipe Advertisement
     *
     * @param peerGroup The peerGroup
     * @param pipeType  The pipeType
     * @return A new Pipe Advertisement
     */
    public static PipeAdvertisement createPipeAdvertisement(PeerGroup peerGroup, String pipeType) {
        PipeID pipeID = IDFactory.newPipeID(peerGroup.getPeerGroupID());

        return createPipeAdvertisement(pipeID, pipeType);
    }

    /**
     * Create a Pipe Advertisement
     *
     * @param pipeID   The pipeID
     * @param pipeType The pipeType
     * @return A new Pipe Advertisement
     */
    public static PipeAdvertisement createPipeAdvertisement(ID pipeID, String pipeType) {
        PipeAdvertisement pipeAdvertisement = createPipeAdvertisement();

        pipeAdvertisement.setPipeID(pipeID);
        pipeAdvertisement.setType(pipeType);
        return pipeAdvertisement;
    }

    /**
     * Create a Pipe Advertisement
     *
     * @param peerGroup The peerGroup
     * @param pipeType  The pipeType
     * @param name      The Pime Name
     * @return A new Pipe Advertisement
     */
    public static PipeAdvertisement createPipeAdvertisement(PeerGroup peerGroup, String sPipeID, String pipeType, String name) throws JxtaException {
        PipeAdvertisement pipeAdvertisement = createPipeAdvertisement(peerGroup, pipeType);
        PipeID pipeId;

        try {
            pipeId = (PipeID) IDFactory.fromURI(new URI(sPipeID));
        } catch (URISyntaxException failed) {
            IllegalArgumentException failure = new IllegalArgumentException("Bad pipe id");

            failure.initCause(failed);

            throw failure;
        }
        pipeAdvertisement.setPipeID(pipeId);
        if (name != null) {
            pipeAdvertisement.setName(name);
        }

        return pipeAdvertisement;
    }

    /**
     * Create a Pipe Advertisement
     *
     * @param root The Root element containing the Advertisement
     * @return A new Pipe Advertisement
     * @deprecated These utilities are too specialized for general use.
     */
    @Deprecated
    public static PipeAdvertisement getPipeAdvertisement(Element root) {
        TextElement pipeAdvElement = (TextElement) DocumentUtilities.getChild(root, PipeAdvertisement.getAdvertisementType());

        if (pipeAdvElement == null) {
            return null;
        }

        return (PipeAdvertisement) AdvertisementFactory.newAdvertisement(pipeAdvElement);
    }

    /**
     * Create a Peer Advertisement
     *
     * @param root The Root element containing the Advertisement
     * @return A new Peer Advertisement
     * @deprecated These utilities are too specialized for general use.
     */
    @Deprecated
    public static PeerAdvertisement getPeerAdvertisement(Element root) {
        TextElement peerAdvElement = (TextElement) DocumentUtilities.getChild(root, PeerAdvertisement.getAdvertisementType());

        if (peerAdvElement == null) {
            return null;
        }

        return (PeerAdvertisement) AdvertisementFactory.newAdvertisement(peerAdvElement);
    }

    /**
     * Create a ModuleClassAdvertisement
     *
     * @param name        The name
     * @param description The description
     * @return An ModuleClassAdvertisement
     */
    public static ModuleClassAdvertisement createModuleClassAdvertisement(String name, String description) {
        String moduleClassAdvertisementType = ModuleClassAdvertisement.getAdvertisementType();
        ModuleClassAdvertisement moduleClassAdvertisement = (ModuleClassAdvertisement) AdvertisementFactory.newAdvertisement(
                moduleClassAdvertisementType);

        moduleClassAdvertisement.setName(name);
        moduleClassAdvertisement.setDescription(description);

        ModuleClassID mcID = IDFactory.newModuleClassID();

        moduleClassAdvertisement.setModuleClassID(mcID);
        return moduleClassAdvertisement;
    }

    /**
     * Create a ModuleSpecAdvertisement
     *
     * @param name  The name
     * @param param The param
     * @return An ModuleSpecAdvertisement
     * @deprecated This implementation incompletely initializes the module
     *             spec advertisement. Consider creating Module Spec Advertisements without
     *             this method.
     */
    @Deprecated
    public static ModuleSpecAdvertisement createModuleSpecAdvertisement(String name, StructuredDocument param) {
        return createModuleSpecAdvertisement(name, null, param);
    }

    /**
     * Create a ModuleSpecAdvertisement
     *
     * @param name                     The name
     * @param moduleClassAdvertisement The moduleClassAdvertisement
     * @param param                    The param
     * @return An ModuleSpecAdvertisement
     * @deprecated This implementation incompletely initializes the module
     *             spec advertisement. Consider creating Module Spec Advertisements without
     *             this method.
     */
    @Deprecated
    public static ModuleSpecAdvertisement createModuleSpecAdvertisement(String name, ModuleClassAdvertisement moduleClassAdvertisement, StructuredDocument param) {
        String moduleSpecAdvertisementType = ModuleSpecAdvertisement.getAdvertisementType();
        ModuleSpecAdvertisement moduleSpecAdvertisement = (ModuleSpecAdvertisement) AdvertisementFactory.newAdvertisement(
                moduleSpecAdvertisementType);

        moduleSpecAdvertisement.setName(name);
        moduleSpecAdvertisement.setVersion("Unknown");
        moduleSpecAdvertisement.setCreator("Unknown");

        if (moduleClassAdvertisement != null) {
            ModuleClassID moduleClassID = moduleClassAdvertisement.getModuleClassID();

            moduleSpecAdvertisement.setModuleSpecID(IDFactory.newModuleSpecID(moduleClassID));
        }

        moduleSpecAdvertisement.setSpecURI("Unknown");

        if (param != null) {
            moduleSpecAdvertisement.setParam(param);
        }

        return moduleSpecAdvertisement;
    }

    /**
     * Publish and advertisement to the Cache
     *
     * @param peerGroup         The peerGroup
     * @param peerAdvertisement The Advertisement
     * @throws JxtaException if Unable to cache the Advertisement
     */
    public static void cachePeerAdvertisement(PeerGroup peerGroup, PeerAdvertisement peerAdvertisement) throws JxtaException {
        cachePeerAdvertisement(peerGroup, peerAdvertisement, DiscoveryService.DEFAULT_EXPIRATION
                ,
                DiscoveryService.DEFAULT_EXPIRATION);
    }

    private static void cachePeerAdvertisement(PeerGroup peerGroup, PeerAdvertisement peerAdvertisement, long lifetime, long lifetimeForOthers) throws JxtaException {
        try {
            DiscoveryService discoveryService = peerGroup.getDiscoveryService();

            if (peerAdvertisement.getPeerID().equals(peerGroup.getPeerID())) {
                return;
            }            // no reason to persist our own peer ID

            discoveryService.publish(peerAdvertisement, lifetime, lifetimeForOthers);
        } catch (IOException e) {
            throw new JxtaException("Unable to cache advertisement", e);
        }
    }

    /**
     * Create a ModuleImplAdvertisement
     *
     * @param specID      The specID
     * @param code        The code
     * @param description the advertisement description
     * @return An ModuleImplAdvertisement
     * @deprecated This implementation initializes some fields of the
     *             resulting ModuleImplAdvertisement to constant values who's value may
     *             not be correct for all circumstances. Consider creating ModuleImpl
     *             Advertisements directly in your application.
     */
    @Deprecated
    public static ModuleImplAdvertisement createModuleImplAdvertisement(ModuleSpecID specID, String code, String description) {

        ModuleImplAdvertisement moduleImplAdvertisement = (ModuleImplAdvertisement)
                AdvertisementFactory.newAdvertisement(ModuleImplAdvertisement.getAdvertisementType());

        moduleImplAdvertisement.setModuleSpecID(specID);
        moduleImplAdvertisement.setCompat(STANDARD_COMPATABILITY);
        moduleImplAdvertisement.setCode(code);
        moduleImplAdvertisement.setUri(STANDARD_URI);
        moduleImplAdvertisement.setProvider(STANDARD_PROVIDER);
        moduleImplAdvertisement.setDescription(description);
        return moduleImplAdvertisement;
    }
}
