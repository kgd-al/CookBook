#!/bin/bash

set -euo pipefail

dir=build
./build.sh

ddir="/home/$USER/.local/share/applications/"
desktopname="cookbook.desktop"
l_file="$dir/$desktopname"
s_file="$ddir/$desktopname"
[ -f $s_file ] && exit 0

mkdir -p $ddir

base=$(pwd)
cat >$l_file <<EOF
[Desktop Entry]
Type=Application
Exec=$base/$dir/CookBook %u
# TryExec=your-app
# MimeType=application/x-your-mime-type;
Icon=$base/icons/book.png
# X-DocPath=yourapp/index.html
Terminal=false
Name=CookBook
GenericName=Malenda's cook book
Comment=List of recipies (made with love)
Categories=Qt;KDE;X-Malenda;Utility
EOF

desktop-file-validate $l_file
desktop-file-install $l_file --dir=$ddir
sudo update-desktop-database $ddir -v
