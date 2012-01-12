This module uses YAZ (a Z39.50 API toolkit) to query any Z39.50 server.

To use the module, follow these steps:

1. Acquire the latest YAZ distribution.  This can be found at:
     http://ftp.indexdata.dk/pub/yaz/

2. Save the downloaded files into iRODS/modules/Z3950/yaz/.

3. Build the yaz libraries into /usr/local
     cd iRODS/modules/Z3950/yaz/
     ./configure
     make
     make install (may require 'sudo make install')

4. Build the Z3950 module.
     cd ..
     make

5. Enable this module by changing the "Enabled" option
   in iRODS/modules/Z3950/info.txt to "yes" (instead of "no").

6. Rerun the iRODS configure script and recompile iRODS.
     cd ../../
     ./scripts/configure
     make

-----

7. Test against the server of your choice using the sample rule (z3950rule.ir)
     cd modules/Z3950
     irule -vF z3950rule.ir

You can read more about the query types available to Z39.50 at
  http://www.loc.gov/z3950/agency/defns/bib1.html

  and
  
  http://lists.indexdata.dk/pipermail/zoom/2004-July/000758.html

  The most commonly supported attribute set is BIB1 -- the bibliographic
  attribute set version 1.  BIB1 has 6 attribute types:

  1  Use            The main semantics of the content. Eg title, author, date
  2  Relation       How to compare the term. e.g. equal, less than, within
  3  Position       The position of the term in the field, e.g. first, anywhere
  4  Structure      Structure of the term, e.g. date, phrase, string, numeric
  5  Truncation     How to truncate, e.g., right truncation, regexp, none
  6  Completeness   Complete field versus incomplete

  As per the examples, each type has several values, the most populated
  being, unsurprisingly Use attributes.  There is a Very long list of use
  attributes available, in theory, on the BIB1 reference page.  Common ones
  are:
  1     Personal Name
  4     Title
  7     ISBN
  8     ISSN
  12    Local Number
  21    Subject
  30    Date
  1003  Author
  1010  Body of Text
  1016  Any
  1018  Publisher

