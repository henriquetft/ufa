#!/bin/bash
#
# DISCLAIMER:
# This script comes without warranty of any kind. Use it at your own risk.
#
# This script can help you to convert files categorized in folders
# into the UFAFS format: a repository dir with all files
# and metadata with tags for the files.
#
# It copies all files from a source directory to
# the repository dir and sets tags according to the folders on
# source dir.



if [ "$#" -ne 2 ]; then
	echo
	echo "Usage: $0 SOURCE_DIR REPO_DIR" >&2
	exit 1
fi

UFATAG="./ufatag"
if [ ! -f "$UFATAG" ]; then
	UFATAG=$(which ufatag)
fi

[ -f "$UFATAG" ] || {
	echo "ufatag not found"
	exit 1
}

SRC_DIR="$1"
REPO_DIR="$2"

[ -d "$SRC_DIR" ] || {
	echo "source directory $SRC_DIR does not exist"
	exit 1
}

[ -d "$REPO_DIR" ] || {
	echo "making dir $REPO_DIR ..."
	mkdir "${REPO_DIR}"
	[ "$?" != "0" ] && exit 1
}


find "${SRC_DIR}" -name "*" -type f | while read FILE; do 
	DIR=$(dirname "$FILE")
	TAG="${DIR##*/}"
	FILENAME=$(basename "$FILE")

	TAG=$(echo "$TAG"| awk '{print tolower($0)}')
	echo
	
	# If file is not on a subfolder, apply tag
	if [ ! -f "${SRC_DIR}/${FILENAME}" ]; then
		cp "${FILE}" "${REPO_DIR}/${FILENAME}"
		echo "Copied \"${REPO_DIR}/${FILENAME}\""
		echo "Setting tag \"${TAG}\" on \"${FILENAME}\" ..." 
		$UFATAG -r "${REPO_DIR}" set "${FILENAME}" "${TAG}"
		[ $? -ne 0 ] && echo "Error setting tags on ${FILENAME}"
	else
		echo "File \"${FILENAME}\" will not be copied because it is not in a subfolder"
	
	fi
done

