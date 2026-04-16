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
import linphonesw
@testable import LinphoneApp

class MDMManagerTests: XCTestCase {

	private let managedKey = "com.apple.configuration.managed"
	private let dummyRootCa = """
	-----BEGIN CERTIFICATE-----
	MIIDdummyTESTCERTIFICATEVALUEForUnitTestsOnly1234567890abcdefABCDEF
	-----END CERTIFICATE-----
	"""

	override func setUp() {
		super.setUp()
		UserDefaults.standard.removeObject(forKey: managedKey)
		UserDefaults.standard.removeObject(forKey: "MDMManager.hasMDMConfig")
		UserDefaults.standard.removeObject(forKey: "MDMManager.lastConfigSHA256")
	}

	override func tearDown() {
		UserDefaults.standard.removeObject(forKey: managedKey)
		UserDefaults.standard.removeObject(forKey: "MDMManager.hasMDMConfig")
		UserDefaults.standard.removeObject(forKey: "MDMManager.lastConfigSHA256")
		super.tearDown()
	}

	func testApplyMdmConfigSetsRootCa() throws {
		let mdmConfig: [String: Any] = ["root-ca": dummyRootCa]
		UserDefaults.standard.set(mdmConfig, forKey: managedKey)

		let config = Config.newForSharedCore(
			appGroupId: Bundle.main.object(forInfoDictionaryKey: "APP_GROUP_NAME") as? String ?? "group.test",
			configFilename: "linphonerc-test",
			factoryConfigFilename: nil
		)

		let core = try Factory.Instance.createCoreWithConfig(config: config!, systemContext: nil)

		let appliedExpectation = expectation(forNotification: MDMManager.configurationAppliedNotification, object: nil) { notification in
			guard let config = notification.userInfo?["config"] as? [String: Any] else { return false }
			return (config["root-ca"] as? String) == self.dummyRootCa
		}

		MDMManager.shared.applyMdmConfigToCore(core: core)

		wait(for: [appliedExpectation], timeout: 5)

		XCTAssertEqual(core.rootCaData, dummyRootCa,
					   "core.rootCaData should equal the MDM-provided root-ca after applyMdmConfigToCore")
	}

}
