from . import database_connect
from .exceptions import IrodsError, IrodsWarning
from . import lib

import logging
import os
import re
import tempfile
import time

def run_update(irods_config, cursor):
    l = logging.getLogger(__name__)
    new_schema_version = database_connect.get_schema_version_in_database(cursor) + 1
    l.info('Updating to schema version %d...', new_schema_version)
    if new_schema_version == 2:
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('listQueryByAliasLike', 'SELECT alias, sqlStr FROM R_SPECIFIC_QUERY WHERE alias LIKE ?', '1388534400');")
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('findQueryByAlias', 'SELECT alias, sqlStr FROM R_SPECIFIC_QUERY WHERE alias = ?', '1388534400');")
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('ilsLACollections', 'SELECT c.parent_coll_name, c.coll_name, c.create_ts, c.modify_ts, c.coll_id, c.coll_owner_name, c.coll_owner_zone, c.coll_type, u.user_name, u.zone_name, a.access_type_id, u.user_id FROM R_COLL_MAIN c JOIN R_OBJT_ACCESS a ON c.coll_id = a.object_id JOIN R_USER_MAIN u ON a.user_id = u.user_id WHERE c.parent_coll_name = ? ORDER BY c.coll_name, u.user_name, a.access_type_id DESC LIMIT ? OFFSET ?', '1388534400');")
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('ilsLADataObjects', 'SELECT s.coll_name, s.data_name, s.create_ts, s.modify_ts, s.data_id, s.data_size, s.data_repl_num, s.data_owner_name, s.data_owner_zone, u.user_name, u.user_id, a.access_type_id, u.user_type_name, u.zone_name FROM ( SELECT c.coll_name, d.data_name, d.create_ts, d.modify_ts, d.data_id, d.data_repl_num, d.data_size, d.data_owner_name, d.data_owner_zone FROM R_COLL_MAIN c JOIN R_DATA_MAIN d ON c.coll_id = d.coll_id WHERE c.coll_name = ? ORDER BY d.data_name) s JOIN R_OBJT_ACCESS a ON s.data_id = a.object_id JOIN R_USER_MAIN u ON a.user_id = u.user_id ORDER BY s.coll_name, s.data_name, u.user_name, a.access_type_id DESC LIMIT ? OFFSET ?', '1388534400');")
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('listSharedCollectionsOwnedByUser', 'SELECT DISTINCT R_COLL_MAIN.coll_id, R_COLL_MAIN.parent_coll_name, R_COLL_MAIN.coll_name, R_COLL_MAIN.coll_owner_name, R_COLL_MAIN.coll_owner_zone, R_META_MAIN.meta_attr_name, R_META_MAIN.meta_attr_value, R_META_MAIN.meta_attr_unit FROM R_COLL_MAIN JOIN R_OBJT_METAMAP ON R_COLL_MAIN.coll_id = R_OBJT_METAMAP.object_id JOIN R_META_MAIN ON R_OBJT_METAMAP.meta_id = R_META_MAIN.meta_id WHERE R_META_MAIN.meta_attr_unit = ''iRODSUserTagging:Share'' AND R_COLL_MAIN.coll_owner_name = ? AND R_COLL_MAIN.coll_owner_zone = ? ORDER BY R_COLL_MAIN.parent_coll_name ASC, R_COLL_MAIN.coll_name ASC', '1388534400');")
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('listSharedCollectionsSharedWithUser', 'SELECT DISTINCT R_COLL_MAIN.coll_id, R_COLL_MAIN.parent_coll_name, R_COLL_MAIN.coll_name, R_COLL_MAIN.coll_owner_name, R_COLL_MAIN.coll_owner_zone, R_META_MAIN.meta_attr_name, R_META_MAIN.meta_attr_value, R_META_MAIN.meta_attr_unit, R_USER_MAIN.user_name, R_USER_MAIN.zone_name, R_OBJT_ACCESS.access_type_id FROM R_COLL_MAIN JOIN R_OBJT_METAMAP ON R_COLL_MAIN.coll_id = R_OBJT_METAMAP.object_id JOIN R_META_MAIN ON R_OBJT_METAMAP.meta_id = R_META_MAIN.meta_id JOIN R_OBJT_ACCESS ON R_COLL_MAIN.coll_id = R_OBJT_ACCESS.object_id JOIN R_USER_MAIN ON R_OBJT_ACCESS.user_id = R_USER_MAIN.user_id WHERE R_META_MAIN.meta_attr_unit = ''iRODSUserTagging:Share'' AND R_USER_MAIN.user_name = ? AND R_USER_MAIN.zone_name = ? AND R_COLL_MAIN.coll_owner_name <> ? ORDER BY R_COLL_MAIN.parent_coll_name ASC, R_COLL_MAIN.coll_name ASC', '1388534400');")
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('listUserACLForDataObjViaGroup', 'SELECT R_USER_MAIN.user_name, R_USER_MAIN.user_id, R_OBJT_ACCESS.access_type_id, R_USER_MAIN.user_type_name, R_USER_MAIN.zone_name, R_COLL_MAIN.coll_name, R_DATA_MAIN.data_name, USER_GROUP_MAIN.user_name, R_DATA_MAIN.data_name, R_COLL_MAIN.coll_name FROM R_USER_MAIN AS USER_GROUP_MAIN JOIN R_USER_GROUP JOIN R_USER_MAIN ON R_USER_GROUP.user_id = R_USER_MAIN.user_id ON USER_GROUP_MAIN.user_id = R_USER_GROUP.group_user_id JOIN R_OBJT_ACCESS ON R_USER_GROUP.group_user_id = R_OBJT_ACCESS.user_id JOIN R_DATA_MAIN JOIN R_COLL_MAIN ON R_DATA_MAIN.coll_id = R_COLL_MAIN.coll_id ON R_OBJT_ACCESS.object_id = R_DATA_MAIN.data_id WHERE  R_COLL_MAIN.coll_name = ? AND R_DATA_MAIN.data_name = ? AND R_USER_MAIN.user_name = ? ORDER BY R_COLL_MAIN.coll_name, R_DATA_MAIN.data_name, R_USER_MAIN.user_name, R_OBJT_ACCESS.access_type_id DESC', '1388534400');")
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('listUserACLForCollectionViaGroup', 'SELECT R_USER_MAIN.user_name, R_USER_MAIN.user_id, R_OBJT_ACCESS.access_type_id, R_USER_MAIN.user_type_name, R_USER_MAIN.zone_name, R_COLL_MAIN.coll_name, USER_GROUP_MAIN.user_name, R_COLL_MAIN.coll_name FROM R_USER_MAIN AS USER_GROUP_MAIN JOIN R_USER_GROUP JOIN R_USER_MAIN ON R_USER_GROUP.user_id = R_USER_MAIN.user_id ON USER_GROUP_MAIN.user_id = R_USER_GROUP.group_user_id JOIN R_OBJT_ACCESS ON R_USER_GROUP.group_user_id = R_OBJT_ACCESS.user_id JOIN R_COLL_MAIN ON R_OBJT_ACCESS.object_id = R_COLL_MAIN.coll_id WHERE R_COLL_MAIN.coll_name = ? AND R_USER_MAIN.user_name = ? ORDER BY R_COLL_MAIN.coll_name, R_USER_MAIN.user_name, R_OBJT_ACCESS.access_type_id DESC', '1388534400');")

    elif new_schema_version == 3:
        database_connect.execute_sql_statement(cursor, "insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) values ('DataObjInCollReCur', 'WITH coll AS (SELECT coll_id, coll_name FROM r_coll_main WHERE R_COLL_MAIN.coll_name = ? OR R_COLL_MAIN.coll_name LIKE ?) SELECT DISTINCT d.data_id, (SELECT coll_name FROM coll WHERE coll.coll_id = d.coll_id) coll_name, d.data_name, d.data_repl_num, d.resc_name, d.data_path, d.resc_hier FROM R_DATA_MAIN d WHERE d.coll_id = ANY(ARRAY(SELECT coll_id FROM coll)) ORDER BY coll_name, d.data_name, d.data_repl_num', '1388534400');")

    elif new_schema_version == 4:
        database_connect.execute_sql_statement(cursor, "create index idx_quota_main1 on R_QUOTA_MAIN (user_id);")
        database_connect.execute_sql_statement(cursor, "delete from R_TOKN_MAIN where token_name = 'domainadmin';")
        database_connect.execute_sql_statement(cursor, "delete from R_TOKN_MAIN where token_name = 'rodscurators';")
        database_connect.execute_sql_statement(cursor, "delete from R_TOKN_MAIN where token_name = 'storageadmin';")
        if irods_config.catalog_database_type == 'mysql':
            database_connect.execute_sql_statement(cursor, "delete from R_SPECIFIC_QUERY where alias = 'DataObjInCollReCur';")

    elif new_schema_version == 5:
        if irods_config.catalog_database_type == 'oracle':
            database_connect.execute_sql_statement(cursor, "ALTER TABLE R_DATA_MAIN ADD resc_id integer;")
            database_connect.execute_sql_statement(cursor, "ALTER TABLE R_RESC_MAIN ADD resc_parent_context varchar2(4000);") # max oracle varchar2 for sql is 4000, 32767 pl/sql
        else:
            database_connect.execute_sql_statement(cursor, "ALTER TABLE R_DATA_MAIN ADD resc_id bigint;")
            database_connect.execute_sql_statement(cursor, "ALTER TABLE R_RESC_MAIN ADD resc_parent_context varchar(4000);")

        database_connect.execute_sql_statement(cursor, "UPDATE R_SPECIFIC_QUERY SET sqlstr='WITH coll AS (SELECT coll_id, coll_name FROM R_COLL_MAIN WHERE R_COLL_MAIN.coll_name = ? OR R_COLL_MAIN.coll_name LIKE ?) SELECT DISTINCT d.data_id, (SELECT coll_name FROM coll WHERE coll.coll_id = d.coll_id) coll_name, d.data_name, d.data_repl_num, d.resc_name, d.data_path, d.resc_id FROM R_DATA_MAIN d WHERE d.coll_id = ANY(ARRAY(SELECT coll_id FROM coll)) ORDER BY coll_name, d.data_name, d.data_repl_num' where alias='DataObjInCollReCur';")

        rows = database_connect.execute_sql_statement(cursor, "select resc_id, resc_name from R_RESC_MAIN;").fetchall()
        for row in rows:
            resc_id = row[0]
            resc_name = row[1]
            database_connect.execute_sql_statement(cursor, "update R_DATA_MAIN set resc_id=? where resc_hier=? or resc_hier like ?", int(resc_id), resc_name, ''.join(['%;', resc_name]))
        if irods_config.catalog_database_type == 'postgres':
            database_connect.execute_sql_statement(cursor, "update r_resc_main as rdm set resc_parent = am.resc_id from ( select resc_name, resc_id from r_resc_main ) as am where am.resc_name = rdm.resc_parent;")
        elif irods_config.catalog_database_type == 'cockroachdb':
            rows = database_connect.execute_sql_statement(cursor, "select rdm.resc_id, am.resc_id from r_resc_main rdm, r_resc_main am where am.resc_name = rdm.resc_parent;").fetchall()
            for row in rows:
                resc_id = row[0]
                resc_id2 = row[1]
                database_connect.execute_sql_statement(cursor, "update r_resc_main set resc_parent = ? where resc_id = ?;", resc_id2, resc_id)
        elif irods_config.catalog_database_type == 'mysql':
            database_connect.execute_sql_statement(cursor, "update R_RESC_MAIN as rdm, ( select resc_name, resc_id from R_RESC_MAIN ) as am set rdm.resc_parent = am.resc_id where am.resc_name = rdm.resc_parent;")
        else:
            database_connect.execute_sql_statement(cursor, "update R_RESC_MAIN rdm set resc_parent = ( select resc_id from ( select resc_name, resc_id from R_RESC_MAIN ) am where am.resc_name = rdm.resc_parent );")

        rows = database_connect.execute_sql_statement(cursor, "select resc_id, resc_children from R_RESC_MAIN where resc_children is not null;").fetchall()
        context_expression = re.compile('^([^{}]*)\\{([^{}]*)\\}')
        for row in rows:
            resc_id = row[0]
            child_contexts = [(m.group(1), m.group(2)) for m in [context_expression.match(s) for s in row[1].split(';')] if m]
            for child_name, context in child_contexts:
                database_connect.execute_sql_statement(cursor, "update R_RESC_MAIN set resc_parent_context=? where resc_name=?", context, child_name)

    elif new_schema_version == 6:
        database_connect.execute_sql_statement(cursor, "create index idx_data_main7 on R_DATA_MAIN (resc_id);")
        database_connect.execute_sql_statement(cursor, "create index idx_data_main8 on R_DATA_MAIN (data_is_dirty);")

    elif new_schema_version == 7:
        timestamp = '{0:011d}'.format(int(time.time()))
        sql = ("select distinct group_user_id from R_USER_GROUP "
               "where group_user_id not in (select distinct group_user_id from R_USER_GROUP where group_user_id = user_id);")
        rows = database_connect.execute_sql_statement(cursor, sql).fetchall()
        for row in rows:
            group_id = row[0]
            database_connect.execute_sql_statement(cursor, "insert into R_USER_GROUP values (?,?,?,?);", group_id, group_id, timestamp, timestamp)

        # Add specific query that allows listing all groups a user is a member of.
        sql = ("insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) "
               "values ('listGroupsForUser', "
                        "'select group_user_id, user_name from R_USER_GROUP ug"
                        " inner join R_USER_MAIN u on ug.group_user_id = u.user_id"
                        " where user_type_name = ''rodsgroup'' and ug.user_id = (select user_id from R_USER_MAIN where user_name = ? and user_type_name != ''rodsgroup'')', "
                        "'1580297960');")
        database_connect.execute_sql_statement(cursor, sql)

    elif new_schema_version == 8:
        # Add a new column that will be responsible for holding the Rule Execution Info (context)
        # as JSON for delay rules.
        if irods_config.catalog_database_type == 'oracle':
            # CLOB has a maximum size of 128 TB.
            database_connect.execute_sql_statement(cursor, "alter table R_RULE_EXEC add exe_context clob;")
        elif irods_config.catalog_database_type == 'mysql':
            # LONGTEXT has a maximum size of 2^32.
            database_connect.execute_sql_statement(cursor, "alter table R_RULE_EXEC add column exe_context longtext;")
        else:
            # TEXT has no upper limit on the number of bytes it can hold.
            database_connect.execute_sql_statement(cursor, "alter table R_RULE_EXEC add column exe_context text;")

    elif new_schema_version == 9:
        # Add leader and successor entries to the R_GRID_CONFIGURATION table for delay_server migration protocol
        database_connect.execute_sql_statement(cursor, "insert into R_GRID_CONFIGURATION values ('delay_server','leader',?);", lib.get_hostname())
        database_connect.execute_sql_statement(cursor, "insert into R_GRID_CONFIGURATION values ('delay_server','successor','');")

        # Update permission model to replace spaces with underscores
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'delete_object' where token_name = 'delete object';")
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'modify_object' where token_name = 'modify object';")
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'create_object' where token_name = 'create object';")
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'delete_metadata' where token_name = 'delete metadata';")
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'modify_metadata' where token_name = 'modify metadata';")
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'create_metadata' where token_name = 'create metadata';")
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'read_object' where token_name = 'read object';")
        database_connect.execute_sql_statement(cursor, "update R_TOKN_MAIN set token_name = 'read_metadata' where token_name = 'read metadata';")

    elif new_schema_version == 10:
        if irods_config.catalog_database_type == 'mysql':
            # Assume the database is MySQL 8 or later (support for the SQL WITH-clause was not added until MySQL 8).
            if len(database_connect.execute_sql_statement(cursor, "select alias from R_SPECIFIC_QUERY where alias = 'DataObjInCollReCur';").fetchall()) == 0:
                sql = ("insert into R_SPECIFIC_QUERY (alias, sqlStr, create_ts) "
                       "values ('DataObjInCollReCur', "
                                "'with COLL as (select coll_id, coll_name from R_COLL_MAIN where R_COLL_MAIN.coll_name = ? or R_COLL_MAIN.coll_name like ?)"
                                " select distinct d.data_id, (select coll_name from COLL where COLL.coll_id = d.coll_id) coll_name, d.data_name, d.data_repl_num, d.resc_name, d.data_path, d.resc_id"
                                " from R_DATA_MAIN d"
                                " where d.coll_id = any(select coll_id from COLL) order by coll_name, d.data_name, d.data_repl_num', "
                                "'1580297960');")
                database_connect.execute_sql_statement(cursor, sql)

    elif new_schema_version == 11:
        if irods_config.catalog_database_type == 'mysql':
            # Rerunning the MySQL script, mysql_functions.sql, fixes issues with sequence numbers
            # and the potential for deadlocks in the database.
            with tempfile.NamedTemporaryFile() as f:
                f.write('\n'.join([
                    '[client]',
                    '='.join(['user', irods_config.database_config['db_username']]),
                    '='.join(['password', irods_config.database_config['db_password']]),
                    '='.join(['port', str(irods_config.database_config['db_port'])]),
                    '='.join(['host', irods_config.database_config['db_host']])
                ]).encode())

                f.flush()

                with open(os.path.join(irods_config.irods_directory, 'packaging', 'sql', 'mysql_functions.sql'), 'r') as sql_file:
                    lib.execute_command(
                        ['mysql', '='.join(['--defaults-file', f.name]), irods_config.database_config['db_name']],
                        stdin=sql_file)

        # Add a new column that will be responsible for holding the fractional part of the modify_ts
        # for a resource. The value stored will be in milliseconds.
        if irods_config.catalog_database_type == 'oracle':
            database_connect.execute_sql_statement(cursor, "alter table R_RESC_MAIN add (modify_ts_millis varchar(3) default '000');")
        else:
            database_connect.execute_sql_statement(cursor, "alter table R_RESC_MAIN add column modify_ts_millis varchar(3) default '000';")

        # Get the pam_password configurations from the server_config and populate the R_GRID_CONFIGURATION table with
        # the new configuration options. This is required because set_grid_configuration only updates rows, which means
        # that the rows must already exist in order to be configurable through the API.
        server_config = lib.open_and_load_json(irods_config.server_config_path)
        auth_config = server_config.get('plugin_configuration').get('authentication')

        # 4.3.0 uses 'pam_password'; all other versions use 'pam'; empty dict will result in default values.
        pam_password_config = auth_config.get('pam', auth_config.get('pam_password', {}))

        password_config_dict = {
            # Removing negative language from the option name results in the meaning being reversed.
            'password_extend_lifetime': '0' if True == pam_password_config.get('no_extend', False) else '1',
            'password_min_time': str(pam_password_config.get('password_min_time', 121)),
            'password_max_time': str(pam_password_config.get('password_max_time', 1209600))
        }

        scheme_namespaces = ['authentication']
        statement_str = "insert into R_GRID_CONFIGURATION (namespace, option_name, option_value) values ('{}','{}','{}');"
        # pam_password configurations for password lifetime have always been used with native authentication as well.
        # The new configurations shall continue to configure both schemes, but under a more generic namespace.
        for scheme in scheme_namespaces:
            for option in password_config_dict:
                database_connect.execute_sql_statement(cursor, statement_str.format(scheme, option, password_config_dict[option]))

    else:
        raise IrodsError('Upgrade to schema version %d is unsupported.' % (new_schema_version))

    database_connect.execute_sql_statement(cursor, "update R_GRID_CONFIGURATION set option_value = ? where namespace = 'database' and option_name = 'schema_version';", new_schema_version)
