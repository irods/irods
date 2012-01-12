
# You will need to create this sequence if you want to use sequence id
# values when using generalUpdate.
create sequence R_ExtObjectId increment by 1 start with 10000;

create table CADC_CONFIG_ARCHIVE_CASE
 (
   archive_keyword varchar(8) not null,
   archive_name varchar(8) not null,
   archive_case varchar(8) not null
 );

create table CADC_CONFIG_COMPRESSION
 (
   compression_keyword varchar(16) not null,
   compression_type varchar(8) not null,
   compression_extension varchar(8) not null,
   mime_encoding varchar(16),
   archive_list varchar(250)
 );

create table CADC_CONFIG_FORMAT
 (
   format_keyword varchar(8) not null,
   format_type varchar(8) not null,
   format_extension varchar(8),
   format_name varchar(8),
   mime_types varchar(64),
   archive_list varchar(250)
 );

