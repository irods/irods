.. header:: iRODS Microservice Developers Tutorial
.. footer:: page ###Page###

.. |todaysdate| date::


.. contents:: Table of Contents
.. section-numbering::



-------------
Prerequisites
-------------

This tutorial covers two built-in examples: "Hello World!" and "CURL Get".

To run this tutorial:

#. Download iRODS binary package and Development Tools from http://www.irods.org/download
#. Install iRODS
#. Install the iRODS Development Tools
#. If you want to run the curl based example, install the libcurl and libcurl-dev packages

After installing the iRODS dev tools, make a test directory on your system and copy the contents of /usr/share/irods/examples/microservices into it. This will be our current working directory for the rest of the tutorial.



--------------------------------------------
About microservices and microservice plugins
--------------------------------------------

An iRODS microservice is a C function which takes as parameters:

- One argument of type ruleExecInfo_t* (required)
- Any number of arguments of type msParam_t* (optional)

Input and output values of a microservice are passed through its msParam_t* arguments.

A set of helper functions is commonly used in microservices to interface with the generic msParam_t* type:

- parseMspForXxx() for inputs
- fillXxxInMsParam() for outputs

Where 'Xxx' varies depending on the type of parameter ('Str', 'Int', 'DataObjInp', etc.. ).

For more information on iRODS microservices and related helper functions see https://wiki.irods.org/index.php/iRODS_Introduction sections 6 and 8:

- https://wiki.irods.org/index.php/iRODS_Introduction#iRODS_Micro-services
- https://wiki.irods.org/index.php/iRODS_Introduction#Extending_iRODS

iRODS allows microservices to be dynamically loaded as plugins, without the need to rebuild the iRODS server.



--------------------------------------------------
Generic template for writing a microservice plugin
--------------------------------------------------

.. code-block:: c++

  // =-=-=-=-=-=-=-
  // iRODS Includes
  #include "msParam.h"
  #include "reGlobalsExtern.h"
  #include "irods_ms_plugin.h"

  // =-=-=-=-=-=-=-
  // STL Includes
  #include <iostream>

  extern "C" {

      // =-=-=-=-=-=-=-
      // 1. Write a standard issue microservice
      int my_microservice( ..., ruleExecInfo_t* _rei ) {
          [...]
          return 0;
      }

      // =-=-=-=-=-=-=-
      // 2.  Create the plugin factory function which will
      //     return a microservice table entry
      irods::ms_table_entry* plugin_factory() {

          // =-=-=-=-=-=-=-
          // 3. Allocate a microservice plugin which takes the number of function
          //    params as a parameter to the constructor, not including _rei. With
          //    N as the total number of arguments of my_microservice() we would have:
          irods::ms_table_entry* msvc = new irods::ms_table_entry( N-1 );

          // =-=-=-=-=-=-=-
          // 4. Add the microservice function as an operation to the plugin
          //    the first param is the name / key of the operation, the second
          //    is the name of the function which will be the microservice
          msvc->add_operation( "my_microservice", "my_microservice" );

          // =-=-=-=-=-=-=-
          // 5. Return the newly created microservice plugin
          return msvc;
      }

  }; // extern "C"



---------------------------------------
"Hello World!" as a microservice plugin
---------------------------------------

Given the above template we can write a simple microservice plugin that returns a string to the client. That string needs to be passed out via a microservice parameter (msParam_t*). A simple microservice plugin would then look like this (``irods_hello.cpp``):

.. code-block:: c++

  // =-=-=-=-=-=-=-
  // iRODS Includes
  #include "msParam.h"
  #include "reGlobalsExtern.h"
  #include "irods_ms_plugin.h"

  // =-=-=-=-=-=-=-
  // STL Includes
  #include <iostream>

  extern "C" {

      // =-=-=-=-=-=-=-
      // 1. Write a standard issue microservice
      int irods_hello( msParam_t* _out, ruleExecInfo_t* _rei ) {
          std::string my_str = "Hello World!";
          fillStrInMsParam( _out, my_str.c_str() );
          return 0;
      }

      // =-=-=-=-=-=-=-
      // 2. Create the plugin factory function which will
      //    return a microservice table entry
      irods::ms_table_entry* plugin_factory() {

          // =-=-=-=-=-=-=-
          // 3. Allocate a microservice plugin which takes the number of function
          //    params as a parameter to the constructor, not including _rei.
          irods::ms_table_entry* msvc = new irods::ms_table_entry( 1 );

          // =-=-=-=-=-=-=-
          // 4. Add the microservice function as an operation to the plugin
          //    the first param is the name / key of the operation, the second
          //    is the name of the function which will be the microservice
          msvc->add_operation( "irods_hello", "irods_hello" );

          // =-=-=-=-=-=-=-
          // 5. Return the newly created microservice plugin
          return msvc;
      }

  }; // extern "C"


To run this example from your test directory type::

 $ make hello

This should create a shared object: ``libirods_hello.so``

Copy ``libirods_hello.so`` to the microservices plugin directory (as irods)::

 $ sudo -u irods cp -f libirods_hello.so /var/lib/irods/plugins/microservices/

Now that you have "loaded" your new microservice plugin you can test it with its corresponding rule::

 $ irule -F irods_hello.r



-----------------------------------
"CURL Get" as a microservice plugin
-----------------------------------

In this second example we are using libcurl to make a GET request and write the result to an iRODS object. For the full source see ``irods_curl_get.cpp``.

While the previous example simply returned an arbitrary string, this microservice is manipulating iRODS content and therefore needs an iRODS connection context. This connection context is provided to our microservice via its ruleExecInfo_t* parameter and is required when making iRODS API calls to create and write to iRODS objects. In our example this is done by the CURL write function that writes blocks of data to a new iRODS object, using ``rsDataObjCreate()``, ``rsDataObjWrite()``, and ``rsDataObjClose()``. For this reason the iRODS connection context needs to be passed all the way to the CURL write function.

To keep our microservice code concise we define an irodsCurl object that maintains an iRODS connection context and a (reusable) CURL handler. We can then use the ``irodsCurl::get()`` method to make one or more requests to remote objects, e.g::

 myCurl.get("www.example.com/file1.html", "/tempZone/home/example/file1.html");
 myCurl.get("www.example.com/file2.html", "/tempZone/home/example/file2.html");
 myCurl.get("www.example.com/file3.html", "/tempZone/home/example/file3.html");
 etc...


To run this example from your test directory type::

 $ make curl_get

This should create a shared object: ``libirods_curl_get.so``

Copy ``libirods_curl_get.so`` to the microservices plugin directory (as irods)::

 $ sudo -u irods cp -f libirods_curl_get.so /var/lib/irods/plugins/microservices/

Edit the rulefile ``irods_curl_get.r`` to make sure that the destination object path (\*dest_object) is valid in your environment

Run the rulefile::

 $ irule -F irods_curl_get.r

Check if the new object is there::

 $ ils -L PATH_OF_DEST_OBJECT
 $ iget PATH_OF_DEST_OBJECT -

Note the - at the end of the iget command.  This simply puts the retrieved file into stdout and will print the file contents to your screen.
