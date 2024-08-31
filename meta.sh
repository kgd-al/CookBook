#!/bin/bash

echo "Extracting metadata..."
#f=$(realpath $1)

echo "Generating metadata in $f"
echo "/* Auto-generated file */" > "$f"
echo "#ifndef ABOUT_METADATA_H" >> "$f"
echo "#define ABOUT_METADATA_H" >> "$f"
echo "#define GIT_HASH   \"$(git log -n1 --author=ManN --pretty=format:'%h')\"" >> "$f"
echo "#define GIT_DATE   \"$(git log -n1 --author=ManN --pretty=format:'%aD')\"" >> "$f"
echo "#define GIT_MSG    \"$(git log -n1 --author=ManN --pretty=format:'%s')\"" >> "$f"
echo "#define BUILD_DATE \"$(date -R)\"" >> "$f"
echo "#endif" >> "$f"
# cat $f
