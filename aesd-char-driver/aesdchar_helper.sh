#!/bin/sh
# Helper script used to develop assignment 8 char driver implementation
sudo ./aesdchar_unload

make clean
make

sudo ./aesdchar_load

isloaded=$(lsmod | grep aesd | wc -l)

if [ $isloaded -lt 1 ]; then
  echo "aesdchar not loaded"
  exit 1
fi

#perhaps stack trace and echo?

