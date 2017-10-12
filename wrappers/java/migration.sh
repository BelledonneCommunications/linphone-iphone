#!/bin/sh

SRC_DIR="src/android/org/linphone/*.java"

# Listeners
sed -i 's/AccountCreator.AccountCreatorListener/AccountCreatorListener/g' $SRC_DIR
sed -i 's/LinphoneCoreListenerBase/CoreListenerStub/g' $SRC_DIR
sed -i 's/LinphoneCoreListener/CoreListener/g' $SRC_DIR
sed -i 's/LinphoneChatMessage.LinphoneChatMessageListener/ChatMessageListener/g' $SRC_DIR
sed -i 's/AccountCreator.AccountCreatorListener/AccountCreatorListener/g' $SRC_DIR

# Enums
sed -i 's/Core.LinphoneLimeState/Core.LimeState/g' $SRC_DIR
sed -i 's/LinphoneLimeState/LimeState/g' $SRC_DIR

sed -i 's/GlobalState.GlobalOn/Core.GlobalState.On/g' $SRC_DIR

sed -i 's/RegistrationState.RegistrationOk/RegistrationState.Ok/g' $SRC_DIR
sed -i 's/RegistrationState.RegistrationFailed/RegistrationState.Failed/g' $SRC_DIR
sed -i 's/RegistrationState.RegistrationCleared/RegistrationState.Cleared/g' $SRC_DIR
sed -i 's/RegistrationState.RegistrationProgress/RegistrationState.Progress/g' $SRC_DIR

sed -i 's/RemoteProvisioningState.ConfiguringSuccessful/ConfiguringState.Successful/g' $SRC_DIR
sed -i 's/LinphoneCore.RemoteProvisioningState/Core.ConfiguringState/g' $SRC_DIR
sed -i 's/RemoteProvisioningState/ConfiguringState/g' $SRC_DIR

sed -i 's/CallDirection/Call.Dir/g' $SRC_DIR

sed -i 's/State.CallReleased/State.Released/g' $SRC_DIR
sed -i 's/State.CallEnd/State.End/g' $SRC_DIR
sed -i 's/State.CallUpdatedByRemote/State.UpdatedByRemote/g' $SRC_DIR
sed -i 's/State.CallIncomingEarlyMedia/State.IncomingEarlyMedia/g' $SRC_DIR
sed -i 's/State.CallUpdating/State.Updating/g' $SRC_DIR

sed -i 's/LogCollectionUploadState.LogCollectionUploadStateInProgress/LogCollectionUploadState.InProgress/g' $SRC_DIR
sed -i 's/LogCollectionUploadState.LogCollectionUploadStateDelivered/LogCollectionUploadState.Delivered/g' $SRC_DIR
sed -i 's/LogCollectionUploadState.LogCollectionUploadStateNotDelivered/LogCollectionUploadState.NotDelivered/g' $SRC_DIR

sed -i 's/AccountCreator.RequestStatus/AccountCreator.Status/g' $SRC_DIR
sed -i 's/AccountCreator.Status.Ok/AccountCreator.Status.RequestOk/g' $SRC_DIR
sed -i 's/AccountCreator.PasswordCheck/AccountCreator.PasswordStatus/g' $SRC_DIR
sed -i 's/AccountCreator.PhoneNumberCheck/AccountCreator.PhoneNumberStatus/g' $SRC_DIR
sed -i 's/AccountCreator.EmailCheck/AccountCreator.EmailStatus/g' $SRC_DIR
sed -i 's/AccountCreator.UsernameCheck/AccountCreator.UsernameStatus/g' $SRC_DIR
sed -i 's/AccountCreator.Status.Failed/AccountCreator.Status.RequestFailed/g' $SRC_DIR
sed -i 's/AccountCreator.Status.ErrorServer/AccountCreator.Status.ServerError/g' $SRC_DIR

sed -i 's/Reason.Media/Reason.NotAcceptable/g' $SRC_DIR
sed -i 's/Reason.BadCredentials/Reason.Forbidden/g' $SRC_DIR

sed -i 's/TransportType.LinphoneTransportUdp/TransportType.Udp/g' $SRC_DIR
sed -i 's/TransportType.LinphoneTransportTcp/TransportType.Tcp/g' $SRC_DIR
sed -i 's/TransportType.LinphoneTransportTls/TransportType.Tls/g' $SRC_DIR
sed -i 's/TransportType.LinphoneTransportDtls/TransportType.Dtls/g' $SRC_DIR

# Classes
sed -i 's/LpConfig/Config/g' $SRC_DIR
sed -i 's/LinphoneCoreException/CoreException/g' $SRC_DIR
sed -i 's/LinphoneCoreFactory/Factory/g' $SRC_DIR
sed -i 's/LinphoneAccountCreator/AccountCreator/g' $SRC_DIR
sed -i 's/LinphoneAddress/Address/g' $SRC_DIR
sed -i 's/LinphoneAuthInfo/AuthInfo/g' $SRC_DIR
sed -i 's/LinphoneCallLog/CallLog/g' $SRC_DIR
sed -i 's/LinphoneCallParams/CallParams/g' $SRC_DIR
sed -i 's/LinphoneCallStats/CallStats/g' $SRC_DIR
sed -i 's/LinphoneCall/Call/g' $SRC_DIR
sed -i 's/LinphoneChatMessage/ChatMessage/g' $SRC_DIR
sed -i 's/LinphoneChatRoom/ChatRoom/g' $SRC_DIR
sed -i 's/LinphoneConferenceParams/ConferenceParams/g' $SRC_DIR
sed -i 's/LinphoneConference/Conference/g' $SRC_DIR
sed -i 's/LinphoneConfig/Config/g' $SRC_DIR
sed -i 's/LinphoneContent/Content/g' $SRC_DIR
sed -i 's/LinphoneCore/Core/g' $SRC_DIR
sed -i 's/LinphoneEvent/Event/g' $SRC_DIR
sed -i 's/LinphoneFriendList/FriendList/g' $SRC_DIR
sed -i 's/LinphoneFriend/Friend/g' $SRC_DIR
sed -i 's/LinphoneHeaders/Headers/g' $SRC_DIR
sed -i 's/LinphoneImNotifyPolicy/ImNotifyPolicy/g' $SRC_DIR
sed -i 's/LinphoneInfoMessage/InfoMessage/g' $SRC_DIR
sed -i 's/LinphoneNatPolicy/NatPolicy/g' $SRC_DIR
sed -i 's/LinphonePayloadType/PayloadType/g' $SRC_DIR
sed -i 's/LinphonePlayer/Player/g' $SRC_DIR
sed -i 's/LinphonePresence/Presence/g' $SRC_DIR
sed -i 's/LinphonePrivacy/Privacy/g' $SRC_DIR
sed -i 's/LinphoneProxyConfig/ProxyConfig/g' $SRC_DIR
sed -i 's/LinphonePublishState/PublishState/g' $SRC_DIR
sed -i 's/LinphoneRange/Range/g' $SRC_DIR
sed -i 's/LinphoneStreamType/StreamType/g' $SRC_DIR
sed -i 's/LinphoneSubscription/Subscription/g' $SRC_DIR
sed -i 's/LinphoneTransports/Transports/g' $SRC_DIR
sed -i 's/LinphoneTunnel/Tunnel/g' $SRC_DIR
sed -i 's/LinphoneVcard/Vcard/g' $SRC_DIR
sed -i 's/LinphoneXmlRpc/XmlRpc/g' $SRC_DIR

#Callbacks
sed -i 's/onChatMessageStateChanged/onMsgStateChanged/g' $SRC_DIR
sed -i 's/onChatMessageFileTransferProgressChanged/onFileTransferProgressIndication/g' $SRC_DIR

#Methods
sed -i 's/getFriendList(/getFriendsLists(/g' $SRC_DIR
sed -i 's/getFriendLists()/getFriendsLists()/g' $SRC_DIR
sed -i 's/getIdentity(/getIdentityAddress(/g' $SRC_DIR
sed -i 's/isTunnelAvailable()/tunnelAvailable()/g' $SRC_DIR
sed -i 's/setZrtpSecretsCache(/setZrtpSecretsFile(/g' $SRC_DIR
sed -i 's/setRootCA(/setRootCa(/g' $SRC_DIR
sed -i 's/isInComingInvitePending()/isIncomingInvitePending()/g' $SRC_DIR
sed -i 's/getAudioCodecs()/getAudioPayloadTypes()/g' $SRC_DIR
sed -i 's/getVideoCodecs()/getVideoPayloadTypes()/g' $SRC_DIR
sed -i 's/getMime()/getMimeType()/g' $SRC_DIR
sed -i 's/getFrom()/getFromAddress()/g' $SRC_DIR
sed -i 's/getTo()/getToAddress()/g' $SRC_DIR
sed -i 's/getUserName()/getUsername()/g' $SRC_DIR
sed -i 's/getLimeEncryption()/limeEnabled()/g' $SRC_DIR
sed -i 's/getDirection/getDir/g' $SRC_DIR
sed -i 's/.getVideoEnabled()/.videoEnabled()/g' $SRC_DIR
sed -i 's/.getDataAsString()/.getStringBuffer()/g' $SRC_DIR
sed -i 's/getEventName()/getName()/g' $SRC_DIR
sed -i 's/setPlaybackGain(/setPlaybackGainDb(/g' $SRC_DIR
sed -i 's/isIncall()/inCall()/g' $SRC_DIR
sed -i 's/setVideoEnabled(/enableVideo(/g' $SRC_DIR
sed -i 's/setAudioBandwidth(/setAudioBandwidthLimit(/g' $SRC_DIR
sed -i 's/isAuthenticationTokenVerified()/getAuthenticationTokenVerified()/g' $SRC_DIR
sed -i 's/isMicMuted()/!micEnabled()/g' $SRC_DIR
sed -i 's/isLowBandwidthEnabled()/lowBandwidthEnabled()/g' $SRC_DIR
sed -i 's/muteMic(/enableMic(!/g' $SRC_DIR
sed -i 's/getRate()/getClockRate()/g' $SRC_DIR
sed -i 's/getSentVideoSize()/getSentVideoDefinition()/g' $SRC_DIR
sed -i 's/getReceivedVideoSize()/getReceivedVideoDefinition()/g' $SRC_DIR
sed -i 's/getUsedAudioCodec()/getUsedAudioPayloadType()/g' $SRC_DIR
sed -i 's/getUsedVideoCodec()/getUsedVideoPayloadType()/g' $SRC_DIR
sed -i 's/setVideoWindow(/setNativeVideoWindowId(/g' $SRC_DIR
sed -i 's/setPreviewWindow(/setNativePreviewWindowId(/g' $SRC_DIR
sed -i 's/islimeAvailable()/limeAvailable()/g' $SRC_DIR
sed -i 's/createChatMessage(/createMessage(/g' $SRC_DIR
#For messages only
sed -i 's/message.getStatus()/message.getState()/g' $SRC_DIR
#
sed -i 's/reSend()/resend()/g' $SRC_DIR
sed -i 's/setAppData(/setAppdata(/g' $SRC_DIR
sed -i 's/getAppData()/getAppdata()/g' $SRC_DIR
sed -i 's/getOrCreateChatRoom(/getChatRoomFromUri(/g' $SRC_DIR
sed -i 's/findFriendByAddress(/findFriend(/g' $SRC_DIR
sed -i 's/getTimestamp()/getStartDate()/g' $SRC_DIR
#For ProxyConfigs only 
sed -i 's/lpc.getAddress()/lpc.getIdentityAddress()/g' $SRC_DIR
#
sed -i 's/getCallDuration()/getDuration()/g' $SRC_DIR
sed -i 's/isVCardSupported()/vcardSupported()/g' $SRC_DIR
sed -i 's/getPresenceModelForUri(/getPresenceModelForUriOrTel(/g' $SRC_DIR
sed -i 's/setAvpfRRInterval(/setAvpfRrInterval(/g' $SRC_DIR
sed -i 's/getProxy()/getServerAddr()/g' $SRC_DIR
sed -i 's/setProxy(/setServerAddr(/g' $SRC_DIR
sed -i 's/setIdentity(/setIdentityAddress(/g' $SRC_DIR
sed -i 's/setUserId(/setUserid(/g' $SRC_DIR
sed -i 's/getUserId(/getUserid/g' $SRC_DIR
sed -i 's/getAuthInfosList(/getAuthInfoList(/g' $SRC_DIR
sed -i 's/getPassword/getPasswd(/g' $SRC_DIR
sed -i 's/getSignalingTransportPorts()/getTransports()/g' $SRC_DIR
sed -i 's/setSignalingTransportPorts(/setTransports(/g' $SRC_DIR
sed -i 's/isIpv6Enabled()/ipv6Enabled()/g' $SRC_DIR
sed -i 's/isAdaptiveRateControlEnabled()/adaptiveRateControlEnabled()/g' $SRC_DIR
sed -i 's/setLimeEncryption(/enableLime(/g' $SRC_DIR
#For enums only
sed -i 's/.value()/.toInt()/g' $SRC_DIR
#
sed -i 's/clearAuthInfos()/clearAllAuthInfo()/g' $SRC_DIR
sed -i 's/clearProxyConfigs()/clearProxyConfig()/g' $SRC_DIR
sed -i 's/isVideoSupported()/videoSupported()/g' $SRC_DIR

#Removed methods
sed -i 's/.isRegistered()/.getState() == RegistrationState.Ok/g' $SRC_DIR
sed -i 's/getBool(/getInt(/g' $SRC_DIR
sed -i 's/setBool(/setInt(/g' $SRC_DIR
sed -i 's/isInConference()/getConference() != null/g' $SRC_DIR
sed -i 's/getAudioStats()/getStats(StreamType.Audio)/g' $SRC_DIR
sed -i 's/getVideoStats()/getStats(StreamType.Audio)/g' $SRC_DIR
sed -i 's/getVcardToString()/getVcard().asVcard4String()/g' $SRC_DIR
sed -i 's/getVideoAutoInitiatePolicy()/getVideoActivationPolicy().getAutomaticallyInitiate()/g' $SRC_DIR
sed -i 's/setFamilyName(/getVcard().setFamilyName(/g' $SRC_DIR
sed -i 's/setGivenName(/getVcard().setGivenName(/g' $SRC_DIR
sed -i 's/setOrganization(/getVcard().setOrganization(/g' $SRC_DIR
sed -i 's/getFamilyName()/getVcard().getFamilyName()/g' $SRC_DIR
sed -i 's/getGivenName()/getVcard().getGivenName()/g' $SRC_DIR
sed -i 's/getOrganization()/getVcard().getOrganization()/g' $SRC_DIR
sed -i 's/enableAvpf(/setAvpfMode(AVPFMode.Enabled)/g' $SRC_DIR
sed -i 's/transports.udp = /transports.setUdpPort(/g' $SRC_DIR
sed -i 's/transports.tcp = /transports.setTcpPort(/g' $SRC_DIR
sed -i 's/transports.tls = /transports.setTlsPort(/g' $SRC_DIR
sed -i 's/transports.udp/transports.getUdpPort()/g' $SRC_DIR
sed -i 's/transports.tcp/transports.getTcpPort()/g' $SRC_DIR
sed -i 's/transports.tls/transports.getTlsPort()/g' $SRC_DIR
sed -i 's/getPrimaryContactUsername()/getPrimaryContactParsed().getUsername()/g' $SRC_DIR
sed -i 's/getPrimaryContactDisplayName()/getPrimaryContactParsed().getDisplayName()/g' $SRC_DIR

#Have disapeared, to check
#OpenH264DownloadHelper
#Core.enablePayloadType()
#Core.isPayloadTypeEnabled()
#Core.payloadTypeIsVbr()
#Core.setPayloadTypeBitrate()
#DialPlan
#LinphoneBuffer
#CallParams.getJitterBufferSize()
#Core.getSupportedVideoSizes()
#Core.needsEchoCalibration()
#Core.removeFriend(
#Core.hasCrappyOpenGL()
#Core.enableSpeaker / isSpeakerEnabled()
#Core.getMSFactory() 
#Core.enableVideo(true, true) => Core.enableVideoCapture(bool) & Core.enableVideoDisplay(bool)
#Core.setCpuCount()
#Call.zoomVideo() has been removed temporarily in the wrapper
