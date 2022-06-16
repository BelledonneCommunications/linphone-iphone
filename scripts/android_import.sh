# to be run from this location
# after running : Pick the first section (in swift) outputed by this script and copy it in VoipTexts.swift replacing the section marked as From Android
# look for strings with appName and append at the end .replacingOccurrences(of: "&appName;", with: appName) (just in the swift file)
# then pick the other relevant traduction and put them as is in Localizable.strings (replacing the previously generated ones) 


cat ../Classes/Swift/Voip/Theme/VoipTexts.swift | grep static |  sed s/@objc//g | awk '{print $3}' > keys 
export sub="\\\'"
echo ">>>>> Master to be placed in VoipTexts.swift replacing Android section"
while read key; do grep "name=\"$key\"" ../../linphone-android/app/src/main/res/values/strings.xml ; done < keys | sed s/'<string name="'/'@objc static let '/g | sed s/'">'/' = NSLocalizedString("'/g | sed s/'<\/string>'/'",comment:"")'/g| sed s/'" tools:ignore="PluralsCandidate'//g |sed s/$sub/\'/g
find ../../linphone-android/app/src/main/res/values-*/strings.xml > stringsandroid 
export sub="\\\'"
while read lc; do
  echo
  echo
  echo "Treating android: $lc"
  echo
  while read key; do
        export frvalue=`grep "name=\"$key\"" $lc | cut -d">" -f2- | sed s/'<\/string>'//g| sed s/'" tools:ignore="PluralsCandidate'//g|sed s/$sub/\'/g`
        export envalue=`grep "name=\"$key\"" ../../linphone-android/app/src/main/res/values/strings.xml  | cut -d">" -f2- | sed s/'<\/string>'//g| sed s/'" tools:ignore="PluralsCandidate'//g|sed s/$sub/\'/g`
        if [ -n "$frvalue" ]; then
                if [ -n "$envalue" ]; then
                        echo \"$envalue\"=\"$frvalue\"\;
                fi
        fi
  done < keys
done<stringsandroid



