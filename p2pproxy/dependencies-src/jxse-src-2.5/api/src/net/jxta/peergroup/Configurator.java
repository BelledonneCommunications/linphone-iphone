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
package net.jxta.peergroup;


import net.jxta.exception.ConfiguratorException;
import net.jxta.protocol.ConfigParams;


/**
 * A configurator is responsible for the persistence and validation of
 * configuration parameters.
 *
 * @deprecated This interface has been replaced with the {@link net.jxta.platform.NetworkConfigurator}.
 **/
@Deprecated
public interface Configurator {
    
    /**
     * Retrieve the parameters associated with this configuration from the
     * default location. If necessary the parameters will be created or an
     * opportunity to adjust them will be provided.
     *
     * @return The configuration parameters.
     * @throws ConfiguratorException If there was a failure in retrieving the
     *  parameters. This is normally a chained exception to the underlying
     *  cause.
     **/
    public ConfigParams getConfigParams() throws ConfiguratorException;
    
    /**
     * Sets the parameters associated with this configuration object to the
     * provided values.
     *
     * @deprecated Configuration parameters should be set individually via
     * whatever interfaces implementing configurator provides. This method
     * over-writes all configuration settings in an unpredictable way.
     *
     * @param cp The parameters to be associated with this configuration.
     **/
    @Deprecated
    public void setConfigParams(ConfigParams cp);
    
    /**
     * Retrieves the persisted parameters associated with this configuration
     * from the standard location.
     *
     * @deprecated Loading of existing configuration is best accomplished by use 
     * of specific constructors of the implementing configurator. This method
     * complicates the state management of configuration parameters and may have
     * unpredictable results depending upon the constructor and configuration
     * set methods used prior to it's execution.
     *
     * @return The configuration parameters.
     * @throws ConfiguratorException If there was a failure in retrieving the
     * persisted parameters. This is normally a chained exception to the
     * underlying cause.
     **/
    @Deprecated
    public ConfigParams load() throws ConfiguratorException;
    
    /**
     * Persist the parameters associated with this configuration to the
     * standard location.
     *
     * @return <code>true</code> if the configuration was successfully saved
     *  otherwise <code>false</code>. If the parameters are not persisted then
     *  <code>false/code> is returned.
     * @throws ConfiguratorException If there was a failure in persisting the
     *  parameters. This is normally a chained exception to the underlying
     *  cause.
     **/
    public boolean save() throws ConfiguratorException;
}
