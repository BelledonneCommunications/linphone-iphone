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

import SwiftUI
import CallKit

struct CallView: View {
    
    @ObservedObject private var coreContext = CoreContext.shared
    @ObservedObject private var telecomManager = TelecomManager.shared
    @ObservedObject private var contactsManager = ContactsManager.shared
    
    @ObservedObject var callViewModel: CallViewModel
    
    @State var startDate = Date.now
    @State var timeElapsed: Int = 0
    @State var micMutted: Bool = false
    
    let timer = Timer.publish(every: 1, on: .main, in: .common).autoconnect()
    
    var body: some View {
        GeometryReader { geo in
            if #available(iOS 16.4, *) {
                innerView()
                    .sheet(isPresented: $telecomManager.callStarted) {
                        GeometryReader { _ in
                            VStack(spacing: 0) {
                                HStack(spacing: 12) {
                                    Button {
                                        terminateCall()
                                    } label: {
                                        Image("phone-disconnect")
                                            .renderingMode(.template)
                                            .resizable()
                                            .foregroundStyle(.white)
                                            .frame(width: 32, height: 32)
                                        
                                    }
                                    .frame(width: 90, height: 60)
                                    .background(Color.redDanger500)
                                    .cornerRadius(40)
                                    
                                    Spacer()
                                    
                                    Button {
                                    } label: {
                                        Image("video-camera")
                                            .renderingMode(.template)
                                            .resizable()
                                            .foregroundStyle(.white)
                                            .frame(width: 32, height: 32)
                                        
                                    }
                                    .frame(width: 60, height: 60)
                                    .background(Color.gray500)
                                    .cornerRadius(40)
                                    
                                    Button {
                                        muteCall()
                                    } label: {
                                        Image(micMutted ? "microphone-slash" : "microphone")
                                            .renderingMode(.template)
                                            .resizable()
                                            .foregroundStyle(micMutted ? .black : .white)
                                            .frame(width: 32, height: 32)
                                        
                                    }
                                    .frame(width: 60, height: 60)
                                    .background(micMutted ? .white : Color.gray500)
                                    .cornerRadius(40)
                                    
                                    Button {
                                    } label: {
                                        Image("speaker-high")
                                            .renderingMode(.template)
                                            .resizable()
                                            .foregroundStyle(.white)
                                            .frame(width: 32, height: 32)
                                        
                                    }
                                    .frame(width: 60, height: 60)
                                    .background(Color.gray500)
                                    .cornerRadius(40)
                                }
                                .frame(height: geo.size.height * 0.15)
                                .padding(.horizontal, 20)
                                
                                HStack(spacing: 0) {
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("screencast")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Screen share")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                    
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("users")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Participants")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                    
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("chat-teardrop-text")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Messages")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                    
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("notebook")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Disposition")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                }
                                .frame(height: geo.size.height * 0.15)
                                
                                HStack(spacing: 0) {
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("phone-call")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Call list")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                    
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("pause")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Pause")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                    
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("record-fill")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Record")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                    
                                    VStack {
                                        Button {
                                        } label: {
                                            Image("video-camera")
                                                .renderingMode(.template)
                                                .resizable()
                                                .foregroundStyle(.white)
                                                .frame(width: 32, height: 32)
                                        }
                                        .frame(width: 60, height: 60)
                                        .background(Color.gray500)
                                        .cornerRadius(40)
                                        
                                        Text("Disposition")
                                            .foregroundStyle(.white)
                                            .default_text_style(styleSize: 15)
                                    }
                                    .frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
                                    .hidden()
                                }
                                .frame(height: geo.size.height * 0.15)
                                
                                Spacer()
                            }
                            .frame(maxHeight: .infinity, alignment: .top)
                            .background(.black)
                            .presentationDetents([.fraction(0.1), .medium])
                            .interactiveDismissDisabled()
                            .presentationBackgroundInteraction(.enabled)
                        }
                    }
            }
        }
    }
    
    @ViewBuilder
    func innerView() -> some View {
        VStack {
            Rectangle()
                .foregroundColor(Color.orangeMain500)
                .edgesIgnoringSafeArea(.top)
                .frame(height: 0)
            
            HStack {
                if callViewModel.direction == .Outgoing {
                    Image("outgoing-call")
                        .resizable()
                        .frame(width: 15, height: 15)
                        .padding(.horizontal)
                    
                    Text("Outgoing call")
                        .foregroundStyle(.white)
                } else {
                    Image("incoming-call")
                        .resizable()
                        .frame(width: 15, height: 15)
                        .padding(.horizontal)
                    
                    Text("Incoming call")
                        .foregroundStyle(.white)
                }
                
                Spacer()
            }
            .frame(height: 40)
            
            ZStack {
                VStack {
                    Spacer()
                    
                    if callViewModel.remoteAddress != nil {
                        let addressFriend = contactsManager.getFriendWithAddress(address: callViewModel.remoteAddress!)
                        
                        let contactAvatarModel = addressFriend != nil
                        ? ContactsManager.shared.avatarListModel.first(where: {
                            ($0.friend!.consolidatedPresence == .Online || $0.friend!.consolidatedPresence == .Busy)
                            && $0.friend!.name == addressFriend!.name
                            && $0.friend!.address!.asStringUriOnly() == addressFriend!.address!.asStringUriOnly()
                        })
                        : ContactAvatarModel(friend: nil, withPresence: false)
                        
                        if addressFriend != nil && addressFriend!.photo != nil && !addressFriend!.photo!.isEmpty {
                            if contactAvatarModel != nil {
                                Avatar(contactAvatarModel: contactAvatarModel!, avatarSize: 100, hidePresence: true)
                            }
                        } else {
                            if callViewModel.remoteAddress!.displayName != nil {
                                Image(uiImage: contactsManager.textToImage(
                                    firstName: callViewModel.remoteAddress!.displayName!,
                                    lastName: callViewModel.remoteAddress!.displayName!.components(separatedBy: " ").count > 1
                                    ? callViewModel.remoteAddress!.displayName!.components(separatedBy: " ")[1]
                                    : ""))
                                .resizable()
                                .frame(width: 100, height: 100)
                                .clipShape(Circle())
                                
                            } else {
                                Image(uiImage: contactsManager.textToImage(
                                    firstName: callViewModel.remoteAddress!.username ?? "Username Error",
                                    lastName: callViewModel.remoteAddress!.username!.components(separatedBy: " ").count > 1
                                    ? callViewModel.remoteAddress!.username!.components(separatedBy: " ")[1]
                                    : ""))
                                .resizable()
                                .frame(width: 100, height: 100)
                                .clipShape(Circle())
                            }
                            
                        }
                    } else {
                        Image("profil-picture-default")
                            .resizable()
                            .frame(width: 100, height: 100)
                            .clipShape(Circle())
                    }
                    
                    Text(callViewModel.displayName)
                        .padding(.top)
                        .foregroundStyle(.white)
                    
                    Text(callViewModel.remoteAddressString)
                        .foregroundStyle(.white)
                    
                    Spacer()
                }
                
                if !telecomManager.callStarted {
                    VStack {
                        ActivityIndicator()
                            .frame(width: 20, height: 20)
                            .padding(.top, 100)
                        
                        Text(counterToMinutes())
                            .onReceive(timer) { firedDate in
                                timeElapsed = Int(firedDate.timeIntervalSince(startDate))
                                
                            }
                            .padding(.top)
                            .foregroundStyle(.white)
                        
                        Spacer()
                    }
                    .background(.clear)
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Color.gray600)
            .cornerRadius(20)
            .padding(.horizontal, 4)
            
            if telecomManager.callStarted {
                HStack(spacing: 12) {
                    HStack {
                    }
                    .frame(height: 60)
                }
                .padding(.horizontal, 25)
                .padding(.top, 20)
            } else {
                HStack(spacing: 12) {
                    HStack {
                        Spacer()
                        
                        Button {
                            terminateCall()
                        } label: {
                            Image("phone-disconnect")
                                .renderingMode(.template)
                                .resizable()
                                .foregroundStyle(.white)
                                .frame(width: 32, height: 32)
                            
                        }
                        .frame(width: 90, height: 60)
                        .background(Color.redDanger500)
                        .cornerRadius(40)
                        
                        Button {
                            acceptCall()
                        } label: {
                            Image("phone")
                                .renderingMode(.template)
                                .resizable()
                                .foregroundStyle(.white)
                                .frame(width: 32, height: 32)
                            
                        }
                        .frame(width: 90, height: 60)
                        .background(Color.greenSuccess500)
                        .cornerRadius(40)
                        
                        Spacer()
                    }
                    .frame(height: 60)
                }
                .padding(.horizontal, 25)
                .padding(.top, 20)
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .background(Color.gray900)
    }
    
    func terminateCall() {
        withAnimation {
            telecomManager.callInProgress = false
            telecomManager.callStarted = false
        }
        
        coreContext.doOnCoreQueue { core in
            if core.currentCall != nil {
                telecomManager.terminateCall(call: core.currentCall!)
            }
        }
        
        timer.upstream.connect().cancel()
    }
    
    func acceptCall() {
        withAnimation {
            telecomManager.callInProgress = true
            telecomManager.callStarted = true
        }
        
        coreContext.doOnCoreQueue { core in
            if core.currentCall != nil {
                telecomManager.acceptCall(core: core, call: core.currentCall!, hasVideo: false)
            }
        }
        
        timer.upstream.connect().cancel()
    }
    
    func muteCall() {
        coreContext.doOnCoreQueue { core in
            if core.currentCall != nil {
                micMutted = !micMutted
                core.currentCall!.microphoneMuted = micMutted
            }
        }
    }
    
    func counterToMinutes() -> String {
        let currentTime = timeElapsed
        let seconds = currentTime % 60
        let minutes = String(format: "%02d", Int(currentTime / 60))
        let hours = String(format: "%02d", Int(currentTime / 3600))
        
        if Int(currentTime / 3600) > 0 {
            return "\(hours):\(minutes):\(seconds < 10 ? "0" : "")\(seconds)"
        } else {
            return "\(minutes):\(seconds < 10 ? "0" : "")\(seconds)"
        }
    }
}

#Preview {
    CallView(callViewModel: CallViewModel())
}
