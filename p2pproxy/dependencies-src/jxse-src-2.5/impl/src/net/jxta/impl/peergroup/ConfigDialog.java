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
package net.jxta.impl.peergroup;

import java.awt.BorderLayout;
import java.awt.Button;
import java.awt.CardLayout;
import java.awt.Checkbox;
import java.awt.Choice;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.Label;
import java.awt.Panel;
import java.awt.TextField;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.net.InetAddress;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;

import net.jxta.document.Advertisement;
import net.jxta.document.AdvertisementFactory;
import net.jxta.document.MimeMediaType;
import net.jxta.document.StructuredDocumentFactory;
import net.jxta.document.StructuredDocumentUtils;
import net.jxta.document.XMLDocument;
import net.jxta.document.XMLElement;
import net.jxta.endpoint.EndpointAddress;
import net.jxta.peergroup.PeerGroup;
import net.jxta.protocol.TransportAdvertisement;

import net.jxta.exception.JxtaError;
import net.jxta.exception.ConfiguratorException;

import net.jxta.impl.endpoint.IPUtils;
import net.jxta.impl.membership.pse.PSEUtils;
import net.jxta.impl.protocol.HTTPAdv;
import net.jxta.impl.protocol.PSEConfigAdv;
import net.jxta.impl.protocol.PlatformConfig;
import net.jxta.impl.protocol.RdvConfigAdv;
import net.jxta.impl.protocol.RdvConfigAdv.RendezVousConfiguration;
import net.jxta.impl.protocol.RelayConfigAdv;
import net.jxta.impl.protocol.TCPAdv;

/**
 * The standard and much loved AWT Configuration dialog
 */
@SuppressWarnings("serial")
public class ConfigDialog extends Frame {

    static final GridBagConstraints stdConstr;
    static final GridBagConstraints centerConstr;
    static final GridBagConstraints centerLastConstr;
    static final GridBagConstraints fillConstr;
    static final GridBagConstraints fillInsetConstr;

    static {
        stdConstr = new GridBagConstraints();
        stdConstr.gridwidth = GridBagConstraints.REMAINDER;
        stdConstr.gridheight = 1;
        stdConstr.gridx = 0;
        stdConstr.gridy = GridBagConstraints.RELATIVE;
        stdConstr.fill = GridBagConstraints.NONE;
        stdConstr.weightx = 1;
        stdConstr.anchor = GridBagConstraints.NORTHWEST;
        stdConstr.insets = new Insets(0, 0, 0, 0);

        fillConstr = (GridBagConstraints) stdConstr.clone();
        fillConstr.fill = GridBagConstraints.HORIZONTAL;

        centerConstr = (GridBagConstraints) stdConstr.clone();
        centerConstr.anchor = GridBagConstraints.NORTH;

        centerLastConstr = (GridBagConstraints) centerConstr.clone();
        centerLastConstr.weighty = 1;

        fillInsetConstr = (GridBagConstraints) fillConstr.clone();

        fillInsetConstr.insets = new Insets(5, 5, 5, 5);
    }

    // A few widgets.

    /**
     * Grid Bag layout panel
     */
    static class PanelGBL extends Panel {

        protected Insets insets = new Insets(0, 0, 0, 0);

        GridBagLayout lay = new GridBagLayout();

        private static final GridBagConstraints constrLabel = new GridBagConstraints();

        static {
            constrLabel.gridwidth = GridBagConstraints.REMAINDER;
            constrLabel.gridheight = 1;
            constrLabel.gridy = GridBagConstraints.RELATIVE;
            constrLabel.weightx = 1;
            constrLabel.weighty = 1;
            constrLabel.anchor = GridBagConstraints.FIRST_LINE_START;
            constrLabel.fill = GridBagConstraints.HORIZONTAL;
        }

        public PanelGBL(String label) {
            this();
            add(new Label(label, Label.LEFT), constrLabel);
        }

        public PanelGBL() {
            super();
            setLayout(lay);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public Insets getInsets() {
            return insets;
        }
    }


    /**
     * A Grid Bag Panel with a border
     */
    static class BorderPanelGBL extends PanelGBL {

        public static final int NONE = 0;
        public static final int RAISED = 1;
        public static final int LOWERED = 2;
        public static final int GROOVE = 3;
        public static final int BUMP = 4;

        int style = GROOVE;
        String title;
        int ascent = 0;
        int descent = 0;
        int leading = 0;
        int titleWidth = 0;

        public BorderPanelGBL(String title) {
            super();
            this.title = title;
        }

        public BorderPanelGBL(String title, String advice) {
            super(advice);
            this.title = title;
        }

        public BorderPanelGBL(String title, String advice, int s) {
            super(advice);
            this.title = title;
            if ((s < NONE) && (s > BUMP)) {
                return;
            }
            if ((s == RAISED) || (s == LOWERED)) {
                this.title = null;
            }
            style = s;
        }

        private void checkMetrics() {
            Font font = getFont();

            if ((title == null) || (font == null)) {
                ascent = 2;
            } else {
                FontMetrics fmetrics = getFontMetrics(font);

                ascent = fmetrics.getAscent();
                descent = fmetrics.getDescent();
                leading = fmetrics.getLeading();
                titleWidth = fmetrics.stringWidth(title);
            }
            insets = new Insets(descent + ascent + leading + 2, 7, 7, 7);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public Insets getInsets() {
            checkMetrics();
            return insets;
        }

        private void paintLowered(Graphics g) {
            checkMetrics();
            if (ascent == 0) {
                return;
            }

            Dimension d = getSize();

            g.setColor(Color.black);
            g.drawRect(1, ascent - 2, d.width - 4, d.height - ascent);
            g.setColor(Color.white);
            g.drawRect(2, ascent - 1, d.width - 4, d.height - ascent);
            g.setColor(getBackground());
            g.drawRect(2, ascent - 1, d.width - 5, d.height - ascent - 1);
        }

        private void paintRaised(Graphics g) {
            checkMetrics();
            if (ascent == 0) {
                return;
            }

            Dimension d = getSize();

            g.setColor(Color.white);
            g.drawRect(1, ascent - 2, d.width - 4, d.height - ascent);
            g.setColor(Color.black);
            g.drawRect(2, ascent - 1, d.width - 4, d.height - ascent);
            g.setColor(getBackground());
            g.drawRect(2, ascent - 1, d.width - 5, d.height - ascent - 1);
        }

        private void paintGroove(Graphics g) {
            checkMetrics();
            if (ascent == 0) {
                return;
            }

            Dimension d = getSize();

            g.setColor(Color.black);
            g.drawRect(1, ascent - 2, d.width - 4, d.height - ascent);
            g.setColor(Color.white);
            g.drawRect(2, ascent - 1, d.width - 4, d.height - ascent);

            g.setColor(Color.white);
            g.clearRect(10, 0, titleWidth + 6, descent + ascent + leading + 1);
            g.drawString(title, 12, ascent + 1);
            g.setColor(Color.black);
            g.drawString(title, 13, ascent + 2);

            // Work around a bug of at least the awt implem I'm using.
            // A few wild pixels appear on that line during drawstring.
            g.clearRect(0, 0, d.width, 1);
        }

        private void paintBump(Graphics g) {
            checkMetrics();
            if (ascent == 0) {
                return;
            }

            Dimension d = getSize();

            g.setColor(Color.white);
            g.drawRect(1, ascent - 2, d.width - 4, d.height - ascent);
            g.setColor(Color.black);
            g.drawRect(2, ascent - 1, d.width - 4, d.height - ascent);

            g.setColor(Color.white);
            g.clearRect(10, 0, titleWidth + 6, descent + ascent + leading + 1);
            g.drawString(title, 12, ascent + 1);
            g.setColor(Color.black);
            g.drawString(title, 13, ascent + 2);

            // Work around a bug of at least the awt implem I'm using.
            // A few wild pixels appear on that line during drawstring.
            g.clearRect(0, 0, d.width, 1);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void paint(Graphics g) {
            switch (style) {
                case GROOVE:
                    paintGroove(g);
                    break;

                case BUMP:
                    paintBump(g);
                    break;

                case RAISED:
                    paintRaised(g);
                    break;

                case LOWERED:
                    paintLowered(g);
                    break;

                default:
            }
            super.paint(g);
        }
    }


    /**
     * Panel implementing paged tabs.
     */
    static class PagesPanel extends Panel implements ActionListener {
        private final CardLayout layout;
        private final Panel pages;
        private final Panel buttons;

        public PagesPanel() {
            super(new BorderLayout());
            layout = new CardLayout();
            pages = new Panel(layout);
            buttons = new Panel(new FlowLayout(FlowLayout.LEFT, 0, 0));

            add(pages, BorderLayout.CENTER);
            add(buttons, BorderLayout.NORTH);
        }

        /**
         * {@inheritDoc}
         */
        public void actionPerformed(ActionEvent e) {
            layout.show(pages, e.getActionCommand());
        }

        public PanelGBL addPage(String buttonName, String comment) {
            BorderPanelGBL p = new BorderPanelGBL(buttonName, comment, BorderPanelGBL.RAISED);

            pages.add(p, buttonName);
            Button b = new Button(buttonName);

            buttons.add(b);
            b.addActionListener(this);
            return p;
        }

        public void showPage(String pageName) {
            layout.show(pages, pageName);
        }
    }


    /**
     * Allows for entry of a host address and port number. Besides the host
     * address and port number text fields there are two optional features:
     * <p/>
     * <p/><ul>
     * <li>A checkbox with annotation to enable/disable the control.</li>
     * <li>A label for the address.</li>
     * </ul>
     */
    static class HostPortPanel extends Panel implements ItemListener {

        private final Checkbox useMe;

        private Label addressLabel = null;

        private final TextField host;
        private final TextField port;

        HostPortPanel(String checkLabel, String addrLabel, String defaultHost, String defaultPort, boolean defaultState) {

            super(new GridBagLayout());

            useMe = new Checkbox(checkLabel, defaultState);
            host = new TextField(defaultHost, 25);
            port = new TextField(defaultPort, 6);

            GridBagConstraints constraints = new GridBagConstraints();

            constraints.weightx = 1.0;
            constraints.weighty = 1.0;

            constraints.gridx = 0;
            constraints.gridy = 0;
            constraints.gridwidth = (null == addrLabel) ? 2 : 3;
            constraints.anchor = GridBagConstraints.FIRST_LINE_START;

            if (null != checkLabel) {
                add(useMe, constraints);
                // if check label and addr label then use 2 lines.
                if (null != addrLabel) {
                    constraints.gridy++;
                    constraints.gridx = 0;
                    constraints.anchor = GridBagConstraints.LAST_LINE_START;
                } else {
                    constraints.gridx++;
                    constraints.gridx = GridBagConstraints.RELATIVE;
                }
            }

            if (null != addrLabel) {
                constraints.gridwidth = 1;
                addressLabel = new Label(addrLabel, Label.RIGHT);
                add(addressLabel, constraints);
            }

            constraints.gridx = GridBagConstraints.RELATIVE;

            add(host, constraints);
            add(port, constraints);

            setState(defaultState);
            useMe.addItemListener(this);
        }

        /**
         * {@inheritDoc}
         */
        public void itemStateChanged(ItemEvent e) {
            setState(useMe.getState());
        }

        /**
         * {@inheritDoc}
         */
        public boolean getState() {
            return useMe.getState() && isEnabled();
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void setEnabled(boolean enabling) {
            super.setEnabled(enabling);

            useMe.setEnabled(enabling);

            if (null != addressLabel) {
                addressLabel.setEnabled(useMe.getState());
            }

            host.setEnabled(useMe.getState());
            port.setEnabled(useMe.getState());
        }

        /**
         * {@inheritDoc}
         */
        public void setState(boolean state) {
            useMe.setState(state); // sometimes redundant but not always.

            if (null != addressLabel) {
                addressLabel.setEnabled(state);
            }

            host.setEnabled(state);
            port.setEnabled(state);
        }

        /**
         * Returns the value of the host field
         *
         * @return the value of the hot field
         */
        public String getHost() {
            return host.getText().trim();
        }

        /**
         * Returns the value of the port field
         *
         * @return the value of the port field
         */
        public String getPort() {
            return port.getText().trim();
        }
    }


    /**
     * A list of URIs
     */
    static class HostListPanel extends Panel implements ActionListener {

        private final String purpose;
        private final TextField host;
        private final TextField port;
        private final java.awt.List list;
        private final Label listLabel;

        private final Button insert;
        private final Button remove;

        public HostListPanel(String purpose, String lstLabel, boolean defaultState, boolean showPort) {

            super(new GridBagLayout());
            this.purpose = purpose;

            host = new TextField("", showPort ? 25 : 30);
            if (showPort) {
                port = new TextField("", 5);
            } else {
                port = null;
            }
            insert = new Button("+");
            remove = new Button("-");

            list = new java.awt.List(2, true);
            listLabel = new Label(lstLabel);

            GridBagConstraints c1 = new GridBagConstraints();

            c1.gridx = 0;
            c1.gridy = 0;
            c1.anchor = GridBagConstraints.FIRST_LINE_START;
            c1.fill = GridBagConstraints.NONE;
            add(listLabel, c1);

            c1.gridx = 0;
            c1.gridy++;
            if (!showPort) {
                c1.gridwidth = 2;
            }
            c1.weightx = 2.0;
            c1.anchor = GridBagConstraints.LINE_START;
            c1.fill = GridBagConstraints.HORIZONTAL;
            add(host, c1);

            if (showPort) {
                c1.weightx = 0.0;
                c1.gridx = 1;
                c1.anchor = GridBagConstraints.LINE_END;
                c1.fill = GridBagConstraints.NONE;
                add(port, c1);
            }

            c1.gridx = 0;
            c1.gridy++;
            c1.gridwidth = 1;
            c1.weightx = 2.0;
            c1.anchor = GridBagConstraints.LAST_LINE_START;
            c1.fill = GridBagConstraints.HORIZONTAL;
            add(list, c1);

            Panel p2 = new Panel(new GridLayout(2, 1, 1, 1));

            p2.add(insert);
            p2.add(remove);

            c1.gridx++;
            c1.weightx = 0.0;
            c1.anchor = GridBagConstraints.LAST_LINE_END;
            c1.fill = GridBagConstraints.NONE;
            c1.insets = new Insets(0, 4, 0, 1);
            add(p2, c1);

            host.addActionListener(this);
            insert.addActionListener(this);
            remove.addActionListener(this);
            list.addActionListener(this);

            setEnabled(defaultState);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void setEnabled(boolean state) {
            super.setEnabled(state);

            listLabel.setEnabled(state);
            host.setEnabled(state);
            if (null != port) {
                port.setEnabled(state);
            }
            list.setEnabled(state);
            insert.setEnabled(state);
            remove.setEnabled(state);
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean isEnabled() {
            return listLabel.isEnabled();
        }

        /**
         * {@inheritDoc}
         */
        public void actionPerformed(ActionEvent e) {
            if ((insert == e.getSource()) || (host == e.getSource())) {
                StringBuilder addHost = new StringBuilder(host.getText());

                if (null != port) {
                    String portText = port.getText().trim();

                    if (portText.length() > 0) {
                        // if( !verifyPort( "Host port", portText, false ) ) {
                        // return;
                        // }
                        addHost.append(':');
                        addHost.append(portText);
                    }
                }
                if (addItem(addHost.toString())) {
                    host.setText("");
                    host.setCaretPosition(0);
                    if (null != port) {
                        port.setText("");
                        port.setCaretPosition(0);
                    }
                }
                return;
            }

            if (e.getSource() == remove) {
                int[] sel = list.getSelectedIndexes();
                int i = sel.length;

                while (i-- > 0) {
                    list.remove(sel[i]);
                }

                return;
            }

            // double click on a host in the list
            if (e.getSource() == list) {
                String cmd = e.getActionCommand();

                if (null != port) {
                    int colonAt = cmd.lastIndexOf(':');
                    String newHost = cmd.substring(0, colonAt);
                    String newPort = cmd.substring(colonAt + 1);

                    host.setText(newHost);
                    host.setCaretPosition(newHost.length());
                    port.setText(newPort);
                    port.setCaretPosition(newHost.length());
                } else {
                    host.setText(cmd);
                    host.setCaretPosition(cmd.length());
                }
            }
        }

        public boolean addItem(String item) {
            String hostURI = item.trim();

            if (0 == hostURI.trim().length()) {
                return false;
            }

            // See if it is "really" a URI.
            try {
                new URI(hostURI);
            } catch (URISyntaxException failed) {
                return false;
            }

            try {
                while (true) {
                    try {
                        list.remove(hostURI);
                    } catch (IllegalArgumentException notThere) {
                        break;
                    }
                }

                list.add(hostURI);
            } catch (Exception e) {
                return false;
            }

            return true;
        }

        public String getPurpose() {
            return purpose;
        }

        public String[] getItems() {
            return list.getItems();
        }
    }


    /**
     * An interface and port selection panel.
     */
    static class IfAddrPanel extends Panel implements ItemListener {
        private final Checkbox manual;

        private final CardLayout addrLayout;

        private final Panel addrPanel;
        private final TextField interfaceAddr;
        private final TextField localPort;

        private final Choice ips;

        public IfAddrPanel(String defaultInterfaceAddr, String defaultPort) {

            super(new FlowLayout(FlowLayout.LEADING, 0, 0));

            ips = new Choice();
            boolean modeManual = false;

            ips.add("Any/All Local Addresses");

            try {
                Iterator<InetAddress> allIntf = IPUtils.getAllLocalAddresses();
                boolean sawValid = false;

                while (allIntf.hasNext()) {
                    InetAddress anAddr = allIntf.next();

                    if (IPUtils.LOOPBACK.equals(anAddr)) {
                        continue;
                    }

                    ips.add(IPUtils.getHostAddress(anAddr));
                    sawValid = true;
                }

                if (!sawValid) {
                    modeManual = true;
                }

                // if an address was previously configured, switch to manual
                // if we do not find any interface, switch to manual too.
                if (defaultInterfaceAddr != null) {
                    InetAddress defaultIntf = InetAddress.getByName(defaultInterfaceAddr);

                    if (!IPUtils.ANYADDRESS.equals(defaultIntf)) {
                        modeManual = true;

                        // However, if this address is in the automatic list,
                        // switch back to automatic and select it.
                        allIntf = IPUtils.getAllLocalAddresses();

                        while (allIntf.hasNext()) {
                            InetAddress anAddr = allIntf.next();

                            if (defaultIntf.equals(anAddr)) {
                                modeManual = false;
                                ips.select(defaultInterfaceAddr);
                            }
                        }
                    }
                }
            } catch (Exception e) {
                modeManual = true;
            }

            manual = new Checkbox("Manual", null, modeManual);
            add(manual);
            manual.addItemListener(this);

            addrLayout = new CardLayout();
            addrPanel = new Panel(addrLayout);

            Panel autoPanel = new Panel(new FlowLayout(FlowLayout.LEADING));

            autoPanel.add(ips);

            Panel manPanel = new Panel(new FlowLayout(FlowLayout.LEADING));

            interfaceAddr = new TextField(defaultInterfaceAddr, 20);
            manPanel.add(interfaceAddr);

            addrPanel.add(manPanel, "man");
            addrPanel.add(autoPanel, "auto");

            add(addrPanel);

            localPort = new TextField(defaultPort, 6);
            add(localPort);

            setManual(modeManual);
        }

        /**
         * {@inheritDoc}
         */
        private void setManual(boolean manMode) {
            addrLayout.show(addrPanel, manMode ? "man" : "auto");

            this.validate();
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public void setEnabled(boolean enabled) {
            super.setEnabled(enabled);

            manual.setEnabled(enabled);
            ips.setEnabled(enabled);
            interfaceAddr.setEnabled(enabled);
            localPort.setEnabled(enabled);
        }

        /**
         * {@inheritDoc}
         */
        public void itemStateChanged(ItemEvent e) {
            if (e.getSource() == manual) {
                setManual(manual.getState());
            }
        }

        public String getAddress() {
            if (manual.getState()) {
                return interfaceAddr.getText().trim();
            } else {
                return ips.getSelectedItem().trim();
            }
        }

        public String getPort() {
            return localPort.getText().trim();
        }

        public String getMode() {
            return manual.getState() ? "manual" : "auto";
        }

    }


    static final class IPTptPanel extends BorderPanelGBL implements ItemListener {

        enum TransportType {
            TYPE_HTTP, TYPE_TCP
        }


        ;

        private final Checkbox useMe;
        private final Checkbox pubAddrOnly;
        private final Checkbox multicast;
        private final Checkbox clientEnabled;

        private final IfAddrPanel ifAddr;
        private final HostPortPanel publicAddr;

        public IPTptPanel(TransportType type, boolean defaultState, String name, String defaultInterfaceAddr, String defaultPort, boolean clientState, boolean serverState, String defaultPublicAddr, String defaultPublicPort, boolean pubAddrOnlyState) {
            this(type, defaultState, name, defaultInterfaceAddr, defaultPort, clientState, serverState, defaultPublicAddr
                    ,
                    defaultPublicPort, pubAddrOnlyState, false);
        }

        public IPTptPanel(TransportType type, boolean defaultState, String name, String defaultInterfaceAddr, String defaultPort, boolean clientState, boolean serverState, String defaultPublicAddr, String defaultPublicPort, boolean pubAddrOnlyState, boolean multicastState) {

            super(name);

            ifAddr = new IfAddrPanel(defaultInterfaceAddr, defaultPort);

            useMe = new Checkbox("Enabled", null, defaultState);

            if (type == TransportType.TYPE_TCP) {
                multicast = new Checkbox("Multicast", null, multicastState);
            } else {
                multicast = null;
            }

            clientEnabled = new Checkbox("Enable Outgoing connections", null, clientState);

            pubAddrOnly = new Checkbox("Hide private addresses", null, pubAddrOnlyState);

            publicAddr = new HostPortPanel("Enable Incoming Connections", "(Optional) Public address", defaultPublicAddr
                    ,
                    defaultPublicPort, serverState);

            GridBagConstraints constraints = new GridBagConstraints();

            constraints.weightx = 1.0;
            constraints.weighty = 1.0;

            constraints.gridx = 0;
            constraints.gridy = 1;
            constraints.anchor = GridBagConstraints.FIRST_LINE_START;
            add(useMe, constraints);

            if (type == TransportType.TYPE_TCP) {
                constraints.anchor = GridBagConstraints.FIRST_LINE_END;
                add(multicast, constraints);
            }

            constraints.gridx = 0;
            constraints.gridy++;
            constraints.anchor = GridBagConstraints.LINE_START;
            add(ifAddr, constraints);

            constraints.gridx = 0;
            constraints.gridy++;

            constraints.anchor = GridBagConstraints.LINE_START;
            add(clientEnabled, constraints);

            constraints.anchor = GridBagConstraints.EAST;
            add(pubAddrOnly, constraints);

            constraints.gridx = 0;
            constraints.gridy++;

            constraints.anchor = GridBagConstraints.LINE_START;
            add(publicAddr, constraints);
            publicAddr.setState(serverState);

            setState(defaultState);
            useMe.addItemListener(this);
        }

        /**
         * {@inheritDoc}
         */
        public void setState(boolean state) {
            useMe.setState(state);
            ifAddr.setEnabled(state);
            publicAddr.setEnabled(state);
            if (multicast != null) {
                multicast.setEnabled(state);
            }
            clientEnabled.setEnabled(state);
            pubAddrOnly.setEnabled(state);
        }

        /**
         * {@inheritDoc}
         */
        public void itemStateChanged(ItemEvent e) {
            setState(useMe.getState());
        }

        public String getInterfaceAddress() {
            return ifAddr.getAddress().trim();
        }

        public String getConfigMode() {
            return ifAddr.getMode();
        }

        public boolean getPubAddrOnly() {
            return pubAddrOnly.getState();
        }

        public void setPubAddrOnly(boolean state) {
            pubAddrOnly.setState(state);
        }
    }


    /**
     * Manages Peer Identity configuration
     */
    final class IdPanel extends Panel implements ActionListener {

        private final TextField peerName;
        private final TextField passwd;
        private final TextField vpasswd;

        public IdPanel(String defaultPeerName, boolean needSecurityConfig) {

            super(new GridBagLayout());

            peerName = new TextField(defaultPeerName, 20);

            GridBagConstraints constraints = new GridBagConstraints();

            constraints.gridx = 0;
            constraints.gridy = 0;
            constraints.anchor = GridBagConstraints.FIRST_LINE_END;
            add(new Label("Peer Name", Label.RIGHT), constraints);

            constraints.gridx++;
            constraints.anchor = GridBagConstraints.FIRST_LINE_START;
            add(peerName, constraints);

            if (needSecurityConfig) {
                passwd = new TextField("", 20);
                vpasswd = new TextField("", 20);
                passwd.setEchoChar('*');
                vpasswd.setEchoChar('*');

                constraints.gridx = 0;
                constraints.gridy++;
                constraints.anchor = GridBagConstraints.LINE_END;
                add(new Label("Password", Label.RIGHT), constraints);

                constraints.gridx++;
                constraints.anchor = GridBagConstraints.LINE_START;
                add(passwd, constraints);

                constraints.gridx = 0;
                constraints.gridy++;
                constraints.anchor = GridBagConstraints.LINE_END;
                add(new Label("Verify Password", Label.RIGHT), constraints);

                constraints.gridx++;
                constraints.anchor = GridBagConstraints.LINE_START;
                add(vpasswd, constraints);
            } else {
                passwd = null;
                vpasswd = null;
            }
        }

        /**
         * {@inheritDoc}
         */
        @Override
        public String getName() {
            return peerName.getText().trim();
        }

        public String getPassword() {
            return passwd.getText();
        }

        public String getVerifyPassword() {
            return vpasswd.getText();
        }

        public void clearPasswords() {
            passwd.setText("");
            vpasswd.setText("");
        }

        /**
         * {@inheritDoc}
         */
        public void actionPerformed(ActionEvent e) {
        }
    }


    /**
     * Manages Service Enabling
     */
    final static class EnablingPanel extends BorderPanelGBL {

        private final Checkbox isRelay;
        private final Checkbox isRendezvous;
        private final Checkbox isJxmeProxy;

        EnablingPanel(boolean actAsRelay, boolean actAsRendezvous, boolean actAsJxmeProxy) {
            super("Services Settings");

            isRelay = new Checkbox("Act as a Relay", null, actAsRelay);
            isRendezvous = new Checkbox("Act as a Rendezvous", null, actAsRendezvous);
            isJxmeProxy = new Checkbox("Act as a JXME proxy", null, actAsJxmeProxy);

            add(isRelay, stdConstr);
            add(isRendezvous, stdConstr);
            add(isJxmeProxy, stdConstr);
        }
    }


    /**
     * Manages Rendezvous service options
     */
    final static class RdvPanel extends BorderPanelGBL implements ItemListener {

        private final Checkbox useRdv;
        private final Checkbox useOnlySeeds;
        private final HostListPanel seeding;
        private final HostListPanel seeds;

        RdvPanel(boolean useARdv, boolean onlySeeds) {
            super("Rendezvous Settings");

            useRdv = new Checkbox("Use a rendezvous", null, useARdv);
            useOnlySeeds = new Checkbox("Use only configured seeds", null, onlySeeds);
            seeds = new HostListPanel("Seeds", "Rendezvous seed peers", true, false);
            seeding = new HostListPanel("Seeding", "Rendezvous seeding URIs", true, false);

            GridBagConstraints c1 = new GridBagConstraints();

            c1.gridx = 0;
            c1.gridy = 0;
            c1.anchor = GridBagConstraints.LINE_START;
            add(useRdv, c1);
            useRdv.addItemListener(this);

            c1.gridx++;
            c1.anchor = GridBagConstraints.LINE_END;
            add(useOnlySeeds, c1);

            c1.gridx = 0;
            c1.gridy++;
            c1.gridwidth = 2;
            c1.weightx = 1.0;
            c1.fill = GridBagConstraints.HORIZONTAL;
            c1.anchor = GridBagConstraints.LINE_START;
            add(seeding, c1);

            c1.gridy++;
            add(seeds, c1);
        }

        /**
         * {@inheritDoc}
         */
        public void itemStateChanged(ItemEvent e) {
            seeds.setEnabled(useRdv.getState());
            seeding.setEnabled(useRdv.getState());
            useOnlySeeds.setEnabled(useRdv.getState());
        }
    }


    /**
     * Manages relay service parameters
     */
    final static class RelayPanel extends BorderPanelGBL implements ItemListener {

        private final Checkbox useRelay;
        private final Checkbox useOnlySeeds;
        private final HostListPanel seeding;
        private final HostListPanel seeds;

        public RelayPanel(boolean useARelay, boolean onlySeeds) {

            super("Relay Settings");

            useRelay = new Checkbox("Use a relay", null, useARelay);
            useOnlySeeds = new Checkbox("Use only configured seeds", null, onlySeeds);
            useOnlySeeds.setEnabled(useARelay);
            seeds = new HostListPanel("Seeds", "Relay seed peers", useARelay, false);
            seeding = new HostListPanel("Seeding", "Relay seeding URIs", useARelay, false);

            GridBagConstraints c1 = new GridBagConstraints();

            c1.gridx = 0;
            c1.gridy = 0;
            c1.anchor = GridBagConstraints.LINE_START;
            add(useRelay, c1);
            useRelay.addItemListener(this);

            c1.gridx++;
            c1.anchor = GridBagConstraints.LINE_END;
            add(useOnlySeeds, c1);

            c1.gridx = 0;
            c1.gridy++;
            c1.gridwidth = 2;
            c1.weightx = 1.0;
            c1.fill = GridBagConstraints.HORIZONTAL;
            c1.anchor = GridBagConstraints.LINE_START;
            add(seeding, c1);

            c1.gridy++;
            add(seeds, c1);
        }

        /**
         * {@inheritDoc}
         */
        public void itemStateChanged(ItemEvent e) {
            seeds.setEnabled(useRelay.getState());
            seeding.setEnabled(useRelay.getState());
            useOnlySeeds.setEnabled(useRelay.getState());
        }
    }

    private final PlatformConfig configAdv;

    private final Label helpLabel;
    private final IdPanel idPanel;
    private final EnablingPanel enablingPanel;
    private final IPTptPanel tcpPanel;
    private final IPTptPanel httpPanel;
    private final RdvPanel rdvPanel;
    private final RelayPanel relayPanel;

    private final Button ok;
    private final Button cancel;
    private final PagesPanel pages = new PagesPanel();

    boolean done = false;
    boolean canceled = false;

    String tcpMulticastAddr;
    int tcpMulticastPort;
    int tcpMulticastLength;

    public ConfigDialog(PlatformConfig configAdv) throws ConfiguratorException {
        super("JXTA Configurator");

        this.configAdv = configAdv;

        // Identity settings
        String peerName = configAdv.getName();

        if ((null == peerName) || (0 == peerName.trim().length())) {
            peerName = "";
        }

        // Security settings
        boolean needSecurityConfig = true;

        // If security is already in place, then the security info is not shown.
        XMLElement param = (XMLElement) configAdv.getServiceParam(PeerGroup.membershipClassID);

        if (param != null) {
            Advertisement adv = null;

            try {
                adv = AdvertisementFactory.newAdvertisement(param);
            } catch (NoSuchElementException notAnAdv) {
                ; // that's ok.
            } catch (IllegalArgumentException badAdv) {
                ; // that's ok.
            }

            if (adv instanceof PSEConfigAdv) {
                PSEConfigAdv pseConfig = (PSEConfigAdv) adv;

                // no certificate? That means we need to make one.
                needSecurityConfig = (null == pseConfig.getCertificate());
            }
        }

        // JXME Proxy Settings
        boolean isJxmeProxy = false;

        try {
            param = (XMLElement) configAdv.getServiceParam(PeerGroup.proxyClassID);

            if (param != null && configAdv.isSvcEnabled(PeerGroup.proxyClassID)) {
                isJxmeProxy = true;
            }
        } catch (Exception nobigdeal) {
            nobigdeal.printStackTrace();
        }

        int index;

        // TCP Settings
        boolean tcpEnabled;
        boolean clientDefaultT;
        boolean serverDefaultT;
        String defaultInterfaceAddressT;
        String defaultPortT;
        String defaultServerNameT;
        String defaultServerPortT;
        boolean multicastEnabledT;
        boolean noPublicAddressesT;

        try {
            param = (XMLElement) configAdv.getServiceParam(PeerGroup.tcpProtoClassID);

            tcpEnabled = configAdv.isSvcEnabled(PeerGroup.tcpProtoClassID);

            Enumeration<XMLElement> tcpChilds = param.getChildren(TransportAdvertisement.getAdvertisementType());

            // get the TransportAdv from either TransportAdv or tcpAdv
            if (tcpChilds.hasMoreElements()) {
                param = tcpChilds.nextElement();
            } else {
                throw new IllegalStateException("Missing TCP Advertisment");
            }

            TCPAdv tcpAdv = (TCPAdv) AdvertisementFactory.newAdvertisement(param);

            clientDefaultT = tcpAdv.isClientEnabled();
            serverDefaultT = tcpAdv.isServerEnabled();

            defaultInterfaceAddressT = tcpAdv.getInterfaceAddress();

            if ((null == defaultInterfaceAddressT) || (0 == defaultInterfaceAddressT.trim().length())) {
                defaultInterfaceAddressT = null;
            }

            defaultPortT = Integer.toString(tcpAdv.getPort());
            if ((defaultPortT == null) || (0 == defaultPortT.trim().length())) {
                defaultPortT = "9701";
            }

            defaultServerNameT = tcpAdv.getServer();

            if ((null == defaultServerNameT) || (0 == defaultServerNameT.trim().length())) {
                defaultServerNameT = "";
            }

            if (defaultServerNameT != null && (index = defaultServerNameT.lastIndexOf(":")) != -1) {
                if ((0 == index) || (index == defaultServerNameT.length())) {
                    throw new IllegalArgumentException("Bad TCP server name . Cannot proceed.");
                }
                defaultServerPortT = defaultServerNameT.substring(index + 1);
                defaultServerNameT = defaultServerNameT.substring(0, index);
            } else {
                defaultServerNameT = "";
                defaultServerPortT = "9701";
            }

            noPublicAddressesT = tcpAdv.getPublicAddressOnly();
            multicastEnabledT = tcpAdv.getMulticastState();

            // we will just pass these to save.
            tcpMulticastAddr = tcpAdv.getMulticastAddr();
            tcpMulticastPort = tcpAdv.getMulticastPort();
            tcpMulticastLength = tcpAdv.getMulticastSize();
        } catch (Exception failure) {
            throw new ConfiguratorException("Broken Platform Config. Cannot proceed.", failure);
        }

        // HTTP Settings
        boolean httpEnabled;
        boolean clientDefaultH;
        boolean serverDefaultH;
        String defaultInterfaceAddressH;
        String defaultPortH;
        String defaultServerNameH;
        String defaultServerPortH;
        boolean noPublicAddressesH;

        try {
            param = (XMLElement) configAdv.getServiceParam(PeerGroup.httpProtoClassID);

            httpEnabled = configAdv.isSvcEnabled(PeerGroup.httpProtoClassID);

            Enumeration<XMLElement> httpChilds = param.getChildren(TransportAdvertisement.getAdvertisementType());

            // get the TransportAdv from either TransportAdv
            if (httpChilds.hasMoreElements()) {
                param = httpChilds.nextElement();
            } else {
                throw new IllegalStateException("Missing HTTP Advertisment");
            }

            // Read-in the adv as it is now.
            HTTPAdv httpAdv = (HTTPAdv) AdvertisementFactory.newAdvertisement(param);

            clientDefaultH = httpAdv.isClientEnabled();
            serverDefaultH = httpAdv.isServerEnabled();

            defaultInterfaceAddressH = httpAdv.getInterfaceAddress();

            if ((null == defaultInterfaceAddressH) || (0 == defaultInterfaceAddressH.trim().length())) {
                defaultInterfaceAddressH = null;
            }

            defaultPortH = Integer.toString(httpAdv.getPort());

            if ((defaultPortH == null) || (0 == defaultPortH.trim().length())) {
                defaultPortH = "9700";
            }

            defaultServerNameH = httpAdv.getServer();

            if ((null != defaultServerNameH) && (0 == defaultServerNameH.trim().length())) {
                defaultServerNameH = "";
            }

            defaultServerPortH = "9700";

            if (defaultServerNameH != null && (index = defaultServerNameH.lastIndexOf(":")) != -1) {
                if ((0 == index) || (index == defaultServerNameH.length())) {
                    throw new IllegalArgumentException("Bad HTTP server name. Cannot proceed.");
                }
                defaultServerPortH = defaultServerNameH.substring(index + 1);
                defaultServerNameH = defaultServerNameH.substring(0, index);
            } else {
                defaultServerNameH = "";
                defaultServerPortH = "9700";
            }

            noPublicAddressesH = httpAdv.getPublicAddressOnly();
        } catch (Exception failure) {
            throw new ConfiguratorException("Broken Platform Config. Cannot proceed.", failure);
        }

        // Rendezvous Settings
        boolean isRendezvous;
        boolean isAdhoc;
        boolean onlySeeds;
        List<String> seedRdvs = new ArrayList<String>();
        List<String> seedingRdvs = new ArrayList<String>();

        try {
            RdvConfigAdv rdvConfigAdv;

            param = (XMLElement) configAdv.getServiceParam(PeerGroup.rendezvousClassID);

            rdvConfigAdv = (RdvConfigAdv) AdvertisementFactory.newAdvertisement(param);

            isRendezvous = (RendezVousConfiguration.RENDEZVOUS == rdvConfigAdv.getConfiguration());

            isAdhoc = (RendezVousConfiguration.AD_HOC == rdvConfigAdv.getConfiguration());

            onlySeeds = rdvConfigAdv.getUseOnlySeeds();

            for (URI uri : Arrays.asList(rdvConfigAdv.getSeedRendezvous())) {
                seedRdvs.add(uri.toString());
            }

            for (URI uri1 : Arrays.asList(rdvConfigAdv.getSeedingURIs())) {
                seedingRdvs.add(uri1.toString());
            }
        } catch (Exception failure) {
            throw new ConfiguratorException("Broken Platform Config. Cannot proceed.", failure);
        }

        // Relay Settings
        boolean isRelay;
        boolean useRelay;
        boolean useOnlySeedRelays;
        List<String> seedRelays = new ArrayList<String>();
        List<String> seedingRelays = new ArrayList<String>();

        try {
            param = (XMLElement) configAdv.getServiceParam(PeerGroup.relayProtoClassID);

            RelayConfigAdv relayConfig = (RelayConfigAdv) AdvertisementFactory.newAdvertisement(param);

            isRelay = relayConfig.isServerEnabled();

            useRelay = relayConfig.isClientEnabled();

            useOnlySeedRelays = relayConfig.getUseOnlySeeds();

            for (EndpointAddress endpointAddress : Arrays.asList(relayConfig.getSeedRelays())) {
                seedRelays.add(endpointAddress.toString());
            }

            for (URI uri : Arrays.asList(relayConfig.getSeedingURIs())) {
                seedingRelays.add(uri.toString());
            }
        } catch (Exception failure) {
            throw new ConfiguratorException("Broken Platform Config. Cannot proceed.", failure);
        }

        // BEGIN BUILDING UI

        GridBagLayout layout = new GridBagLayout();

        setLayout(layout);

        addWindowListener(new WindowAdapter() {

            @Override
            public void windowClosing(WindowEvent e) {
                beCanceled();
            }
        });

        helpLabel = new Label("See \"http://jxta-jxse.dev.java.net/confighelp.html\" for config help", Label.CENTER);
        helpLabel.setBackground(new Color(220, 220, 220));
        helpLabel.setForeground(Color.black);

        helpLabel.addMouseListener(new MouseAdapter() {

            @Override
            public void mouseClicked(MouseEvent e) {
                helpLabel.setForeground(Color.black);
                helpLabel.setText("See \"http://jxta-jxse.dev.java.net/confighelp.html\" for config help");
            }
        });

        idPanel = new IdPanel(peerName, needSecurityConfig);

        enablingPanel = new EnablingPanel(isRelay, isRendezvous, isJxmeProxy);

        tcpPanel = new IPTptPanel(IPTptPanel.TransportType.TYPE_TCP, tcpEnabled, "TCP Settings", defaultInterfaceAddressT,
                defaultPortT, clientDefaultT, serverDefaultT, defaultServerNameT, defaultServerPortT, noPublicAddressesT,
                multicastEnabledT);

        httpPanel = new IPTptPanel(IPTptPanel.TransportType.TYPE_HTTP, httpEnabled, "HTTP Settings", defaultInterfaceAddressH,
                defaultPortH, clientDefaultH, serverDefaultH, defaultServerNameH, defaultServerPortH, noPublicAddressesH);

        rdvPanel = new RdvPanel(!isAdhoc, onlySeeds);

        // add the relays

        for (Object seedRdv : seedRdvs) {
            rdvPanel.seeds.addItem((String) seedRdv);
        }

        for (Object seedingRdv : seedingRdvs) {
            rdvPanel.seeding.addItem((String) seedingRdv);
        }

        relayPanel = new RelayPanel(useRelay, useOnlySeedRelays);

        // add the relays
        for (Object seedRelay : seedRelays) {
            relayPanel.seeds.addItem((String) seedRelay);
        }

        for (Object seedingRelay : seedingRelays) {
            relayPanel.seeding.addItem((String) seedingRelay);
        }

        ok = new Button("  OK  ");

        ok.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                if (verifyInput()) {
                    if (saveValues()) {
                        beDone();
                    } else {
                        beCanceled();
                    }
                }
            }
        });

        cancel = new Button("Cancel");
        cancel.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                beCanceled();
            }
        });

        Panel okPanel = new Panel();

        okPanel.add(ok);
        okPanel.add(cancel);

        // build basic panel
        Panel basicsPanel = pages.addPage("Basic", "Basic settings");

        GridBagConstraints centerWConstr = (GridBagConstraints) centerConstr.clone();

        centerWConstr.weighty = 1;

        basicsPanel.add(idPanel, centerWConstr);

        // build Advanced panel
        Panel advancedPanel = pages.addPage("Advanced", "Experienced Users Only");

        advancedPanel.add(enablingPanel, fillInsetConstr);
        advancedPanel.add(tcpPanel, fillInsetConstr);
        advancedPanel.add(httpPanel, fillInsetConstr);

        Panel proxyRdvRelayPanel = pages.addPage("Rendezvous/Relays", "Experienced Users Only");

        proxyRdvRelayPanel.add(rdvPanel, fillInsetConstr);
        proxyRdvRelayPanel.add(relayPanel, fillInsetConstr);

        add(helpLabel, fillConstr);
        add(pages, fillInsetConstr);
        add(okPanel, centerLastConstr);

        pack();
        setVisible(true);
    }

    public synchronized boolean untilDone() {
        try {
            while (!done) {
                wait();
            }
        } catch (InterruptedException e) {
            Thread.interrupted();
        }

        if (canceled) {
            throw new JxtaError("Canceled during configuration");
        }
        return (done);
    }

    private synchronized boolean beDone() {
        done = true;
        notify();
        dispose();

        return canceled;
    }

    private synchronized boolean beCanceled() {
        canceled = true;
        done = true;
        notify();
        dispose();

        return canceled;
    }

    private boolean verifyPort(String portName, String ports, boolean dynamicok) {
        int p1;

        if ((null == ports) || (0 == ports.trim().length())) {
            ports = "0";
        }

        try {
            p1 = Integer.parseInt(ports);
        } catch (Exception ex) {
            helpLabel.setForeground(Color.red.darker());
            helpLabel.setText(portName + " port number must be an integer: " + ports);
            return false;
        }
        if ((p1 > 65535) || (p1 < (dynamicok ? 0 : 1))) {
            helpLabel.setForeground(Color.red.darker());
            helpLabel.setText(
                    portName + " port number must be an integer between " + (dynamicok ? "0" : "1") + " and 65535, found " + p1);
            return false;
        }
        return true;
    }

    private boolean verifyAddr(String proto, boolean serverOn, String localPort, String publicAddress, String publicPort) {

        // if a public name is specified, check its port.
        if (serverOn && (publicAddress.length() > 0)) {
            if (!verifyPort(proto + " local", localPort, false)) {
                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Dynamic tcp port selection not supported when server public address is specified.");
                pages.showPage("Advanced");
                return false;
            }

            if (!verifyPort(proto + " public", publicPort, false)) {
                pages.showPage("Advanced");
                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Dynamic tcp port selection not supported for server public address.");
                return false;
            }
        } else if (!verifyPort(proto + " local", localPort, true)) {
            pages.showPage("Advanced");
            return false;
        }

        return true;
    }

    private boolean verifyInput() {

        if (0 == idPanel.getName().trim().length()) {
            helpLabel.setForeground(Color.red.darker());
            helpLabel.setText("A peer name is required.");
            pages.showPage("Basic");
            return false;
        }

        // Verify security parameters if we are not initialized
        // Password and principal
        if (null != idPanel.passwd) {
            String passwd = idPanel.getPassword();
            String vpasswd = idPanel.getVerifyPassword();

            // Verify password
            // must be at least 8 chars a la unix
            if (passwd.length() < 8) {
                // Clear password text boxes
                idPanel.clearPasswords();

                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Passwords must be at least 8 characters");
                pages.showPage("Basic");
                return false;
            }

            // must be identical
            if (!passwd.equals(vpasswd)) {
                // Clear password text boxes
                idPanel.clearPasswords();

                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Password does not match Verify Password");
                pages.showPage("Basic");
                return false;
            }
        }

        // make sure *some* transport is enabled.
        if ((!(httpPanel.useMe.getState())) && (!(tcpPanel.useMe.getState()))) {
            helpLabel.setForeground(Color.red.darker());
            helpLabel.setText("At least one of TCP or HTTP must be enabled.");
            pages.showPage("Advanced");
            return false;
        }

        // http settings
        if (httpPanel.useMe.getState()) {
            // make sure at least incoming or outgoing enabled.
            if (!httpPanel.clientEnabled.getState() && !httpPanel.publicAddr.getState()) {
                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Must enable incoming and/or outcoming to enable HTTP");
                pages.showPage("Advanced");
                return false;
            }

            // Check the http port fields.
            boolean valid = verifyAddr("HTTP", httpPanel.publicAddr.getState(), httpPanel.ifAddr.getPort(),
                    httpPanel.publicAddr.getHost(), httpPanel.publicAddr.getPort());

            if (!valid) {
                return false;
            }
        }

        // tcp settings
        if (tcpPanel.useMe.getState()) {
            // make sure at least incoming or outgoing enabled.
            if (!tcpPanel.clientEnabled.getState() && !tcpPanel.publicAddr.getState() && !tcpPanel.multicast.getState()) {
                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Must enable at least one of incoming, outcoming or multicast to enable TCP");
                pages.showPage("Advanced");
                return false;
            }

            // Check the tcp port fields.
            boolean valid = verifyAddr("TCP", tcpPanel.publicAddr.getState(), tcpPanel.ifAddr.getPort(),
                    tcpPanel.publicAddr.getHost(), tcpPanel.publicAddr.getPort());

            if (!valid) {
                return false;
            }
        }

        if (!relayPanel.useRelay.getState() && (!httpPanel.useMe.getState() || !httpPanel.publicAddr.getState())
                && (!tcpPanel.useMe.getState() || !tcpPanel.publicAddr.getState())) {
            helpLabel.setForeground(Color.red.darker());
            helpLabel.setText("Must use Relay if incoming not enabled for TCP and/or HTTP");
            pages.showPage("Relay/Rendezvous");
            return false;
        }

        if (enablingPanel.isRelay.getState() && (!httpPanel.useMe.getState() || !httpPanel.publicAddr.getState())
                && (!tcpPanel.useMe.getState() || !tcpPanel.publicAddr.getState())) {
            helpLabel.setForeground(Color.red.darker());
            helpLabel.setText("Must enable incoming for TCP and/or HTTP to enable Relay");
            pages.showPage("Advanced");
            return false;
        }

        if (enablingPanel.isRendezvous.getState() && (!httpPanel.useMe.getState() || !httpPanel.publicAddr.getState())
                && (!tcpPanel.useMe.getState() || !tcpPanel.publicAddr.getState())) {
            helpLabel.setForeground(Color.red.darker());
            helpLabel.setText("Must enable incoming for TCP and/or HTTP to enable Rendezvous");
            pages.showPage("Advanced");
            return false;
        }

        // if use only seeds is specified then at least one seed must be
        // provided.
        if (rdvPanel.useOnlySeeds.getState()) {
            String[] rdvAddrs = rdvPanel.seeds.getItems();

            String[] rdvSeedAddrs = rdvPanel.seeding.getItems();

            if ((rdvAddrs.length == 0) && (rdvSeedAddrs.length == 0)) {
                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Must provide at least one seed rendezvous");
                pages.showPage("Rendezvous/Relays");
                return false;
            }
        }

        // if relay is to be used, make sure we have atleast one relay
        // addr for the enabled transport(s)
        if (relayPanel.useRelay.getState()) {
            String[] relayaddrs = relayPanel.seeds.getItems();

            String[] relaySeedaddrs = relayPanel.seeding.getItems();

            if ((relayaddrs.length == 0) && (relaySeedaddrs.length == 0)) {
                helpLabel.setForeground(Color.red.darker());
                helpLabel.setText("Must provide at least one seed Relay address");
                pages.showPage("Rendezvous/Relays");
                return false;
            }
        }

        return true;
    }

    /*
     * Updates the PlatformConfig Advertisement
     */
    private boolean saveValues() {
        try {
            // set the peer name
            configAdv.setName(idPanel.getName());

            // Save the http config
            HTTPAdv httpAdv = (HTTPAdv) AdvertisementFactory.newAdvertisement(HTTPAdv.getAdvertisementType());

            httpAdv.setConfigMode(httpPanel.getConfigMode());

            String chosenIntf = httpPanel.getInterfaceAddress();

            if (chosenIntf.startsWith("A")) {
                httpAdv.setInterfaceAddress(null);
            } else {
                httpAdv.setInterfaceAddress(chosenIntf);
            }

            httpAdv.setPort(Integer.parseInt(httpPanel.ifAddr.getPort()));

            httpAdv.setClientEnabled(httpPanel.clientEnabled.getState());

            httpAdv.setServerEnabled(httpPanel.publicAddr.getState());

            // If there's nothing interesting inthere, do not save it.
            if (0 == httpPanel.publicAddr.getHost().trim().length()) {
                httpAdv.setServer(null);
            } else {
                httpAdv.setServer(httpPanel.publicAddr.getHost() + ":" + httpPanel.publicAddr.getPort());
            }

            httpAdv.setPublicAddressOnly(httpPanel.getPubAddrOnly());

            configAdv.putServiceParam(PeerGroup.httpProtoClassID, wrapParm(httpAdv, httpPanel.useMe.getState()));

            // Save tcp configuration
            TCPAdv tcpAdv = (TCPAdv) AdvertisementFactory.newAdvertisement(TCPAdv.getAdvertisementType());

            tcpAdv.setConfigMode(tcpPanel.getConfigMode());

            chosenIntf = tcpPanel.getInterfaceAddress();
            if (chosenIntf.startsWith("A")) {
                tcpAdv.setInterfaceAddress(null);
            } else {
                tcpAdv.setInterfaceAddress(chosenIntf);
            }

            try {
                int theTcpPort = Integer.parseInt(tcpPanel.ifAddr.getPort());

                tcpAdv.setPort(theTcpPort);
                if (0 == theTcpPort) {
                    tcpAdv.setStartPort(0);
                    tcpAdv.setEndPort(0);
                }
            } catch (NumberFormatException ignored) {
                /* verifyInput already checked it */
            }

            tcpAdv.setClientEnabled(tcpPanel.clientEnabled.getState());

            tcpAdv.setServerEnabled(tcpPanel.publicAddr.getState());

            if (0 == tcpPanel.publicAddr.getHost().trim().length()) {
                tcpAdv.setServer(null);
            } else {
                tcpAdv.setServer(tcpPanel.publicAddr.getHost() + ":" + tcpPanel.publicAddr.getPort());
            }

            tcpAdv.setMulticastState(tcpPanel.multicast.getState());
            tcpAdv.setMulticastAddr(tcpMulticastAddr);
            tcpAdv.setMulticastPort(tcpMulticastPort);
            tcpAdv.setMulticastSize(tcpMulticastLength);

            tcpAdv.setPublicAddressOnly(tcpPanel.getPubAddrOnly());

            configAdv.putServiceParam(PeerGroup.tcpProtoClassID, wrapParm(tcpAdv, tcpPanel.useMe.getState()));

            // save the proxy service settings
            XMLDocument proxy = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");

            if (!enablingPanel.isJxmeProxy.getState()) {
                proxy.appendChild(proxy.createElement("isOff"));
            }

            configAdv.putServiceParam(PeerGroup.proxyClassID, proxy);

            // Save the Rendezvous Configuration
            RdvConfigAdv rdvConf = (RdvConfigAdv) AdvertisementFactory.newAdvertisement(RdvConfigAdv.getAdvertisementType());

            rdvConf.setConfiguration(
                    enablingPanel.isRendezvous.getState() ? RendezVousConfiguration.RENDEZVOUS :
                            rdvPanel.useRdv.getState() ? RendezVousConfiguration.EDGE : RendezVousConfiguration.AD_HOC);
            rdvConf.setUseOnlySeeds(rdvPanel.useOnlySeeds.getState());

            for (String s2 : Arrays.asList(rdvPanel.seeds.getItems())) {
                rdvConf.addSeedRendezvous(s2);
            }

            for (String s3 : Arrays.asList(rdvPanel.seeding.getItems())) {
                rdvConf.addSeedingURI(s3);
            }

            XMLDocument rdvDoc = (XMLDocument) rdvConf.getDocument(MimeMediaType.XMLUTF8);

            configAdv.putServiceParam(PeerGroup.rendezvousClassID, rdvDoc);

            // save the relay settings
            RelayConfigAdv relayConfig = (RelayConfigAdv) AdvertisementFactory.newAdvertisement(
                    RelayConfigAdv.getAdvertisementType());

            relayConfig.setServerEnabled(enablingPanel.isRelay.getState());
            relayConfig.setClientEnabled(relayPanel.useRelay.getState());

            for (String s : Arrays.asList(relayPanel.seeds.getItems())) {
                relayConfig.addSeedRelay(s);
            }

            for (String s1 : Arrays.asList(relayPanel.seeding.getItems())) {
                relayConfig.addSeedingURI(s1);
            }

            relayConfig.setUseOnlySeeds(relayPanel.useOnlySeeds.getState());

            XMLDocument relayDoc = (XMLDocument) relayConfig.getDocument(MimeMediaType.XMLUTF8);

            // check if the relay service should be disabled completely
            boolean relayDisabled = (!enablingPanel.isRelay.getState() && !relayPanel.useRelay.getState());

            if (relayDisabled) {
                relayDoc.appendChild(relayDoc.createElement("isOff"));
            }

            configAdv.putServiceParam(PeerGroup.relayProtoClassID, relayDoc);

            // Save the security configuration parameters
            // If we initialized security
            // Otherwise they come from the security login dialog
            if (null != idPanel.passwd) {
                PSEConfigAdv pseConf = (PSEConfigAdv) AdvertisementFactory.newAdvertisement(PSEConfigAdv.getAdvertisementType());

                PSEUtils.IssuerInfo info = PSEUtils.genCert(idPanel.getName(), null);

                pseConf.setCertificate(info.cert);
                pseConf.setPrivateKey(info.subjectPkey, idPanel.getPassword().toCharArray());

                XMLDocument pseDoc = (XMLDocument) pseConf.getDocument(MimeMediaType.XMLUTF8);

                configAdv.putServiceParam(PeerGroup.membershipClassID, pseDoc);
            }
        } catch (Throwable bad) {
            bad.printStackTrace();
            return false;
        }

        return true;
    }

    private XMLDocument wrapParm(Advertisement srcAdv, boolean enabled) {
        try {
            XMLDocument advDoc = (XMLDocument) srcAdv.getDocument(MimeMediaType.XMLUTF8);

            XMLDocument doc = (XMLDocument) StructuredDocumentFactory.newStructuredDocument(MimeMediaType.XMLUTF8, "Parm");

            StructuredDocumentUtils.copyElements(doc, doc, advDoc);
            if (!enabled) {
                doc.appendChild(doc.createElement("isOff"));
            }

            return doc;
        } catch (Throwable ez1) {
            ez1.printStackTrace();
            return null;
        }
    }
}
