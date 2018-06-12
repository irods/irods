import argparse
import contextlib

import irods.lib
from irods import database_connect, database_interface
from irods.configuration import IrodsConfig

#--------------------------------------
# update_deprecated_database_columns.py
#
# Several columns in R_DATA_MAIN are obsolete/no longer updated as of 4.2:
#     resc_name
#     resc_hier
#     resc_group_name
#
# Entries made before 4.2 (resc_name, resc_hier) and before 4.0 (resc_group_name) have potentially stale information stored in these columns.
# Entries made after 4.2 and before 4.2.4 populate the columns inconsistently (or not at all, as the case may be).
# This script will populate these columns for all entries in R_DATA_MAIN with known values to indicate their obsoletion.
#
# This script is only useful for catalogs existent in or upgrading from a pre-4.2.4 installation.
# Remember to back up the database before running the script.
# NOTE: This script only supports the 3 database plugins available in versions <4.2.4: postgreSQL, MySQL, and Oracle.
#--------------------------------------

# Gets the count of rows which meet update condition and valid resc_id
def get_scrubbable_row_count(cursor, select_resc_ids, update_condition):
    select_scrubbable_count = 'SELECT COUNT(*) AS "count" FROM R_DATA_MAIN WHERE (({update_condition}) AND resc_id IN ({select_resc_ids}));'.format(**locals())
    return int(database_connect.execute_sql_statement(cursor, select_scrubbable_count).fetchone()['count'])

# Runs provided update statement and returns number of rows updated
def update_rows(connection, cursor, update_statement):
    rows_updated_count = 0
    try:
        sql_rows_affected = database_connect.execute_sql_statement(cursor, update_statement).rowcount
        connection.commit()
        rows_updated_count = sql_rows_affected
    except (KeyboardInterrupt, SystemExit):
        print('\nWrite process interrupted. Rolling back any pending changes.')
        connection.rollback()
    except:
        print('\nError occurred. Rolling back any pending changes.')
        connection.rollback()
        raise
    return rows_updated_count

# Updates rows in batches and reports change count after operation completes
def scrub_rows(connection, batch_size, select_resc_ids, update_columns, update_condition):
    with contextlib.closing(connection.cursor()) as cursor:
        initial_scrubbable_count = get_scrubbable_row_count(cursor, select_resc_ids, update_condition)
        try:
            if 0 == initial_scrubbable_count:
                print('No rows will be updated. Exiting...')
                return
            print('Rows to update: {}'.format(initial_scrubbable_count))
            print('Batch size: {}'.format(batch_size))
            user_input = irods.lib.default_prompt('Would you like to continue?', default=['No'])
            if 'y' != user_input.lower() and 'yes' != user_input.lower():
                print('User declined. Exiting...')
                return
            rows_updated = 0
            # Generate SQL for updating rows
            column_assignments = ','.join(["{key} = '{val}'".format(key=key, val=update_columns[key]) for key in update_columns.keys()])
            db_type = database_interface.get_database_type()
            if db_type == "oracle":
                select_data_ids = 'SELECT data_id FROM R_DATA_MAIN WHERE (({update_condition}) AND resc_id IN ({select_resc_ids}) AND ROWNUM <= {batch_size})'.format(**locals())
            else:
                select_data_ids = 'SELECT data_id FROM R_DATA_MAIN WHERE (({update_condition}) AND resc_id IN ({select_resc_ids})) LIMIT {batch_size}'.format(**locals())
            update_statement = 'UPDATE R_DATA_MAIN SET {column_assignments} WHERE data_id IN ({select_data_ids});'.format(column_assignments=column_assignments, select_data_ids=select_data_ids)
            while True:
                rows_updated = update_rows(connection, cursor, update_statement)
                if 0 == rows_updated:
                    break
        except (KeyboardInterrupt, SystemExit):
            print('\nExiting...')
        remaining_scrubbable_rows = get_scrubbable_row_count(cursor, select_resc_ids, update_condition)
        total_rows_updated_count = int(initial_scrubbable_count - remaining_scrubbable_rows)
        print('Total rows updated: {}'.format(total_rows_updated_count))
        print('Remaining rows to update: {}'.format(remaining_scrubbable_rows))

# Prints information about rows that need updating, and rows which are safe to update
def dry_run(connection, select_resc_ids, update_condition):
    with contextlib.closing(connection.cursor()) as cursor:
        scrubbable_count = 0
        select_update_count = 'SELECT COUNT(*) AS "count" FROM R_DATA_MAIN WHERE {update_condition};'.format(**locals())
        total_update_count = int(database_connect.execute_sql_statement(cursor, select_update_count).fetchone()['count'])
        print('Total rows in need of update: {total_update_count}'.format(**locals()))
        if total_update_count > 0:
            scrubbable_count = get_scrubbable_row_count(cursor, select_resc_ids, update_condition)
            print('Rows that are safe to update: {scrubbable_count}'.format(**locals()))
            if (total_update_count - scrubbable_count) > 0:
                print('Rows with invalid resc_id: {}\n'
                      'Invalid resc_ids reference a coordinating resource or a non-existent resource.\n'
                      'These rows will not be updated until their resc_ids reference a non-coordinating resource.'.format(int(total_update_count - scrubbable_count)))
        return scrubbable_count

def scrub_main():
    # Condition states that one of these columns does not have its associated value
    update_columns = {'resc_name': 'EMPTY_RESC_NAME',
                      'resc_hier': 'EMPTY_RESC_HIER',
                      'resc_group_name': 'EMPTY_RESC_GROUP_NAME'}
    update_condition = ' or '.join(["({key} != '{val}' or {key} is NULL)".format(key=key, val=update_columns[key]) for key in update_columns.keys()])

    # Selects resc_ids of known non-coordinating resources
    coordinating_resources = ['compound', 'deferred', 'load_balanced', 'random', 'replication', 'roundrobin', 'passthru']
    select_resc_ids = 'SELECT resc_id FROM R_RESC_MAIN WHERE resc_type_name NOT IN ({})'.format(','.join(["'{}'".format(resc) for resc in coordinating_resources]))

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=
'''
Replaces data in now unused columns of R_DATA_MAIN with the following values:
{}

WARNING: This script directly overwrites data in the columns listed above, making
resc_id the only column to hold the location of the replica. Make sure catalog
information is backed up before proceeding. The resc_id column must be valid for a
row to be updated. This script must be run on the catalog provider.
'''.format('\n'.join(['\t{}\t\t{}'.format(key, update_columns[key]) for key in update_columns.keys()])))
    parser.add_argument('-d', '--dry-run', action='store_true', dest='dry_run', help='Count rows to be overwritten (no changes made to database)')
    parser.add_argument('-b', '--batch-size', action='store', dest='batch_size', type=int, default=500, help='Number of records to update per database commit (default: 500)')
    args = parser.parse_args()

    try:
        with contextlib.closing(database_connect.get_database_connection(IrodsConfig())) as connection:
            connection.autocommit = False
            if args.dry_run:
                dry_run(connection, select_resc_ids, update_condition)
            else:
                scrub_rows(connection, args.batch_size, select_resc_ids, update_columns, update_condition)
    except (TypeError):
        print('Failed getting database connection. Note: This script should be run on the iRODS catalog provider.')

if __name__ == "__main__":
    scrub_main()
