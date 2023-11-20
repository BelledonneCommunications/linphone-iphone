//
//  ToastViewModel.swift
//  Linphone
//
//  Created by Benoît Martins on 20/11/2023.
//

import Foundation

class ToastViewModel: ObservableObject {
	
	static let shared = ToastViewModel()
	
	var toastMessage: String = ""
	@Published var displayToast = false
	
	private init() {
	}
}
