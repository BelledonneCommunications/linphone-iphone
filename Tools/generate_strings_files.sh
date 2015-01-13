#!/bin/sh

root_directory=$(cd "$(dirname $0)" && pwd)/../

# The 2 only specific cases of the application: since we are length limited for push
# notifications, the ID is not matching the English translation... so we must keep
# the translations!
localizable_en=$root_directory/Resources/en.lproj/Localizable.strings
IC_MSG_EN=$(sed -nE 's/"IC_MSG" = "(.*)";/\1/p' $localizable_en)
IM_MSG_EN=$(sed -nE 's/"IM_MSG" = "(.*)";/\1/p' $localizable_en)
rm -f $localizable_en
find $root_directory/Classes -name '*.m' | xargs genstrings -u -a -o $(dirname $localizable_en)
iconv -f utf-16 -t utf-8 $localizable_en > $localizable_en.tmp
mv $localizable_en.tmp $localizable_en
sed -i.bak "s/= \"IC_MSG\";/= \"$IC_MSG_EN\";/" $localizable_en
sed -i.bak "s/= \"IM_MSG\";/= \"$IM_MSG_EN\";/" $localizable_en
rm $localizable_en.bak

to_utf8=$(mktemp -t linphone)
for xibfile in $(find $(find $root_directory/Classes -name Base.lproj) -name '*.xib'); do
	stringsfile=${xibfile/.xib/.strings}

	ibtool --generate-strings-file $stringsfile $xibfile

    # remove if empty
    iconv -f utf-16 -t utf-8 $stringsfile > $to_utf8
    if [ ! -s $to_utf8 ]; then
    	echo "$(basename $stringsfile) is empty, removing"
    	rm $stringsfile
    else
		echo "$(basename $xibfile)->$(basename $stringsfile)"

		res_name=$(basename $stringsfile | tr -d '_.~-' | tr '[:upper:]' '[:lower:]')
		dir_name=$(echo $(dirname $stringsfile) | sed -E "s|$root_directory/||")
		# if not registered in transifex config file, register it
		if ! grep -q $res_name $root_directory/.tx/config; then
			echo "not found in .tx/config, adding it"
			echo "
[linphone-ios.$res_name]
file_filter = $(echo $dir_name| sed 's/Base.lproj/<lang>.lproj/')/$(basename $stringsfile)
source_file = $dir_name/$(basename $stringsfile)
source_lang = en
" >> $root_directory/.tx/config
		fi
    fi
done
rm $to_utf8
