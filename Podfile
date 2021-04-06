# Uncomment the next line to define a global platform for your project
platform :ios, '9.0'
source "https://gitlab.linphone.org/BC/public/podspec.git"
source "https://github.com/CocoaPods/Specs.git"

def all_pods
	if ENV['PODFILE_PATH'].nil?
		pod 'linphone-sdk', '~> 5.0.0-alpha'
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

target 'linphone' do
  # Uncomment the next line if you're using Swift or would like to use dynamic frameworks
  use_frameworks!

  # Pods for linphone
	pod 'SVProgressHUD'
	all_pods

end

target 'msgNotificationService' do
  # Uncomment the next line if you're using Swift or would like to use dynamic frameworks
  use_frameworks!

  # Pods for messagesNotification
  all_pods

end

target 'msgNotificationContent' do
  # Uncomment the next line if you're using Swift or would like to use dynamic frameworks
  use_frameworks!

  # Pods for messagesNotification
  all_pods

end

post_install do |installer|
	# Get the version of linphone-sdk
	installer.pod_targets.each do |target|
		if target.pod_name == 'linphone-sdk'
			target.specs.each do |spec|
				$linphone_sdk_version = spec.version
			end
		end
	end
			
	app_project = Xcodeproj::Project.open(Dir.glob("*.xcodeproj")[0])
	app_project.native_targets.each do |target|
		target.build_configurations.each do |config|
			if target.name == "linphone" || target.name == 'msgNotificationService' || target.name == 'msgNotificationContent'
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

			if target.name == "linphone"
				config.build_settings['OTHER_CFLAGS'] = '-DBCTBX_LOG_DOMAIN=\"ios\"',
																							'-DCHECK_VERSION_UPDATE=FALSE',
																							'-DENABLE_QRCODE=TRUE',
																							'-DENABLE_SMS_INVITE=TRUE',
																							'$(inherited)',
																							"-DLINPHONE_SDK_VERSION=\\\"#{$linphone_sdk_version}\\\""
			end

			app_project.save
		end
	end
end
