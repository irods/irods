from . import database_connect
from .exceptions import IrodsError, IrodsWarning

import logging
import re

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
            database_connect.execute_sql_statement(cursor, "update R_DATA_MAIN set resc_id=? where resc_hier=? or resc_hier like ?", resc_id, resc_name, ''.join(['%;', resc_name]))
        if irods_config.catalog_database_type == 'postgres':
            database_connect.execute_sql_statement(cursor, "update r_resc_main as rdm set resc_parent = am.resc_id from ( select resc_name, resc_id from r_resc_main ) as am where am.resc_name = rdm.resc_parent;")
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

    else:
        raise IrodsError('Upgrade to schema version %d is unsupported.' % (new_schema_version))

    database_connect.execute_sql_statement(cursor, "update R_GRID_CONFIGURATION set option_value = ? where namespace = 'database' and option_name = 'schema_version';", new_schema_version)
