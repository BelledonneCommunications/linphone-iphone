#!/bin/bash

if [ ! -z "$KIF_SCREENSHOTS" ]; then
	cd $KIF_SCREENSHOTS

	if [ ! -z "$(find . -name "*.png" -maxdepth 1)" ]; then
		# Prepare location to collect delete commands
		if test "$TRAVIS_BUILD_NUMBER" = ""; then
			TRAVIS_BUILD_NUMBER="dev"
		fi
		download_cmds=""

		# curl from http://imgur.com/tools/imgurbash.sh via http://imgur.com/tools
		# Documentation: http://code.google.com/p/imgur-api/source/browse/wiki/ImageUploading.wiki?r=82
		api_key=$IMGUR_KEY
		for filepath in *.png; do
			# echo "File $filepath"
			# echo "Command: curl https://api.imgur.com/3/upload.json -H \"Authorization: Client-ID $api_key\" -F "image=@\"$filepath\"""
			result="$(curl -s https://api.imgur.com/3/upload.json -H "Authorization: Client-ID $api_key" -F "image=@\"$filepath\"" )"

			# result='{"rsp":{"stat":"ok","image":{"image_hash":"dKZ0YK9","delete_hash":"r0MsZp11K9vawLf","original_image":"http:\/\/i.imgur.com\/dKZ0YK9.png","large_thumbnail":"http:\/\/i.imgur.com\/dKZ0YK9l.jpg","small_thumbnail":"http:\/\/i.imgur.com\/dKZ0YK9s.jpg","imgur_page":"http:\/\/imgur.com\/dKZ0YK9","delete_page":"http:\/\/imgur.com\/delete\/r0MsZp11K9vawLf"}}}'
			if ! grep -q '"success":true' <<< $result; then
				echo "There was a problem uploading \"$filepath\": $result"
				exit 1
			else
				url=$(echo "$result" | tr ',' '\n' | grep '"link":"http' | cut -d '"' -f 4)
				download_cmds="${download_cmds}\nwget $url"
			fi
		done
		echo "All uploads complete!"
		printf "Download via: $download_cmds\n"
	else
		echo "Could not find any PNG in $PWD, something must be broken!"
	fi
else
	echo "Please initialize KIF_SCREENSHOTS env variable first!"
fi
