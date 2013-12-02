/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
  header file for the low level, uses either Oracle or Odbc
 */

#if defined(ORA_ICAT) || defined(ORA_DBR)
#include "icatLowLevelOracle.h"
#else
#include "icatLowLevelOdbc.h"
#endif
