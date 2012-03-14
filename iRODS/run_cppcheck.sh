#!/bin/sh

H="~/dev/irods/E-iRODS/e-irods/iRODS/"

~/bin/cppcheck -j 8 -f -I/usr/include -I$H/nt/include -I$H/lib/md5/include -I$H/lib/rbudp/include -I$H/lib/api/include -I$H/lib/core/include -I$H/lib/isio/include -I$H/server/re/include -I$H/server/drivers/include -I$H/server/core/include -I$H/server/icat/include -i nt/ -i modules/msoDrivers/ -i modules/webservices/ -i boost_irods/ -i clients/icommands/rulegen/ . | grep error
