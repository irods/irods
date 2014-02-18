TARGET = libgsi.so

SRCS = libgsi.cpp

HEADERS = 

EXTRALIBS = ../../../iRODS/lib/core/obj/irods_plugin_base.o \
            ../../../iRODS/lib/core/obj/irods_kvp_string_parser.o \
            ../../../iRODS/lib/api/obj/rcAuthPluginRequest.o \
            -lglobus_gss_assist$(GSI_INSTALL_TYPE2) \
	    -lglobus_gssapi_gsi$(GSI_INSTALL_TYPE2) \
            -lglobus_gsi_credential$(GSI_INSTALL_TYPE2) \
            -lglobus_gsi_proxy_core$(GSI_INSTALL_TYPE2) \
	    -lglobus_gsi_callback$(GSI_INSTALL_TYPE2) \
	    -lglobus_oldgaa$(GSI_INSTALL_TYPE2) \
	    -lglobus_gsi_sysconfig$(GSI_INSTALL_TYPE2) \
	    -lglobus_gsi_cert_utils$(GSI_INSTALL_TYPE2) \
	    -lglobus_openssl_error$(GSI_INSTALL_TYPE2) \
	    -lglobus_openssl$(GSI_INSTALL_TYPE2) \
	    -lglobus_proxy_ssl$(GSI_INSTALL_TYPE2) \
            -lssl \
            -lcrypto \
	    -lglobus_common$(GSI_INSTALL_TYPE2) \
	    -lglobus_callout$(GSI_INSTALL_TYPE2) \
	    -lltdl$(GSI_INSTALL_TYPE2) 


include ../Makefile.base
