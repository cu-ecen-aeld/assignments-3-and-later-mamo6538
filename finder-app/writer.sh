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

#check if directory exists and make it if not
#got help from https://unix.stackexchange.com/questions/305844/how-to-create-a-file-and-parent-directories-in-one-command 
mkdir -p "$(dirname "$1")"
touch $1

#error checking for if file was created
if [ ! -f $1 ]; then
 echo $1 could not be created
 return 1
fi

echo $2 > $1
