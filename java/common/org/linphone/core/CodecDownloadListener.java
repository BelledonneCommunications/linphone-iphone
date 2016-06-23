package org.linphone.core;

public interface CodecDownloadListener{
    void listenerUpdateProgressBar(int current, int max);
    void listenerDownloadFailed(String error);
}
