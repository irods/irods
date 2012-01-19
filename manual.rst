==================
iRODS-E 3.0 Manual
==================

.. contents:: Table of Contents
.. section-numbering::

----------------
ReStructuredText
----------------

Some links for learning in place:

 http://docutils.sourceforge.net/docs/index.html

 http://docutils.sourceforge.net/docs/user/rst/cheatsheet.txt
 
 http://docutils.sourceforge.net/docs/user/rst/quickstart.txt

 http://docutils.sourceforge.net/docs/user/rst/quickstart.html
 
 http://docutils.sourceforge.net/docs/user/rst/demo.txt

 http://docutils.sourceforge.net/docs/user/rst/demo.html
 
 http://rst2pdf.googlecode.com/svn/trunk/doc/manual.txt

Generate HTML:
 rst2html.py -stg manual.rst > manual.html

Generate PDF:
 rst2pdf manual.rst -o manual.pdf

--------
Overview
--------

iRODS is very powerful.

 http://www.irods.org

------------
Installation
------------

iRODS-E is released in binary form.  RPM and DEB formats are available for both iCAT-enabled servers and resource-only servers.  There are variations available for your combination of platform, operating system, and database type.

Installation of the x86-Ubuntu-Postgres RPM:
 rpm -i irodse-3.0-x86-ubuntu-postgres.rpm

Installation of the x86-CentOS-MySQL DEB:
 dpkg -i irodse-3.0-x86-centos-mysql.deb

----------
Quickstart
----------

Successful installation will complete and leave a running iRODS server.  If you installed an iCAT-enabled iRODS server, a database of your choice will also have been created and running.  The i-command `ils` will list your administrator's empty home directory in the iRODS virtual filesystem::

 $ ils
 /tempZone/home/rods:
 $


-------------------
History of Releases
-------------------

==========   =======    =====================================================
Date         Version    Description
==========   =======    =====================================================
2012-03-01   3.0        Initial Release.
                         This is the first release from RENCI, based on the
                         iRODS 3.0 community codebase.
==========   =======    =====================================================
