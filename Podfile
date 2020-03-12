# Uncomment the next line to define a global platform for your project
platform :ios, '9.0'
source "https://gitlab.linphone.org/BC/public/podspec.git"
source "https://github.com/CocoaPods/Specs.git"

def basic_pods
	if ENV['PODFILE_PATH'].nil?
		pod 'linphone-sdk/basic-frameworks', '~> 4.4.0-alpha'
		else
		pod 'linphone-sdk/basic-frameworks', :path => ENV['PODFILE_PATH']  # loacl sdk
	end
	
	crashlythics
end

def ext_pods
	if ENV['PODFILE_PATH'].nil?
		pod 'linphone-sdk/app-extension-swift', '~> 4.4.0-alpha'
		else
		pod 'linphone-sdk/app-extension-swift', :path => ENV['PODFILE_PATH']  # loacl sdk
	end
	
	crashlythics
end

def crashlythics
	if not ENV['USE_CRASHLYTHICS'].nil?
		pod 'Firebase/Analytics'
	end
end

target 'linphone' do
  # Uncomment the next line if you're using Swift or would like to use dynamic frameworks
  use_frameworks!

  # Pods for linphone
  basic_pods
	ext_pods
	pod 'SVProgressHUD'

end

target 'linphoneExtension' do
  # Uncomment the next line if you're using Swift or would like to use dynamic frameworks
  use_frameworks!

  # Pods for linphoneExtension

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
		if target.name == 'linphone'
			target.build_configurations.each do |config|
				if ENV['USE_CRASHLYTHICS'].nil?
					if config.name == "Debug" then
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited) DEBUG=1'
					else
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited)'
					end
				else
					# activate crashlythics
					if config.name == "Debug" then
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited) DEBUG=1 USE_CRASHLYTHICSS=1'
						else
						config.build_settings['GCC_PREPROCESSOR_DEFINITIONS'] = '$(inherited) USE_CRASHLYTHICSS=1'
					end
				end

				config.build_settings['OTHER_CFLAGS'] = '-DBCTBX_LOG_DOMAIN=\"ios\"',
																								'-DCHECK_VERSION_UPDATE=FALSE',
																								'-DENABLE_QRCODE=TRUE',
																								'-DENABLE_SMS_INVITE=TRUE',
																								'$(inherited)',
																								"-DLINPHONE_SDK_VERSION=\\\"#{$linphone_sdk_version}\\\""
				
				app_project.save
			end
		end
	end
end
