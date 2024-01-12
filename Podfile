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
	
end



target 'Linphone' do
  # Comment the next line if you don't want to use dynamic frameworks
  use_frameworks!

  # Pods for Linphone
	pod 'SwiftLint'
	basic_pods

end

post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      config.build_settings['IPHONEOS_DEPLOYMENT_TARGET'] = '15.0'
    end
  end
end