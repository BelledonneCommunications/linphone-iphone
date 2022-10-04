import XCTest

class ActiveCallViewUITestsMethods : IncomingOutgoingCallViewUITestsMethods {
    
    func startActiveCall() {
		XCTContext.runActivity(named: "Start Active Call") { _ in
        	startIncomingCall()
        	acceptIncomingCall()
		}
    }
    
    func openCallStatsFromStatusBar() {
		XCTContext.runActivity(named: "Display Call Stats From Status Bar") { _ in
			app.representationWithElements.makeBackup(named: "call_stats_closed")
			app.statusBar.buttons["status_bar_incall_quality"].tap()
			
			//app.callStatsView.representation.reMake()
			app.representationWithElements.otherElement = app.callStatsView
			app.representationWithElements.withElementVariations(mainView: ["shadow"], statusBar: ["call_view"], tabBar: []).check()
			
		}
    }
	
	func closeCallStatsFromStatusBar() {
		XCTContext.runActivity(named: "Hide Call Stats From Status Bar") { _ in
			app.statusBar.buttons["status_bar_incall_quality"].tap()
			app.representationWithElements.reloadBackup(named: "call_stats_closed").check()
		}
	}
    
    func startCallRecord(remote: Bool = false) {
		XCTContext.runActivity(named: "Start \(remote ? "Remote" : "") Call Record") { _ in
			app.representationWithElements.makeBackup(named: "record_end")
			if (!remote) {
				app.activeCallView.buttons["active_call_center_section_record"].tap()
				//app.activeCallView.representation.withVariations(named: ["record"]).reMake()
				app.representationWithElements.updateElementVariations(mainView: ["record"], statusBar: [], tabBar: []).check()
			} else {
				ghostAccount.startRecording()
				//app.activeCallView.representation.withVariations(named: ["remote_record"]).reMake()
				app.representationWithElements.updateElementVariations(mainView: ["remote_record"], statusBar: [], tabBar: []).check()
			}
			ghostAccount.waitForRecordingState(recording: true, onRemote: !remote, timeout: 5)
		}
    }
    
    func stopCallRecord(remote: Bool = false) {
		XCTContext.runActivity(named: "Stop \(remote ? "Remote" : "") Call Record") { _ in
			if (!remote) {
				app.activeCallView.buttons["active_call_center_section_record"].tap()
			} else {
				ghostAccount.mCore.currentCall?.stopRecording()
			}
			ghostAccount.waitForRecordingState(recording: false, onRemote: !remote, timeout: 5)
			app.representationWithElements.reloadBackup(named: "record_end").check()
		}
    }
    
    func pauseActiveCall() {
		XCTContext.runActivity(named: "Pause Active Call") { _ in
			app.representationWithElements.makeBackup(named: "pause_end")
			app.activeCallView.buttons["active_call_center_section_pause"].tap()
			//app.activeCallView.representation.withVariations(named: ["pause"]).reMake()
			app.representationWithElements.updateElementVariations(mainView: ["pause_shadow","pause"], statusBar: [], tabBar: []).check()
			ghostAccount.waitForCallState(callState: .PausedByRemote, timeout: 5)
		}
    }
    
    func resumeActiveCall() {
		XCTContext.runActivity(named: "Resume Active Call") { _ in
			app.activeCallView.images["paused_call_view_icon"].tap()
			app.representationWithElements.reloadBackup(named: "pause_end").check()
			ghostAccount.waitForCallState(callState: .StreamsRunning, timeout: 5)
		}
    }
    
    func pauseRemoteCall() {
		XCTContext.runActivity(named: "Pause Remote Call") { _ in
			app.representationWithElements.makeBackup(named: "pause_end")
        	ghostAccount.pauseCall()
			ghostAccount.waitForCallState(callState: .Paused, timeout: 5)
			//app.activeCallView.representation.withVariations(named: ["remote_pause"]).reMake()
			app.representationWithElements.updateElementVariations(mainView: ["pause_shadow","remote_pause"], statusBar: [], tabBar: []).check()
		}
    }
	
	func resumeRemoteCall() {
		XCTContext.runActivity(named: "Resume Remote Call") { _ in
			ghostAccount.resumeCall()
			ghostAccount.waitForCallState(callState: .StreamsRunning, timeout: 5)
			app.representationWithElements.reloadBackup(named: "pause_end").check()
		}
	}
    
    func openExtraButtonMenu() {
		XCTContext.runActivity(named: "Open Extra Menu Button") { _ in
			app.representationWithElements.makeBackup(named: "extra_menu_closed")
			app.activeCallView.buttons["active_call_view_extra_buttons"].tap()
			//app.activeCallView.representation.withVariations(named: ["extra_menu"]).reMake()
			app.representationWithElements.updateElementVariations(mainView: ["shadow","extra_menu"], statusBar: [], tabBar: []).check()
		}
    }
    
    func closeExtraButtonMenu() {
		XCTContext.runActivity(named: "Check Extra Menu View Integrity") { _ in
			app.activeCallView.otherElements["active_call_view_shading_mask"].tap()
			app.representationWithElements.reloadBackup(named: "extra_menu_closed").check()
		}
    }
    
    func hangupActiveCall() {
		XCTContext.runActivity(named: "Hangup Active Call") { _ in
			app.activeCallView.buttons["active_call_view_hangup"].tap()
			app.representationWithElements.reloadBackup(named: "call_end").check()
			ghostAccount.waitForCallState(callState: .Released, timeout: 5)
		}
    }
    
}

