#!/bin/sh

# 1st pass
find ./src/android/org/linphone/ -type f -exec sed -i -e "s/import org.linphone.tools/import org.linphone.core.tools/g; \
s/import org.linphone.core.OpenH264DownloadHelperListener/import org.linphone.core.tools.OpenH264DownloadHelperListener/g; \
s/import org.linphone.core.LinphoneCore.Transports/import org.linphone.core.Transports/g; \
s/import org.linphone.core.LinphoneXmlRpcRequest.LinphoneXmlRpcRequestListener/import org.linphone.core.XmlRpcRequestListener/g; \
s/import org.linphone.core.LinphoneXmlRpcRequestImpl/\/\/import org.linphone.core.XmlRpcRequestImpl/g; \
s/import org.linphone.core.LinphoneXmlRpcSessionImpl/\/\/import org.linphone.core.XmlRpcSessionImpl/g; \
s/LinphoneAccountCreator.LinphoneAccountCreatorListener/AccountCreatorListener/g; \
s/AccountCreator.LinphoneAccountCreatorListener/AccountCreatorListener/g; \
s/LinphoneCoreListenerBase/CoreListenerStub/g; \
s/LinphoneCoreListener/CoreListener/g; \
s/LinphoneChatMessage.LinphoneChatMessageListener/ChatMessageListener/g; \
s/Core.LinphoneLimeState/Core.LimeState/g; \
s/LinphoneLimeState/LimeState/g; \
s/GlobalState.GlobalOn/Core.GlobalState.On/g; \
s/RegistrationState.RegistrationOk/RegistrationState.Ok/g; \
s/RegistrationState.RegistrationFailed/RegistrationState.Failed/g; \
s/RegistrationState.RegistrationCleared/RegistrationState.Cleared/g; \
s/RegistrationState.RegistrationProgress/RegistrationState.Progress/g; \
s/RegistrationState.RegistrationNone/RegistrationState.None/g; \
s/RemoteProvisioningState.ConfiguringSuccessful/ConfiguringState.Successful/g; \
s/LinphoneCore.RemoteProvisioningState/Core.ConfiguringState/g; \
s/RemoteProvisioningState/ConfiguringState/g; \
s/ConfiguringFailed/Failed/g; \
s/CallDirection/Call.Dir/g; \
s/State.CallReleased/State.Released/g; \
s/State.CallEnd/State.End/g; \
s/State.CallUpdatedByRemote/State.UpdatedByRemote/g; \
s/State.CallIncomingEarlyMedia/State.IncomingEarlyMedia/g; \
s/State.CallUpdating/State.Updating/g; \
s/LogCollectionUploadState.LogCollectionUploadStateInProgress/LogCollectionUploadState.InProgress/g; \
s/LogCollectionUploadState.LogCollectionUploadStateDelivered/LogCollectionUploadState.Delivered/g; \
s/LogCollectionUploadState.LogCollectionUploadStateNotDelivered/LogCollectionUploadState.NotDelivered/g; \
s/AccountCreator.RequestStatus/AccountCreator.Status/g; \
s/RequestStatus/Status/g; \
s/AccountCreator.Status.Ok/AccountCreator.Status.RequestOk/g; \
s/AccountCreator.PasswordCheck/AccountCreator.PasswordStatus/g; \
s/AccountCreator.PhoneNumberCheck/AccountCreator.PhoneNumberStatus/g; \
s/AccountCreator.EmailCheck/AccountCreator.EmailStatus/g; \
s/AccountCreator.UsernameCheck/AccountCreator.UsernameStatus/g; \
s/AccountCreator.Status.Failed/AccountCreator.Status.RequestFailed/g; \
s/AccountCreator.Status.ErrorServer/AccountCreator.Status.ServerError/g; \
s/PhoneNumberStatus.CountryCodeInvalid/PhoneNumberStatus.InvalidCountryCode/g; \
s/Reason.Media/Reason.NotAcceptable/g; \
s/Reason.BadCredentials/Reason.Forbidden/g; \
s/TransportType.LinphoneTransportUdp/TransportType.Udp/g; \
s/TransportType.LinphoneTransportTcp/TransportType.Tcp/g; \
s/TransportType.LinphoneTransportTls/TransportType.Tls/g; \
s/TransportType.LinphoneTransportDtls/TransportType.Dtls/g; \
s/AddressFamily.INET_6.getInt()/AddressFamily.Inet6/g; \
s/AddressFamily.INET.getInt()/AddressFamily.Inet/g; \
s/LpConfig/Config/g; \
s/LinphoneCoreException/CoreException/g; \
s/LinphoneCoreFactory/Factory/g; \
s/LinphoneAccountCreator/AccountCreator/g; \
s/LinphoneAddress/Address/g; \
s/LinphoneAuthInfo/AuthInfo/g; \
s/LinphoneBuffer/Buffer/g; \
s/LinphoneCallLog/CallLog/g; \
s/LinphoneCallParams/CallParams/g; \
s/LinphoneCallStats/CallStats/g; \
s/LinphoneCall/Call/g; \
s/LinphoneChatMessage/ChatMessage/g; \
s/LinphoneChatRoom/ChatRoom/g; \
s/LinphoneConferenceParams/ConferenceParams/g; \
s/LinphoneConference/Conference/g; \
s/LinphoneConfig/Config/g; \
s/LinphoneContent/Content/g; \
s/LinphoneCore/Core/g; \
s/LinphoneEvent/Event/g; \
s/LinphoneFriendList/FriendList/g; \
s/LinphoneFriend/Friend/g; \
s/LinphoneHeaders/Headers/g; \
s/LinphoneImNotifyPolicy/ImNotifyPolicy/g; \
s/LinphoneInfoMessage/InfoMessage/g; \
s/LinphoneNatPolicy/NatPolicy/g; \
s/LinphonePayloadType/PayloadType/g; \
s/LinphonePlayer/Player/g; \
s/LinphonePresence/Presence/g; \
s/LinphonePrivacy/Privacy/g; \
s/LinphoneProxyConfig/ProxyConfig/g; \
s/LinphonePublishState/PublishState/g; \
s/LinphoneRange/Range/g; \
s/LinphoneStreamType/StreamType/g; \
s/LinphoneSubscription/Subscription/g; \
s/LinphoneTransports/Transports/g; \
s/LinphoneTunnel/Tunnel/g; \
s/LinphoneVcard/Vcard/g; \
s/LinphoneXmlRpc/XmlRpc/g; \
s/onAccountCreatorIsAccountUsed/onIsAccountExist/g; \
s/onAccountCreatorAccountCreated/onCreateAccount/g; \
s/onAccountCreatorAccountActivated/onActivateAccount/g; \
s/onAccountCreatorAccountLinkedWithPhoneNumber/onLinkAccount/g; \
s/onAccountCreatorPhoneNumberLinkActivated/onActivateAlias/g; \
s/onAccountCreatorIsAccountActivated/onIsAccountActivated/g; \
s/onAccountCreatorPhoneAccountRecovered/onRecoverAccount/g; \
s/onAccountCreatorIsAccountLinked/onIsAccountLinked/g; \
s/onAccountCreatorIsPhoneNumberUsed/onIsAliasUsed/g; \
s/onAccountCreatorPasswordUpdated/onUpdateAccount/g; \
s/(AccountCreator accountCreator, Status status)/(AccountCreator accountCreator, Status status, String resp)/g; \
s/(AccountCreator accountCreator, AccountCreator.Status status)/(AccountCreator accountCreator, AccountCreator.Status status, String resp)/g; \
s/onChatMessageStateChanged/onMsgStateChanged/g; \
s/onChatMessageFileTransferProgressChanged/onFileTransferProgressIndication/g; \
s/onChatMessageFileTransferSent/onFileTransferSend/g; \
s/onChatMessageFileTransferReceived/onFileTransferRecv/g; \
s/authInfoRequested/authInfoRequested_removed/g; \
s/show(Core/show_removed(Core/g; \
s/displayStatus/displayStatus_removed/g; \
s/displayMessage/displayMessage_removed/g; \
s/displayWarning/displayWarning_removed/g; \
s/fileTransferProgressIndication/fileTransferProgressIndication_removed/g; \
s/fileTransferRecv/fileTransferRecv_removed/g; \
s/fileTransferSend/fileTransferSend_removed/g; \
s/notifyReceived(Core lc, Event/onNotifyReceived(Core lc, Event/g; \
s/notifyReceived/notifyReceived_removed/g; \
s/ecCalibrationStatus/onEcCalibrationResult/g; \
s/publishStateChanged/onPublishStateChanged/g; \
s/messageReceivedUnableToDecrypted/messageReceivedUnableToDecrypted_removed/g; \
s/callStatsUpdated/onCallStatsUpdated/g; \
s/authenticationRequested/onAuthenticationRequested/g; \
s/newSubscriptionRequest/onNewSubscriptionRequested/g; \
s/notifyPresenceReceived/onNotifyPresenceReceived/g; \
s/dtmfReceived/onDtmfReceived/g; \
s/transferState/onTransferStateChanged/g; \
s/infoReceived/onInfoReceived/g; \
s/subscriptionStateChanged/onSubscriptionStateChanged/g; \
s/globalState/onGlobalStateChanged/g; \
s/registrationState/onRegistrationStateChanged/g; \
s/configuringStatus/onConfiguringStatus/g; \
s/messageReceived/onMessageReceived/g; \
s/callState/onCallStateChanged/g; \
s/callEncryptionChanged/onCallEncryptionChanged/g; \
s/isComposingReceived/onIsComposingReceived/g; \
s/uploadProgressIndication/onLogCollectionUploadProgressIndication/g; \
s/uploadStateChanged/onLogCollectionUploadStateChanged/g; \
s/friendListCreated/onFriendListCreated/g; \
s/friendListRemoved/onFriendListRemoved/g; \
s/networkReachableChanged/onNetworkReachable/g; \
s/onFriendCreated/onContactCreated/g; \
s/onFriendUpdated/onContactUpdated/g; \
s/onFriendDeleted/onContactDeleted/g; \
s/onFriendSyncStatusChanged/onSyncStatusChanged/g; \
s/onXmlRpcRequestResponse/onResponse/g; \
s/getFriendsLists()/getFriends()/g; \
s/getFriendLists()/getFriendsLists()/g; \
s/getFriendList(/getFriendsLists(/g; \
s/getIdentity(/getIdentityAddress(/g; \
s/isTunnelAvailable()/tunnelAvailable()/g; \
s/tunnelSetMode(/getTunnel().setMode(/g; \
s/tunnelAddServer(/getTunnel().addServer(/g; \
s/tunnelCleanServers(/getTunnel().cleanServers(/g; \
s/setZrtpSecretsCache(/setZrtpSecretsFile(/g; \
s/setRootCA(/setRootCa(/g; \
s/isInComingInvitePending()/isIncomingInvitePending()/g; \
s/getAudioCodecs()/getAudioPayloadTypes()/g; \
s/getVideoCodecs()/getVideoPayloadTypes()/g; \
s/getMime()/getMimeType()/g; \
s/getFrom()/getFromAddress()/g; \
s/getTo()/getToAddress()/g; \
s/getUserName()/getUsername()/g; \
s/getLimeEncryption()/limeEnabled()/g; \
s/getDirection/getDir/g; \
s/.getVideoEnabled()/.videoEnabled()/g; \
s/.getDataAsString()/.getStringBuffer()/g; \
s/getEventName()/getName()/g; \
s/setPlaybackGain(/setPlaybackGainDb(/g; \
s/isIncall()/inCall()/g; \
s/setVideoEnabled(/enableVideo(/g; \
s/setAudioBandwidth(/setAudioBandwidthLimit(/g; \
s/isAuthenticationTokenVerified()/getAuthenticationTokenVerified()/g; \
s/\(\s*\)\([a-zA-Z()\.]*\)isMicMuted()/\1!\2micEnabled()/g; \
s/isLowBandwidthEnabled()/lowBandwidthEnabled()/g; \
s/muteMic(/enableMic(!/g; \
s/getRate()/getClockRate()/g; \
s/getSentVideoSize()/getSentVideoDefinition()/g; \
s/getReceivedVideoSize()/getReceivedVideoDefinition()/g; \
s/getUsedAudioCodec()/getUsedAudioPayloadType()/g; \
s/getUsedVideoCodec()/getUsedVideoPayloadType()/g; \
s/setVideoWindow(/setNativeVideoWindowId(/g; \
s/setPreviewWindow(/setNativePreviewWindowId(/g; \
s/islimeAvailable()/limeAvailable()/g; \
s/createChatMessage(/createMessage(/g; \
s/message.getStatus()/message.getState()/g; \
s/reSend()/resend()/g; \
s/setAppData(/setAppdata(/g; \
s/getAppData()/getAppdata()/g; \
s/getOrCreateChatRoom(/getChatRoomFromUri(/g; \
s/findFriendByAddress(/findFriend(/g; \
s/getTimestamp()/getStartDate()/g; \
s/lpc.getAddress()/lpc.getIdentityAddress()/g; \
s/cfg.getAddress()/cfg.getIdentityAddress()/g; \
s/prxCfg.getAddress()/prxCfg.getIdentityAddress()/g; \
s/proxy.getAddress()/proxy.getIdentityAddress()/g; \
s/getProxyConfig(n).getAddress()/getProxyConfig(n).getIdentityAddress()/g; \
s/getCallDuration()/getDuration()/g; \
s/isVCardSupported()/vcardSupported()/g; \
s/getPresenceModelForUri(/getPresenceModelForUriOrTel(/g; \
s/setAvpfRRInterval(/setAvpfRrInterval(/g; \
s/getAvpfRRInterval(/getAvpfRrInterval(/g; \
s/hasBuiltInEchoCanceler()/hasBuiltinEchoCanceller()/g; \
s/getProxy()/getServerAddr()/g; \
s/setProxy(/setServerAddr(/g; \
s/setIdentity(/setIdentityAddress(/g; \
s/setUserId(/setUserid(/g; \
s/getUserId(/getUserid(/g; \
s/getAuthInfosList(/getAuthInfoList(/g; \
s/getSignalingTransportPorts()/getTransports()/g; \
s/setSignalingTransportPorts(/setTransports(/g; \
s/isIpv6Enabled()/ipv6Enabled()/g; \
s/isAdaptiveRateControlEnabled()/adaptiveRateControlEnabled()/g; \
s/setLimeEncryption(/enableLime(/g; \
s/.value()/.toInt()/g; \
s/clearAuthInfos()/clearAllAuthInfo()/g; \
s/clearProxyConfigs()/clearProxyConfig()/g; \
s/isVideoSupported()/videoSupported()/g; \
s/getReceivedVideoDefinition().width/getReceivedVideoDefinition().getWidth()/g; \
s/getReceivedVideoDefinition().height/getReceivedVideoDefinition().getHeight()/g; \
s/VideoDefinition().toDisplayableString()/VideoDefinition().getName()/g; \
s/isAccountUsed()/isAccountExist()/g; \
s/loadXmlFile(/loadFromXmlFile(/g; \
s/activatePhoneNumberLink()/activateAlias()/g; \
s/isPhoneNumberUsed()/isAliasUsed()/g; \
s/recoverPhoneAccount()/recoverAccount()/g; \
s/isLimeEncryptionAvailable()/limeAvailable()/g; \
s/getUseRfc2833ForDtmfs/getUseRfc2833ForDtmf/g; \
s/setUseRfc2833ForDtmfs/setUseRfc2833ForDtmf/g; \
s/getUseSipInfoForDtmfs/getUseInfoForDtmf/g; \
s/setUseSipInfoForDtmfs/setUseInfoForDtmf/g; \
s/getIncomingTimeout/getIncTimeout/g; \
s/setIncomingTimeout/setIncTimeout/g; \
s/migrateCallLogs()/migrateLogsFromRcToDb()/g; \
s/setRLSUri/setRlsUri/g; \
s/hasCrappyOpenGL(/hasCrappyOpengl(/g; \
s/needsEchoCalibration(/isEchoCancellerCalibrationRequired(/g; \
s/getCountryCode()/getCountryCallingCode()/g; \
s/isEchoCancellationEnabled()/echoCancellationEnabled()/g; \
s/startEchoCalibration(/startEchoCancellerCalibration(/g; \
s/.isRegistered()/.getState() == RegistrationState.Ok/g; \
s/\(\s*\)\([a-zA-Z()\.]*\)isInConference()/\1\2getConference() != null/g; \
s/getAudioStats()/getStats(StreamType.Audio)/g; \
s/getVideoStats()/getStats(StreamType.Video)/g; \
s/getVcardToString()/getVcard().asVcard4String()/g; \
s/getVideoAutoAcceptPolicy(/getVideoActivationPolicy().getAutomaticallyAccept(/g; \
s/getVideoAutoInitiatePolicy()/getVideoActivationPolicy().getAutomaticallyInitiate()/g; \
s/setFamilyName(/getVcard().setFamilyName(/g; \
s/setGivenName(/getVcard().setGivenName(/g; \
s/\.setOrganization(/\.getVcard().setOrganization(/g; \
s/getFamilyName()/getVcard().getFamilyName()/g; \
s/getGivenName()/getVcard().getGivenName()/g; \
s/\.getOrganization()/\.getVcard().getOrganization()/g; \
s/enableAvpf(/setAvpfMode(AVPFMode.Enabled)/g; \
s/transports.udp\s*=\s*\([a-zA-Z0-9_]*\)/transports.setUdpPort(\1)/g; \
s/transports.tcp\s*=\s*\([a-zA-Z0-9_]*\)/transports.setTcpPort(\1)/g; \
s/transports.tls\s*=\s*\([a-zA-Z0-9_]*\)/transports.setTlsPort(\1)/g; \
s/transports.udp/transports.getUdpPort()/g; \
s/transports.tcp/transports.getTcpPort()/g; \
s/transports.tls/transports.getTlsPort()/g; \
s/getPrimaryContactUsername()/getPrimaryContactParsed().getUsername()/g; \
s/getPrimaryContactDisplayName()/getPrimaryContactParsed().getDisplayName()/g; \
s/.sendDtmf(/.getCurrentCall().sendDtmf(/g; \
s/content.getData() == null/content.getSize() == 0/g; \
s/lc.downloadOpenH264Enabled()/OpenH264DownloadHelper.isOpenH264DownloadEnabled()/g; \
s/LinphoneManager.getLc().downloadOpenH264Enabled()/OpenH264DownloadHelper.isOpenH264DownloadEnabled()/g; \
s/getLc().enableDownloadOpenH264(/OpenH264DownloadHelper.setOpenH264DownloadEnabled(/g; \
s/enableDownloadOpenH264(/OpenH264DownloadHelper.enableDownloadOpenH264(/g; \
s/mLc.destroy()/mLc = null/g; \
s/getAllDialPlan()/getDialPlans()/g; \
s/getCountryName()/getCountry()/g; \
s/getMSFactory()/getMediastreamerFactory()/g; \
s/accountCreator.getPrefix(/org.linphone.core.Utils.getPrefixFromE164(/g; \
s/proxyConfig.lookupCCCFromIso(/org.linphone.core.Utils.getCccFromIso(/g; \
s/linkPhoneNumberWithAccount()/linkAccount()/g; \
s/zoomVideo(/zoom(/g; \
s/mLc.setCpuCount(/\/\/mLc.setCpuCount(/g; \
s/new XmlRpcRequestImpl(/xmlRpcSession.createRequest(/g; \
s/new XmlRpcSessionImpl(LinphoneManager.getLcIfManagerNotDestroyedOrNull(), /LinphoneManager.getLcIfManagerNotDestroyedOrNull().createXmlRpcSession(/g; \
s/FriendImpl/Friend/g; \
s/PresenceActivityType/PresenceActivity.Type/g; \
s/org\.linphone\.core\.VideoSize/org.linphone.core.VideoDefinition/g;" {} \;

# 2nd pass
find ./src/android/org/linphone/ -type f -exec sed -i -e "s/Address\.TransportType/TransportType/g; \
s/\(CallLog\.\)\?CallStatus\([^[:alnum:]_]\)/Call.Status\2/g; \
s/CallStats\.AddressFamily/AddressFamily/g; \
s/CallStats\.StreamType/StreamType/g; \
s/Core\.AuthMethod/AuthMethod/g; \
s/Core\.ConfiguringState/ConfiguringState/g; \
s/Core\.EcCalibratorStatus/EcCalibratorStatus/g; \
s/Core\.GlobalState/GlobalState/g; \
s/Core\.LimeState/LimeState/g; \
s/Core\.LogCollectionState/LogCollectionState/g; \
s/Core\.MediaEncryption/MediaEncryption/g; \
s/Core\.RegistrationState/RegistrationState/g; \
s/Core\.TunnelMode/Tunnel.Mode/g; \
s/Core\.VersionUpdateCheckResult/VersionUpdateCheckResult/g; \
s/Event\.PublishState/PublishState/g; \
s/Friend\.SubscribePolicy/SubscribePolicy/g; \
s/XmlRpcRequest\.ArgType/XmlRpcArgType/g; \
s/XmlRpcRequest\.Status/XmlRpcStatus/g; \
s/org\.linphone\.core\.PresenceActivity\.Type/org.linphone.core.PresenceActivity/g;" {} \;


# Manual changes required:
#
# Some callbacks no longer exist, their name will be "removed", remove them
#
# This script may create build errors in your application, especially if you defined methods that have the same name as methods
# of the linphone SDK. For example, with the Linphone application we get errors for the enableAVPF(int n, boolean enabled) method of
# LinphonePreferences that is wrongly replaced into setAVPFMode(AVPFMode.Enabled)int n, boolean enabled).
#
# You may need to import some classes, eg. AccountCreatorListener, AVPFMode, Call, ChatMessageListener, ConfiguringState, LimeState,
# MediaEncryption, RegistrationState, StreamType, VideoDefinition, XmlRpcArgType...
#
# If you are using the LinphoneContact class, some getVcard() method may have been wrongly introduced by this script that you will need
# to delete.
#
# Some methods that used to take or return String or LinphoneAddress now take the other
#
# createAddress, addAddress, addFriend, acceptCall, acceptCallWithParams no longer throw a CoreException
#
# AccountCreator's Status.Ok must be renamed in Status.RequestOk
#
# VideoDevices were int, now are String
#
# XmlRpcSessionImpl => XmlRpcSession
#
# getFriendsLists() returned Friend[], now is a FriendList[]
#
# No need anymore to cast to a Impl class to be able to use setUserData or getUserData
#
# findFriend now takes an Address instead of a String
#
# createOpenH264DownloadHelper() now takes a Context
#
# Factory.createCore(this, mConfigFile, mLinphoneFactoryConfigFile, null, c) => createCore(this, mConfigFile, mLinphoneFactoryConfigFile)
#
# startEchoTester and stopEchoTester now return void
#
# createProxyConfig no longer takes any parameter
#
# setPrimaryContact only takes one String argument
#
# AdaptiveRateAlgorithm was an enum, now is replaced by a String in the methods that were using it
#
# createAddress(userName,domain,null); no longer exists
#
# Buffer.setContent now takes the size as second parameter
#
# ChatMessageListener onFileTransferSend now returns the Buffer instead of having it as part of his arguments
#
# Core.startEchoCancellerCalibration no longer takes a parameter
#
# XmlRpcSession.createRequest takes first the return arg type and then the name of the method, until now it was the other way around
#
# ProxyConfig.normalizePhoneNumber returns null if it fails instead of the value given as parameter in the previous version!
#
# ChatMessage.getStorageId() no longer exists
#
# ChatRoom.getHistory() no needs an integer parameter corresponding to the number of messages to get (use 0 to get the full history)
#
# Factory.createContent() no longer takes arguments. Use the Content methods to fill the content.
#
# AccountCreator.updatePassword() no longer exist. Use setPassword() and updateAccount() instead.

# # Factory
#Factory.createAccountCreator() => Core.createAccountCreator()
#Factory.createPresenceModel() => Core.createPresenceModel() & PresenceModel.setActivity()
#Factory.instance().enableLogCollection(isDebugEnabled); now takes a LogCollectionState

# # Core
#Core.getVideoDevice and Core.setVideoDevice now takes/returns String instead of int
#Core.getSupportedVideoSizes() => Factory.getSupportedVideoDefinitions()
#Core.removeFriend() => FriendList.removeFriend()
#Core.getFriendsLists() => now returns a FriendList[] instead of a Friend[]
#Core.enableSpeaker / isSpeakerEnabled() => mAudioManager.setSpeakerphoneOn(speakerOn);
#Core.enableVideo(true, true) => Core.enableVideoCapture(bool) & Core.enableVideoDisplay(bool)
#Core.setVideoPolicy(initiate, accept) => Core.getVideoActivationPolicy().setAutomaticallyInitiate(initiate) & Core.getVideoActivationPolicy().setAutomaticallyAccept(accept)

# # Other
#CallParams.getJitterBufferSize() => CallStatsImpl.getJitterBufferSizeMs()
#Core.findAuthInfo was (username, realm, domain) now is (realm, username, domain)

# # Payloads
#Core.enablePayloadType() => PayloadType.enable()
#Core.isPayloadTypeEnabled() => PayloadType.enabled()
#Core.payloadTypeIsVbr() => PayloadType.isVbr()
#Core.setPayloadTypeBitrate() => PayloadType.setNormalBitrate()
