#!/bin/sh
# this script is used to generate a file list of gtk+ files necessary for
# execution. This is useful for generating linphone packages.
# It must be run within a gtk+ binary bundle tree, such as in the zip bundle
# downloadable from www.gtk.org
echo bin
find bin -name *.dll
find lib/gtk-2.0
find etc
find share/locale/cs
find share/locale/de
find share/locale/es
find share/locale/fr
find share/locale/he
find share/locale/hu
find share/locale/it
find share/locale/ja
find share/locale/nb
find share/locale/nl
find share/locale/pl
find share/locale/pt
find share/locale/pt_BR
find share/locale/ru
find share/locale/sr
find share/locale/sv
find share/locale/zh_CN
find share/locale/zh_TW
find share/themes
