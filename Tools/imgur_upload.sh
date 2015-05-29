#!/bin/bash -x

# Install underscore-cli for hacking
if ! which underscore &> /dev/null; then
	npm install -g underscore-cli
fi

cd Screens

if "*.png" 2>/dev/null; then
	# Prepare location to collect delete commands
	if test "$TRAVIS_BUILD_NUMBER" = ""; then
		TRAVIS_BUILD_NUMBER="dev"
	fi
	output_dir="Screens"
	download_cmds=""

	# curl from http://imgur.com/tools/imgurbash.sh via http://imgur.com/tools
	# Documentation: http://code.google.com/p/imgur-api/source/browse/wiki/ImageUploading.wiki?r=82
	api_key=$IMGUR_KEY
	for filepath in *.png; do
		# echo "File $filepath"
		# echo "Command: curl https://api.imgur.com/3/upload.json -H \"Authorization: Client-ID $api_key\" -F "image=@\"$filepath\"""
		result="$(curl https://api.imgur.com/3/upload.json -H "Authorization: Client-ID $api_key" -F "image=@\"$filepath\"" )"

		# result='{"rsp":{"stat":"ok","image":{"image_hash":"dKZ0YK9","delete_hash":"r0MsZp11K9vawLf","original_image":"http:\/\/i.imgur.com\/dKZ0YK9.png","large_thumbnail":"http:\/\/i.imgur.com\/dKZ0YK9l.jpg","small_thumbnail":"http:\/\/i.imgur.com\/dKZ0YK9s.jpg","imgur_page":"http:\/\/imgur.com\/dKZ0YK9","delete_page":"http:\/\/imgur.com\/delete\/r0MsZp11K9vawLf"}}}'
		succeeded="$(echo $result | underscore extract success)"
		if [ "$succeeded" != "true" ]; then
			echo "There was a problem uploading \"$filepath\": $result"
		else
			download_cmds="${download_cmds}wget $(echo "$result" | underscore extract 'data.link')\n"
		fi
	done
	echo "All uploads complete!"
	echo ""
	echo "Download via: $download_cmds"
else
	echo "Could not find any PNG in $PWD, something must be broken!"
fi
