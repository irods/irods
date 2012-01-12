
# You will need to create this sequence if you want to use sequence id
# values when using generalUpdate.
create sequence R_ExtObjectId increment by 1 start with 10000;

create table TEST
 (
  id bigint,
  name varchar(32),
  time_stamp varchar(32)
 );
