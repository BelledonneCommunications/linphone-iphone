/*
 * IncompleteConfigurationException.java
 *
 * Created on September 13, 2005, 12:11 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */
package net.jxta.impl.peergroup;

import net.jxta.exception.ConfiguratorException;

import java.util.List;

/**
 * A configurator exception which is generated when intervention
 */
public class IncompleteConfigurationException extends ConfiguratorException {

    /**
     * Constucts a {@link IncompleteConfigurationException} with no specified details.
     */
    public IncompleteConfigurationException() {
        super();
    }

    /**
     * Constructs a {@link IncompleteConfigurationException} with the specified message.
     *
     * @param msg message
     */
    public IncompleteConfigurationException(String msg) {
        super(msg);
    }

    /**
     * Constructs a {@link IncompleteConfigurationException} with the specified {@link
     * java.lang.Throwable cause}.
     *
     * @param ex cause
     */
    public IncompleteConfigurationException(Throwable ex) {
        super(ex);
    }

    /**
     * Constructs a {@link IncompleteConfigurationException} with the specified message and {@link
     * java.lang.Throwable cause}.
     *
     * @param msg message
     * @param ex  cause
     */
    public IncompleteConfigurationException(String msg, Throwable ex) {
        super(msg, ex);
    }

    /**
     * Constructs a {@link IncompleteConfigurationException} with the specified {@link
     * java.util.List} of {@link java.lang.Throwable causes}.
     *
     * @param ex causes
     */
    public IncompleteConfigurationException(List ex) {
        super(ex);
    }
}
