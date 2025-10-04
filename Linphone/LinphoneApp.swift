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
import linphonesw
import UserNotifications

let accountTokenNotification = Notification.Name("AccountCreationTokenReceived")
var displayedChatroomPeerAddr: String?

class AppDelegate: NSObject, UIApplicationDelegate, UNUserNotificationCenterDelegate {
	
	var launchNotificationCallId: String?
	var launchNotificationPeerAddr: String?
	var launchNotificationLocalAddr: String?
	
	var coreContext: CoreContext?
 	var navigationManager: NavigationManager?
	
	func application(_ application: UIApplication, didRegisterForRemoteNotificationsWithDeviceToken deviceToken: Data) {
		let tokenStr = deviceToken.map { String(format: "%02.2hhx", $0) }.joined()
		Log.info("Received remote push token : \(tokenStr)")
		if let coreContext = coreContext {
			coreContext.doOnCoreQueue { core in
				Log.info("Forwarding remote push token to core")
				core.didRegisterForRemotePushWithStringifiedToken(deviceTokenStr: tokenStr + ":remote")
			}
		}
	}
	
	func application(_ application: UIApplication, didFailToRegisterForRemoteNotificationsWithError error: Error) {
		Log.error("Failed to register for push notifications : \(error.localizedDescription)")
	}
	
	func application(_ application: UIApplication, didReceiveRemoteNotification userInfo: [AnyHashable: Any], fetchCompletionHandler completionHandler: @escaping (UIBackgroundFetchResult) -> Void) {
		Log.info("Received background push notification, payload = \(userInfo.description)")
		let creationToken = (userInfo["customPayload"] as? NSDictionary)?["token"] as? String
		if let creationToken = creationToken {
			NotificationCenter.default.post(name: accountTokenNotification, object: nil, userInfo: ["token": creationToken])
		}
		
		completionHandler(UIBackgroundFetchResult.newData)
	}
					 
	func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
		// Set up notifications
		UNUserNotificationCenter.current().delegate = self
		
		return true
	}
	
	// Called when the user interacts with the notification
	func userNotificationCenter(_ center: UNUserNotificationCenter, didReceive response: UNNotificationResponse, withCompletionHandler completionHandler: @escaping () -> Void) {
		let userInfo = response.notification.request.content.userInfo
		
		if let callId = userInfo["CallId"] as? String, let peerAddr = userInfo["peer_addr"] as? String, let localAddr = userInfo["local_addr"] as? String {
			if self.navigationManager != nil {
				self.navigationManager!.selectedCallId = callId
				self.navigationManager!.peerAddr = peerAddr
				self.navigationManager!.localAddr = localAddr
			} else {
				launchNotificationCallId = callId
				launchNotificationPeerAddr = peerAddr
				launchNotificationLocalAddr = localAddr
			}
		}
		
		completionHandler()
	}
	
	// Display notifications on foreground
	func userNotificationCenter(_ center: UNUserNotificationCenter, willPresent notification: UNNotification, withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
		let userInfo = notification.request.content.userInfo
		Log.info("Received push notification in foreground, payload= \(userInfo)")
		
		let strPeerAddr = userInfo["peer_addr"] as? String
		if strPeerAddr == nil {
			completionHandler([.banner, .sound])
		} else {
			// Only display notification if we're not in the chatroom they come from
			if displayedChatroomPeerAddr != strPeerAddr {
				if let coreContext = coreContext {
					coreContext.doOnCoreQueue { core in
						let nilParams: ConferenceParams? = nil
						if 	let peerAddr = try? Factory.Instance.createAddress(addr: strPeerAddr!)
								, let chatroom = core.searchChatRoom(params: nilParams, localAddr: nil, remoteAddr: peerAddr, participants: nil), chatroom.muted {
							Log.info("message comes from a muted chatroom, ignore it")
							return
						}
						completionHandler([.banner, .sound])
					}
				}
			}
		}
			
	}
	
	func applicationWillTerminate(_ application: UIApplication) {
		Log.info("IOS applicationWillTerminate")
		if let coreContext = coreContext {
			coreContext.doOnCoreQueue(synchronous: true) { core in
				Log.info("applicationWillTerminate - Stopping linphone core")
				MagicSearchSingleton.shared.destroyMagicSearch()
				if core.globalState != GlobalState.Off {
					core.stop()
				} else {
					Log.info("applicationWillTerminate - Core already stopped")
				}
			}
		}
	}
}

@main
struct LinphoneApp: App {
	@Environment(\.scenePhase) var scenePhase
	@UIApplicationDelegateAdaptor(AppDelegate.self) var delegate

	@StateObject private var coreContext = CoreContext.shared
	@StateObject private var navigationManager = NavigationManager()
	@StateObject private var telecomManager = TelecomManager.shared
	@StateObject private var sharedMainViewModel = SharedMainViewModel.shared

	var body: some Scene {
		WindowGroup {
			RootView(
				coreContext: coreContext,
				telecomManager: telecomManager,
				sharedMainViewModel: sharedMainViewModel,
				navigationManager: navigationManager,
				appDelegate: delegate
			)
			.environmentObject(coreContext)
			.environmentObject(navigationManager)
			.environmentObject(telecomManager)
			.environmentObject(sharedMainViewModel)
		}
		.onChange(of: scenePhase) { newPhase in
			if !telecomManager.callInProgress {
				switch newPhase {
				case .active:
					Log.info("Entering foreground")
					coreContext.onEnterForeground()
				case .background:
					Log.info("Entering background")
					coreContext.onEnterBackground()
				default:
					break
				}
			}
		}
	}
}

struct RootView: View {
	@ObservedObject var coreContext: CoreContext
	@ObservedObject var telecomManager: TelecomManager
	@ObservedObject var sharedMainViewModel: SharedMainViewModel
	@ObservedObject var navigationManager: NavigationManager
	@State private var pendingURL: URL?
	let appDelegate: AppDelegate

	var body: some View {
		Group {
			if coreContext.coreHasStartedOnce {
				if showWelcome {
					ZStack {
						WelcomeView()
						ToastView().zIndex(3)
					}
					.onAppear {
						appDelegate.coreContext = coreContext
					}
				} else if showAssistant {
					ZStack {
						AssistantView()
						ToastView().zIndex(3)
					}
					.onAppear {
						appDelegate.coreContext = coreContext
					}
					
					if coreContext.coreIsStarted {
						   VStack {} // Force trigger .onAppear
							   .onAppear {
								   if let url = pendingURL {
									   URIHandler.handleURL(url: url)
									   pendingURL = nil
								   }
							   }
					   }
				} else {
					ZStack {
						MainViewSwitcher(
							coreContext: coreContext,
							navigationManager: navigationManager,
							sharedMainViewModel: sharedMainViewModel,
							pendingURL: $pendingURL,
							appDelegate: appDelegate
						)
						
						if coreContext.coreIsStarted {
							VStack {} // Force trigger .onAppear
								.onAppear {
									if let url = pendingURL {
										URIHandler.handleURL(url: url)
										pendingURL = nil
									}
								}
						}
					}
				}
			} else {
				SplashScreen()
			}
		}
		.onOpenURL { url in
			if SharedMainViewModel.shared.displayedConversation != nil && url.absoluteString.contains("linphone-message://") {
				SharedMainViewModel.shared.displayedConversation = nil
			}
			if coreContext.coreIsStarted {
				URIHandler.handleURL(url: url)
			} else {
				pendingURL = url
			}
		}
	}
	
	
	var showWelcome: Bool {
		!sharedMainViewModel.welcomeViewDisplayed
	}

	var showAssistant: Bool {
		(coreContext.coreIsStarted && coreContext.accounts.isEmpty)
		|| sharedMainViewModel.displayProfileMode
	}
}

struct MainViewSwitcher: View {
	let coreContext: CoreContext
	let navigationManager: NavigationManager
	let sharedMainViewModel: SharedMainViewModel
	@Binding var pendingURL: URL?
	let appDelegate: AppDelegate
	@ObservedObject private var colors = ColorProvider.shared

	var body: some View {
		selectedMainView()
	}
	
	@ViewBuilder
	func selectedMainView() -> some View {
		ContentView()
			.onAppear {
				appDelegate.coreContext = coreContext
				appDelegate.navigationManager = navigationManager
				
				if let callId = appDelegate.launchNotificationCallId,
				   let peerAddr = appDelegate.launchNotificationPeerAddr,
				   let localAddr = appDelegate.launchNotificationLocalAddr {
					navigationManager.openChatRoom(callId: callId, peerAddr: peerAddr, localAddr: localAddr)
				}
			}
			.id(colors.theme.name)
	}
}
