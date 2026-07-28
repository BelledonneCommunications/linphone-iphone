/*
 * Copyright (c) 2010-2026 Belledonne Communications SARL.
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

final class DialerPage {

	let app: XCUIApplication

	init(app: XCUIApplication) {
		self.app = app
	}

	private var callButton: XCUIElement { app.buttons["dialer_call_button"] }
	private var inputField: XCUIElement { app.textFields["dialer_input"] }
	private var backspaceButton: XCUIElement { app.buttons["dialer_backspace"] }

	@discardableResult
	func open() -> DialerPage {
		let callsTab = app.buttons["Calls"]
		XCTAssertTrue(callsTab.waitForExistence(timeout: 20), "Calls tab not found")
		callsTab.tap()

		let fab = app.buttons["start_call_fab"]
		XCTAssertTrue(fab.waitForExistence(timeout: 10), "Dialer FAB (start_call_fab) not found")
		fab.tap()

		let toggle = app.buttons["dialer_keypad_toggle"]
		XCTAssertTrue(toggle.waitForExistence(timeout: 5), "Dialer keypad toggle not found")
		toggle.tap()

		XCTAssertTrue(callButton.waitForExistence(timeout: 5), "Dialer keypad not shown")
		return self
	}

	var input: String {
		(inputField.value as? String) ?? ""
	}

	func setInput(_ number: String) {
		var iterations = 0
		while !input.isEmpty && iterations < 40 {
			backspaceButton.tap()
			iterations += 1
		}
		for character in number {
			digitButton(character).tap()
		}
	}

	private func digitButton(_ character: Character) -> XCUIElement {
		if character == "0" {
			return app.buttons.matching(NSPredicate(format: "label BEGINSWITH %@", "0")).firstMatch
		}
		return app.buttons[String(character)]
	}

	func pressCall() {
		callButton.tap()
	}

	func callWasStarted() -> Bool {
		callButton.waitForNonExistence(timeout: 8)
	}

	func callWasNotStarted() -> Bool {
		!callButton.waitForNonExistence(timeout: 2)
	}
}
