import XCTest
import linphonesw

class IncomingOutgoingCallViewUITestsMethods {
    
	let app = XCUIApplication()
	let manager = UITestsCoreManager.instance
	let appAccountAuthInfo: AuthInfo = UITestsCoreManager.instance.appAccountAuthInfo!
	let ghostAccount: UITestsRegisteredLinphoneCore = UITestsCoreManager.instance.ghostAccounts[0]

    func startIncomingCall() {
		XCTContext.runActivity(named: "Start Incoming Call") { _ in
			if (ghostAccount.callState != .Released) {ghostAccount.terminateCall()}
			app.representationWithElements.makeBackup(named: "call_end")
			
			ghostAccount.startCall(adress: manager.createAdress(authInfo: appAccountAuthInfo))
			ghostAccount.waitForCallState(callState: .OutgoingRinging, timeout: 5)
			
			_ = app.callView.waitForExistence(timeout: 5)
			checkCallTime(element: app.callView.staticTexts["IO_call_view_duration"])
			app.callView.images["IO_call_view_spinner"].representation.isAnimated(timeInterval: 0.5)
			
			//app.callView.representation.reMake()
			//app.statusBar.representation.withVariations(named: ["call_view"]).reMake()
			app.representationWithElements.mainView = app.callView
			app.representationWithElements.withElementVariations(mainView: [], statusBar: ["call_view"], tabBar: []).check()
		}
    }
    
    func startOutgoingCall() {
		XCTContext.runActivity(named: "Start Outgoing Call") { _ in
			if (ghostAccount.callState != .Released) {ghostAccount.terminateCall()}
			if (!app.dialerView.exists) { app.launch()}
			app.representationWithElements.makeBackup(named: "call_end")
			
			app.dialerView.textFields["adress_field"].fillTextField(ghostAccount.mAuthInfo.username)
			checkCallTime(element: app.callView.staticTexts["IO_call_view_duration"])
			
			//app.callView.representation.withVariations(named: ["outgoing"]).reMake()
			//app.statusBar.representation.withVariations(named: ["call_view"]).reMake()
			app.representationWithElements.mainView = app.callView
			app.representationWithElements.withElementVariations(mainView: ["outgoing"], statusBar: ["call_view"], tabBar: []).check()
			
			ghostAccount.waitForCallState(callState: .IncomingReceived, timeout: 5)
		}
    }

	func endCall() {
		XCTContext.runActivity(named: "End Call (from remote)") { _ in
			if (ghostAccount.callState == .Released) {return}
			
			ghostAccount.terminateCall()
			ghostAccount.waitForCallState(callState: .Released, timeout: 5)
			
			app.representationWithElements.reloadBackup(named: "call_end").check()
		}
    }
    
	//expected format : "mm:ss"
    func checkCallTime(element: XCUIElement) {
			XCTContext.runActivity(named: "Check call time increment") { _ in
				let timerArray: [Int] = (0..<3).map{_ in
					sleep(1)
					return Int(element.label.split(separator: ":").last ?? "") ?? 0
				}
				XCTAssert(Set(timerArray).count >= 2, "Call Time is not correctly incremented, less than 2 differents values are displayed in 3 seconds")
				XCTAssert(timerArray == timerArray.sorted(), "Call Time is not correctly incremented, it is not increasing")
				XCTAssert(timerArray.first! <= 3, "Call Time is not correctly initialized, it is more than 3 right after the start (found: \(timerArray.first!))")
			}
    }

    
	func toggleCallControls(buttonTag: String, parentView: XCUIElement) {
		XCTContext.runActivity(named: "Toggle call control Button : \"\(buttonTag)\"") { _ in
			app.representationWithElements.makeBackup(named: buttonTag)
			parentView.buttons["call_control_view_\(buttonTag)"].tap()
			app.representationWithElements.updateElementVariations(mainView: [buttonTag], statusBar: [], tabBar: []).check()
			
			parentView.buttons["call_control_view_\(buttonTag)"].tap()
			app.representationWithElements.reloadBackup(named: buttonTag).check()
		}
    }

	func noAnswerIncomingCall() {
		XCTContext.runActivity(named: "Let Incoming Call Ring Until Stop") { _ in
			XCTAssert(app.callView.waitForExistence(timeout: 5), "call already abort after less than 10 seconds ringing")
			XCTAssert(app.callView.waitForNonExistence(timeout: 30), "call still not abort after 30 seconds ringing")
			ghostAccount.waitForCallState(callState: .Released, timeout: 5)
			app.representationWithElements.reloadBackup(named: "call_end").check()
		}
    }
    
    func noAnswerOutgoingCall() {
		XCTContext.runActivity(named: "Check Outgoing Call Failed Popup Integrity And Close") { context in
			XCTAssert(app.callView.waitForExistence(timeout: 5), "call already abort after less than 10 seconds ringing")
			XCTAssert(app.callView.waitForNonExistence(timeout: 30), "call still not abort after 30 seconds ringing")
			ghostAccount.waitForCallState(callState: .Released, timeout: 5)
			app.callFailedView.representation.check()
			app.callFailedView.buttons["call_failed_error_view_action"].tap()
			app.representationWithElements.reloadBackup(named: "call_end").check()
		}
		
    }
    
    func cancelOutgoingCall() {
		XCTContext.runActivity(named: "Cancel Outgoing Call") { _ in
			app.callView.buttons["O_call_view_cancel"].tap()
			app.representationWithElements.reloadBackup(named: "call_end").check()
			ghostAccount.waitForCallState(callState: .Released, timeout: 5)
		}
    }
     
	func declineIncomingCall() {
		XCTContext.runActivity(named: "Decline Incoming Call") { _ in
			app.callView.buttons["I_call_view_decline"].tap()
			app.representationWithElements.reloadBackup(named: "call_end").check()
			ghostAccount.waitForCallState(callState: .Released, timeout: 5)
		}
    }

	func acceptIncomingCall() {
		XCTContext.runActivity(named: "Accept Incoming Call") { _ in
			app.callView.buttons["I_call_view_accept"].tap()
			checkCallTime(element: app.activeCallView.staticTexts["active_call_upper_section_duration"])
			app.representationWithElements.mainView = app.activeCallView
			//app.activeCallView.representation.reMake()
			app.representationWithElements.check()
			ghostAccount.waitForCallState(callState: .StreamsRunning, timeout: 5)
		}
    }

}

