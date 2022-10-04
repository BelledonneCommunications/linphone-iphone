import XCTest

class ActiveCallUITests: XCTestCase {
	var methods: ActiveCallViewUITestsMethods!

	override func setUpWithError() throws {
		continueAfterFailure = true
		UITestsUtils.testAppSetup()
		methods = ActiveCallViewUITestsMethods() //to reload accounts infos if testAppSetup change them
	}


    func testViewDisplay() throws {
		methods.startActiveCall()
		methods.endCall()
    }
    
    func testOpenCallStats() throws {
		methods.startActiveCall()
        methods.openCallStatsFromStatusBar()
		methods.endCall()
    }
    
    func testCloseCallStats() throws {
		methods.startActiveCall()
        methods.openCallStatsFromStatusBar()
        methods.closeCallStatsFromStatusBar()
		methods.endCall()
    }
    
    func testCallRecord() throws {
		methods.startActiveCall()
        methods.startCallRecord()
        methods.stopCallRecord()
		methods.endCall()
    }
    
    func testRemoteCallRecord() throws {
		methods.startActiveCall()
        methods.startCallRecord(remote: true)
        methods.stopCallRecord(remote: true)
		methods.endCall()
    }
    
    func testPauseCall() throws {
		methods.startActiveCall()
        methods.pauseActiveCall()
		methods.endCall()
    }
    
    func testResumeCall() throws {
		methods.startActiveCall()
        methods.pauseActiveCall()
        methods.resumeActiveCall()
		methods.endCall()
    }
    
    func testRemotePauseCall() throws {
		methods.startActiveCall()
        methods.pauseRemoteCall()
		methods.endCall()
    }
    
    func testRemoteResumeCall() throws {
		methods.startActiveCall()
        methods.pauseRemoteCall()
        methods.resumeRemoteCall()
		methods.endCall()
    }
    
    func testToggleControls() throws {
		methods.startActiveCall()
		methods.toggleCallControls(buttonTag: "speaker", parentView: methods.app.activeCallView)
		methods.toggleCallControls(buttonTag: "mute",parentView: methods.app.activeCallView)
		methods.endCall()
    }
    
    func testOpenExtraMenu() throws {
		methods.startActiveCall()
        methods.openExtraButtonMenu()
		methods.endCall()
    }
    
    func testCloseExtraMenu() throws {
		methods.startActiveCall()
        methods.openExtraButtonMenu()
        methods.closeExtraButtonMenu()
		methods.endCall()
    }
    
    func testHangup() throws {
		methods.startActiveCall()
        methods.hangupActiveCall()
    }
}

