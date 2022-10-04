import XCTest

class OutgoingCallUITests: XCTestCase {
	var methods: IncomingOutgoingCallViewUITestsMethods!

	override func setUpWithError() throws {
		continueAfterFailure = true
		UITestsUtils.testAppSetup()
		methods = IncomingOutgoingCallViewUITestsMethods() //to reload accounts infos if testAppSetup change them
	}
	
    func testViewDisplay() throws {
		methods.startOutgoingCall()
		methods.endCall()
    }
    
    func testNoAnswer() throws {
		methods.startOutgoingCall()
        methods.noAnswerOutgoingCall()
		
    }
    
    func testToggleMute() throws {
		methods.startOutgoingCall()
		methods.toggleCallControls(buttonTag: "mute", parentView: methods.app.callView)
		methods.endCall()
    }
    
    func testToggleSpeaker() throws {
        methods.startOutgoingCall()
        methods.toggleCallControls(buttonTag: "speaker", parentView: methods.app.callView)
        methods.endCall()
    }
    
    func testCancel() throws {
		methods.startOutgoingCall()
		methods.cancelOutgoingCall()
	}
}
