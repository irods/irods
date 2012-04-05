#!/bin/bash

# locates system postgres on centos and ubuntu
result=$(locate plpgsql.so | grep usr)
echo $result
