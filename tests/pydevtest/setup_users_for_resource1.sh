#!/bin/bash
iadmin mkuser alice rodsuser
iadmin mkuser bobby rodsuser
iadmin moduser alice password apass
iadmin moduser bobby password bpass

iadmin mkgroup testgroup
iadmin atg testgroup alice
iadmin atg testgroup bobby

