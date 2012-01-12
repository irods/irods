
# You will need to create this sequence if you want to use sequence id
# values when using generalUpdate.
create sequence R_ExtObjectId increment by 1 start with 10000;

create table TEST1
 (
  id bigint,
  name varchar(32),
  time_stamp varchar(32)
 );

create table TEST2
 (
  id bigint,
  name varchar(32)
 );
