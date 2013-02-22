This is an example for writing a very basic microservice plugin.

The example code will create a microservice which will be loaded
and run by the accompanying rule file and write out some text to
the screen.

To install this example plugin:
 $ make
 $ cp libeirods_msvc_test.so /var/lib/eirods/iRODS/server/bin/
 $ chown eirods:eirods /var/lib/eirods/iRODS/server/bin/libeirods_msvc_test.so

Running make will generate the plugin, which will then
need be copied to the /var/lib/eirods/iRODS/server/bin directory
and chown'ed to eirods:eirods by the Data Grid Administrator for the
plugin to be loadable and the microservice made available to be
executed.

The example can then be run with the command:
 $ irule -F run_eirods_msvc_test.r
