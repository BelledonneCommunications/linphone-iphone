#!/bin/sh
# iOS shows the raw key when a language's Localizable.strings lacks it, so fill
# the gaps in the built bundle with English. Existing translations are kept.
resources="${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}"
english="${resources}/en.lproj/Localizable.strings"
[ -f "$english" ] || exit 0

for strings in "$resources"/*.lproj/Localizable.strings; do
	[ "$strings" = "$english" ] && continue
	/usr/libexec/PlistBuddy -c "Merge \"$english\"" "$strings" > /dev/null \
		|| echo "warning: could not merge English strings into $strings"
done
exit 0
