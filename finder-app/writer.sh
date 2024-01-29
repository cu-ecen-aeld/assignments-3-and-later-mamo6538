#!/bin/sh
# Assignment 1 script
# Author: Madeleine Monfort

##ERROR CHECKING----------------------------
if [ $# -lt 2 ]; then #check parameters
 echo You are using $# parameters instead of 2
 echo "writefile and writestr are required."
 return 1
fi
##-------------------------------------------

writefile=$1
writestr=$2

#check if directory exists and make it if not
#got help from https://unix.stackexchange.com/questions/305844/how-to-create-a-file-and-parent-directories-in-one-command 
mkdir -p "$(dirname "$writefile")"
touch $writefile

#error checking for if file was created
if [ ! -f $writefile ]; then
 echo $writefile could not be created
 return 1
fi

echo $writestr > $writefile
