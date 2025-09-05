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

import SwiftUI
import ContactsUI
import linphonesw

struct EditContactView: UIViewControllerRepresentable {
	
	class Coordinator: NSObject, CNContactViewControllerDelegate, UINavigationControllerDelegate {
		func contactViewController(_ viewController: CNContactViewController, didCompleteWith contact: CNContact?) {
			if let cnc = contact {
				self.parent.contact = cnc
				
				let newContact = Contact(
					identifier: cnc.identifier,
					firstName: cnc.givenName,
					lastName: cnc.familyName,
					organizationName: cnc.organizationName,
					jobTitle: "",
					displayName: cnc.nickname,
					sipAddresses: cnc.instantMessageAddresses.map { $0.value.service == "SIP" ? $0.value.username : "" },
					phoneNumbers: cnc.phoneNumbers.map { PhoneNumber(numLabel: $0.label ?? "", num: $0.value.stringValue)},
					imageData: ""
				)
				
				let imageThumbnail = UIImage(data: contact!.thumbnailImageData ?? Data())
				ContactsManager.shared.saveImage(
					image: imageThumbnail
					?? ContactsManager.shared.textToImage(
						firstName: cnc.givenName.isEmpty
						&& cnc.familyName.isEmpty
						&& cnc.phoneNumbers.first?.value.stringValue != nil
						? cnc.phoneNumbers.first!.value.stringValue
						: cnc.givenName, lastName: cnc.familyName),
					name: cnc.givenName + cnc.familyName,
					prefix: ((imageThumbnail == nil) ? "-default" : ""),
					contact: newContact,
					linphoneFriend: "Native address-book",
					existingFriend: ContactsManager.shared.getFriendWithContact(contact: newContact)) {
						MagicSearchSingleton.shared.searchForContacts()
					}
			}
			viewController.dismiss(animated: true, completion: {})
		}
		
		func contactViewController(_ viewController: CNContactViewController, shouldPerformDefaultActionFor property: CNContactProperty) -> Bool {
			return true
		}

		var parent: EditContactView

		init(_ parent: EditContactView) {
			self.parent = parent
		}
	}

	@Binding var contact: CNContact?

	init(contact: Binding<CNContact?>) {
		self._contact = contact
	}

	typealias UIViewControllerType = CNContactViewController

	func makeCoordinator() -> Coordinator {
		return Coordinator(self)
	}

	func makeUIViewController(context: UIViewControllerRepresentableContext<EditContactView>) -> EditContactView.UIViewControllerType {
		let vcontact = contact != nil ? CNContactViewController(for: contact!) : CNContactViewController(forNewContact: CNContact())
		vcontact.isEditing = true
		vcontact.delegate = context.coordinator
		return vcontact
	}

	func updateUIViewController(_ uiViewController: EditContactView.UIViewControllerType, context: UIViewControllerRepresentableContext<EditContactView>) {
	}
}
