# Uncomment the next line to define a global platform for your project
platform :ios, '15.0'
source "https://gitlab.linphone.org/BC/public/podspec.git"
source "https://github.com/CocoaPods/Specs.git"

def basic_pods
	if ENV['PODFILE_PATH'].nil?
		pod 'linphone-sdk', '~> 5.4.0-alpha'
	else
		pod 'linphone-sdk', :path => ENV['PODFILE_PATH']  # local sdk
	end

	crashlytics
end

def crashlytics
	if not ENV['USE_CRASHLYTICS'].nil?
		pod 'Firebase/Analytics'
		pod 'Firebase/Crashlytics'
	end
end

target 'Linphone' do
  # Comment the next line if you don't want to use dynamic frameworks
  use_frameworks!

  # Pods for Linphone
	pod 'SwiftLint'
	pod 'AppAuth'
	basic_pods

end

target 'msgNotificationService' do
  # Uncomment the next line if you're using Swift or would like to use dynamic frameworks
  use_frameworks!

  # Pods for messagesNotification
  basic_pods

end

post_install do |installer|
	app_project = Xcodeproj::Project.open(Dir.glob("*.xcodeproj")[0])
	app_project.native_targets.each do |target|
		target.build_configurations.each do |config|
			if target.name == "Linphone" || target.name == 'msgNotificationService' || target.name == 'msgNotificationContent'
				if ENV['USE_CRASHLYTICS'].nil?
					if config.name == "Debug" then
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited) DEBUG=1'
						else
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited)'
					end
					config.build_settings['OTHER_SWIFT_FLAGS'] = '$(inherited)'
				else
					# activate crashlytics
					if config.name == "Debug" then
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited) DEBUG=1 USE_CRASHLYTICS=1'
					else
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited) USE_CRASHLYTICS=1'
					end
					config.build_settings['OTHER_SWIFT_FLAGS'] = '$(inherited) -DUSE_CRASHLYTICS'
				end
			end

			app_project.save
		end
	end
	installer.pods_project.targets.each do |target|
		target.build_configurations.each do |config|
			config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '15.0'
		end
	end
end

