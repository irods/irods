create table RCORE_SCHEMAS
 (
  schema_name varchar(250) not null,
  schema_subject varchar(250),
  schema_owner bigint not null,
  r_comment varchar(1000),
  create_ts varchar(32),
  modify_ts varchar(32)
 );

create table RCORE_TABLES
 (
  table_id bigint not null,
  table_name varchar(250) not null,
  database_name varchar(250) not null,
  schema_name varchar(250) not null,
  dbschema_name varchar(250),
  table_resc_id bigint not null,
  r_comment varchar(1000),
  create_ts varchar(32),
  modify_ts varchar(32)
 );

create table RCORE_ATTRIBUTES
 (
  attr_id bigint not null,
  table_id bigint not null,
  attr_name varchar(250) not null,
  attr_data_type varchar(250) not null,
  attr_iden_type varchar(10) not null,
  external_attr_name varchar(250) not null,
  default_value varchar(1000),
  attr_expose bigint not null,
  attr_presentation varchar(1000),
  attr_units varchar(250),
  maxsize INTEGER,
  r_comment varchar(1000),
  create_ts varchar(32),
  modify_ts varchar(32)
 );

create table RCORE_FK_RELATIONS
 (
   fk_relation varchar(1000),
   fk_owner bigint not null,
   r_comment varchar(1000),
   create_ts varchar(32),
   modify_ts varchar(32)
 );

create table RCORE_USER_SCHEMAS
 (
  user_schema_name varchar(250) not null,
  uschema_owner bigint not null,
  r_comment varchar(1000),
  create_ts varchar(32),
  modify_ts varchar(32)
 );

create table RCORE_USCHEMA_ATTR
 (
  user_schema_name varchar(250) not null,
  attr_id bigint not null,
  r_comment varchar(1000),
  create_ts varchar(32),
  modify_ts varchar(32)
 );
