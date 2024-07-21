#!/bin/bash

#Accepts the following arguments: 
# the first argument is a full path to a file (including filename) on the filesystem, referred to below as writefile; 
# the second argument is a text string which will be written within this file, referred to below as writestr

#Exits with value 1 error and print statements if any of the arguments above were not specified

#Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist. 
#Exits with value 1 and error print statement if the file could not be created.



#echo $#
#echo $0 $1 $2

if [ $# -lt 2 ] ; then 
    echo no enough argument!
    echo "usage:$0 <file path> <string to write>"
    exit 1
fi

if [ -z "$2" ] ; then
    echo Second argument is not a valid string!
    exit 1
fi

mkdir -p $(dirname $1)

if [ ! -e $1 ] ; then
    echo "The file $1 doesn't exit! Creating new file ..."
    touch  $1
else 
    echo "The file ALREADY exit! rewriting ..."
fi
echo $2
rm $1
if  ! $(echo "$2" > $1)  ; then 
    echo error creating or rewriting the the file $1
    exit 1
fi



echo The string $2 written into file $1 successfully!

cat $1


