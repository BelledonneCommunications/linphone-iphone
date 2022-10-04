import XCTest

class ExtraMenuUITests: XCTestCase {
	var methods: ExtraMenuActiveCallActionsUITestsMethods!

	override func setUpWithError() throws {
		continueAfterFailure = true
		UITestsUtils.testAppSetup()
		methods = ExtraMenuActiveCallActionsUITestsMethods() //to reload accounts infos if testAppSetup change them
	}


    func testViewDisplay() throws {
		methods.displayExtraMenuButtonView()
		methods.endCall()
    }
	
	func testOpenCallStats() throws {
		methods.displayExtraMenuButtonView()
		methods.openCallStatsFromExtraMenuButtonView()
		methods.endCall()
	}
	
	func testCloseCallStats() throws {
		methods.displayExtraMenuButtonView()
		methods.openCallStatsFromExtraMenuButtonView()
		methods.closeCallStatsFromItself()
		methods.endCall()
	}
	
	func testOpenCallNumpad() throws {
		methods.displayExtraMenuButtonView()
		methods.openCallNumpad()
		methods.endCall()
	}
	
	func testCloseCallNumpad() throws {
		methods.displayExtraMenuButtonView()
		methods.openCallNumpad()
		methods.closeCallNumpad()
		methods.endCall()
	}
	
	func testNumpadtyping() {
		methods.displayExtraMenuButtonView()
		methods.openCallNumpad()
		methods.composeNumpadNumbers()
		methods.endCall()
	}

	
	//to complete with other buttons
}
