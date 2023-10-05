//
//  ToastView.swift
//  Linphone
//
//  Created by Benoît Martins on 06/10/2023.
//

import SwiftUI

struct ToastView: ViewModifier {
	
	@Binding var isShowing: String
	
	func body(content: Content) -> some View {
		ZStack {
			content
			toastView
		}
	}
	
	private var toastView: some View {
		VStack {
			if !isShowing.isEmpty {
				HStack {
					Image(isShowing == "Successful" ? "success" : "danger")
						.resizable()
						.frame(width: 25, height: 25, alignment: .leading)
					
					Text(isShowing == "Successful" ? "QR code validated!" : (isShowing == "Failed" ? "Invalid QR code!" : "Invalide URI"))
						.multilineTextAlignment(.center)
						.foregroundStyle(isShowing == "Successful" ? Color.green_success_500 : Color.red_danger_500)
						.default_text_style(styleSize: 15)
						.padding(8)
				}
				.frame(maxWidth: .infinity)
				.frame(height: 40)
				.background(.white)
				.cornerRadius(50)
				.overlay(
					RoundedRectangle(cornerRadius: 50)
						.inset(by: 0.5)
						.stroke(isShowing == "Successful" ? Color.green_success_500 : Color.red_danger_500, lineWidth: 1)
				)
				.onTapGesture {
				  isShowing = ""
				}
				.onAppear {
					DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
						isShowing = ""
					}
				}
			}
			Spacer()
		}
		.padding(.horizontal, 16)
		.padding(.bottom, 18)
		.animation(.linear(duration: 0.3), value: isShowing)
		.transition(.opacity)
	}
}

extension View {	
	func toast(isShowing: Binding<String>) -> some View {
		self.modifier(ToastView(isShowing: isShowing))
	}
}

//#Preview {
//ToastView()
//}
