TARGET = liboracle.so

SRCS = db_plugin.cpp                \
       general_query.cpp            \
       general_update.cpp           \
       general_query_setup.cpp      \
       mid_level_routines.cpp       \
       low_level_oracle.cpp         \
       irods_catalog_properties.cpp \
       irods_sql_logger.cpp         \
       irods_tmp_string.cpp

HEADERS = 

EXTRALIBS = ../../../iRODS/lib/core/obj/irods_plugin_base.o 

CLL_OBJS =	\
		$(OBJDIR)/low_level_oracle.o \
		$(OBJDIR)/irods_tmp_string.o  

GENQ_OBJS = \
        $(OBJDIR)/general_query.o \
        $(OBJDIR)/general_query_setup.o \
        ${OBJDIR}/mid_level_routines.o

GENU_OBJS = \
        $(OBJDIR)/general_update.o \
        $(OBJDIR)/general_query.o \
        $(OBJDIR)/general_query_setup.o \
        ${OBJDIR}/mid_level_routines.o

EXTRA_CFLAGS = \
               -DORA_ICAT \
               -I/usr/include/oracle/11.2/client64/
#               -I../../../external/OCI/instantclient_12_1/sdk/include/ \

EXTRA_LDRFLAGS = \
            -L/usr/lib/oracle/11.2/client64/lib/ \
            -lclntsh -locci -lnnz11
#           -lnnz12 -lociei -lclntsh -lclntshcore -lons
#           -L../../../external/OCI/instantclient_12_1/ \


include ../Makefile.base
