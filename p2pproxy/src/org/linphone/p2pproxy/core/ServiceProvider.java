// Decompiled by Jad v1.5.8g. Copyright 2001 Pavel Kouznetsov.
// Jad home page: http://www.kpdus.com/jad.html
// Decompiler options: packimports(3) 
// Source File Name:   ServiceProvider.java

package org.linphone.p2pproxy.core;

import org.linphone.p2pproxy.api.P2pProxyException;

public interface ServiceProvider
{

    public abstract void start(long aTimeOut) throws P2pProxyException;

    public abstract void stop();
}
