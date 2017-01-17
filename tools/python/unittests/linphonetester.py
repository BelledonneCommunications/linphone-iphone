from datetime import timedelta, datetime
from nose.tools import assert_equals
from copy import deepcopy
import linphone
import logging
import os
import sys
import time
import weakref


test_domain = "sipopen.example.org"
auth_domain = "sip.example.org"
test_username = "liblinphone_tester"
test_password = "secret"
test_route = "sip2.linphone.org"
if os.path.isdir(os.path.join(os.path.dirname(__file__), "rcfiles")):
    # Running unit tests from an installed package
    tester_resources_path = os.path.abspath(os.path.dirname(__file__))
else:
    # Running unit tests from the linphone sources
    tester_resources_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../../tester/"))


def linphonetester_log_handler(level, msg):
    import logging
    method = getattr(logging.getLogger("linphonetester"), level)
    if not msg.strip().startswith('[PYLINPHONE]'):
        msg = '[CORE] ' + msg
    method(msg)

linphonetester_logger = logging.getLogger("linphonetester")
handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.INFO)
formatter = logging.Formatter('%(asctime)s.%(msecs)03d %(levelname)s: %(message)s', '%H:%M:%S')
handler.setFormatter(formatter)
linphonetester_logger.addHandler(handler)
linphone.set_log_handler(linphonetester_log_handler)


def create_address(domain):
    addr = linphone.Factory.get().create_address(None)
    assert addr != None
    addr.username = test_username
    assert_equals(addr.username, test_username)
    if domain is None:
        domain = test_route
    addr.domain = domain
    assert_equals(addr.domain, domain)
    addr.display_name = None
    addr.display_name = "Mr Tester"
    assert_equals(addr.display_name, "Mr Tester")
    return addr


class Account:
    def __init__(self, id_addr, unique_id):
        self.created = False
        self.done = False
        self.registered = False
        self.identity = id_addr.clone()
        self.password = linphone.testing.get_random_token(8)
        self.modified_identity = id_addr.clone()
        modified_username = "{username}_{unique_id}".format(username=id_addr.username, unique_id=unique_id)
        self.modified_identity.username = modified_username


class AccountManager:
    def __init__(self):
        self.unique_id = linphone.testing.get_random_token(6)
        self.accounts = []

    @classmethod
    def wait_for_until(cls, lc1, lc2, func, timeout):
        lcs = []
        if lc1 is not None:
            lcs.append(lc1)
        if lc2 is not None:
            lcs.append(lc2)
        return cls.wait_for_list(lcs, func, timeout)

    @classmethod
    def wait_for_list(cls, lcs, func, timeout):
        start = datetime.now()
        end = start + timedelta(milliseconds = timeout)
        res = func(*lcs)
        while not res and datetime.now() < end:
            for lc in lcs:
                lc.iterate()
            time.sleep(0.02)
            res = func(*lcs)
        return res

    @classmethod
    def account_created_on_server_cb(cls, lc, cfg, state, message):
        if state == linphone.RegistrationState.Ok:
            if cfg.error_info.phrase == "Test account created":
                lc.user_data().created = True
            else:
                lc.user_data().registered = True
        elif state == linphone.RegistrationState.Cleared:
            lc.user_data().done = True

    @classmethod
    def account_created_auth_requested_cb(cls, lc, realm, username, domain):
        lc.user_data().created = True

    def check_account(self, cfg):
        create_account = False
        lc = cfg.core
        id_addr = cfg.identity_address
        account = self._get_account(id_addr)
        original_ai = lc.find_auth_info(None, id_addr.username, id_addr.domain)
        if account is None:
            linphonetester_logger.info("[TESTER] No account for {identity} exists, going to create one.".format(identity=id_addr.as_string()))
            account = Account(id_addr, self.unique_id)
            self.accounts.append(account)
            create_account = True
        cfg.identity_address = account.modified_identity
        if create_account:
            self._create_account_on_server(account, cfg)
        if original_ai is not None:
            lc.remove_auth_info(original_ai)
        ai = linphone.Factory.get().create_auth_info(account.modified_identity.username, None, account.password, None, None, account.modified_identity.domain)
        lc.add_auth_info(ai)
        return account.modified_identity

    def _get_account(self, id_addr):
        for account in self.accounts:
            if account.identity.weak_equal(id_addr):
                return account
        return None

    def _create_account_on_server(self, account, refcfg):
        tmp_identity = account.modified_identity.clone()
        cbs = linphone.Factory.get().create_core_cbs()
        cbs.registration_state_changed = AccountManager.account_created_on_server_cb
        cbs.authentication_requested = AccountManager.account_created_auth_requested_cb
        lc = CoreManager.configure_lc_from(cbs, tester_resources_path, None, account)
        lc.sip_transports = linphone.SipTransports(-1, -1, -1, -1)
        cfg = lc.create_proxy_config()
        tmp_identity.secure = False
        tmp_identity.password = account.password
        tmp_identity.set_header("X-Create-Account", "yes")
        cfg.identity_address = tmp_identity
        server_addr = linphone.Factory.get().create_address(refcfg.server_addr)
        server_addr.secure = False
        server_addr.transport = linphone.TransportType.Tcp;
        server_addr.port = 0
        cfg.server_addr = server_addr.as_string()
        cfg.expires = 3 * 3600 # Accounts are valid 3 hours
        lc.add_proxy_config(cfg)
        if AccountManager.wait_for_until(lc, None, lambda lc: lc.user_data().created == True, 10000) != True:
            linphonetester_logger.critical("[TESTER] Account for {identity} could not be created on server.".format(identity=refcfg.identity_address.as_string()))
            sys.exit(-1)
        cfg.edit()
        cfg.identity_address = account.modified_identity.clone()
        cfg.identity_address.secure = False
        cfg.done()
        ai = linphone.Factory.get().create_auth_info(account.modified_identity.username, None, account.password, None, None, account.modified_identity.domain)
        lc.add_auth_info(ai)
        if AccountManager.wait_for_until(lc, None, lambda lc: lc.user_data().registered == True, 3000) != True:
            linphonetester_logger.critical("[TESTER] Account for {identity} is not working on server.".format(identity=refcfg.identity_address.as_string()))
            sys.exit(-1)
        lc.remove_proxy_config(cfg)
        if AccountManager.wait_for_until(lc, None, lambda lc: lc.user_data().done == True, 3000) != True:
            linphonetester_logger.critical("[TESTER] Account creation could not clean the registration context.")
            sys.exit(-1)


account_manager = AccountManager()


class CoreManagerStats:
    def __init__(self):
        self.reset()

    def reset(self):
        self.number_of_LinphoneRegistrationNone = 0
        self.number_of_LinphoneRegistrationProgress = 0
        self.number_of_LinphoneRegistrationOk = 0
        self.number_of_LinphoneRegistrationCleared = 0
        self.number_of_LinphoneRegistrationFailed = 0
        self.number_of_auth_info_requested = 0

        self.number_of_LinphoneCallIncomingReceived = 0
        self.number_of_LinphoneCallOutgoingInit = 0
        self.number_of_LinphoneCallOutgoingProgress = 0
        self.number_of_LinphoneCallOutgoingRinging = 0
        self.number_of_LinphoneCallOutgoingEarlyMedia = 0
        self.number_of_LinphoneCallConnected = 0
        self.number_of_LinphoneCallStreamsRunning = 0
        self.number_of_LinphoneCallPausing = 0
        self.number_of_LinphoneCallPaused = 0
        self.number_of_LinphoneCallResuming = 0
        self.number_of_LinphoneCallRefered = 0
        self.number_of_LinphoneCallError = 0
        self.number_of_LinphoneCallEnd = 0
        self.number_of_LinphoneCallPausedByRemote = 0
        self.number_of_LinphoneCallUpdatedByRemote = 0
        self.number_of_LinphoneCallIncomingEarlyMedia = 0
        self.number_of_LinphoneCallUpdating = 0
        self.number_of_LinphoneCallReleased = 0

        self.number_of_LinphoneTransferCallOutgoingInit = 0
        self.number_of_LinphoneTransferCallOutgoingProgress = 0
        self.number_of_LinphoneTransferCallOutgoingRinging = 0
        self.number_of_LinphoneTransferCallOutgoingEarlyMedia = 0
        self.number_of_LinphoneTransferCallConnected = 0
        self.number_of_LinphoneTransferCallStreamsRunning = 0
        self.number_of_LinphoneTransferCallError = 0

        self.number_of_LinphoneMessageReceived = 0
        self.number_of_LinphoneMessageReceivedWithFile = 0
        self.number_of_LinphoneMessageReceivedLegacy = 0
        self.number_of_LinphoneMessageExtBodyReceived = 0
        self.number_of_LinphoneMessageInProgress = 0
        self.number_of_LinphoneMessageDelivered = 0
        self.number_of_LinphoneMessageNotDelivered = 0
        self.number_of_LinphoneIsComposingActiveReceived = 0
        self.number_of_LinphoneIsComposingIdleReceived = 0
        self.number_of_LinphoneFileTransferDownloadSuccessful = 0
        self.progress_of_LinphoneFileTransfer = 0

        self.number_of_IframeDecoded = 0

        self.number_of_NewSubscriptionRequest = 0
        self.number_of_NotifyReceived = 0
        self.number_of_NotifyPresenceReceived = 0
        self.number_of_LinphonePresenceBasicStatusOpen = 0
        self.number_of_LinphonePresenceBasicStatusClosed = 0
        self.number_of_LinphonePresenceActivityOffline = 0
        self.number_of_LinphonePresenceActivityOnline = 0
        self.number_of_LinphonePresenceActivityAppointment = 0
        self.number_of_LinphonePresenceActivityAway = 0
        self.number_of_LinphonePresenceActivityBreakfast = 0
        self.number_of_LinphonePresenceActivityBusy = 0
        self.number_of_LinphonePresenceActivityDinner = 0
        self.number_of_LinphonePresenceActivityHoliday = 0
        self.number_of_LinphonePresenceActivityInTransit = 0
        self.number_of_LinphonePresenceActivityLookingForWork = 0
        self.number_of_LinphonePresenceActivityLunch = 0
        self.number_of_LinphonePresenceActivityMeal = 0
        self.number_of_LinphonePresenceActivityMeeting = 0
        self.number_of_LinphonePresenceActivityOnThePhone = 0
        self.number_of_LinphonePresenceActivityOther = 0
        self.number_of_LinphonePresenceActivityPerformance = 0
        self.number_of_LinphonePresenceActivityPermanentAbsence = 0
        self.number_of_LinphonePresenceActivityPlaying = 0
        self.number_of_LinphonePresenceActivityPresentation = 0
        self.number_of_LinphonePresenceActivityShopping = 0
        self.number_of_LinphonePresenceActivitySleeping = 0
        self.number_of_LinphonePresenceActivitySpectator = 0
        self.number_of_LinphonePresenceActivitySteering = 0
        self.number_of_LinphonePresenceActivityTravel = 0
        self.number_of_LinphonePresenceActivityTV = 0
        self.number_of_LinphonePresenceActivityUnknown = 0
        self.number_of_LinphonePresenceActivityVacation = 0
        self.number_of_LinphonePresenceActivityWorking = 0
        self.number_of_LinphonePresenceActivityWorship = 0
        self.last_received_presence = None

        self.number_of_inforeceived = 0

        self.number_of_LinphoneSubscriptionIncomingReceived = 0
        self.number_of_LinphoneSubscriptionOutgoingInit = 0
        self.number_of_LinphoneSubscriptionPending = 0
        self.number_of_LinphoneSubscriptionActive = 0
        self.number_of_LinphoneSubscriptionTerminated = 0
        self.number_of_LinphoneSubscriptionError = 0
        self.number_of_LinphoneSubscriptionExpiring = 0

        self.number_of_LinphonePublishProgress = 0
        self.number_of_LinphonePublishOk = 0
        self.number_of_LinphonePublishExpiring = 0
        self.number_of_LinphonePublishError = 0
        self.number_of_LinphonePublishCleared = 0

        self.number_of_LinphoneConfiguringSkipped = 0
        self.number_of_LinphoneConfiguringFailed = 0
        self.number_of_LinphoneConfiguringSuccessful = 0

        self.number_of_LinphoneCallEncryptedOn = 0
        self.number_of_LinphoneCallEncryptedOff = 0

        self.last_received_chat_message = None


class CoreManager:

    @classmethod
    def configure_lc_from(cls, cbs, resources_path, rc_path, user_data=None):
        filepath = None
        if rc_path is not None:
            filepath = os.path.join(resources_path, rc_path)
            assert_equals(os.path.isfile(filepath), True)
        lc = linphone.Factory.get().create_core(cbs, None, filepath)
        linphone.testing.set_dns_user_hosts_file(lc, os.path.join(resources_path, 'tester_hosts'))
        lc.root_ca = os.path.join(resources_path, 'certificates', 'cn', 'cafile.pem')
        lc.ring = os.path.join(resources_path, 'sounds', 'oldphone.wav')
        lc.ringback = os.path.join(resources_path, 'sounds', 'ringback.wav')
        lc.static_picture = os.path.join(resources_path, 'images', 'nowebcamCIF.jpg')
        lc.user_data = weakref.ref(user_data)
        return lc

    @classmethod
    def wait_for_until(cls, manager1, manager2, func, timeout):
        managers = []
        if manager1 is not None:
            managers.append(manager1)
        if manager2 is not None:
            managers.append(manager2)
        return cls.wait_for_list(managers, func, timeout)

    @classmethod
    def wait_for_list(cls, managers, func, timeout):
        start = datetime.now()
        end = start + timedelta(milliseconds = timeout)
        res = func(*managers)
        while not res and datetime.now() < end:
            for manager in managers:
                manager.lc.iterate()
            time.sleep(0.02)
            res = func(*managers)
        return res

    @classmethod
    def wait_for(cls, manager1, manager2, func):
        return cls.wait_for_until(manager1, manager2, func, 10000)

    @classmethod
    def call(cls, caller_manager, callee_manager, caller_params = None, callee_params = None, build_callee_params = False):
        initial_caller_stats = deepcopy(caller_manager.stats)
        initial_callee_stats = deepcopy(callee_manager.stats)

        # Use playfile for callee to avoid locking on capture card
        callee_manager.lc.use_files = True
        callee_manager.lc.play_file = os.path.join(tester_resources_path, 'sounds', 'hello8000.wav')

        if caller_params is None:
            call = caller_manager.lc.invite_address(callee_manager.identity)
        else:
            call = caller_manager.lc.invite_address_with_params(callee_manager.identity, caller_params)
        assert call is not None

        assert_equals(CoreManager.wait_for(callee_manager, caller_manager,
            lambda callee_manager, caller_manager: callee_manager.stats.number_of_LinphoneCallIncomingReceived == initial_callee_stats.number_of_LinphoneCallIncomingReceived + 1), True)
        assert_equals(callee_manager.lc.incoming_invite_pending, True)
        assert_equals(caller_manager.stats.number_of_LinphoneCallOutgoingProgress, initial_caller_stats.number_of_LinphoneCallOutgoingProgress + 1)

        retry = 0
        while (caller_manager.stats.number_of_LinphoneCallOutgoingRinging != initial_caller_stats.number_of_LinphoneCallOutgoingRinging + 1) and \
            (caller_manager.stats.number_of_LinphoneCallOutgoingEarlyMedia != initial_caller_stats.number_of_LinphoneCallOutgoingEarlyMedia + 1) and \
            retry < 20:
            retry += 1
            caller_manager.lc.iterate()
            callee_manager.lc.iterate()
            time.sleep(0.1)
        assert ((caller_manager.stats.number_of_LinphoneCallOutgoingRinging == initial_caller_stats.number_of_LinphoneCallOutgoingRinging + 1) or \
            (caller_manager.stats.number_of_LinphoneCallOutgoingEarlyMedia == initial_caller_stats.number_of_LinphoneCallOutgoingEarlyMedia + 1)) == True

        assert callee_manager.lc.current_call_remote_address is not None
        if caller_manager.lc.current_call is None or callee_manager.lc.current_call is None or callee_manager.lc.current_call_remote_address is None:
            return False
        callee_from_address = caller_manager.identity.clone()
        callee_from_address.port = 0 # Remove port because port is never present in from header
        assert_equals(callee_from_address.weak_equal(callee_manager.lc.current_call_remote_address), True)

        if callee_params is not None:
            callee_manager.lc.accept_call_with_params(callee_manager.lc.current_call, callee_params)
        elif build_callee_params:
            default_params = callee_manager.lc.create_call_params(callee_manager.lc.current_call)
            callee_manager.lc.accept_call_with_params(callee_manager.lc.current_call, default_params)
        else:
            callee_manager.lc.accept_call(callee_manager.lc.current_call)
        assert_equals(CoreManager.wait_for(callee_manager, caller_manager,
            lambda callee_manager, caller_manager: (callee_manager.stats.number_of_LinphoneCallConnected == initial_callee_stats.number_of_LinphoneCallConnected + 1) and \
                (caller_manager.stats.number_of_LinphoneCallConnected == initial_caller_stats.number_of_LinphoneCallConnected + 1)), True)
        # Just to sleep
        result = CoreManager.wait_for(callee_manager, caller_manager,
            lambda callee_manager, caller_manager: (callee_manager.stats.number_of_LinphoneCallStreamsRunning == initial_callee_stats.number_of_LinphoneCallStreamsRunning + 1) and \
                (caller_manager.stats.number_of_LinphoneCallStreamsRunning == initial_caller_stats.number_of_LinphoneCallStreamsRunning + 1))

        if caller_manager.lc.media_encryption != linphone.MediaEncryption.MediaEncryptionNone and callee_manager.lc.media_encryption != linphone.MediaEncryption.None:
            # Wait for encryption to be on, in case of zrtp, it can take a few seconds
            if caller_manager.lc.media_encryption == linphone.MediaEncryption.ZRTP:
                CoreManager.wait_for(callee_manager, caller_manager,
                    lambda callee_manager, caller_manager: caller_manager.stats.number_of_LinphoneCallEncryptedOn == initial_caller_stats.number_of_LinphoneCallEncryptedOn + 1)
            if callee_manager.lc.media_encryption == linphone.MediaEncryption.ZRTP:
                CoreManager.wait_for(callee_manager, caller_manager,
                    lambda callee_manager, caller_manager: callee_manager.stats.number_of_LinphoneCallEncryptedOn == initial_callee_stats.number_of_LinphoneCallEncryptedOn + 1)
            assert_equals(callee_manager.lc.current_call.current_params.media_encryption, caller_manager.lc.media_encryption)
            assert_equals(caller_manager.lc.current_call.current_params.media_encryption, callee_manager.lc.media_encryption)

        return result

    @classmethod
    def end_call(cls, caller_manager, callee_manager):
        caller_manager.lc.terminate_all_calls()
        assert_equals(CoreManager.wait_for(caller_manager, callee_manager,
            lambda caller_manager, callee_manager: caller_manager.stats.number_of_LinphoneCallEnd == 1 and callee_manager.stats.number_of_LinphoneCallEnd == 1), True)

    @classmethod
    def registration_state_changed(cls, lc, cfg, state, message):
        manager = lc.user_data()
        linphonetester_logger.info("[TESTER] New registration state {state} for user id [{identity}] at proxy [{addr}]".format(
            state=linphone.RegistrationState.string(state), identity=cfg.identity_address.as_string(), addr=cfg.server_addr))
        if state == linphone.RegistrationState.None:
            manager.stats.number_of_LinphoneRegistrationNone += 1
        elif state == linphone.RegistrationState.Progress:
            manager.stats.number_of_LinphoneRegistrationProgress += 1
        elif state == linphone.RegistrationState.Ok:
            manager.stats.number_of_LinphoneRegistrationOk += 1
        elif state == linphone.RegistrationState.Cleared:
            manager.stats.number_of_LinphoneRegistrationCleared += 1
        elif state == linphone.RegistrationState.Failed:
            manager.stats.number_of_LinphoneRegistrationFailed += 1
        else:
            raise Exception("Unexpected registration state")

    @classmethod
    def authentication_requested(cls, lc, auth_info, method):
        manager = lc.user_data()
        linphonetester_logger.info("[TESTER] Auth info requested  for user id [{username}] at realm [{realm}]".format(
            username=auth_info.username, realm=auth_info.realm))
        manager.stats.number_of_auth_info_requested +=1

    @classmethod
    def call_state_changed(cls, lc, call, state, msg):
        manager = lc.user_data()
        to_address = call.call_log.to_address.as_string()
        from_address = call.call_log.from_address.as_string()
        direction = "Outgoing"
        if call.call_log.dir == linphone.CallDir.Incoming:
            direction = "Incoming"
        linphonetester_logger.info("[TESTER] {direction} call from [{from_address}] to [{to_address}], new state is [{state}]".format(
            direction=direction, from_address=from_address, to_address=to_address, state=linphone.CallState.string(state)))
        if state == linphone.CallState.IncomingReceived:
            manager.stats.number_of_LinphoneCallIncomingReceived += 1
        elif state == linphone.CallState.OutgoingInit:
            manager.stats.number_of_LinphoneCallOutgoingInit += 1
        elif state == linphone.CallState.OutgoingProgress:
            manager.stats.number_of_LinphoneCallOutgoingProgress += 1
        elif state == linphone.CallState.OutgoingRinging:
            manager.stats.number_of_LinphoneCallOutgoingRinging += 1
        elif state == linphone.CallState.OutgoingEarlyMedia:
            manager.stats.number_of_LinphoneCallOutgoingEarlyMedia += 1
        elif state == linphone.CallState.Connected:
            manager.stats.number_of_LinphoneCallConnected += 1
        elif state == linphone.CallState.StreamsRunning:
            manager.stats.number_of_LinphoneCallStreamsRunning += 1
        elif state == linphone.CallState.Pausing:
            manager.stats.number_of_LinphoneCallPausing += 1
        elif state == linphone.CallState.Paused:
            manager.stats.number_of_LinphoneCallPaused += 1
        elif state == linphone.CallState.Resuming:
            manager.stats.number_of_LinphoneCallResuming += 1
        elif state == linphone.CallState.Refered:
            manager.stats.number_of_LinphoneCallRefered += 1
        elif state == linphone.CallState.Error:
            manager.stats.number_of_LinphoneCallError += 1
        elif state == linphone.CallState.End:
            manager.stats.number_of_LinphoneCallEnd += 1
        elif state == linphone.CallState.PausedByRemote:
            manager.stats.number_of_LinphoneCallPausedByRemote += 1
        elif state == linphone.CallState.UpdatedByRemote:
            manager.stats.number_of_LinphoneCallUpdatedByRemote += 1
        elif state == linphone.CallState.IncomingEarlyMedia:
            manager.stats.number_of_LinphoneCallIncomingEarlyMedia += 1
        elif state == linphone.CallState.Updating:
            manager.stats.number_of_LinphoneCallUpdating += 1
        elif state == linphone.CallState.Released:
            manager.stats.number_of_LinphoneCallReleased += 1
        else:
            raise Exception("Unexpected call state")

    @classmethod
    def message_received(cls, lc, room, message):
        manager = lc.user_data()
        from_str = message.from_address.as_string()
        text_str = message.text
        external_body_url = message.external_body_url
        linphonetester_logger.info("[TESTER] Message from [{from_str}] is [{text_str}], external URL [{external_body_url}]".format(
            from_str=from_str, text_str=text_str, external_body_url=external_body_url))
        manager.stats.number_of_LinphoneMessageReceived += 1
        manager.stats.last_received_chat_message = message
        if message.file_transfer_information is not None:
            manager.stats.number_of_LinphoneMessageReceivedWithFile += 1
        elif message.external_body_url is not None:
            manager.stats.number_of_LinphoneMessageExtBodyReceived += 1

    @classmethod
    def new_subscription_requested(cls, lc, lf, url):
        manager = lc.user_data()
        linphonetester_logger.info("[TESTER] New subscription request: from [{from_str}], url [{url}]".format(
            from_str=lf.address.as_string(), url=url))
        manager.stats.number_of_NewSubscriptionRequest += 1
        lc.default_friend_list.add_friend(lf) # Accept subscription

    @classmethod
    def notify_presence_received(cls, lc, lf):
        manager = lc.user_data()
        linphonetester_logger.info("[TESTER] New notify request: from [{from_str}]".format(
            from_str=lf.address.as_string()))
        manager.stats.number_of_NotifyPresenceReceived += 1
        manager.stats.last_received_presence = lf.presence_model
        if manager.stats.last_received_presence.basic_status == linphone.PresenceBasicStatus.Open:
            manager.stats.number_of_LinphonePresenceBasicStatusOpen += 1
        elif manager.stats.last_received_presence.basic_status == linphone.PresenceBasicStatus.Closed:
            manager.stats.number_of_LinphonePresenceBasicStatusClosed += 1
        else:
            linphonetester_logger.error("[TESTER] Unexpected basic status {status}".format(status=manager.status.last_received_presence.basic_status))
        for i in range(0, manager.stats.last_received_presence.nb_activities):
            acttype = manager.stats.last_received_presence.get_nth_activity(i).type
            if acttype == linphone.PresenceActivityType.Offline:
                manager.stats.number_of_LinphonePresenceActivityOffline += 1
            elif acttype == linphone.PresenceActivityType.Online:
                manager.stats.number_of_LinphonePresenceActivityOnline += 1
            elif acttype == linphone.PresenceActivityType.Appointment:
                manager.stats.number_of_LinphonePresenceActivityAppointment += 1
            elif acttype == linphone.PresenceActivityType.Away:
                manager.stats.number_of_LinphonePresenceActivityAway += 1
            elif acttype == linphone.PresenceActivityType.Breakfast:
                manager.stats.number_of_LinphonePresenceActivityBreakfast += 1
            elif acttype == linphone.PresenceActivityType.Busy:
                manager.stats.number_of_LinphonePresenceActivityBusy += 1
            elif acttype == linphone.PresenceActivityType.Dinner:
                manager.stats.number_of_LinphonePresenceActivityDinner += 1
            elif acttype == linphone.PresenceActivityType.Holiday:
                manager.stats.number_of_LinphonePresenceActivityHoliday += 1
            elif acttype == linphone.PresenceActivityType.InTransit:
                manager.stats.number_of_LinphonePresenceActivityInTransit += 1
            elif acttype == linphone.PresenceActivityType.LookingForWork:
                manager.stats.number_of_LinphonePresenceActivityLookingForWork += 1
            elif acttype == linphone.PresenceActivityType.Lunch:
                manager.stats.number_of_LinphonePresenceActivityLunch += 1
            elif acttype == linphone.PresenceActivityType.Meal:
                manager.stats.number_of_LinphonePresenceActivityMeal += 1
            elif acttype == linphone.PresenceActivityType.Meeting:
                manager.stats.number_of_LinphonePresenceActivityMeeting += 1
            elif acttype == linphone.PresenceActivityType.OnThePhone:
                manager.stats.number_of_LinphonePresenceActivityOnThePhone += 1
            elif acttype == linphone.PresenceActivityType.Other:
                manager.stats.number_of_LinphonePresenceActivityOther += 1
            elif acttype == linphone.PresenceActivityType.Performance:
                manager.stats.number_of_LinphonePresenceActivityPerformance += 1
            elif acttype == linphone.PresenceActivityType.PermanentAbsence:
                manager.stats.number_of_LinphonePresenceActivityPermanentAbsence += 1
            elif acttype == linphone.PresenceActivityType.Playing:
                manager.stats.number_of_LinphonePresenceActivityPlaying += 1
            elif acttype == linphone.PresenceActivityType.Presentation:
                manager.stats.number_of_LinphonePresenceActivityPresentation += 1
            elif acttype == linphone.PresenceActivityType.Shopping:
                manager.stats.number_of_LinphonePresenceActivityShopping += 1
            elif acttype == linphone.PresenceActivityType.Sleeping:
                manager.stats.number_of_LinphonePresenceActivitySleeping += 1
            elif acttype == linphone.PresenceActivityType.Spectator:
                manager.stats.number_of_LinphonePresenceActivitySpectator += 1
            elif acttype == linphone.PresenceActivityType.Steering:
                manager.stats.number_of_LinphonePresenceActivitySteering += 1
            elif acttype == linphone.PresenceActivityType.Travel:
                manager.stats.number_of_LinphonePresenceActivityTravel += 1
            elif acttype == linphone.PresenceActivityType.TV:
                manager.stats.number_of_LinphonePresenceActivityTV += 1
            elif acttype == linphone.PresenceActivityType.Unknown:
                manager.stats.number_of_LinphonePresenceActivityUnknown += 1
            elif acttype == linphone.PresenceActivityType.Vacation:
                manager.stats.number_of_LinphonePresenceActivityVacation += 1
            elif acttype == linphone.PresenceActivityType.Working:
                manager.stats.number_of_LinphonePresenceActivityWorking += 1
            elif acttype == linphone.PresenceActivityType.Worship:
                manager.stats.number_of_LinphonePresenceActivityWorship += 1
        if manager.stats.last_received_presence.nb_activities == 0:
            if manager.stats.last_received_presence.basic_status == linphone.PresenceBasicStatus.Open:
                manager.stats.number_of_LinphonePresenceActivityOnline += 1
            else:
                manager.stats.number_of_LinphonePresenceActivityOffline += 1

    def __init__(self, rc_file = None, check_for_proxies = True, additional_cbs = None):
        cbs = linphone.Factory.get().create_core_cbs()
        cbs.registration_state_changed = CoreManager.registration_state_changed
        cbs.authentication_requested = CoreManager.authentication_requested
        cbs.call_state_changed = CoreManager.call_state_changed
        cbs.message_received = CoreManager.message_received
        cbs.new_subscription_requested = CoreManager.new_subscription_requested
        cbs.notify_presence_received = CoreManager.notify_presence_received
        self.identity = None
        self.stats = CoreManagerStats()
        rc_path = None
        if rc_file is not None:
            rc_path = os.path.join('rcfiles', rc_file)
        self.lc = CoreManager.configure_lc_from(cbs, tester_resources_path, rc_path, self)
        if additional_cbs:
            self.lc.add_callbacks(additional_cbs)
        self.check_accounts()

        self.lc.play_file = os.path.join(tester_resources_path, 'sounds', 'hello8000.wav')
        self.lc.user_certificates_path = os.getcwd()

        if check_for_proxies:
            proxy_count = len(self.lc.proxy_config_list)
        else:
            proxy_count = 0
            self.lc.network_reachable = False
        if proxy_count:
            nb_seconds = 20
            success = CoreManager.wait_for_until(self, None, lambda manager: manager.stats.number_of_LinphoneRegistrationOk == proxy_count, nb_seconds * 1000 * proxy_count)
            if not success:
                linphonetester_logger.info("[TESTER] Did not register after {nb_seconds} for {proxy_count} proxies".format(nb_seconds=nb_seconds, proxy_count=proxy_count))
        assert_equals(self.stats.number_of_LinphoneRegistrationOk, proxy_count)
        self.enable_audio_codec("PCMU", 8000)

        if self.lc.default_proxy_config is not None:
            self.lc.default_proxy_config.identity_address.clean()
        if not check_for_proxies:
            self.lc.network_reachable = True

    def enable_audio_codec(self, mime, rate):
        codecs = self.lc.audio_codecs
        for codec in codecs:
            self.lc.enable_payload_type(codec, False)
        codec = self.lc.find_payload_type(mime, rate, 1)
        assert codec is not None
        if codec is not None:
            self.lc.enable_payload_type(codec, True)

    def disable_all_audio_codecs_except_one(self, mime):
        self.enable_audio_codec(mime, -1)

    def check_accounts(self):
        pcl = self.lc.proxy_config_list
        for cfg in pcl:
            self.identity = account_manager.check_account(cfg)
