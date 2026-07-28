/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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

final class TestContext {

	static let shared = TestContext()

	let app = XCUIApplication()

	private init() {}

	func launch() {
		app.launch()
	}

	func launchOnDialer(lastManuallyDialedNumber: String) {
		app.launchEnvironment["UITEST_MDM_CONFIG"] = managedConfigJSON()
		app.launchEnvironment["UITEST_LAST_DIALED"] = lastManuallyDialedNumber
		if app.state == .runningForeground {
			app.terminate()
		}
		app.launch()
		dismissOnboardingIfNeeded()
	}

	private func dismissOnboardingIfNeeded() {
		let welcomeSkip = app.buttons["welcome_skip_button"]
		if welcomeSkip.waitForExistence(timeout: 5) {
			welcomeSkip.tap()
		}
		let permissionsSkip = app.buttons["permissions_skip_button"]
		if permissionsSkip.waitForExistence(timeout: 5) {
			permissionsSkip.tap()
		}
	}

	private func managedConfigJSON() -> String {
		let config: [String: Any] = ["xmlConfig": xmlConfig()]
		let data = (try? JSONSerialization.data(withJSONObject: config)) ?? Data()
		return String(data: data, encoding: .utf8) ?? ""
	}

	private func xmlConfig() -> String {
		"""
		<?xml version="1.0" encoding="UTF-8"?>
		<config xmlns="http://www.linphone.org/xsds/lpconfig.xsd">
		  <section name="proxy_0">
		    <entry name="reg_proxy">&lt;sip:sip.example.org;transport=tls&gt;</entry>
		    <entry name="reg_identity">sip:uitest@sip.example.org</entry>
		    <entry name="reg_expires">3600</entry>
		    <entry name="reg_sendregister">0</entry>
		    <entry name="publish">0</entry>
		  </section>
		  <section name="auth_info_0">
		    <entry name="username">uitest</entry>
		    <entry name="passwd">uitest</entry>
		    <entry name="realm">sip.example.org</entry>
		    <entry name="domain">sip.example.org</entry>
		  </section>
		  <section name="sip">
		    <entry name="default_proxy">0</entry>
		  </section>
		</config>
		"""
	}
}
