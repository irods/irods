TARGET = liboracle.so

SRCS = db_plugin.cpp                \
       general_query.cpp            \
       general_update.cpp           \
       general_query_setup.cpp      \
       mid_level_routines.cpp       \
       low_level_odbc.cpp         \
       irods_catalog_properties.cpp \
       irods_sql_logger.cpp

HEADERS =

CLL_OBJS = \
	$(OBJDIR)/low_level_odbc.o

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
	-DORA_ICAT

EXTRA_LDRFLAGS = -lodbc

include ../Makefile.base
