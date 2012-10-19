This is an example for writing a very basic microservice plugin.  

The example code will create a microservice which will be loaded
and run by the accompanying rule file and write out some text to
the screen.

Running make will generate the plugin, which will then
need be copied to the /var/lib/e-irods/iRODS/server/bin directory
and chown'ed to eirods:eirods by the Data Grid Administrator for the 
plugin to be loadable and the microservice made available to be
executed.

The example can then be run with the command:
  irule -F run_eirods_msvc_test.r
