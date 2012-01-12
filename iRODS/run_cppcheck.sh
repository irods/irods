#!/bin/sh
cppcheck -I/usr/include -I./nt/include -I./lib/md5/include -I./lib/rbudp/include -I./lib/api/include -I./lib/core/include -I./lib/isio/include -I./server/re/include -I./server/drivers/include -I./server/core/include -I./server/icat/include -i nt/ -i modules/msoDrivers/ -i modules/webservices/ -i boost_irods/ -i clients/icommands/rulegen/ . | grep error
