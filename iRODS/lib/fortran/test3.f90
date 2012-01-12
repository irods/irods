program main

!
!  This is an example fortran program and that uses the iRODS Fortran I/O
!  library to do direct (thru the network) I/O to iRODS files.  See
!  the Makefile for how it is built.
!
!  The fortran I/O library functions have an _ at the end of their
!  names in the .c file, so that with many fortran compliers, the
!  linker will match up the functions as defined in here (for example,
!  'fio_setup' in this source file is 'fio_setup_' in the fortran_io.c
!  library).  There are other ways to do this (with -assume
!  nounderscore, or -fno-underscoring for example), so this can be
!  done differently if needed for your compiler.
!
  implicit none

  integer status

! The irods interface functions
  integer irods_connect
  integer irods_file_open
  integer irods_file_create
  integer irods_file_read
  integer irods_file_write
  integer irods_file_seek
  integer irods_file_close
  integer irods_disconnect
!

  character*40 filename
  character*200 buffer
  integer fd

  write ( *, '(a)' ) ' '
  write ( *, '(a)' ) '  This is an example/test FORTRAN program '
  write ( *, '(a)' ) '  that calls iRODS interface functions'
  write ( *, '(a)' ) '  to do iRODS I/O.'

  write ( *, '(a)' ) '  Enter the iRODS path/filename to create and write to:'
  read *, filename

  status = irods_connect()
  write ( *, '(a,i8)' ) 'irods_connect = ', status
  if (status < 0) then
     call exit(0)
  end if

  status = irods_file_create(filename)
  write ( *, '(a,i8)' ) 'irods_file_create status = ', status
  if (status < 0) then
     call exit(0)
  end if
  fd = status

  buffer = "0123456789"
  status = irods_file_write( fd, buffer, 10)
  write ( *, '(a,i8)' ) 'irods_file_write status = ', status
  if (status < 0) then
     call exit(0)
  end if

  status = irods_file_close(fd)
  write ( *, '(a,i8)' ) 'irods_file_close status = ', status

  status = irods_disconnect()
  write ( *, '(a,i8)' ) 'irods_disconnect = ', status

  stop
end
