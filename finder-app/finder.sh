#!/bin/sh
# Assignment 1 script
# Author: Madeleine Monfort

##ERROR CHECKING----------------------------
if [ $# -lt 2 ]; then #check parameters
 echo You are using $# parameters instead of 2
 echo "filesdir and searchstr are required."
 return 1
fi

if [ ! -d "$1" ]; then #check directory exists
 echo $1 does not represent a directory on the filesystem
 return 1
fi
##-------------------------------------------

#Got assistance on the lines below from https://www.howtogeek.com/442332/how-to-work-with-variables-in-bash/
X_var=$(find -L $1 -type f | wc -l) #gives number of files in filesdir
Y_var=$(grep "$2" $(find -L $1 -type f) | wc -l) #gives number of searchstring matches in filesdir

echo The number of files are $X_var and the number of matching lines are $Y_var

