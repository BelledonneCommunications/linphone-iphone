#!/bin/sh
#this script pickups eXosip files usefull for linphone and put them into exosip/ .
if test -z $1 ; then
	echo "make_exosip.sh <exosip root tree>"
	exit 1
fi

exosip_src=$1
for file in $exosip_src/src/*.c ; do
	echo "processing $file ..." 
	sed -e 's/eXosip\/eXosip.h/eXosip.h/' -e 's/eXosip\/eXosip_cfg.h/eXosip_cfg.h/' $file > exosip/`basename $file` 
done
for file in $exosip_src/src/*.h ; do
	echo "processing $file ..." 
	sed -e 's/eXosip\/eXosip.h/eXosip.h/' -e 's/eXosip\/eXosip_cfg.h/eXosip_cfg.h/' $file > exosip/`basename $file` 
done
for file in $exosip_src/include/eXosip/*.h ; do
	echo "processing $file ..." 
	sed -e 's/eXosip\/eXosip.h/eXosip.h/' -e 's/eXosip\/eXosip_cfg.h/eXosip_cfg.h/' $file > exosip/`basename $file` 
done

echo "Finished !"
