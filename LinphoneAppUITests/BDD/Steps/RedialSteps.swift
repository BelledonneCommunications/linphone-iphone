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
import CucumberSwift
import CucumberSwiftExpressions

extension Cucumber {

	func redialSteps() {
		var dialer: DialerPage!

		func openDialer(lastManuallyDialedNumber: String) {
			TestContext.shared.launchOnDialer(lastManuallyDialedNumber: lastManuallyDialedNumber)
			dialer = DialerPage(app: TestContext.shared.app).open()
		}

		MatchAll("the user is on the dialer" as CucumberExpression) { _, _ in
			openDialer(lastManuallyDialedNumber: "")
		}

		MatchAll("the dialer input is empty" as CucumberExpression) { _, _ in
			XCTAssertEqual(dialer.input, "")
		}

		MatchAll("the user has previously dialed the number {string} manually from the dialer" as CucumberExpression) { match, _ in
			openDialer(lastManuallyDialedNumber: try match.first(\.string))
		}

		MatchAll("the user has previously called {string} from contacts" as CucumberExpression) { _, _ in
			openDialer(lastManuallyDialedNumber: "")
		}

		MatchAll("the user has previously called {string} from call history" as CucumberExpression) { _, _ in
			openDialer(lastManuallyDialedNumber: "")
		}

		MatchAll("the user has previously called {string} from favorites" as CucumberExpression) { _, _ in
			openDialer(lastManuallyDialedNumber: "")
		}

		MatchAll("the user has never dialed a number manually from the dialer" as CucumberExpression) { _, _ in
			openDialer(lastManuallyDialedNumber: "")
		}

		MatchAll("the user presses the call button" as CucumberExpression) { _, _ in
			dialer.pressCall()
		}

		MatchAll("the user presses the call button again" as CucumberExpression) { _, _ in
			dialer.pressCall()
		}

		MatchAll("the user changes the dialer input to {string}" as CucumberExpression) { match, _ in
			dialer.setInput(try match.first(\.string))
		}

		MatchAll("the dialer input contains {string}" as CucumberExpression) { match, _ in
			XCTAssertEqual(dialer.input, try match.first(\.string))
		}

		MatchAll("no call is started" as CucumberExpression) { _, _ in
			XCTAssertTrue(dialer.callWasNotStarted())
		}

		MatchAll("a call is started to {string}" as CucumberExpression) { _, _ in
			XCTAssertTrue(dialer.callWasStarted())
		}
	}
}
