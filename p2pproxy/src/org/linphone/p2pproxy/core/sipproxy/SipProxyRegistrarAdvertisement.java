/*
p2pproxy
Copyright (C) 2007  Jehan Monnier ()

P2pUserRegistrationAdvertisement.java - .

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
package org.linphone.p2pproxy.core.sipproxy;


import java.io.Serializable;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Enumeration;

import org.apache.log4j.Logger;




import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.Attributable;
import net.jxta.document.Document;
import net.jxta.document.Element;
import net.jxta.document.ExtendableAdvertisement;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocument;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.TextElement;
import net.jxta.id.ID;
import net.jxta.id.IDFactory;

/**
 * derivated from jxta Advertisement tutorial
 * <pre>
 * &lt;?xml version="1.0"?>
 * &lt;!DOCTYPE jxta:System>
 * &lt;jxta:System xmlns:jxta="http://jxta.org">
 *   &lt;id> &lt;/id>
 *   &lt;Name> &lt;/Name>
 *   &lt;address>address where a sip endpoint can join&lt;/registrar-address>
 * &lt;/jxta:System>
 * </pre>
 */
public class SipProxyRegistrarAdvertisement extends ExtendableAdvertisement implements Comparable, Cloneable, Serializable {
   /**
    * Instantiator
    */
   public static class Instantiator implements AdvertisementFactory.Instantiator {

      /**
       * Returns the identifying type of this Advertisement.
       *
       * @return String the type of advertisement
       */
      public String getAdvertisementType() {
         return SipProxyRegistrarAdvertisement.getAdvertisementType();
      }

      /**
       * Constructs an instance of <CODE>Advertisement</CODE> matching the
       * type specified by the <CODE>advertisementType</CODE> parameter.
       *
       * @return The instance of <CODE>Advertisement</CODE> or null if it
       *         could not be created.
       */
      public Advertisement newInstance() {
         return new SipProxyRegistrarAdvertisement();
      }

      /**
       * Constructs an instance of <CODE>Advertisement</CODE> matching the
       * type specified by the <CODE>advertisementType</CODE> parameter.
       *
       * @param root Specifies a portion of a StructuredDocument which will
       *             be converted into an Advertisement.
       * @return The instance of <CODE>Advertisement</CODE> or null if it
       *         could not be created.
       */
      public Advertisement newInstance(net.jxta.document.Element root) {
         return new SipProxyRegistrarAdvertisement(root);
      }
   }
   private ID mId ;;
   private String mAddress;
   public final static String ADDRESS_TAG = "address";
   private final static String ID_TAG = "ID";
   final static String NAME_TAG = "Name";
   public final static String NAME = "p2p-proxy-proxyregistrar";
   private final static String[] mIndexs = {NAME_TAG};
   private final static Logger mLog = Logger.getLogger(SipProxyRegistrarAdvertisement.class);
   /**
    * 
    */
   public SipProxyRegistrarAdvertisement(Element root) {

      TextElement doc = (TextElement) root;

      if (!getAdvertisementType().equals(doc.getName())) {
         throw new IllegalArgumentException("Could not construct : " + getClass().getName() + "from doc containing a " + doc.getName());
      }
      initialize(doc);

   }
   public SipProxyRegistrarAdvertisement() {

      // TODO Auto-generated constructor stub
   }
   /* (non-Javadoc)
    * @see net.jxta.document.ExtendableAdvertisement#getDocument(net.jxta.document.MimeMediaType)
    */
   @Override
   public Document getDocument(MimeMediaType asMimeType) {

      StructuredDocument adv = StructuredDocumentFactory.newStructuredDocument(asMimeType,
            getAdvertisementType());
      if (adv instanceof Attributable) {
         ((Attributable) adv).addAttribute("xmlns:jxta", "http://jxta.org");
      }
      Element e;
      e = adv.createElement(ID_TAG, getID().toString());
      adv.appendChild(e);
      e = adv.createElement(NAME_TAG, getName().toString());
      adv.appendChild(e);
      e = adv.createElement(ADDRESS_TAG, getAddress().trim());
      adv.appendChild(e);
      return adv;
   }

   public String getName() {
      return NAME;
   }
   @Override
   public ID getID() {
      return mId;
   }

   @Override
   public String[] getIndexFields() {
      return mIndexs;
   }
   public static String getAdvertisementType() {
      return "jxta:" +NAME;
   }
   /* (non-Javadoc)
    * @see net.jxta.document.Advertisement#toString()
    */
   @Override
   public String toString() {
      // TODO Auto-generated method stub
      return super.toString();
   }
   public int compareTo(Object other) {
      return getID().toString().compareTo(other.toString());
   }
   /**
    * Intialize a System advertisement from a portion of a structured document.
    *
    * @param root document root
    */
   protected void initialize(Element root) {
      if (!TextElement.class.isInstance(root)) {
         throw new IllegalArgumentException(getClass().getName() +
               " only supports TextElement");
      }
      TextElement doc = (TextElement) root;
      if (!doc.getName().equals(getAdvertisementType())) {
         throw new IllegalArgumentException("Could not construct : "
               + getClass().getName() + "from doc containing a " +
               doc.getName());
      }
      Enumeration elements = doc.getChildren();
      while (elements.hasMoreElements()) {
         TextElement elem = (TextElement) elements.nextElement();
         if (!handleElement(elem)) {
            mLog.warn("Unhandleded element \'" + elem.getName() + "\' in " +  doc.getName());
         }
      }
   }
   /**
    * Process an individual element from the document.
    *
    * @param elem the element to be processed.
    * @return true if the element was recognized, otherwise false.
    */
   protected boolean handleElement(TextElement elem) {
      if (elem.getName().equals(ID_TAG)) {
         try {
            URI id = new URI(elem.getTextValue());
            setID(IDFactory.fromURI(id));
         } catch (URISyntaxException badID) {
            throw new IllegalArgumentException("unknown ID format in advertisement: " +
                  elem.getTextValue());
         }
         catch (ClassCastException badID) {
            throw new IllegalArgumentException("Id is not a known id type: " +
                  elem.getTextValue());
         }
         return true;
      } else if (elem.getName().equals(ADDRESS_TAG)) {
         setAddress(elem.getTextValue());
         return true;
      } else if (elem.getName().equals(NAME_TAG)) {
         //nop, name is static
         return true;
      } else {
         return false;
      }
   }
   public void setID(ID id) {
      mId = id;
   }
   @Override
   public String getBaseAdvType() {
      // TODO Auto-generated method stub
      return null;
   }
   /**
    * @return Returns the address as a sip uri.
    */
   public String getAddress() {
      return mAddress;
   }
   /**
    * @param name The mName to set.
    */
   public void setAddress(String anAddress) {
      mAddress = anAddress;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj) {

      if (this == obj) {
          return true;
      }
      if (obj instanceof SipProxyRegistrarAdvertisement) {
         SipProxyRegistrarAdvertisement adv = (SipProxyRegistrarAdvertisement) obj;
          return getID().equals(adv.getID());
      }

      return false;
  
   }

}
