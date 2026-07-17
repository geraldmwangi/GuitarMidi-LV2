TARGET_ROOT_DIR="/data2/converted_audio_jams"
JAMS_SOURCE_DIR="/data/SynthTab/all_jams_midi_V2_60000_tracks/outall"


# Find all .flac files in the  directory in the parameter and its subdirectories, and for each .flac file, do the following:
# 1. Get the directory of the .flac file
# 2. Get the basename of the directory
# 3. Create a target directory in TARGET_ROOT_DIR with the same structure as the source directory
# 4. Find and copy the corresponding .jams file from JAMS_SOURCE_DIR/BASENAME_DIR to TARGET_DIR, and rename it to annotations.jams
# 5. Copy the .flac file to TARGET_DIR and convert it to 48kHz float 32bit using sox, and save it as audio_48k.flac
# Measure the time taken for each .flac file and print it to the console, and estimate the total time remaining based on the number of .flac files and the time taken for each file, and print it to the console as well.

START_TIME=$(date +%s)
TOTAL_FILES=$(find . -iname "*.flac" | wc -l)
CURRENT_FILE=0
find $1 -iname "*.flac" -print0 | while IFS= read -r -d '' flac; do
    echo "Processing ${flac}"
    #echo "$flac"
    DIR=$(dirname "${flac}")


    #echo $DIR
    # Get the basename of the directory
    BASENAME_DIR=$(basename "${DIR}")
    #echo $BASENAME_DIR
    TARGET_DIR="${TARGET_ROOT_DIR}/${DIR}"
    # Create the target directory if it doesn't exist, otherwise continue to the next iteration
    if [ -d "${TARGET_DIR}" ]; then
        echo "Directory ${TARGET_DIR} already exists, skipping..."
        continue
    fi
  

    mkdir -p "${TARGET_DIR}"

    # Find and copy the corresponding .jams file from JAMS_SOURCE_DIR/BASENAME_DIR to TARGET_DIR, and rename it to annotations.jams
    find "${JAMS_SOURCE_DIR}/${BASENAME_DIR}" -iname "*.jams" -print0 | while IFS= read -r -d '' jams; do
        echo "Copying ${jams} to ${TARGET_DIR}/annotations.jams"
        cp "${jams}" "${TARGET_DIR}/annotations.jams"
    done

    # Copy the .flac file to TARGET_DIR and convert it to 48kHz float 32bit using sox, and save it as audio_48k.flac
    echo "Converting ${flac} to ${TARGET_DIR}/audio_48k.flac"
    sox -SG "${flac}"  -r 48000 -e float -b 32 "${TARGET_DIR}/audio_48k.flac"

    CURRENT_FILE=$((CURRENT_FILE + 1))
    # Get correct ellapsed time and estimated remaining time which is nonzero
        ELAPSED_TIME=$(($(date +%s) - START_TIME))
    AVERAGE_TIME=$(($ELAPSED_TIME / $CURRENT_FILE))
    REMAINING_TIME=$(($AVERAGE_TIME * ($TOTAL_FILES - $CURRENT_FILE)))
    echo "Time taken for ${flac}: ${ELAPSED_TIME} seconds"
    echo "Estimated time remaining: ${REMAINING_TIME} seconds"

    #read -p "Press Enter to continue" </dev/tty
done
