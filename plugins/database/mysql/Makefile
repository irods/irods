TARGET = libmysql.so

SRCS = db_plugin.cpp                \
       general_query.cpp            \
       general_update.cpp           \
       general_query_setup.cpp      \
       mid_level_routines.cpp       \
       low_level_odbc.cpp           \
       irods_catalog_properties.cpp \
       irods_sql_logger.cpp         \
       irods_tmp_string.cpp

HEADERS = 

EXTRALIBS = ../../../iRODS/lib/core/obj/irods_plugin_base.o

# works for linux
# does not work on macosx - look at postgres/Makefile for .dylib information
EXTRA_LDRFLAGS = -lodbc

CLL_OBJS =	\
		$(OBJDIR)/low_level_odbc.o    \
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

EXTRA_CFLAGS = -DMY_ICAT
include ../Makefile.base
