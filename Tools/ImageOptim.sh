#!/bin/sh

#  ImageOptim.sh
#  linphone
#
#  Created by guillaume on 14/10/13.
#

if [ "$CONFIGURATION" == "Debug" ]; then
    exit 0
fi

CONVERT=$(which convert)
CONVERTFILTER="-sharpen 1x0.0 -filter Catrom"
OPTIPNG=$(which optipng)
CMDS="${CONVERT} ${OPTIPNG}"
for i in $CMDS; do
    command -v $i > /dev/null && continue || { echo "$i command not found"; exit 1; }
done

DIR=${CONFIGURATION_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}
PNGS=$(find $DIR -type f -name *.png)

echo "Running PNG optimization in $DIR"

if [[ -f $DIR/optimized ]]; then
    echo "Resources already optimized, exit"
    exit 0
fi

for PNG in $PNGS; do

    BASENAME=$(basename $PNG ".png")
    SUFFIX=
    PROCESS=true

    # detect images for iPad, in which case basename has to be stripped
    case $BASENAME in
         *~ipad)
            SUFFIX="~ipad"
            BASENAME=$(echo ${BASENAME} |cut -f1 -d~)
            ;;
    # don't ever resize 9Patch assets, otherwise it can't handle the resizing
        *9)
            PROCESS=false
            ;;
    esac

    STANDARDFILE=${BASENAME}${SUFFIX}.png
    RETINAFILE=${BASENAME}@2x${SUFFIX}.png


    # skip resize if the retina version already exist, which means the asset was optimized manually
    if [ -f $DIR/$BASENAME"@2x"$SUFFIX".png" ]; then
        echo "Don't process $BASENAME";
        PROCESS=false
    fi

    case $BASENAME in 
        *@2x$SUFFIX)
            continue
            ;;
    esac

    # for all resources that don't have retina versions, consider the normal version as retina and resize to 50%
    if $PROCESS ; then

        echo -n "Processing ${STANDARDFILE} (${CONVERTFILTER})..."

        mv ${DIR}/$STANDARDFILE ${DIR}/$RETINAFILE
        $CONVERT ${DIR}/$RETINAFILE $CONVERTFILTER -resize "50%" -strip ${DIR}/$STANDARDFILE > /dev/null
    fi

    echo "Optimizing ${BASENAME} and ${BASENAME}@2x ..."
    $OPTIPNG $DIR/$RETINAFILE > /dev/null
    $OPTIPNG -quiet $DIR/$STANDARDFILE > /dev/null

done

# make sure we dont over-optimize
touch $DIR/optimized
