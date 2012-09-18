This is an example for writing a very basic microservice plugin.  

The example code will create a microservice which will be loaded
and run by the accompanying rule file and write out some text to
the screen.  

Running make will generate the plugin, which will then
need be copied to the /var/lib/e-irods/iRODS/server/bin directory
and chown eirods:eirods by the Data Grid Administrator for the 
plugin to be loaded and the microservice then executed.  The example 
can then be run by the command: irule -F run_eirods_msvc_test.r

