#!/bin/bash

result=$(which psql)

newdir=$(dirname $result)
cd $newdir

while [[ -L $result ]]; do
    link=$(ls -l $result | awk '/->/ {print $NF}');
    echo "$result pointing to $link"
    newdir=$(dirname $link)
    filename=$(basename $link)
    cd $newdir
    fulldir=$(pwd)
    result=$fulldir/$filename
done

echo $result

