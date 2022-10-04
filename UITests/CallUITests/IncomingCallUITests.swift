import XCTest

class IncomingCallUITests: XCTestCase {
	var methods: IncomingOutgoingCallViewUITestsMethods!
	
    override func setUpWithError() throws {
        continueAfterFailure = true
		UITestsUtils.testAppSetup()
		methods = IncomingOutgoingCallViewUITestsMethods() //to reload accounts infos if testAppSetup changes them
    }

    func testViewDisplay() throws {
		methods.startIncomingCall()
		methods.endCall()
    }
    
    func testNoAnswer() throws {
		methods.startIncomingCall()
        methods.noAnswerIncomingCall()
    }
    
    func testDecline() throws {
		methods.startIncomingCall()
		methods.declineIncomingCall()
		methods.endCall()
    }
    
    func testAccept() throws {
		methods.startIncomingCall()
        methods.acceptIncomingCall()
		methods.endCall()
    }
}



