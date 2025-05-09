#!/bin/bash

currentDirIsobmff=$(pwd)
isobmffDir=$currentDirIsobmff/../libisomediafile
gitBaseDir=$currentDirIsobmff/isobmff
vcmTool="vcmSVN"
promptRequest=1
verbose=0

# loop over arguments
for var in "$@"
do
  if [ "$var" == "GIT" ]; then vcmTool="vcmGIT"; fi
  if [ "$var" == "-p" ]; then promptRequest=0; fi
  if [ "$var" == "v" ]; then verbose=1; fi
done

# ask user for existing GitHub account
if [ $promptRequest -eq 1 ]; then
  echo -e "Is an GitHub ID registered? (Y/N)"
  read -n 1 GitHubID

  if [[ $GitHubID =~ ^(y|Y)$ ]]; then
    echo "       Checkout failed."
    exit 1
  fi
fi

# create output directories
if [ ! -d "$isobmffDir" ]; then
  mkdir "$isobmffDir"
else
  rm -rf $isobmffDir/*
fi

# checkout the software
if [ "$vcmTool" == "vcmSVN" ]; then
  echo -e "downloading IsoLib from GitHub using SVN...\n"

  # checkout IsoLib from GitHub via SVN
  svn co https://github.com/MPEGGroup/isobmff.git/trunk/IsoLib/libisomediafile $isobmffDir
else
  echo -e "downloading IsoLib from GitHub using GIT...\n"

  # checkout IsoLib from GitHub via GIT
  if [ -d "$gitBaseDir" ]; then
    rm -rf $gitBaseDir
  fi
  git clone https://github.com/MPEGGroup/isobmff.git $gitBaseDir

  # copy libisomediafile in to the destination directory
  cp -r $gitBaseDir/IsoLib/libisomediafile $isobmffDir
  if [ $? -ne 0 ]; then
    echo "       Checkout failed."
    exit 1
  fi
fi

echo -e "\n       Checkout complete."
exit 0
