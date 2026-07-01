#!/bin/bash

# this script copies a random subset of files from the source directory to the target directory, while ensuring that files listed in usedrecords-electric.txt are excluded. 
# It also has options to remove source files after copying and to randomly delete a percentage of files from the target directory before copying (use with caution!).
# Usage: ./copy_trainingsubset_electric.sh [SOURCE_DIR] [TARGET_DIR] [USEDRECORDS_FILE] [REMOVE_SOURCE_FILES] [REMOVE_TARGET_FILES] [RETAIN_TARGET_FILES_PERCENTAGE] [NUM_FILES_TO_COPY] [SHUFFLE_FILES (default: true)]
# check the parameter input and output usage if not enough parameters are provided
if [ "$#" -lt 7 ] || [ "$#" -gt 8 ]; then
    echo "Usage: $0 [SOURCE_DIR] [TARGET_DIR] [USEDRECORDS_FILE] [REMOVE_SOURCE_FILES] [REMOVE_TARGET_FILES] [RETAIN_TARGET_FILES_PERCENTAGE] [NUM_FILES_TO_COPY] [SHUFFLE_FILES (default: true)]"
    exit 1
fi

SOURCE_DIR=$1
TARGET_DIR=$2
USEDRECORDS_FILE=$3
REMOVE_SOURCE_FILES=$4 # Set to true if you want to move instead of copy
REMOVE_TARGET_FILES=$5 # Set to true if you want to delete files from target after copying (use with caution!)
RETAIN_TARGET_FILES_PERCENTAGE=$6 # Percentage of files to retain in target when REMOVE_TARGET_FILES is true (0-100)
NUM_FILES_TO_COPY=$7 #5690 #20000
SHUFFLE_FILES=${8:-true} # Set to true to shuffle files, false to grab them in standard find order. Defaults to true.

# remove retain percentage from num files to copy if target files will be removed. calculate lower integer bound of numfiles = numfiles * (100 - retain percentage) / 100
if [ "$REMOVE_TARGET_FILES" = true ] && [ "$RETAIN_TARGET_FILES_PERCENTAGE" -ge 0 ] && [ "$RETAIN_TARGET_FILES_PERCENTAGE" -lt 100 ]; then
    NUM_FILES_TO_COPY=$(( NUM_FILES_TO_COPY * (100 - RETAIN_TARGET_FILES_PERCENTAGE) / 100 ))   
    echo "Adjusted number of files to copy after accounting for target file removal: $NUM_FILES_TO_COPY"
fi
# first ensure target directory exists

mkdir -p "$TARGET_DIR"

# remove a percentage of files from target if enabled (use with caution!)
if [ "$REMOVE_TARGET_FILES" = true ] && [ "$RETAIN_TARGET_FILES_PERCENTAGE" -ge 0 ] && [ "$RETAIN_TARGET_FILES_PERCENTAGE" -lt 100 ]; then
    NUM_FILES_TO_DELETE=$(( $(find "$TARGET_DIR" -type f | wc -l) * (100 - RETAIN_TARGET_FILES_PERCENTAGE) / 100 ))
    echo "Removing $((100 - RETAIN_TARGET_FILES_PERCENTAGE))% of files from $TARGET_DIR..."
    
    if [ "$SHUFFLE_FILES" = true ]; then
        find "$TARGET_DIR" -type f | shuf | head -n "$NUM_FILES_TO_DELETE" | xargs rm -f
    else
        find "$TARGET_DIR" -type f | head -n "$NUM_FILES_TO_DELETE" | xargs rm -f
    fi
    
    echo "Finished removing files from $TARGET_DIR."
fi

# Create usedrecords file if it doesn't exist
touch "$USEDRECORDS_FILE"

# Create temp files
TMP_EXCLUDE=$(mktemp)
TMP_SELECTED=$(mktemp)

# Strip comments/blanks and get basenames to exclude
while IFS= read -r line; do
    [[ -z "$line" || "$line" =~ ^# ]] && continue
    basename "$line"
done < "$USEDRECORDS_FILE" > "$TMP_EXCLUDE"

# Find all files, filter out excluded ones, optionally shuffle, pick N, save to temp file
if [ "$SHUFFLE_FILES" = true ]; then
    find "$SOURCE_DIR" -type f | \
        awk -F/ -v excludefile="$TMP_EXCLUDE" '
            BEGIN { while ((getline line < excludefile) > 0) exclude[line]=1 }
            !($NF in exclude)
        ' | \
        shuf | \
        head -n "$NUM_FILES_TO_COPY" > "$TMP_SELECTED"
else
    find "$SOURCE_DIR" -type f | \
        awk -F/ -v excludefile="$TMP_EXCLUDE" '
            BEGIN { while ((getline line < excludefile) > 0) exclude[line]=1 }
            !($NF in exclude)
        ' | \
        head -n "$NUM_FILES_TO_COPY" > "$TMP_SELECTED"
fi

# Check if we actually found files to copy
FILE_COUNT=$(wc -l < "$TMP_SELECTED")
echo "Selected $FILE_COUNT files to copy."

if [ "$FILE_COUNT" -gt 0 ]; then
    echo "Starting bulk copy to $TARGET_DIR..."
    # Ask user for confirmation before proceeding when remove source files is enabled
    if [ "$REMOVE_SOURCE_FILES" = true ]; then
        read -p "This will MOVE files from $SOURCE_DIR to $TARGET_DIR. Are you sure? (y/n) " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Aborting."
            rm -f "$TMP_EXCLUDE" "$TMP_SELECTED" "${TMP_SELECTED}_relative"
            exit 1
        fi
    fi

    # rsync with a single call. 
    # Note: --files-from preserves paths by default. We cd into SOURCE_DIR 
    # and strip the SOURCE_DIR prefix from the list to copy them flat.
    sed "s|^$SOURCE_DIR/*||" "$TMP_SELECTED" > "${TMP_SELECTED}_relative"
    if [ "$REMOVE_SOURCE_FILES" = true ]; then
        rsync -a --remove-source-files --info=progress2 --files-from="${TMP_SELECTED}_relative" "$SOURCE_DIR" "$TARGET_DIR/"
    else
        rsync -a --info=progress2  --files-from="${TMP_SELECTED}_relative" "$SOURCE_DIR" "$TARGET_DIR/"
    fi

    # Append all successfully copied files to used records in one operation
    cat "$TMP_SELECTED" >> "$USEDRECORDS_FILE"
    echo "Finished copying $FILE_COUNT files and updated $USEDRECORDS_FILE"
else
    echo "No new files to copy."
fi

# Cleanup
rm -f "$TMP_EXCLUDE" "$TMP_SELECTED" "${TMP_SELECTED}_relative"
