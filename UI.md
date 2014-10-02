
Quick UI reference for Linphone iOS:

- The app is contained in a window, which resides in the MainStoryboard file.
- The delegate is set to LinphoneAppDelegate in main.m, in the UIApplicationMain() by passing its class
- Basic layout:


MainStoryboard
		|
		| (rootViewController)
		|
	PhoneMainView ---> view #--> app background
		|					|
		|					#--> statusbar background
		|			
		| (mainViewController)
		|
	UICompositeViewController : TPMultilayout
				|
				#---> view 	#--> stateBar
							|
							#--> contentView
							|
							#--> tabBar


When the app is started, the phoneMainView gets asked to transition to the Dialer view or the Wizard view.
PhoneMainView exposes the -changeCurrentView: method, which will setup its 
Any Linphone view is actually presented in the UICompositeViewController, with or without a stateBar and tabBar.

The UICompositeViewController consists of 3 areas laid out vertically. From top to bottom: StateBar, Content and TabBar.
The TabBar is usually the UIMainBar, which is used as a navigation controller: clicking on each of the buttons will trigger
a transition to another "view".