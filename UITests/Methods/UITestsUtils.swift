import XCTest

//
//  Definition of some main views usually used in tests.
//
//	Real utility in order to have more explicit test failed reports when child elements
//	are defined with them. When an element will be not find, the error message will specify
//	if it was because of the element non-display or its parent view non-display.
//
//  It Allow to really simplify the screenshot comparison too.
//

extension XCUIApplication {

	private static let _phoneView = XCUIApplication().descendants(matching: .other)["phone_view"]
	private static let _mainView = _phoneView.otherElements["main_view"]
	
	var phoneView: XCUIElement {get{return XCUIApplication._phoneView}}
	
	//SubViews
	private static let _callStatsView = _activeCallView.otherElements["call_stats_view"]
	private static let _extraMenuView = _activeCallView.otherElements["active_call_extra_buttons_view"]
	private static let _numpadCallView = _activeCallView.otherElements["call_numpad_view"]
	private static let _pauseCallView = _activeCallView.otherElements["paused_call_view"]
	
	var callStatsView: XCUIElement {get{return XCUIApplication._callStatsView}}
	var extraMenuView: XCUIElement {get{return XCUIApplication._extraMenuView}}
	var numpadCallView: XCUIElement {get{return XCUIApplication._numpadCallView}}
	var pauseCallView: XCUIElement {get{return XCUIApplication._pauseCallView}}

	//MainViews
	private static let _activeCallView = _mainView.otherElements["active_call_view"]
	private static let _assistantLinkView = _mainView.otherElements["assistant_link_view"]
	private static let _assistantView = _mainView.otherElements["assistant_view"]
	private static let _callView = _mainView.otherElements["IO_call_view"]
	private static let _callsListView = _mainView.otherElements["calls_list_view"]
	private static let _chatsListView = _mainView.otherElements["chats_list_view"]
	private static let _dialerView = _mainView.otherElements["dialer_view"]
	private static let _loginView = _mainView.otherElements["assistant_login_view"]
	private static let _loginWarningView = _mainView.otherElements["assistant_login_warning_view"]
	private static let _settingsView = _mainView.otherElements["settings_view"]

	var activeCallView: XCUIElement {get{return XCUIApplication._activeCallView}}
	var assistantLinkView: XCUIElement {get{return XCUIApplication._assistantLinkView}}
	var assistantView: XCUIElement {get{return XCUIApplication._assistantView}}
	var callView: XCUIElement {get{return XCUIApplication._callView}}
	var callsListView: XCUIElement {get{return XCUIApplication._callsListView}}
	var chatsListView: XCUIElement {get{return XCUIApplication._chatsListView}}
	var dialerView: XCUIElement {get{return XCUIApplication._dialerView}}
	var loginView: XCUIElement {get{return XCUIApplication._loginView}}
	var loginWarningView: XCUIElement {get{return XCUIApplication._loginWarningView}}
	var settingsView: XCUIElement {get{return XCUIApplication._settingsView}}
	
	//StatusBar
	private static let _statusBar = _phoneView.otherElements["status_bar"]
	var statusBar: XCUIElement {get{return XCUIApplication._statusBar}}
	
	//TabBar
	private static let _tabBar = _phoneView.otherElements["tab_bar"]
	var tabBar: XCUIElement {get{return XCUIApplication._tabBar}}
	
	//SideMenuView
	private static let _sideMenuView: XCUIElement = _phoneView.otherElements["side_menu_view"]
	var sideMenuView: XCUIElement {get{return XCUIApplication._sideMenuView}}
	
	//AlertPopups
	private static let _callFailedView: XCUIElement = XCUIApplication().otherElements["call_failed_error_view"]
	
	var callFailedView: XCUIElement {get{return XCUIApplication._callFailedView}}
}



class UITestsUtils {
	static let app = XCUIApplication()
	
	// function which is launched before every tests to setup
	// it checks and fix account connection status and checks the view displayed (Dialer View)
	static func testAppSetup() {
		if (app.state != .runningForeground && !app.sideMenuView.staticTexts["side_menu_view_sip_adress"].exists) {
			app.launch()
		}
		if (!rightAccountConnected() || !accountIsConnected()) {
			deleteApp() // easiest methods to clear configured accounts and reset the app
			app.launch()
			removeSystemAlerts() // accept all initial permissions
			connectAccount()
			XCTAssert(accountIsConnected(), "registration state on the Status Bar is still not : Connected after 5 seconds")
		}
	
		//app.representation.reMake()
		//app.dialerView.representation.reMake()
		app.representationWithElements.mainView = app.dialerView
		app.representationWithElements.withElementVariations(mainView: [], statusBar: [], tabBar: []).check()
	}
	
	static func accountIsConnected() -> Bool {
		XCTContext.runActivity(named: "Check connection state") { _ in
			let connection = app.statusBar.buttons["status_bar_registration_state"]
			for i in 1...5 {
				if (connection.value as? String == "Connected") {return true}
				if (connection.value as? String == "Connecté") {app.launch()} //relauch the app if it is in French to fix (happen when the app is launch manually)
				else if (i>1 && connection.value as? String != "Connection in progress") {break} // wait only if connection is in progress
				_ = XCTWaiter.wait(for: [XCTestExpectation()], timeout: 1)
			}
			return false
		}
	}
	
	static func rightAccountConnected() -> Bool {
		let manager = UITestsCoreManager.instance
		return XCTContext.runActivity(named: "Check connected adress") { _ -> Bool in
			let sipAdress = "sip:\(manager.appAccountAuthInfo.username)@\(manager.appAccountAuthInfo.domain)"
			let sipLabel = app.sideMenuView.staticTexts["side_menu_view_sip_adress"].label
			XCTContext.runActivity(named: "expected adress : \(sipAdress) \nfound adress: \(sipLabel)") {_ in}
			return sipLabel == sipAdress
		}
	}
	
	static func connectAccount() {
		let manager = UITestsCoreManager.instance
		XCTContext.runActivity(named: "Login \(manager.appAccountAuthInfo.username) in \(app)") { _ in
			manager.accountsReset()
			
			let sideMenu = app.statusBar.buttons["side_menu_button"]
			sideMenu.tap()
			app.sideMenuView.staticTexts["Settings"].tap()
			app.settingsView.staticTexts["Network"].tap()
			app.settingsView.tables.cells.containing(.staticText, identifier:"DNS Server").children(matching: .textField).element.fillTextField(manager.dnsServer)
			app.settingsView.buttons["settings_view_back"].tap()
			
			sideMenu.tap()
			app.sideMenuView.staticTexts["Assistant"].tap()
			app.assistantView.buttons["assistant_view_accept"].tap()
			app.assistantView.buttons["assistant_view_sip_login"].tap()
			app.loginWarningView.buttons["assistant_login_warning_view_skip"].tap()
			app.loginView.textFields["assistant_login_view_username_field"].fillTextField(manager.appAccountAuthInfo.username)
			app.loginView.secureTextFields["assistant_login_view_password_field"].fillTextField(manager.appAccountAuthInfo.password)
			app.loginView.textFields["assistant_login_view_domain_field"].fillTextField(manager.appAccountAuthInfo.domain)
			app.loginView.buttons["TCP"].tap()
			app.loginView.buttons["assistant_login_view_login"].tap()
		}
	}
	
	static func deleteApp() {
		XCTContext.runActivity(named: "Delete \(app)") { _ in
			app.terminate()
			let springboard = XCUIApplication(bundleIdentifier: "com.apple.springboard")
			let linphoneApp = springboard/*@START_MENU_TOKEN@*/.icons["Linphone"]/*[[".otherElements[\"Home screen icons\"]",".icons.icons[\"Linphone\"]",".icons[\"Linphone\"]"],[[[-1,2],[-1,1],[-1,0,1]],[[-1,2],[-1,1]]],[0]]@END_MENU_TOKEN@*/
			if (linphoneApp.waitForExistence(timeout: 2)) {
				linphoneApp.press(forDuration: 2)
				for i in [2,0,1] {
					let button = springboard.buttons.element(boundBy: i)
					if (button.exists || button.waitForExistence(timeout: 2)) {
						button.tap()
					}
				}
			}
		}
	}
	
	static func removeSystemAlerts() {
		XCTContext.runActivity(named: "Remove all system alerts \(app)") { _ in
			var alertsEnd = false
			while (!alertsEnd) {
				let springboard = XCUIApplication(bundleIdentifier: "com.apple.springboard")
				let alertAllowButton = springboard.buttons.element(boundBy: 1)
				if (alertAllowButton.exists || alertAllowButton.waitForExistence(timeout: 3)) {
					alertAllowButton.tap()
				} else {
					alertsEnd = true
				}
			}
		}
	}

}

//
//	Adding some features to XCUIElement like an UITestsScreenshots object called "representaion"
//	to simplify the screenshot comparison integration, a custom and complete fillTextFill methods
//	and the opposite of the native method XCUIElement.waitForExistence(timeout: TimeInterval) -> Bool
//
extension XCUIElement {
	
	func fillTextField(_ text: String) {
		tap()
		let initValue = String(value as? String ?? "")
		if (initValue != text) {
			let deleteString = (0..<initValue.count).map{_ in XCUIKeyboardKey.delete.rawValue}.joined(separator: "")
			typeText(deleteString)
			typeText(text)
		}
		typeText(XCUIKeyboardKey.return.rawValue)
	}
	
	func waitForNonExistence(timeout: TimeInterval) -> Bool {
		let doesNotExistPredicate = NSPredicate(format: "exists == false")
		let exept = XCTNSPredicateExpectation(predicate: doesNotExistPredicate, object: self)
		let result = XCTWaiter.wait(for: [exept], timeout: timeout)
		return result == .completed
	}
}

