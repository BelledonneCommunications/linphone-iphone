/*
* Copyright (c) 2010-2020 Belledonne Communications SARL.
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

import UIKit
import Foundation
import Security
import CommonCrypto
import linphonesw
import os


@objc class VFSUtil: NSObject {
	
	@objc static let keyChainSharingGroup = "org.linphone.phone" // Enable Keychain Sharing capabilities in app and all app extensions that need to activate VFS and set key chain group to be the bundle ID for all and here
	@objc static let TEAM_ID = "Z2V957B3D6" // Apple TEAM ID
	
	@objc static let keyName = "\(keyChainSharingGroup).vfskey"
	@objc static let prefName = "\(keyChainSharingGroup).vfspref"
	@objc static let accessGroup = "\(TEAM_ID).\(keyChainSharingGroup)"
	
	@objc static func generateKey(requiresBiometry: Bool = false) throws {
		
		let flags: SecAccessControlCreateFlags
		if #available(iOS 11.3, *) {
			flags = requiresBiometry ?
				[.privateKeyUsage, .biometryCurrentSet] : .privateKeyUsage
		} else {
			flags = requiresBiometry ?
				[.privateKeyUsage, .touchIDCurrentSet] : .privateKeyUsage
		}
		let access =
			SecAccessControlCreateWithFlags(kCFAllocatorDefault, kSecAttrAccessibleAlwaysThisDeviceOnly,flags,nil)!
		let tag = keyName.data(using: .utf8)!
		let attributes: [String: Any] = [
			kSecAttrKeyType as String           : kSecAttrKeyTypeECSECPrimeRandom,
			kSecAttrKeySizeInBits as String     : 256,
			kSecAttrTokenID as String           : kSecAttrTokenIDSecureEnclave,
			kSecPrivateKeyAttrs as String : [
				kSecAttrIsPermanent as String       : true,
				kSecAttrApplicationTag as String    : tag,
				kSecAttrAccessControl as String     : access
			],
			kSecAttrAccessGroup as String : accessGroup
		]
		
		var error: Unmanaged<CFError>?
		guard let _ = SecKeyCreateRandomKey(attributes as CFDictionary, &error) else {
			throw error!.takeRetainedValue() as Error
		}
	}
	
	@objc static func loadKey(name: String) -> SecKey? {
		let tag = name.data(using: .utf8)!
		let query: [String: Any] = [
			kSecClass as String                 : kSecClassKey,
			kSecAttrApplicationTag as String    : tag,
			kSecAttrKeyType as String           : kSecAttrKeyTypeEC,
			kSecReturnRef as String             : true,
			kSecAttrAccessGroup as String : accessGroup
		]
		
		var item: CFTypeRef?
		let status = SecItemCopyMatching(query as CFDictionary, &item)
		guard status == errSecSuccess else {
			return nil
		}
		return (item as! SecKey)
	}
	
	
	@objc static func encrypt(clearText: String) -> String? {
		let algorithm: SecKeyAlgorithm = .eciesEncryptionCofactorX963SHA512AESGCM
		guard let privateKey = loadKey(name: keyName), let publicKey = SecKeyCopyPublicKey(privateKey),  SecKeyIsAlgorithmSupported(publicKey, .encrypt, algorithm) else {
			return nil
		}
		var error: Unmanaged<CFError>?
		let clearTextData = clearText.data(using: .utf8)!
		guard let encryptedData =  SecKeyCreateEncryptedData(publicKey, algorithm,clearTextData as CFData, &error) as Data? else {
			return nil
		}
		return encryptedData.base64EncodedString()
	}
	
	@objc static func decrypt(encryptedText: String) -> String? {
		let algorithm: SecKeyAlgorithm = .eciesEncryptionCofactorX963SHA512AESGCM
		guard let key = loadKey(name: keyName), SecKeyIsAlgorithmSupported(key, .decrypt, algorithm) else {
			return nil
		}
		var error: Unmanaged<CFError>?
		guard let clearTextData = SecKeyCreateDecryptedData(key,algorithm,Data(base64Encoded: encryptedText)! as CFData,&error) as Data? else {
			print("[VFS] failed deciphering data \(String(describing: error))")
			return nil
		}
		return String(decoding: clearTextData, as: UTF8.self)
	}
	
	
	@objc static func addSecuredPreference(key:String, value:String) -> Bool {
		let delQuery: [String: Any] = [kSecClass as String: kSecClassGenericPassword,
									   kSecAttrAccount as String: key.data(using: .utf8)!,
									   kSecAttrAccessGroup as String : accessGroup]
		SecItemDelete(delQuery as CFDictionary)
		
		
		let insertQUery: [String: Any] = [kSecClass as String: kSecClassGenericPassword,
										  kSecAttrAccessGroup as String : accessGroup,
										  kSecAttrAccessible as String : kSecAttrAccessibleAlwaysThisDeviceOnly,
										  kSecAttrService as String: Bundle.main.bundleIdentifier!,
										  kSecAttrAccount as String: key.data(using: .utf8)!,
										  kSecValueData as String:value.data(using: .utf8)!]
		let insertStatus = SecItemAdd(insertQUery as CFDictionary, nil)
		log("[VFS] addSecuredPreference : SecItemAdd status \(insertStatus)", .info)
		return  insertStatus == errSecSuccess
		
	}
	
	@objc static func deleteSecurePreference(key:String) {
		let delQuery: [String: Any] = [kSecClass as String: kSecClassGenericPassword,
									   kSecAttrAccount as String: key.data(using: .utf8)!,
									   kSecAttrAccessGroup as String : accessGroup]
		let deleteSatus = SecItemDelete(delQuery as CFDictionary)
		log("[VFS] deleteSecurePreference : SecItemDelete status for removing key \(key) = \(deleteSatus)", .info)
	}
	
	
	
	@objc static func getSecuredPreference(key:String) -> String? {
		let query: [String:Any] = [
			kSecClass as String: kSecClassGenericPassword,
			kSecAttrAccount as String: key.data(using: .utf8)!,
			kSecReturnData as String: kCFBooleanTrue,
			kSecAttrAccessGroup as String : accessGroup,
		]
		
		var result: AnyObject?
		let status: OSStatus = withUnsafeMutablePointer(to: &result) {
			SecItemCopyMatching(query as CFDictionary, UnsafeMutablePointer($0))
		}
		log("[VFS] getSecuredPreference : SecItemCopyMatching status \(status)", .info)
		return status == errSecSuccess ?  String(decoding: result as! Data , as: UTF8.self) : nil
	}
	
	@objc static func randomSha512() -> String {
		let data = UUID.init().uuidString.data(using: .utf8)!
		var digest = [UInt8](repeating: 0, count: Int(CC_SHA512_DIGEST_LENGTH))
		data.withUnsafeBytes({
			_ = CC_SHA512($0, CC_LONG(data.count), &digest)
		})
		return digest.map({ String(format: "%02hhx", $0) }).joined(separator: "")
	}
	
	
	
	@objc static func activateVFS(forFirstTime: Bool = false) -> Bool {
		do {
			if (forFirstTime) {
				removeExistingVFSKeyIfAny()
			}
			if (getSecuredPreference(key: prefName) == nil) {
				log("[VFS] no secret key set, building one.", .info)
				try generateKey(requiresBiometry: false)
				guard let encryptedHash = encrypt(clearText: randomSha512()) else {
					return false
				}
				if (!addSecuredPreference(key: prefName, value: encryptedHash)) {
					log("[VFS] Unable to save encrypted key in secured defaults.", .error)
				}
			}
			guard let encryptedKey = getSecuredPreference(key: prefName) else {
				log("[VFS] Unable to retrieve encrypted key.", .error)
				return false
			}
			guard let secret = decrypt(encryptedText: encryptedKey) else {
				log("[VFS] Unable to decryt encrypted key.", .error)
				removeExistingVFSKeyIfAny()
				return false
			}
			Factory.Instance.setVfsEncryption(encryptionModule: 2, secret: secret, secretSize: 32)
			log("[VFS] activated", .info)
			return true
		} catch {
			log("[VFS] Error setting up VFS: \(error)", .info)
			return false
		}
	}
	
	@objc static func vfsEnabled(groupName: String) -> Bool {
		let defaults = UserDefaults.init(suiteName: groupName)
		if (defaults == nil) {
			log("[VFS] Unable to get VFS enabled preference userDefaults is null",.error);
		}
		return defaults?.bool(forKey: "vfs_enabled_preference") == true
	}
	
	@objc static func setVfsEnabbled(enabled: Bool, groupName: String) {
		let defaults = UserDefaults.init(suiteName: groupName)
		if (defaults == nil) {
			log("[VFS] Unable to set VFS enabled preferece userDefaults is null",.error);
		}
		defaults?.setValue(enabled, forKey: "vfs_enabled_preference")
	}
	
	@objc static func log(_ log:String, _ level: OSLogType) {
		switch (level) {
		case.info:LoggingService.Instance.message(message: log)
		case.debug:LoggingService.Instance.debug(message: log)
		case.error:LoggingService.Instance.error(message: log)
		case.fault:LoggingService.Instance.fatal(message: log)
		default:LoggingService.Instance.message(message: log)
		}
		if #available(iOS 10.0, *) {
				os_log("%{public}@", type: level,log)
		} else {
			NSLog(log)
		}
	}
	
	@objc static func removeExistingVFSKeyIfAny() {
		log("[VFS] removing existing key if any",.debug)
		if (getSecuredPreference(key: prefName) != nil) {
			deleteSecurePreference(key: prefName)
		}
	}
	
	
	
}
