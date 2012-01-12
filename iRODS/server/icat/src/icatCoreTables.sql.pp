
/*******************************************************************************
  These are the Core Tables in the RODS Catalog 
    RSYSOCORE_SCHEMAS        - Schemas defines in the RODS catalog
    RCORE_TABLES         - Tables in the RCORE_SCHEMAS
    RCORE_ATTRIBUTES     - ATtributes in RCORE_TABLES
    RCORE_FK_RELATIONS   - Relationships across two tables
    RCORE_USER_SCHEMAS   - User Schemas 
    RCORE_USCHEMA_ATTR   - Attributes in User Schemas

  The column lengths are used as follows:
    ids                - bigint (64 bits)
    dates              - varchar(32)
    short strings      - varchar(250)
    long strings       - varchar(1000)
    very long strings  - varchar(2700)

*******************************************************************************/

#if defined(postgres) || defined(mysql)
#define INT64TYPE bigint
#else
#define INT64TYPE integer
#endif

#if defined(mysql)
SET SESSION storage_engine='InnoDB';
#endif

create table RCORE_SCHEMAS
 (
  schema_name  		varchar(250) not null,
  schema_subject        varchar(250),
  schema_owner          INT64TYPE not null,
  r_comment		varchar(1000),
  create_ts             varchar(32),
  modify_ts		varchar(32)
 );

create table RCORE_TABLES
 (
  table_id 		INT64TYPE not null,
  table_name		varchar(250) not null,
  database_name		varchar(250) not null,
  schema_name           varchar(250) not null,
  dbschema_name         varchar(250),
  table_resc_id		INT64TYPE not null,
  r_comment     	varchar(1000),
  create_ts             varchar(32),
  modify_ts		varchar(32)
 );

create table RCORE_ATTRIBUTES
 (
  attr_id            	INT64TYPE not null,
  table_id		INT64TYPE not null,
  attr_name		varchar(250) not null,
  attr_data_type	varchar(250) not null,
  attr_iden_type        varchar(10) not null,
  external_attr_name    varchar(250) not null,
  default_value         varchar(1000),
  attr_expose           INT64TYPE not null,
  attr_presentation     varchar(1000),
  attr_units            varchar(250),
  maxsize               INTEGER,
  r_comment		varchar(1000),
  create_ts         	varchar(32),
  modify_ts		varchar(32)
 );

create table RCORE_FK_RELATIONS
 (
   fk_relation          varchar(1000),
   fk_owner             INT64TYPE not null,
   r_comment     	varchar(1000),
   create_ts         	varchar(32),
   modify_ts		varchar(32)
 );

create table RCORE_USER_SCHEMAS
 (
  user_schema_name     	varchar(250) not null,
  uschema_owner         INT64TYPE not null,
  r_comment     	varchar(1000),
  create_ts        	varchar(32),
  modify_ts		varchar(32)
 );

create table RCORE_USCHEMA_ATTR
 (
  user_schema_name     	varchar(250) not null,
  attr_id              	INT64TYPE not null,
  r_comment     	varchar(1000),
  create_ts  		varchar(32),
  modify_ts  		varchar(32)
 );


