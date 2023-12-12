//
//  ActivityIndicator.swift
//  Linphone
//
//  Created by Martins Benoît on 13/12/2023.
//

import SwiftUI

struct ActivityIndicator: View {
    
    let style = StrokeStyle(lineWidth: 3, lineCap: .round)
    @State var animate = false
    let color1 = Color.white
    let color2 = Color.white.opacity(0.5)
    
    var body: some View {
        ZStack {
            Circle()
                .trim(from: 0, to: 0.7)
                .stroke(
            AngularGradient(gradient: .init(colors: [color1, color2]), center: .center), style: style)
                .rotationEffect(Angle(degrees: animate ? 360: 0))
                .animation(Animation.linear(duration: 0.7).repeatForever(autoreverses: false), value: UUID())
        }.onAppear {
            self.animate.toggle()
        }
    }
}

#Preview {
    ActivityIndicator()
}
