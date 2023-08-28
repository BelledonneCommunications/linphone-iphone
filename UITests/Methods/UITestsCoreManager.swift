import XCTest
import linphonesw


class UITestsCoreManager {
	
	private var mCore: Core!
	private var coreVersion: String = Core.getVersion
	
	private var mAccountCreator: AccountCreator!
	
	var appAccountAuthInfo: AuthInfo!
	var ghostAccounts: UITestsGhostAccounts!
	let dnsServer = "51.255.123.121"
	
	static let instance = UITestsCoreManager()
	
	
	init() {
		LoggingService.Instance.logLevel = LogLevel.Debug
		Core.enableLogCollection(state: .Enabled)
		
		//Config account creator for flexiapi
		let config: Config! = try! Factory.Instance.createConfig(path: "\(Factory.Instance.getConfigDir(context: nil))/linphonerc")
		config.setInt(section: "account_creator", key: "backend", value: AccountCreator.Backend.FlexiAPI.rawValue)
		config.setString(section: "account_creator", key: "url", value: "http://subscribe.example.org/flexiapi/api/")
		try! mCore = Factory.Instance.createCoreWithConfig(config: config, systemContext: nil)
		mCore.dnsServersApp = [dnsServer]
		mAccountCreator = try! mCore.createAccountCreator(xmlrpcUrl: nil)
		
		try? mCore.start()

		ghostAccounts = UITestsGhostAccounts(coreCreationFunction: newRegisteredLinphoneCore)
		
		appAccountAuthInfo = mCore.authInfoList.isEmpty ? createAccount() : mCore.authInfoList[0]
	}
	
	deinit {
		mCore.stop()
		mCore = nil
	}
	
	func newRegisteredLinphoneCore() -> UITestsRegisteredLinphoneCore {
		let authInfo = mCore.authInfoList.indices.contains(ghostAccounts.count+1) ? mCore.authInfoList[ghostAccounts.count+1] : createAccount()
		return UITestsRegisteredLinphoneCore(authInfo: authInfo)
	}
	
	func createAccount() -> AuthInfo {
		XCTContext.runActivity(named: "Create new account") { _ in
			mAccountCreator.username = "uitester_\(String(Int(Date().timeIntervalSince1970*1000)).suffix(5))"
			mAccountCreator.password = String((0..<15).map{ _ in mAccountCreator!.username.randomElement()! })
			mAccountCreator.domain = "sip.example.org"
			mAccountCreator.email = "\(mAccountCreator!.username)@\(mAccountCreator!.domain)"
			mAccountCreator.transport = TransportType.Tcp
			_ = try! mAccountCreator.createAccount()
			waitForAccountCreationStatus(status: .RequestOk, timeout: 5)
			
			let authInfo = try! Factory.Instance.createAuthInfo(username: mAccountCreator.username, userid: "", passwd: mAccountCreator.password, ha1: "", realm: "", domain: mAccountCreator.domain)
			mCore.addAuthInfo(info: authInfo)
			XCTContext.runActivity(named: "username : \(mAccountCreator.username)\npassword : \(mAccountCreator.password)\ndomain : \(mAccountCreator.domain)") { _ in}
			return authInfo
		}
	}
	
	func accountsReset() {
		XCTContext.runActivity(named: "Clear all accounts") { _ in
			mCore.clearAllAuthInfo()
			ghostAccounts.reset()
			appAccountAuthInfo = createAccount()
		}
	}
	
	func createAdress(authInfo: AuthInfo) -> Address {
		return try! Factory.Instance.createAddress(addr: "sip:\(authInfo.username)@\(authInfo.domain)")
	}
	
	func waitForAccountCreationStatus(status: AccountCreator.Status, timeout: Double) {
		let expectation = XCTestExpectation(description: "account status is successfully : \(status)")
		XCTContext.runActivity(named: "Waiting for account status : \(status)") { _ in
			let accountCreatorDelegate = AccountCreatorDelegateStub(onCreateAccount: { (creator: AccountCreator, status: AccountCreator.Status, response: String) in
				if (status == status) {
					expectation.fulfill()
				}
			})
			self.mAccountCreator?.addDelegate(delegate: accountCreatorDelegate)
			let result = XCTWaiter().wait(for: [expectation], timeout: timeout)
			self.mAccountCreator?.removeDelegate(delegate: accountCreatorDelegate)
			XCTAssert(result == .completed, "\"\(status)\" account status still not verified after \(timeout) seconds")
		}
	}
}


class UITestsGhostAccounts {
	
	private var mCores = [UITestsRegisteredLinphoneCore]() {
		didSet {
			count = mCores.count
		}
	}
	private(set) var count: Int!
	
	private let newCore: (()->UITestsRegisteredLinphoneCore)!
	
	init(coreCreationFunction: @escaping ()->UITestsRegisteredLinphoneCore) {
		count = mCores.count
		newCore = coreCreationFunction
	}
	
	func reset() {
		mCores = []
	}
	
	subscript (index: Int) -> UITestsRegisteredLinphoneCore {
		while (index >= mCores.count) {
			mCores.append(newCore())
		}
		return mCores[index]
	}
	
}


class UITestsRegisteredLinphoneCore {
	
	var mCore: Core!
	var coreVersion: String = Core.getVersion
	var description: String
	
	private let manager = UITestsCoreManager.instance
	
	private(set) var mCoreDelegate : CoreDelegate!
	private(set) var mAccount: Account!
	private(set) var mAuthInfo: AuthInfo!
	
	private(set) var callState : Call.State = .Released
	private(set) var registrationState : RegistrationState = .Cleared
	
	init(authInfo: AuthInfo) {
		
		description = "Ghost Account (\(authInfo.username))"
		LoggingService.Instance.logLevel = LogLevel.Debug
		Core.enableLogCollection(state: .Enabled)
		
		try! mCore = Factory.Instance.createCore(configPath: "", factoryConfigPath: "", systemContext: nil)
		mCore.dnsServers = [manager.dnsServer]
		
		mCore.videoCaptureEnabled = true
		mCore.videoDisplayEnabled = true
		mCore.recordAwareEnabled = true
		mCore.videoActivationPolicy!.automaticallyAccept = true
		
		mCoreDelegate = CoreDelegateStub(onCallStateChanged: { (core: Core, call: Call, state: Call.State, message: String) in
			self.callState = state
			NSLog("\(call.params?.account?.params?.identityAddress) current call state is \(self.callState)\n")

		}, onAccountRegistrationStateChanged: { (core: Core, account: Account, state: RegistrationState, message: String) in
			self.registrationState = state
			NSLog("New registration state is \(state) for user id \(account.params?.identityAddress)\n")
		})
		mCore.addDelegate(delegate: mCoreDelegate)
		
		mCore.playFile = "sounds/hello8000.wav"
		mCore.useFiles = true
		
		try? mCore.start()
		
		mAuthInfo = authInfo
		login(transport: TransportType.Tcp)
	}
	
	deinit {
		mCore.stop()
		mCore = nil
		mCoreDelegate = nil
	}
	
	func login(transport: TransportType) {
		XCTContext.runActivity(named: "\(description) : Login") { _ in
			do {
				let accountParams = try mCore.createAccountParams()
				let identity = manager.createAdress(authInfo: mAuthInfo)
				try accountParams.setIdentityaddress(newValue: identity)
				let address = try Factory.Instance.createAddress(addr: String("sip:" + mAuthInfo.domain))
				try address.setTransport(newValue: transport)
				try accountParams.setServeraddress(newValue: address)
				accountParams.registerEnabled = true
				let account = try mCore.createAccount(params: accountParams)
				mCore.addAuthInfo(info: mAuthInfo)
				try mCore.addAccount(account: account)
				mAccount = account
				mCore.defaultAccount = mAccount
				waitForRegistrationState(registrationState: .Ok, timeout: 5)
				
			} catch { NSLog(error.localizedDescription) }
		}
	}
	
	private func makeRecordFilePath() -> String{
		var filePath = "recording_"
		let now = Date()
		let dateFormat = DateFormatter()
		dateFormat.dateFormat = "E-d-MMM-yyyy-HH-mm-ss"
		let date = dateFormat.string(from: now)
		filePath = filePath.appending("\(date).mkv")
		
		let paths = NSSearchPathForDirectoriesInDomains(.cachesDirectory, .userDomainMask, true)
		let writablePath = paths[0]
		return writablePath.appending("/\(filePath)")
	}
	
	func startCall(adress: Address) {
		XCTContext.runActivity(named: "\(description) : Start calling \(adress.username)") { _ in
			do {
				let params = try mCore.createCallParams(call: nil)
				params.mediaEncryption = MediaEncryption.None
				params.recordFile = makeRecordFilePath()
				let _ = mCore.inviteAddressWithParams(addr: adress, params: params)
			} catch { NSLog(error.localizedDescription) }
		}
		
	}
	
	func terminateCall() {
		XCTContext.runActivity(named: "\(description) : End call") { _ in
			do {
				if (mCore.callsNb == 0) { return }
				let coreCall = (mCore.currentCall != nil) ? mCore.currentCall : mCore.calls[0]
				if let call = coreCall {
					try call.terminate()
				}
			} catch { NSLog(error.localizedDescription) }
		}
	}
	
	func acceptCall() {
		XCTContext.runActivity(named: "\(description) : Accept call from \(mAuthInfo.username)") { _ in
			// IMPORTANT : Make sure you allowed the use of the microphone (see key "Privacy - Microphone usage description" in Info.plist) !
			do {
				// if we wanted, we could create a CallParams object
				// and answer using this object to make changes to the call configuration
				// (see OutgoingCall tutorial)
				try mCore.currentCall?.accept()
			} catch { NSLog(error.localizedDescription) }
		}
	}
	
	func toggleMicrophone() {
		// The following toggles the microphone, disabling completely / enabling the sound capture
		// from the device microphone
		mCore.micEnabled = !mCore.micEnabled
	}
	
	func toggleSpeaker() {
		// Get the currently used audio device
		let currentAudioDevice = mCore.currentCall?.outputAudioDevice
		let speakerEnabled = currentAudioDevice?.type == AudioDevice.Kind.Speaker
		
		let test = currentAudioDevice?.deviceName
		// We can get a list of all available audio devices using
		// Note that on tablets for example, there may be no Earpiece device
		for audioDevice in mCore.audioDevices {
			
			// For IOS, the Speaker is an exception, Linphone cannot differentiate Input and Output.
			// This means that the default output device, the earpiece, is paired with the default phone microphone.
			// Setting the output audio device to the microphone will redirect the sound to the earpiece.
			if (speakerEnabled && audioDevice.type == AudioDevice.Kind.Microphone) {
				mCore.currentCall?.outputAudioDevice = audioDevice
				return
			} else if (!speakerEnabled && audioDevice.type == AudioDevice.Kind.Speaker) {
				mCore.currentCall?.outputAudioDevice = audioDevice
				return
			}
			/* If we wanted to route the audio to a bluetooth headset
			else if (audioDevice.type == AudioDevice.Type.Bluetooth) {
			core.currentCall?.outputAudioDevice = audioDevice
			}*/
		}
	}
	
	func toggleVideo() {
		do {
			if (mCore.callsNb == 0) { return }
			let coreCall = (mCore.currentCall != nil) ? mCore.currentCall : mCore.calls[0]
			if let call = coreCall {
				let params = try mCore.createCallParams(call: call)
				params.videoEnabled = !(call.currentParams!.videoEnabled)
				try call.update(params: params)
			}
		} catch { NSLog(error.localizedDescription) }
	}
	
	func toggleCamera() {
		do {
			let currentDevice = mCore.videoDevice
			for camera in mCore.videoDevicesList {
				if (camera != currentDevice && camera != "StaticImage: Static picture") {
					try mCore.setVideodevice(newValue: camera)
					break
				}
			}
		} catch { NSLog(error.localizedDescription) }
	}
	
	func pauseCall() {
		do {
			if (mCore.callsNb == 0) { return }
			let coreCall = (mCore.currentCall != nil) ? mCore.currentCall : mCore.calls[0]
			try coreCall!.pause()
		} catch { NSLog(error.localizedDescription) }
	}
	
	func resumeCall() {
		do {
			if (mCore.callsNb == 0) { return }
			let coreCall = (mCore.currentCall != nil) ? mCore.currentCall : mCore.calls[0]
			try coreCall!.resume()
		} catch { NSLog(error.localizedDescription) }
	}
	
	func startRecording() {
		mCore.currentCall?.startRecording()
	}
	
	func stopRecording() {
		mCore.currentCall?.stopRecording()
	}
	
	func waitForRegistrationState(registrationState: RegistrationState, timeout: TimeInterval) {
		let expectation = XCTestExpectation(description: "registration state is successfully : \(registrationState)")
		XCTContext.runActivity(named: "Waiting for registration state : \(registrationState)") { _ in
			if (registrationState == self.registrationState) { return}
			let registeredDelegate = AccountDelegateStub(onRegistrationStateChanged: { (account: Account, state: RegistrationState, message: String) in
				if (registrationState == state) {
					expectation.fulfill()
				}
			})
			self.mCore.defaultAccount!.addDelegate(delegate: registeredDelegate)
			let result = XCTWaiter().wait(for: [expectation], timeout: timeout)
			self.mCore.defaultAccount!.removeDelegate(delegate: registeredDelegate)
			XCTAssert(result == .completed, "\"\(registrationState)\" registration state still not verified after \(timeout) seconds")
		}
	}
	
	func waitForCallState(callState: Call.State, timeout: TimeInterval) {
		let expectation = XCTestExpectation(description: "call state is successfully : \(callState)")
		XCTContext.runActivity(named: "Waiting for call state : \(callState)") { _ in
			if (callState == self.callState) { return}
			let callStateDelegate = CoreDelegateStub(onCallStateChanged: { (lc: Core, call: Call, state: Call.State, message: String) in
				if (callState == state) {
					expectation.fulfill()
				}
			})
			self.mCore.addDelegate(delegate: callStateDelegate)
			let result = XCTWaiter().wait(for: [expectation], timeout: timeout)
			self.mCore.removeDelegate(delegate: callStateDelegate)
			XCTAssert(result == .completed, "\"\(callState)\" call state still not verified after \(timeout) seconds")
		}
	}
	
	func waitForRecordingState(recording: Bool, onRemote: Bool = false, timeout: TimeInterval) {
		XCTContext.runActivity(named: "Waiting for call recording state : \(recording)") { _ in
			var result = XCTWaiter.Result.timedOut
			for _ in 0...Int(timeout) {
				if (!onRemote && recording == mCore.currentCall?.params?.isRecording) {
					result = .completed
					break
				}
				if (onRemote && recording == mCore.currentCall?.remoteParams?.isRecording) {
					result = .completed
					break
				}
				_ = XCTWaiter().wait(for: [XCTestExpectation()], timeout: 1)
			}
			let remoteText = onRemote ? "remote" : ""
			XCTAssert(result == .completed, "\(remoteText) call recording is still not \(recording) after \(timeout) seconds")
		}
	}

}
