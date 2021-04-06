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
	
	@objc static let keyName = Bundle.main.bundleIdentifier!+".vfskey"
	@objc static let prefName = Bundle.main.bundleIdentifier!+".vfspref"
	
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
			SecAccessControlCreateWithFlags(kCFAllocatorDefault, kSecAttrAccessibleWhenUnlockedThisDeviceOnly,flags,nil)!
		let tag = keyName.data(using: .utf8)!
		let attributes: [String: Any] = [
			kSecAttrKeyType as String           : kSecAttrKeyTypeECSECPrimeRandom,
			kSecAttrKeySizeInBits as String     : 256,
			kSecAttrTokenID as String           : kSecAttrTokenIDSecureEnclave,
			kSecPrivateKeyAttrs as String : [
				kSecAttrIsPermanent as String       : true,
				kSecAttrApplicationTag as String    : tag,
				kSecAttrAccessControl as String     : access
			]
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
			kSecReturnRef as String             : true
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
		let delQuery: [String: Any] = [kSecClass as String: kSecClassGenericPassword,kSecAttrAccount as String: key.data(using: .utf8)!]
		SecItemDelete(delQuery as CFDictionary)

		
		let insertQUery: [String: Any] = [kSecClass as String: kSecClassGenericPassword,kSecAttrAccount as String: key.data(using: .utf8)!,  kSecValueData as String:value.data(using: .utf8)!]
		let insertStatus = SecItemAdd(insertQUery as CFDictionary, nil)
		return  insertStatus == errSecSuccess
		
	}
	
	@objc static func getSecuredPreference(key:String) -> String? {
		let query: [String:Any] = [
			kSecClass as String: kSecClassGenericPassword,
			kSecAttrAccount as String: key.data(using: .utf8)!,
			kSecReturnData as String: kCFBooleanTrue
		]
		
		var result: AnyObject?
		let status: OSStatus = withUnsafeMutablePointer(to: &result) {
			SecItemCopyMatching(query as CFDictionary, UnsafeMutablePointer($0))
		}
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
	
	
	
	@objc static func activateVFS() -> Bool {
		do {
			if (getSecuredPreference(key: prefName) == nil) {
				oslog(log: "[VFS] no secret key set, building one.", level: .info)
				try generateKey(requiresBiometry: false)
				guard let encryptedHash = encrypt(clearText: randomSha512()) else {
					return false
				}
				if (!addSecuredPreference(key: prefName, value: encryptedHash)) {
					oslog(log: "[VFS] Unable to save encrypted key in secured defaults.", level: .error)
				}
			}
			guard let encryptedKey = getSecuredPreference(key: prefName) else {
				oslog(log: "[VFS] Unable to retrieve encrypted key.", level: .error)
				return false
			}
			let secret = decrypt(encryptedText: encryptedKey)
			Factory.Instance.setVfsEncryption(encryptionModule: 2, secret: secret, secretSize: 32)
			oslog(log: "[VFS] activated", level: .info)
			return true
		} catch {
			oslog(log: "[VFS] Error setting up VFS: \(error)", level: .info)
			return false
		}
	}
	
	
	@objc static func oslog(log:String, level: OSLogType) {
		if #available(iOS 10.0, *) {
			os_log("%{public}@", type: level,log)
		} else {
			NSLog(log)
		}
	}
	

}
