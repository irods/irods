This is an example for writing a very basic microservice plugin.

The example code will create a microservice which will be loaded
and run by the accompanying rule file and write out some text to
the screen.

To install this example plugin:
 $ make msvc_test
 $ cp libirods_msvc_test.so /var/lib/irods/plugins/microservices
 $ chown irods:irods /var/lib/irods/plugins/microservices/libirods_msvc_test.so

Running make will generate the plugin, which will then
need be copied to the /var/lib/irods/plugins/microservices directory
and chown'ed to irods:irods by the Data Grid Administrator for the
plugin to be loadable and the microservice made available to be
executed.

The example can then be run with the command:
 $ irule -F run_irods_msvc_test.r
