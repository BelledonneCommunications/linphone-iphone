#!/bin/sh

SED_START='find ./src/android/org/linphone/ -type f -exec sed -i -e '
SED_END='{} \;'

# Imports
eval "$SED_START 's/import org.linphone.tools/import org.linphone.core.tools/g' $SED_END"
eval "$SED_START 's/import org.linphone.core.OpenH264DownloadHelperListener/import org.linphone.core.tools.OpenH264DownloadHelperListener/g' $SED_END"

# Listeners
eval "$SED_START 's/AccountCreator.AccountCreatorListener/AccountCreatorListener/g' $SED_END"
eval "$SED_START 's/LinphoneCoreListenerBase/CoreListenerStub/g' $SED_END"
eval "$SED_START 's/LinphoneCoreListener/CoreListener/g' $SED_END"
eval "$SED_START 's/LinphoneChatMessage.LinphoneChatMessageListener/ChatMessageListener/g' $SED_END"

# Enums
eval "$SED_START 's/Core.LinphoneLimeState/Core.LimeState/g' $SED_END"
eval "$SED_START 's/LinphoneLimeState/LimeState/g' $SED_END"

eval "$SED_START 's/GlobalState.GlobalOn/Core.GlobalState.On/g' $SED_END"

eval "$SED_START 's/RegistrationState.RegistrationOk/RegistrationState.Ok/g' $SED_END"
eval "$SED_START 's/RegistrationState.RegistrationFailed/RegistrationState.Failed/g' $SED_END"
eval "$SED_START 's/RegistrationState.RegistrationCleared/RegistrationState.Cleared/g' $SED_END"
eval "$SED_START 's/RegistrationState.RegistrationProgress/RegistrationState.Progress/g' $SED_END"
eval "$SED_START 's/RegistrationState.RegistrationNone/RegistrationState.None/g' $SED_END"

eval "$SED_START 's/RemoteProvisioningState.ConfiguringSuccessful/ConfiguringState.Successful/g' $SED_END"
eval "$SED_START 's/LinphoneCore.RemoteProvisioningState/Core.ConfiguringState/g' $SED_END"
eval "$SED_START 's/RemoteProvisioningState/ConfiguringState/g' $SED_END"
eval "$SED_START 's/ConfiguringFailed/Failed/g' $SED_END"

eval "$SED_START 's/CallDirection/Call.Dir/g' $SED_END"

eval "$SED_START 's/State.CallReleased/State.Released/g' $SED_END"
eval "$SED_START 's/State.CallEnd/State.End/g' $SED_END"
eval "$SED_START 's/State.CallUpdatedByRemote/State.UpdatedByRemote/g' $SED_END"
eval "$SED_START 's/State.CallIncomingEarlyMedia/State.IncomingEarlyMedia/g' $SED_END"
eval "$SED_START 's/State.CallUpdating/State.Updating/g' $SED_END"

eval "$SED_START 's/LogCollectionUploadState.LogCollectionUploadStateInProgress/LogCollectionUploadState.InProgress/g' $SED_END"
eval "$SED_START 's/LogCollectionUploadState.LogCollectionUploadStateDelivered/LogCollectionUploadState.Delivered/g' $SED_END"
eval "$SED_START 's/LogCollectionUploadState.LogCollectionUploadStateNotDelivered/LogCollectionUploadState.NotDelivered/g' $SED_END"

eval "$SED_START 's/AccountCreator.RequestStatus/AccountCreator.Status/g' $SED_END"
eval "$SED_START 's/RequestStatus/Status/g' $SED_END"
eval "$SED_START 's/AccountCreator.Status.Ok/AccountCreator.Status.RequestOk/g' $SED_END"
eval "$SED_START 's/AccountCreator.PasswordCheck/AccountCreator.PasswordStatus/g' $SED_END"
eval "$SED_START 's/AccountCreator.PhoneNumberCheck/AccountCreator.PhoneNumberStatus/g' $SED_END"
eval "$SED_START 's/AccountCreator.EmailCheck/AccountCreator.EmailStatus/g' $SED_END"
eval "$SED_START 's/AccountCreator.UsernameCheck/AccountCreator.UsernameStatus/g' $SED_END"
eval "$SED_START 's/AccountCreator.Status.Failed/AccountCreator.Status.RequestFailed/g' $SED_END"
eval "$SED_START 's/AccountCreator.Status.ErrorServer/AccountCreator.Status.ServerError/g' $SED_END"
eval "$SED_START 's/PhoneNumberStatus.CountryCodeInvalid/PhoneNumberStatus.InvalidCountryCode/g' $SED_END"

eval "$SED_START 's/Reason.Media/Reason.NotAcceptable/g' $SED_END"
eval "$SED_START 's/Reason.BadCredentials/Reason.Forbidden/g' $SED_END"

eval "$SED_START 's/TransportType.LinphoneTransportUdp/TransportType.Udp/g' $SED_END"
eval "$SED_START 's/TransportType.LinphoneTransportTcp/TransportType.Tcp/g' $SED_END"
eval "$SED_START 's/TransportType.LinphoneTransportTls/TransportType.Tls/g' $SED_END"
eval "$SED_START 's/TransportType.LinphoneTransportDtls/TransportType.Dtls/g' $SED_END"

eval "$SED_START 's/AddressFamily.INET_6.getInt()/AddressFamily.Inet6.toInt()/g' $SED_END"
eval "$SED_START 's/AddressFamily.INET.getInt()/AddressFamily.Inet.toInt()/g' $SED_END"

# Classes
eval "$SED_START 's/LpConfig/Config/g' $SED_END"
eval "$SED_START 's/LinphoneCoreException/CoreException/g' $SED_END"
eval "$SED_START 's/LinphoneCoreFactory/Factory/g' $SED_END"
eval "$SED_START 's/LinphoneAccountCreator/AccountCreator/g' $SED_END"
eval "$SED_START 's/LinphoneAddress/Address/g' $SED_END"
eval "$SED_START 's/LinphoneAuthInfo/AuthInfo/g' $SED_END"
eval "$SED_START 's/LinphoneCallLog/CallLog/g' $SED_END"
eval "$SED_START 's/LinphoneCallParams/CallParams/g' $SED_END"
eval "$SED_START 's/LinphoneCallStats/CallStats/g' $SED_END"
eval "$SED_START 's/LinphoneCall/Call/g' $SED_END"
eval "$SED_START 's/LinphoneChatMessage/ChatMessage/g' $SED_END"
eval "$SED_START 's/LinphoneChatRoom/ChatRoom/g' $SED_END"
eval "$SED_START 's/LinphoneConferenceParams/ConferenceParams/g' $SED_END"
eval "$SED_START 's/LinphoneConference/Conference/g' $SED_END"
eval "$SED_START 's/LinphoneConfig/Config/g' $SED_END"
eval "$SED_START 's/LinphoneContent/Content/g' $SED_END"
eval "$SED_START 's/LinphoneCore/Core/g' $SED_END"
eval "$SED_START 's/LinphoneEvent/Event/g' $SED_END"
eval "$SED_START 's/LinphoneFriendList/FriendList/g' $SED_END"
eval "$SED_START 's/LinphoneFriend/Friend/g' $SED_END"
eval "$SED_START 's/LinphoneHeaders/Headers/g' $SED_END"
eval "$SED_START 's/LinphoneImNotifyPolicy/ImNotifyPolicy/g' $SED_END"
eval "$SED_START 's/LinphoneInfoMessage/InfoMessage/g' $SED_END"
eval "$SED_START 's/LinphoneNatPolicy/NatPolicy/g' $SED_END"
eval "$SED_START 's/LinphonePayloadType/PayloadType/g' $SED_END"
eval "$SED_START 's/LinphonePlayer/Player/g' $SED_END"
eval "$SED_START 's/LinphonePresence/Presence/g' $SED_END"
eval "$SED_START 's/LinphonePrivacy/Privacy/g' $SED_END"
eval "$SED_START 's/LinphoneProxyConfig/ProxyConfig/g' $SED_END"
eval "$SED_START 's/LinphonePublishState/PublishState/g' $SED_END"
eval "$SED_START 's/LinphoneRange/Range/g' $SED_END"
eval "$SED_START 's/LinphoneStreamType/StreamType/g' $SED_END"
eval "$SED_START 's/LinphoneSubscription/Subscription/g' $SED_END"
eval "$SED_START 's/LinphoneTransports/Transports/g' $SED_END"
eval "$SED_START 's/LinphoneTunnel/Tunnel/g' $SED_END"
eval "$SED_START 's/LinphoneVcard/Vcard/g' $SED_END"
eval "$SED_START 's/LinphoneXmlRpc/XmlRpc/g' $SED_END"

# Callbacks
eval "$SED_START 's/onChatMessageStateChanged/onMsgStateChanged/g' $SED_END"
eval "$SED_START 's/onChatMessageFileTransferProgressChanged/onFileTransferProgressIndication/g' $SED_END"
eval "$SED_START 's/registrationState/onRegistrationStateChanged/g' $SED_END"

# Methods
eval "$SED_START 's/getFriendsLists()/getFriends()/g' $SED_END"
eval "$SED_START 's/getFriendLists()/getFriendsLists()/g' $SED_END"
eval "$SED_START 's/getFriendList(/getFriendsLists(/g' $SED_END"
eval "$SED_START 's/getIdentity(/getIdentityAddress(/g' $SED_END"
eval "$SED_START 's/isTunnelAvailable()/tunnelAvailable()/g' $SED_END"
eval "$SED_START 's/setZrtpSecretsCache(/setZrtpSecretsFile(/g' $SED_END"
eval "$SED_START 's/setRootCA(/setRootCa(/g' $SED_END"
eval "$SED_START 's/isInComingInvitePending()/isIncomingInvitePending()/g' $SED_END"
eval "$SED_START 's/getAudioCodecs()/getAudioPayloadTypes()/g' $SED_END"
eval "$SED_START 's/getVideoCodecs()/getVideoPayloadTypes()/g' $SED_END"
eval "$SED_START 's/getMime()/getMimeType()/g' $SED_END"
eval "$SED_START 's/getFrom()/getFromAddress()/g' $SED_END"
eval "$SED_START 's/getTo()/getToAddress()/g' $SED_END"
eval "$SED_START 's/getUserName()/getUsername()/g' $SED_END"
eval "$SED_START 's/getLimeEncryption()/limeEnabled()/g' $SED_END"
eval "$SED_START 's/getDirection/getDir/g' $SED_END"
eval "$SED_START 's/.getVideoEnabled()/.videoEnabled()/g' $SED_END"
eval "$SED_START 's/.getDataAsString()/.getStringBuffer()/g' $SED_END"
eval "$SED_START 's/getEventName()/getName()/g' $SED_END"
eval "$SED_START 's/setPlaybackGain(/setPlaybackGainDb(/g' $SED_END"
eval "$SED_START 's/isIncall()/inCall()/g' $SED_END"
eval "$SED_START 's/setVideoEnabled(/enableVideo(/g' $SED_END"
eval "$SED_START 's/setAudioBandwidth(/setAudioBandwidthLimit(/g' $SED_END"
eval "$SED_START 's/isAuthenticationTokenVerified()/getAuthenticationTokenVerified()/g' $SED_END"
eval "$SED_START 's/isMicMuted()/!micEnabled()/g' $SED_END"
eval "$SED_START 's/isLowBandwidthEnabled()/lowBandwidthEnabled()/g' $SED_END"
eval "$SED_START 's/muteMic(/enableMic(!/g' $SED_END"
eval "$SED_START 's/getRate()/getClockRate()/g' $SED_END"
eval "$SED_START 's/getSentVideoSize()/getSentVideoDefinition()/g' $SED_END"
eval "$SED_START 's/getReceivedVideoSize()/getReceivedVideoDefinition()/g' $SED_END"
eval "$SED_START 's/getUsedAudioCodec()/getUsedAudioPayloadType()/g' $SED_END"
eval "$SED_START 's/getUsedVideoCodec()/getUsedVideoPayloadType()/g' $SED_END"
eval "$SED_START 's/setVideoWindow(/setNativeVideoWindowId(/g' $SED_END"
eval "$SED_START 's/setPreviewWindow(/setNativePreviewWindowId(/g' $SED_END"
eval "$SED_START 's/islimeAvailable()/limeAvailable()/g' $SED_END"
eval "$SED_START 's/createChatMessage(/createMessage(/g' $SED_END"
#For messages only
eval "$SED_START 's/message.getStatus()/message.getState()/g' $SED_END"
#
eval "$SED_START 's/reSend()/resend()/g' $SED_END"
eval "$SED_START 's/setAppData(/setAppdata(/g' $SED_END"
eval "$SED_START 's/getAppData()/getAppdata()/g' $SED_END"
eval "$SED_START 's/getOrCreateChatRoom(/getChatRoomFromUri(/g' $SED_END"
eval "$SED_START 's/findFriendByAddress(/findFriend(/g' $SED_END"
eval "$SED_START 's/getTimestamp()/getStartDate()/g' $SED_END"
#For ProxyConfigs only 
eval "$SED_START 's/lpc.getAddress()/lpc.getIdentityAddress()/g' $SED_END"
eval "$SED_START 's/cfg.getAddress()/cfg.getIdentityAddress()/g' $SED_END"
eval "$SED_START 's/prxCfg.getAddress()/prxCfg.getIdentityAddress()/g' $SED_END"
eval "$SED_START 's/proxy.getAddress()/proxy.getIdentityAddress()/g' $SED_END"
#
eval "$SED_START 's/getCallDuration()/getDuration()/g' $SED_END"
eval "$SED_START 's/isVCardSupported()/vcardSupported()/g' $SED_END"
eval "$SED_START 's/getPresenceModelForUri(/getPresenceModelForUriOrTel(/g' $SED_END"
eval "$SED_START 's/setAvpfRRInterval(/setAvpfRrInterval(/g' $SED_END"
eval "$SED_START 's/getProxy()/getServerAddr()/g' $SED_END"
eval "$SED_START 's/setProxy(/setServerAddr(/g' $SED_END"
eval "$SED_START 's/setIdentity(/setIdentityAddress(/g' $SED_END"
eval "$SED_START 's/setUserId(/setUserid(/g' $SED_END"
eval "$SED_START 's/getUserId(/getUserid(/g' $SED_END"
eval "$SED_START 's/getAuthInfosList(/getAuthInfoList(/g' $SED_END"
eval "$SED_START 's/getSignalingTransportPorts()/getTransports()/g' $SED_END"
eval "$SED_START 's/setSignalingTransportPorts(/setTransports(/g' $SED_END"
eval "$SED_START 's/isIpv6Enabled()/ipv6Enabled()/g' $SED_END"
eval "$SED_START 's/isAdaptiveRateControlEnabled()/adaptiveRateControlEnabled()/g' $SED_END"
eval "$SED_START 's/setLimeEncryption(/enableLime(/g' $SED_END"
#For enums only
eval "$SED_START 's/.value()/.toInt()/g' $SED_END"
#
eval "$SED_START 's/clearAuthInfos()/clearAllAuthInfo()/g' $SED_END"
eval "$SED_START 's/clearProxyConfigs()/clearProxyConfig()/g' $SED_END"
eval "$SED_START 's/isVideoSupported()/videoSupported()/g' $SED_END"
eval "$SED_START 's/VideoDefinition().toDisplayableString()/VideoDefinition().getName()/g' $SED_END"
eval "$SED_START 's/isAccountUsed/isAccountExist()/g' $SED_END"
eval "$SED_START 's/loadXmlFile(/loadFromXmlFile(/g' $SED_END"
eval "$SED_START 's/activatePhoneNumberLink()/activateAlias()/g' $SED_END"
eval "$SED_START 's/isPhoneNumberUsed()/isAliasUsed()/g' $SED_END"
eval "$SED_START 's/recoverPhoneAccount()/recoverAccount()/g' $SED_END"
eval "$SED_START 's/isLimeEncryptionAvailable()/limeAvailable()/g' $SED_END"
eval "$SED_START 's/getUseRfc2833ForDtmfs/getUseRfc2833ForDtmf/g' $SED_END"
eval "$SED_START 's/setUseRfc2833ForDtmfs/setUseRfc2833ForDtmf/g' $SED_END"
eval "$SED_START 's/getUseSipInfoForDtmfs/getUseInfoForDtmf/g' $SED_END"
eval "$SED_START 's/setUseSipInfoForDtmfs/setUseInfoForDtmf/g' $SED_END"
eval "$SED_START 's/getIncomingTimeout/getIncTimeout/g' $SED_END"
eval "$SED_START 's/setIncomingTimeout/setIncTimeout/g' $SED_END"
eval "$SED_START 's/migrateCallLogs()/migrateLogsFromRcToDb()/g' $SED_END"
eval "$SED_START 's/setRLSUri/setRlsUri/g' $SED_END"

# Removed methods
eval "$SED_START 's/.isRegistered()/.getState() == RegistrationState.Ok/g' $SED_END"
eval "$SED_START 's/getBool(/getInt(/g' $SED_END"
eval "$SED_START 's/setBool(/setInt(/g' $SED_END"
eval "$SED_START 's/isInConference()/(getConference() != null)/g' $SED_END"
eval "$SED_START 's/getAudioStats()/getStats(StreamType.Audio)/g' $SED_END"
eval "$SED_START 's/getVideoStats()/getStats(StreamType.Video)/g' $SED_END"
eval "$SED_START 's/getVcardToString()/getVcard().asVcard4String()/g' $SED_END"
eval "$SED_START 's/getVideoAutoInitiatePolicy()/getVideoActivationPolicy().getAutomaticallyInitiate()/g' $SED_END"
eval "$SED_START 's/setFamilyName(/getVcard().setFamilyName(/g' $SED_END"
eval "$SED_START 's/setGivenName(/getVcard().setGivenName(/g' $SED_END"
eval "$SED_START 's/setOrganization(/getVcard().setOrganization(/g' $SED_END"
eval "$SED_START 's/getFamilyName()/getVcard().getFamilyName()/g' $SED_END"
eval "$SED_START 's/getGivenName()/getVcard().getGivenName()/g' $SED_END"
eval "$SED_START 's/getOrganization()/getVcard().getOrganization()/g' $SED_END"
eval "$SED_START 's/enableAvpf(/setAvpfMode(AVPFMode.Enabled)/g' $SED_END"
eval "$SED_START 's/transports.udp = /transports.setUdpPort(/g' $SED_END"
eval "$SED_START 's/transports.tcp = /transports.setTcpPort(/g' $SED_END"
eval "$SED_START 's/transports.tls = /transports.setTlsPort(/g' $SED_END"
eval "$SED_START 's/transports.udp/transports.getUdpPort()/g' $SED_END"
eval "$SED_START 's/transports.tcp/transports.getTcpPort()/g' $SED_END"
eval "$SED_START 's/transports.tls/transports.getTlsPort()/g' $SED_END"
eval "$SED_START 's/getPrimaryContactUsername()/getPrimaryContactParsed().getUsername()/g' $SED_END"
eval "$SED_START 's/getPrimaryContactDisplayName()/getPrimaryContactParsed().getDisplayName()/g' $SED_END"
eval "$SED_START 's/.sendDtmf(/.getCurrentCall().sendDtmf(/g' $SED_END"
eval "$SED_START 's/content.getData() == null/content.getSize() == 0/'g $SED_END"
eval "$SED_START 's/lc.downloadOpenH264Enabled()/OpenH264DownloadHelper.isOpenH264DownloadEnabled()/g' $SED_END"
eval "$SED_START 's/enableDownloadOpenH264(/OpenH264DownloadHelper.enableDownloadOpenH264(/g' $SED_END"
eval "$SED_START 's/mLc.destroy()/mLc = null/g' $SED_END"
eval "$SED_START 's/getAllDialPlan()/getDialPlans()/g' $SED_END"
eval "$SED_START 's/getCountryName()/getCountry()/g' $SED_END"

#Changes in library required
#Tunnel
#DialPlan
#LinphoneBuffer
#Call.zoomVideo()
#AccountCreator.updatePassword

#Android specifics not wrapped automatically
#Core.needsEchoCalibration()
#Core.hasCrappyOpenGL()
#Core.getMSFactory()
#Core.startEchoCalibration
#Core.startEchoTester
#Core.stopEchoTester

# For the payloads, get the list from the Core, call the method on the object directly and set it back if required
#Core.enablePayloadType()
#Core.isPayloadTypeEnabled()
#Core.payloadTypeIsVbr()
#Core.setPayloadTypeBitrate()

#Factory.createLpConfigFromString => Config.newFromBuffer
#Factory.createLpConfig => Config.newWithFactory or Core.createConfig
#Core.getVideoDevice and Core.setVideoDevice now takes/returns String instead of int
#Factory.createAccountCreator() => Core.createAccountCreator()
#Factory.createPresenceModel() => Core.createPresenceModel()
#CallParams.getJitterBufferSize() => CallStatsImpl.getJitterBufferSizeMs()
#Core.getSupportedVideoSizes() => Factory.getSupportedVideoDefinitions()
#Core.removeFriend() => FriendList.removeFriend()
#Core.getFriendsLists() => now returns a FriendList[] instead of a Friend[]
#Core.enableSpeaker / isSpeakerEnabled() => mAudioManager.setSpeakerphoneOn(speakerOn);
#Core.enableVideo(true, true) => Core.enableVideoCapture(bool) & Core.enableVideoDisplay(bool)
#Core.setCpuCount() => Not needed anymore, can be removed
#AccountCreator.getPrefix => linphone_dial_plan_lookup_ccc_from_e164
#ProxyConfig.lookupCCCFromIso=> linphone_dial_plan_lookup_ccc_from_iso
#Factory.instance().setLogCollectionPath(getFilesDir().getAbsolutePath()); => Core.setLogCollectionPath
#Factory.instance().enableLogCollection(isDebugEnabled); => COre.enableLogCollection
#Factory.instance().setDebugMode(isDebugEnabled, getString(R.string.app_name)); => Core.setLogLevelMask
