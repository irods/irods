#!/bin/bash

exec=$(which psql)

link=$exec

while [[ -L $exec ]]; do 
    link=$(ls -l $exec | awk '/->/ {print $NF}'); 
    exec=$link; 
done

echo $link; 
