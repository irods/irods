This is an example for writing a very basic resource plugin.

The example code will create a resource plugin which will support
basic unix file system operations.

To install this example plugin:
 $ make
 $ cp libexamplefilesystem.so /var/lib/irods/plugins/resources
 $ chown irods:irods /var/lib/irods/plugins/resources/libexamplefilesystem.so

Running make will generate the plugin.

The plugin (.so) will then need be copied to the appropriate plugin
directory and chown'ed to irods:irods by the Data Grid Administrator
for the plugin to be loadable and the resource made available to the
iRODS server.

