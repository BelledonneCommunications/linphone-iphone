/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

import XCTest

class MDMChatFeatureUITests: XCTestCase {

	let app = XCUIApplication()
	private var permissionMonitor: NSObjectProtocol?

	override func setUp() {
		super.setUp()
		continueAfterFailure = false

		permissionMonitor = addUIInterruptionMonitor(withDescription: "System Permission Alert") { alert in
			let prefixMatches = ["Share All", "Allow"]
			for prefix in prefixMatches {
				let match = alert.buttons.matching(NSPredicate(format: "label BEGINSWITH %@", prefix)).firstMatch
				if match.exists {
					match.tap()
					return true
				}
			}
			let allowLabels = ["Continue", "OK", "Allow Once", "Always Allow"]
			for label in allowLabels {
				let button = alert.buttons[label]
				if button.exists {
					button.tap()
					return true
				}
			}
			return false
		}
	}

	override func tearDown() {
		if let monitor = permissionMonitor {
			removeUIInterruptionMonitor(monitor)
		}
		super.tearDown()
	}

	private func requiredEnv(_ name: String) -> String {
		guard let value = ProcessInfo.processInfo.environment[name], !value.isEmpty else {
			XCTFail("Missing required env var \(name). Export TEST_RUNNER_\(name) before running scripts/run-mdm-tests.sh")
			return ""
		}
		return value
	}

	private func launchWithMDM(_ mdm: [String: Any]) {
		let data = try! JSONSerialization.data(withJSONObject: mdm)
		app.launchEnvironment["UITEST_MDM_CONFIG"] = String(data: data, encoding: .utf8)!
		app.launch()
		_ = app.wait(for: .runningForeground, timeout: 15)
	}

	private func skipOnboardingIfShown() {
		let welcomeSkip = app.buttons["welcome_skip_button"]
		if welcomeSkip.waitForExistence(timeout: 5) {
			welcomeSkip.tap()
		}
		let permissionsSkip = app.buttons["permissions_skip_button"]
		if permissionsSkip.waitForExistence(timeout: 5) {
			permissionsSkip.tap()
		}
	}

	private func waitForMainPage(timeout: TimeInterval) -> Bool {
		let callsButton = app.buttons.matching(NSPredicate(format: "label == %@", "Calls")).firstMatch
		let springboard = XCUIApplication(bundleIdentifier: "com.apple.springboard")
		let alertPrefixes = ["Share All", "Allow", "Continue", "OK"]

		let deadline = Date().addingTimeInterval(timeout)
		while Date() < deadline {
			for prefix in alertPrefixes {
				let button = springboard.buttons.matching(
					NSPredicate(format: "label BEGINSWITH %@", prefix)
				).firstMatch
				if button.exists {
					button.tap()
					break
				}
			}
			if callsButton.exists {
				return true
			}
			_ = callsButton.waitForExistence(timeout: 1)
		}
		return false
	}

	private func dumpOnFailure(_ label: String) {
		let springboard = XCUIApplication(bundleIdentifier: "com.apple.springboard")
		print("=== APP DEBUG DESCRIPTION (\(label)) ===")
		print(app.debugDescription)
		print("=== SPRINGBOARD DEBUG DESCRIPTION =====================")
		print(springboard.debugDescription)
		print("=======================================================")
	}

	func testChatButtonHiddenWithMDMDisableChat() {
		let username = requiredEnv("LINPHONE_TEST_USERNAME")
		let ha1 = requiredEnv("LINPHONE_TEST_HA1")
		let domain = ProcessInfo.processInfo.environment["LINPHONE_TEST_DOMAIN"] ?? "sip.linphone.org"

		let xmlConfig = """
		<?xml version="1.0" encoding="UTF-8"?>
		<config xmlns="http://www.linphone.org/xsds/lpconfig.xsd">
		  <section name="ui">
		    <entry name="disable_chat_feature">1</entry>
		  </section>
		  <section name="proxy_0">
		    <entry name="reg_identity" overwrite="true">sip:\(username)@\(domain)</entry>
		    <entry name="reg_proxy" overwrite="true">&lt;sip:\(domain);transport=tls&gt;</entry>
		    <entry name="reg_route" overwrite="true">&lt;sip:\(domain);transport=tls&gt;</entry>
		    <entry name="realm" overwrite="true">\(domain)</entry>
		    <entry name="lime_server_url" overwrite="true">https://lime.linphone.org:443/lime-server/lime-server.php</entry>
		  </section>
		  <section name="auth_info_0">
		    <entry name="username" overwrite="true">\(username)</entry>
		    <entry name="domain" overwrite="true">\(domain)</entry>
		    <entry name="ha1" overwrite="true">\(ha1)</entry>
		    <entry name="realm" overwrite="true">\(domain)</entry>
		    <entry name="algorithm" overwrite="true">MD5</entry>
		  </section>
		</config>
		"""

		launchWithMDM(["xmlConfig": xmlConfig])
		skipOnboardingIfShown()

		let landed = waitForMainPage(timeout: 30)
		if !landed { dumpOnFailure("main screen not reached") }
		XCTAssertTrue(landed, "Should have landed on the main screen after Welcome + Permissions")

		let chatButton = app.buttons.matching(NSPredicate(format: "label == %@", "Conversations")).firstMatch
		XCTAssertFalse(chatButton.exists,
					   "Chat button should NOT be visible when MDM disables chat feature")
	}

	func testConfigUriMDMLandsOnMainPage() {
		let configUri = requiredEnv("LINPHONE_TEST_CONFIG_URI")

		launchWithMDM(["configUri": configUri])
		skipOnboardingIfShown()

		let landed = waitForMainPage(timeout: 60)
		if !landed { dumpOnFailure("main screen not reached with configUri \(configUri)") }
		XCTAssertTrue(landed,
					  "Should have landed on the main screen after remote provisioning from \(configUri)")
	}
}
