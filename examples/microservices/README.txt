This is an example for writing a very basic microservice plugin.  

The example code will create a microservice which will be loaded
and ran by the accompanying rule file and write out some text to
the screen.  

Running make will generate the plugin, which will then
need be copied to the /var/lib/e-irods/iRODS/modules/bin directory
by the Data Grid Administrator for the plugin to be loaded and the 
microservice then executed.  The example can then be ran by the 
command: irule -F run_eirods_msvc_test.r

