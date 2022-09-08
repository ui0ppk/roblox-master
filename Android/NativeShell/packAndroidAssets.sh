(set -o igncr) 2>/dev/null && set -o igncr; # comment is needed on Windows to ignore this lines trailing \r

function do_md5()
{
	if [ $(uname) == "Darwin" ]; then
        md5 -q $1
    else
        md5sum $1 | awk '{print $1;}'
    fi
}

# Parameters
OBB_PACKAGE_NAME="com.roblox.client"
OBB_VERSION="1"

# The contents of the obb can be specified with variable.
# Format: "SOURCE_DIR:MAPPED_DIR"
INCLUDE_DIRS=("content:content"
	          "shaders:shaders"
	          "PlatformContent/android:android")

if [[ $(uname) = "Darwin" ]]; then
    APP_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
else
    READLINK="readlink -nm"
    APP_DIR=$(${READLINK} $(dirname $0))
fi

INITIAL_DIR=$(pwd)

# Setup the output path
OBB_NAME="main.$OBB_VERSION.$OBB_PACKAGE_NAME.obb"
OUTPUT_FOLDER="$APP_DIR/obb"
OUTPUT_FILENAME="$OUTPUT_FOLDER/$OBB_NAME"

LOCAL_UP_TO_DATE=1
if [ ! -f "$OUTPUT_FILENAME" ]; then
	LOCAL_UP_TO_DATE=0
fi

for folder in "${INCLUDE_DIRS[@]}" ; do
	FROM=${folder%%:*}
	TO=${folder#*:}

    COMPARE_RESULTS=$(diff -arq $APP_DIR/../../$FROM $OUTPUT_FOLDER/$TO -x .DS_Store 2>/dev/null)
	if [ $? != 0 -o -n "$COMPARE_RESULTS" ]; then
        LOCAL_UP_TO_DATE=0
    fi
done

if [ $LOCAL_UP_TO_DATE -eq 1 ]; then
    echo "-- Local OBB is up to date --"
else
    #Generate the OBB
    FORCE_COPY="true"

    #echo $DEVICE_OBB_DATE
    echo "-- Packing assets into OBB --"

	chmod -Rf +rwx $OUTPUT_FOLDER # Cygwin cp creates files without any permissions.
    rm -R -f $OUTPUT_FOLDER
    mkdir -p $OUTPUT_FOLDER

    $(rm -R -f $OUTPUT_FOLDER &>/dev/null)
    $(mkdir -p $OUTPUT_FOLDER &>/dev/null)

    cd $OUTPUT_FOLDER

    # Copy the required resources into the tmp folder
    for folder in "${INCLUDE_DIRS[@]}" ; do
        FROM=${folder%%:*}
        TO=${folder#*:}
        echo "Packing $FROM => $TO"
        cp -R $APP_DIR/../../$FROM $OUTPUT_FOLDER/$TO
    done

	chmod -Rf +rwx . # Cygwin cp creates files without any permissions.

    # Pack the resources with JOBB
    zip -9 -r -q $OUTPUT_FILENAME .

    # Compute the fingerprint and embed it in the container
    do_md5 $OUTPUT_FILENAME > $OUTPUT_FOLDER/fingerprint.txt
    #zip -9 -qj $OUTPUT_FILENAME $OUTPUT_FOLDER/fingerprint.txt

    #$(rm $OUTPUT_FOLDER/fingerprint.txt &>/dev/null)
fi

# Copy the obb to the assets folder (so it's embeded in the APK)
[[ ! -d $APP_DIR/assets/ ]] && mkdir $APP_DIR/assets/
cp -f $OUTPUT_FILENAME $APP_DIR/assets/
cp -f $OUTPUT_FOLDER/fingerprint.txt $APP_DIR/assets/

# Copy the obb to the device
#DEVICE_EXTERNAL_STORAGE="/mnt/shell/emulated/obb"
#DEVICE_EXTERNAL_OBB_PATH=$DEVICE_EXTERNAL_STORAGE/$OBB_PACKAGE_NAME
#DEVICE_FILE_EXISTS=$(adb shell ls $DEVICE_EXTERNAL_OBB_PATH/$OBB_NAME)

#if [ "$FORCE_COPY" == "true" ] || [[ "$DEVICE_FILE_EXISTS" == *"No such file"* ]]; then
#    # Copy the file to the device, if needed
#    echo "-- Copying OBB to the device --"
#    $(adb shell mkdir $DEVICE_EXTERNAL_OBB_PATH &>/dev/null)
#    $(adb shell rm $DEVICE_EXTERNAL_OBB_PATH/$OBB_NAME &>/dev/null)
#    $(adb push $OUTPUT_FILENAME $DEVICE_EXTERNAL_OBB_PATH/$OBB_NAME)
#
#    echo "-- OBB saved to the device --"
#else
#    echo "-- Device OBB is up to date --"
#fi

cd $INITIAL_DIR
