#!/bin/sh
# Assignment 1 script
# Author: Madeleine Monfort

##ERROR CHECKING----------------------------
if [ $# -ne 2 ]; then #check parameters
 echo You are using $# parameters instead of 2
 echo "use: ./finder filesdir searchstr"
 echo "filesdir and searchstr are required parameters."
 return 1
fi

filesdir=$1
searchstr=$2

if [ ! -d "$filesdir" ]; then #check directory exists
 echo $filesdir does not represent a directory on the filesystem
 return 1
fi
##-------------------------------------------

#Got assistance on the lines below from https://www.howtogeek.com/442332/how-to-work-with-variables-in-bash/
File_num=$(find -L $filesdir -type f | wc -l) #gives number of files in filesdir
Match_num=$(grep "$searchstr" $(find -L $filesdir -type f) | wc -l) #gives number of searchstring matches in filesdir

echo The number of files are $File_num and the number of matching lines are $Match_num

