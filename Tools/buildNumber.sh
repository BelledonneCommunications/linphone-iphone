#!/bin/bash

#increment build number each time we build

if [[ $CONFIGURATION = "Release" ]]; then
	buildNumber=$(/usr/libexec/PlistBuddy -c "Print CFBundleVersion" "$INFOPLIST_FILE")
	buildNumber=$(($buildNumber + 1))
	/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $buildNumber" "$INFOPLIST_FILE"
	echo "Incremented build number to $buildNumber"
else
	echo "Skip build increment when building for $CONFIGURATION"
fi

