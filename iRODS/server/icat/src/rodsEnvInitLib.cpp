/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#ifdef RODS_RCAT

/*
 These routines are currently unused, but may come in handy
 (they are based on some post-1.1.8 SDSC SRB MCAT code).

 If we do make use of these, be sure to 1) change the sprintf's to
 snprintf's (perhaps with a wrapper routine to also call rodsLog if
 there's an error) and 2) add some comments briefly describing what
 the routine does (and more as needed).
 */

#include "rcatC_db_externs.hpp"
#include "rodsC_rcat_externs.hpp"
#include "icatGlobalsExtern.hpp"
#include "rodsLog.hpp"
#include "rods.hpp"
#include <string.h>

#include <string>

int debugging4 = 0;

removeblanks( char *Str ) {
    int i, j, k;
    k = 0;
    j = strlen( Str );
    for ( i = 0; i < j ; i++ ) {
        if ( Str[i] != ' ' ) {
            Str[k++] = Str[i];
        }
    }
    Str[k] = '\0';
}

int
splitListString( char *List, char Array[][MAX_TOKEN], char Key ) {
    int i, j, k, ln,  b;

    ln = strlen( List );
    j = 0;
    b = 1;
    k = 0;
    for ( i = 0; i < ln ; i++ ) {
        if ( b == 1 && List[i] == ' ' ) {
            continue;
        }
        b = 0;
        if ( List[i] == ',' ) {
            Array[j][k] = '\0';
            b = 1;
            j++;
            k = 0;
        }
        else {
            Array[j][k] = List[i];
            k++;
        }
    }
    Array[j][k] = '\0';
    return( j + 1 );
}

int
getFkrelEntrySet( char *schemaset,
                  rcat_metadata_fkrel_entry *fkrel_info,
                  int max_fkrel_set_size )


{
    int i, j, max_fkrel_entries;
    char tmpStr[1000];
    /* filling RCOREmetadata_fkrel structure */
    sprintf( sqlq, "select t0.* from %sRCOREFK_RELATIONS t0 where t0.schema_name_in in (%s) and t0.schema_name_out in (%s) ",
             RCATSCHEMENAME, schemaset, schemaset );

    i = exec_sql_db( henv, hdbc,  &hstmt, ( unsigned char * ) sqlq );
    if ( i != 0 ) {
        return ( i );
    }
    i = get_no_of_columns_db( hstmt, &num_of_cols );
    if ( i < 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = bind_default_row_db( hstmt, data,  data_size, &num_of_cols );
    if ( i != 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = get_row_db( hstmt );
    if ( i != 0 ) {
        done_with_default_result_db( hstmt, data, data_size, num_of_cols );
        return ( i );
    }
    max_fkrel_entries = 0;
    while ( i == 0 ) {

        if ( max_fkrel_entries ==  max_fkrel_set_size ) {
            done_with_default_result_db( hstmt, data, data_size, num_of_cols );
            return ( MCAT_MAX_FKREL_ENTRIES_OVERFLOW );
        }
        fkrel_info[max_fkrel_entries].tab1 =  atoi( ( char * ) data[0] );
        fkrel_info[max_fkrel_entries].tab2 =  atoi( ( char * ) data[2] );
        fkrel_info[max_fkrel_entries].fg1 = 0;
        fkrel_info[max_fkrel_entries].fg2 = 0;
        fkrel_info[max_fkrel_entries].fg3 = 0;
        truncate_end_blanks( data_size[20], data[20] );
        truncate_end_blanks( data_size[21], data[21] );
        truncate_end_blanks( data_size[22], data[22] );
        strcpy( tmpStr, " " );
        if ( strlen( ( char * ) data[20] ) != 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], "%s(", data[20] );
        }
        sprintf( &tmpStr[strlen( tmpStr )], "%i", atoi( ( char * ) data[1] ) );
        if ( atoi( ( char * )data[5] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[5] ) );
        }
        if ( atoi( ( char * )data[9] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[9] ) );
        }
        if ( atoi( ( char * )data[13] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[13] ) );
        }
        if ( atoi( ( char * )data[17] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[17] ) );
        }
        if ( strlen( ( char * )data[20] ) != 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ")" );
        }
        sprintf( &tmpStr[strlen( tmpStr )], " %s ", data[22] );
        if ( strlen( ( char * )data[21] ) != 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], "%s(", data[21] );
        }
        sprintf( &tmpStr[strlen( tmpStr )], "%i", atoi( ( char * ) data[3] ) );
        if ( atoi( ( char * )data[7] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[7] ) );
        }
        if ( atoi( ( char * )data[11] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[11] ) );
        }
        if ( atoi( ( char * )data[15] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[15] ) );
        }
        if ( atoi( ( char * )data[19] ) >= 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ",%i", atoi( ( char * ) data[19] ) );
        }
        if ( strlen( ( char * )data[21] ) != 0 ) {
            sprintf( &tmpStr[strlen( tmpStr )], ") " );
        }
        fkrel_info[max_fkrel_entries].pred =
            ( char * ) malloc( ( strlen( tmpStr ) + 1 ) * sizeof( char ) );
        strcpy( fkrel_info[max_fkrel_entries].pred, tmpStr );

        max_fkrel_entries++;
        i = get_row_db( hstmt );

    }
    i = done_with_default_result_db( hstmt, data, data_size, num_of_cols );
    return( RCAT_SUCCESS );
}


int
getGraphPathEntrySet( char *schemaset,
                      rcat_metadata_graph_entry *graph_info,
                      int max_graph_set_size )

{
    int i, j, max_graph_entries;
    char tmpStr[1000];
    /* filling rcat_metadata_graph_path structure */
    sprintf( sqlq, "select t1.table_name, t2.table_name, t3.attr_name, t4.attr_name, t5.attr_name, t6.attr_name, t7.attr_name, t8.attr_name, t9.attr_name, t10.attr_name, t11.attr_name, t12.attr_name, t0.func_in, t0.func_out,t0.attr_rel_name, t1.dbschema_name, t2.dbschema_name , t1.rsrc_id, t2.rsrc_id from %sRCOREFK_RELATIONS t0, %sRCOREtables t1, %srcore_tables t2, %srcore_attributes t3, %srcore_attributes t4, %srcore_attributes t5, %srcore_attributes t6, %srcore_attributes t7, %srcore_attributes t8, %srcore_attributes t9, %srcore_attributes t10, %srcore_attributes t11, %srcore_attributes t12 where t0.table_id_in1 =t1.table_id and t0.table_id_out1 =t2.table_id and t0.attr_id_in1 = t3.attr_id and t0.attr_id_in2 = t4.attr_id and t0.attr_id_in3 = t5.attr_id and t0.attr_id_in4 = t6.attr_id and t0.attr_id_in5 = t7.attr_id and t0.attr_id_out1 = t8.attr_id and t0.attr_id_out2 = t9.attr_id and t0.attr_id_out3 = t10.attr_id and t0.attr_id_out4 = t11.attr_id and t0.attr_id_out5 = t12.attr_id and t0.schema_name_in in (%s)  and t0.schema_name_out in (%s) ",
             RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME,
             RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME,
             RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME, schemaset, schemaset );

    i = exec_sql_db( henv, hdbc,  &hstmt, ( unsigned char * ) sqlq );
    if ( i != 0 ) {
        return ( i );
    }
    i = get_no_of_columns_db( hstmt, &num_of_cols );
    if ( i < 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = bind_default_row_db( hstmt, data,  data_size, &num_of_cols );
    if ( i != 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = get_row_db( hstmt );
    if ( ( i != 0 ) && ( i != MCAT_NO_DATA_FOUND ) ) {
        done_with_default_result_db( hstmt, data, data_size, num_of_cols );
        return ( i );
    }

    max_graph_entries = 0;
    while ( i == 0 ) {
        if ( max_graph_entries == max_graph_set_size ) {
            done_with_default_result_db( hstmt, data, data_size, num_of_cols );
            return ( MCAT_MAX_GRAPH_PATH_OVERFLOW );
        }
        for ( j = 0; j < num_of_cols; j++ ) {
            truncate_end_blanks( data_size[j], data[j] );
        }
        sprintf( graph_info[max_graph_entries].name1, "%s%s",
                 ( char * )data[15], ( char * )data[0] );

        sprintf( graph_info[max_graph_entries].name2, "%s%s",
                 ( char * )data[16], ( char * )data[1] );
        graph_info[max_graph_entries].rsrc_id1 = atoi( ( char * )data[17] );
        graph_info[max_graph_entries].rsrc_id2 = atoi( ( char * )data[18] );
        strcpy( graph_info[max_graph_entries].relop, ( char * )data[14] );

        graph_info[max_graph_entries].fg1 = 0;
        graph_info[max_graph_entries].fg2 = 0;
        graph_info[max_graph_entries].fg3 = 0;

        strcpy( tmpStr, "" );
        if ( strlen( ( char * ) data[12] ) != 0 || strcmp( ( char * ) data[3], "$NULL" ) ) {
            sprintf( &tmpStr[strlen( tmpStr )], "%s(", ( char * )data[12] );
        }
        sprintf( &tmpStr[strlen( tmpStr )], "%s", ( char * )data[2] );
        for ( j = 3; j < 7; j++ ) {
            if ( strcmp( ( char * ) data[j], "$NULL" ) ) {
                sprintf( &tmpStr[strlen( tmpStr )], ",%s", ( char * )data[j] );
            }
        }
        if ( strlen( ( char * ) data[12] ) != 0 || strcmp( ( char * ) data[3], "$NULL" ) ) {
            sprintf( &tmpStr[strlen( tmpStr )], ")" );
        }
        strcpy( graph_info[max_graph_entries].att1, tmpStr );
        strcpy( tmpStr, "" );
        if ( strlen( ( char * ) data[13] ) != 0 || strcmp( ( char * ) data[8], "$NULL" ) ) {
            sprintf( &tmpStr[strlen( tmpStr )], "%s(", ( char * )data[13] );
        }
        sprintf( &tmpStr[strlen( tmpStr )], "%s" , ( char * )data[7] );
        for ( j = 8; j < 12; j++ ) {
            if ( strcmp( ( char * ) data[j], "$NULL" ) ) {
                sprintf( &tmpStr[strlen( tmpStr )], ",%s", ( char * )data[j] );
            }
        }
        if ( strlen( ( char * ) data[13] ) != 0 || strcmp( ( char * ) data[8], "$NULL" ) ) {
            sprintf( &tmpStr[strlen( tmpStr )], ")" );
        }
        strcpy( graph_info[max_graph_entries].att2, tmpStr );
        i = get_row_db( hstmt );
        max_graph_entries++;

    }
    i = done_with_default_result_db( hstmt, data, data_size, num_of_cols );
    if ( max_graph_entries != max_graph_set_size ) {
        strcpy( GENERATEDSQL, sqlq );
        printf( "SQL:%s\n<br>", sqlq );
        printf( "A:%i:%i\n",      max_graph_entries, max_graph_set_size );
        return( MCAT_MAX_GRAPH_PATH_UNDERFLOW );
    }
    else {
        return( RCAT_SUCCESS );
    }
}


int
getAttrEntrySet( char *schemaset,
                 rcat_metadata_attr_entry *attr_info,
                 int max_attr_set_size, char *known ) {
    int i;
    int max_entries;

    /* filling rcat_metadata_attr structure */

    if ( !strcmp( known, "all" ) ) {
        sprintf( sqlq, "select t0.attr_id, schema_name, dbschema_name, database_name, table_name, attr_name, external_attr_name, attr_data_type, attr_iden_type, attr_val_type, t0.table_id,  presentation, default_value, t0.sameas, t0.maxsize, t0.primary_key, t0.expose, t0.exter_attrib_id, rsrc_id,  attr_dimension, at_comments, cluster_name from %srcore_attributes t0,  %srcore_tables t1, %srcore_uschema_attr t2 where t0.table_id = t1.table_id and t0.attr_id = t2.attr_id and t2.user_schema_name in (%s) and t0.expose >= 0 order by 1",
                 RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME, schemaset );
    }
    else {
        sprintf( sqlq, "select t0.attr_id, schema_name, dbschema_name, database_name, table_name, attr_name, external_attr_name, attr_data_type, attr_iden_type, attr_val_type, t0.table_id,  presentation,   default_value, t0.sameas, t0.maxsize, t0.primary_key, t0.expose, t0.exter_attrib_id, rsrc_id, attr_dimension, at_comments, cluster_name  from %srcore_attributes t0,  %srcore_tables t1, %srcore_uschema_attr t2 where t0.table_id = t1.table_id and t0.attr_id = t2.attr_id and t2.user_schema_name in (%s) and t0.expose = 1 order by 1",
                 RCATSCHEMENAME, RCATSCHEMENAME, RCATSCHEMENAME, schemaset );
    }



    i = exec_sql_db( henv, hdbc,  &hstmt, ( unsigned char * ) sqlq );
    if ( i != 0 ) {
        return ( i );
    }
    i = get_no_of_columns_db( hstmt, &num_of_cols );
    if ( i < 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = bind_default_row_db( hstmt, data,  data_size, &num_of_cols );
    if ( i != 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = get_row_db( hstmt );
    if ( i != 0 ) {
        done_with_default_result_db( hstmt, data, data_size, num_of_cols );
        return ( i );
    }
    max_entries = 0;
    while ( i == 0 ) {
        if ( max_entries == max_attr_set_size ) {
            done_with_default_result_db( hstmt, data, data_size, num_of_cols );
            return ( MCAT_MAX_ATTR_ENTRIES_OVERFLOW );
        }
        for ( j = 0; j < num_of_cols; j++ ) {
            truncate_end_blanks( data_size[j], data[j] );
        }
        attr_info[max_entries].schema_name =
            ( char * ) malloc( ( strlen( ( char * )data[1] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].dbschema_name =
            ( char * ) malloc( ( strlen( ( char * )data[2] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].database_name =
            ( char * ) malloc( ( strlen( ( char * )data[3] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].table_name =
            ( char * ) malloc( ( strlen( ( char * )data[2] ) + 2 + strlen( ( char * )data[4] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].extended_table_name =
            ( char * ) malloc( ( strlen( ( char * )data[2] ) + 2 + strlen( ( char * )data[4] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].attr_name =
            ( char * ) malloc( ( strlen( ( char * )data[5] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].ext_attr_name =
            ( char * ) malloc( ( strlen( ( char * )data[6] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].attr_data_type =
            ( char * ) malloc( ( strlen( ( char * )data[7] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].attr_iden_type =
            ( char * ) malloc( ( strlen( ( char * )data[8] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].presentation =
            ( char * ) malloc( ( strlen( ( char * )data[11] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].attr_dimension =
            ( char * ) malloc( ( strlen( ( char * )data[19] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].default_value =
            ( char * ) malloc( ( strlen( ( char * )data[12] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].at_comments =
            ( char * ) malloc( ( strlen( ( char * )data[20] ) + 1 ) * sizeof( char ) );
        attr_info[max_entries].cluster_name =
            ( char * ) malloc( ( strlen( ( char * )data[21] ) + 1 ) * sizeof( char ) );

        attr_info[max_entries].attr_id = atoi( ( char * )data[0] );
        strcpy( attr_info[max_entries].schema_name, ( char * )data[1] );
        strcpy( attr_info[max_entries].dbschema_name, ( char * )data[2] );
        strcpy( attr_info[max_entries].database_name, ( char * )data[3] );
        sprintf( attr_info[max_entries].extended_table_name, "%s%s",
                 ( char * )data[2],	( char * )data[4] );
        sprintf( attr_info[max_entries].table_name, "%s", ( char * )data[4] );
        strcpy( attr_info[max_entries].attr_name, ( char * )data[5] );
        strcpy( attr_info[max_entries].ext_attr_name, ( char * )data[6] );
        strcpy( attr_info[max_entries].attr_data_type, ( char * )data[7] );
        strcpy( attr_info[max_entries].attr_iden_type, ( char * )data[8] );
        attr_info[max_entries].attr_val_type = atoi( ( char * )data[9] );
        attr_info[max_entries].tab_id = atoi( ( char * )data[10] );
        strcpy( attr_info[max_entries].presentation, ( char * )data[11] );
        strcpy( attr_info[max_entries].default_value , ( char * )data[12] );
        attr_info[max_entries].sameas = atoi( ( char * )data[13] );
        attr_info[max_entries].maxsize = atoi( ( char * )data[14] );
        attr_info[max_entries].primary_key = atoi( ( char * )data[15] );
        attr_info[max_entries].expose = atoi( ( char * )data[16] );
        attr_info[max_entries].exter_attrib_id = atoi( ( char * )data[17] );
        attr_info[max_entries].rsrc_id = atoi( ( char * )data[18] );
        strcpy( attr_info[max_entries].attr_dimension, ( char * )data[19] );
        strcpy( attr_info[max_entries].at_comments, ( char * )data[20] );
        strcpy( attr_info[max_entries].cluster_name, ( char * )data[21] );

        i = get_row_db( hstmt );
        max_entries++;
    }
    i = done_with_default_result_db( hstmt, data, data_size, num_of_cols );

    if ( max_entries != max_attr_set_size ) {
        return( MCAT_MAX_ATTR_ENTRIES_UNDERFLOW );
    }
    else {
        return( RCAT_SUCCESS );
    }

}

void
printFkrelEntrySet( rcat_metadata_fkrel_entry *fkrel_info,
                    int max_entries ) {
    int i;

    for ( i = 0; i < max_entries ; i++ ) {
        printf( "%i  %i %s\n",
                fkrel_info[i].tab1,
                fkrel_info[i].tab2,
                fkrel_info[i].pred );
    }

}

void
printAttrEntrySet( rcat_metadata_attr_entry *attr_info,
                   int max_entries ) {
    int i;

    for ( i = 0; i < max_entries ; i++ ) {
        printf( "%i %i %s %s %s %s.%s\n %s %s %s %s %s %s  %s %s %i %i %i %i %i %i %i\n",
                attr_info[i].attr_id,
                attr_info[i].tab_id,
                attr_info[i].schema_name,
                attr_info[i].database_name,
                attr_info[i].dbschema_name,
                attr_info[i].table_name,
                attr_info[i].attr_name,
                attr_info[i].ext_attr_name,
                attr_info[i].attr_data_type,
                attr_info[i].attr_iden_type,
                attr_info[i].presentation,
                attr_info[i].attr_dimension,
                attr_info[i].at_comments,
                attr_info[i].cluster_name,
                attr_info[i].default_value,
                attr_info[i].attr_val_type,
                attr_info[i].sameas,
                attr_info[i].maxsize,
                attr_info[i].primary_key,
                attr_info[i].expose,
                attr_info[i].exter_attrib_id,
                attr_info[i].rsrc_id );
    }
}


void
printGraphPathEntrySet( rcat_metadata_graph_entry graph_info[],
                        int max_entries ) {
    int i;

    for ( i = 0; i < max_entries ; i++ ) {
        printf( "%s %s %s %s\n",
                graph_info[i].name1,
                graph_info[i].att1,
                graph_info[i].name2,
                graph_info[i].att2 );
    }

}

int
allocAttrEntrySet( rcat_metadata_attr_entry **attr_info,  int max_entries ) {
    if ( attr_info == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );    // JMC cppcheck - nullptr
    }

    *attr_info = ( rcat_metadata_attr_entry * )
                 malloc( max_entries * sizeof( rcat_metadata_attr_entry ) );
    if ( *attr_info == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );    // JMC cppcheck - nullptr
    }
    else {
        return ( RCAT_SUCCESS );
    }

}


int
allocFkrelEntrySet( rcat_metadata_fkrel_entry **fkrel_info,  int max_entries ) {
    if ( fkrel_info == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );    // JMC cppcheck - nullptr
    }
    *fkrel_info = ( rcat_metadata_fkrel_entry * )
                  malloc( max_entries * sizeof( rcat_metadata_fkrel_entry ) );
    if ( *fkrel_info == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );    // JMC cppcheck - nullptr
    }
    else {
        return ( RCAT_SUCCESS );
    }

}



int
allocGraphPathEntrySet( rcat_metadata_graph_entry **graph_info,
                        int max_entries ) {
    if ( graph_info == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );    // JMC cppcheck - nullptr
    }
    *graph_info = ( rcat_metadata_graph_entry * )
                  malloc( max_entries * sizeof( rcat_metadata_graph_entry ) );
    if ( *graph_info == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );    // JMC cppcheck - nullptr
    }
    else {
        return ( RCAT_SUCCESS );
    }

}



void
freeAttrEntrySet( rcat_metadata_attr_entry attr_info[],
                  int actual_attr_set_size ) {
    int i;
    for ( i = 0; i < actual_attr_set_size ; i ++ ) {
        free( attr_info[i].schema_name );
        free( attr_info[i].dbschema_name );
        free( attr_info[i].database_name );
        free( attr_info[i].table_name );
        free( attr_info[i].extended_table_name );
        free( attr_info[i].attr_name );
        free( attr_info[i].ext_attr_name );
        free( attr_info[i].attr_data_type );
        free( attr_info[i].attr_iden_type );
        free( attr_info[i].presentation );
        free( attr_info[i].attr_dimension );
        free( attr_info[i].at_comments );
        free( attr_info[i].cluster_name );
        free( attr_info[i].default_value );
    }
    free( attr_info );
}
void
freeFkrelEntrySet( rcat_metadata_fkrel_entry fkrel_info[],
                   int actual_fkrel_set_size ) {
    int i;
    for ( i = 0; i < actual_fkrel_set_size ; i ++ ) {
        free( fkrel_info[i].pred );
    }
    free( fkrel_info );
}

void
freeGraphPathEntrySet( rcat_metadata_graph_entry graph_info[],
                       int actual_graph_set_size ) {
    int i;
    /*  for (i = 0; i < actual_graph_set_size ; i ++)
      {
       free(graph_info[i]. );
       free(graph_info[i]. );
       free(graph_info[i]. );
       free(graph_info[i]. );
       free(graph_info[i]. );
       free(graph_info[i]. );
       free(graph_info[i]. );
      }
    */
    free( graph_info );
}

int
getAttrCountinSchemaSet( char *schemaset, char *known ) {
    int i, cnt;
    /*
      sprintf(sqlq, "select count(*) from %srcore_attributes t0,  %srcore_tables t1 where t0.table_id = t1.table_id and t1.schema_name in (%s)",
              RCATSCHEMENAME,RCATSCHEMENAME, schemaset);
    */
    if ( !strcmp( known, "all" ) ) {
        sprintf( sqlq, "select count(*) from %srcore_uschema_attr t0 where  t0.user_schema_name in (%s) and t0.expose >= 0 ", RCATSCHEMENAME, schemaset );
    }
    else {
        sprintf( sqlq, "select count(*) from %srcore_uschema_attr t0 where  t0.user_schema_name in (%s) and t0.expose =  1 ", RCATSCHEMENAME, schemaset );
    }

    i = exec_sql_db( henv, hdbc,  &hstmt, ( unsigned char * ) sqlq );
    if ( i != 0 ) {
        return ( i );
    }
    num_of_cols = 1;
    i = bind_default_row_db( hstmt, data,  data_size, &num_of_cols );
    if ( i != 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = get_row_db( hstmt );
    if ( i != 0 ) {
        done_with_default_result_db( hstmt, data, data_size, num_of_cols );
        return ( i );
    }
    truncate_end_blanks( data_size[0], data[0] );
    cnt = atoi( ( char * )data[0] );
    done_with_default_result_db( hstmt, data, data_size, num_of_cols );
    return ( cnt );
}

int
getFkrelCountinSchemaSet( char *schemaset ) {
    int i, cnt;
    sprintf( sqlq, "select count(*) from %sRCORE_FK_RELATIONS t0 where t0.schema_name_in in (%s) and t0.schema_name_out in (%s) ",
             RCATSCHEMENAME, schemaset, schemaset );

    i = exec_sql_db( henv, hdbc,  &hstmt, ( unsigned char * ) sqlq );
    if ( i != 0 ) {
        return ( i );
    }
    num_of_cols = 1;
    i = bind_default_row_db( hstmt, data,  data_size, &num_of_cols );
    if ( i != 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = get_row_db( hstmt );
    if ( i != 0 ) {
        done_with_default_result_db( hstmt, data, data_size, num_of_cols );
        return ( i );
    }
    truncate_end_blanks( data_size[0], data[0] );
    cnt = atoi( ( char * )data[0] );
    done_with_default_result_db( hstmt, data, data_size, num_of_cols );
    return ( cnt );
}

int
getGraphPathCountinSchemaSet( char *schemaset ) {
    int i, cnt;
    sprintf( sqlq, "select count(*) from %sRCORE_FK_RELATIONS t0 where t0.schema_name_in in (%s) and t0.schema_name_out in (%s) ",
             RCATSCHEMENAME, schemaset, schemaset );

    i = exec_sql_db( henv, hdbc,  &hstmt, ( unsigned char * ) sqlq );
    if ( i != 0 ) {
        return ( i );
    }
    num_of_cols = 1;
    i = bind_default_row_db( hstmt, data,  data_size, &num_of_cols );
    if ( i != 0 ) {
        done_with_result_db( hstmt );
        return ( i );
    }
    i = get_row_db( hstmt );
    if ( i != 0 ) {
        done_with_default_result_db( hstmt, data, data_size, num_of_cols );
        return ( i );
    }
    truncate_end_blanks( data_size[0], data[0] );
    cnt = atoi( ( char * )data[0] );
    done_with_default_result_db( hstmt, data, data_size, num_of_cols );
    return ( cnt );
}


int
getFreeMcatContxtDesc() {
    int i;

    for ( i = 0; i < MAX_NO_OF_MCAT_CONTEXTS ; i++ ) {
        if ( svrMcatContext[i].used == 0 ) {
            return ( i );
        }
    }
    return ( MCAT_NO_MORE_CONTEXTS );

}

int
getParentSchemaSet( char *parschemaset, char *schemaset ) {
    char tmpStr[MAX_TOKEN];
    char sqlq[HUGE_STRING];
    int  colcount, i, jj;
    char  cval[MAX_TOKEN][MAX_TOKEN];

    sprintf( sqlq, "select PARENT_SCHEMA from %sRCORE_USER_SCHEMAS where user_schema_name in (%s)", RCATSCHEMENAME, schemaset );
    i = get_ctype_list_from_query( cval, &colcount, sqlq );
    if ( i != RCAT_SUCCESS ) {
        return( i );
    }
    if ( colcount == 0 ) {
        return ( UNKNOWN_USER_SCHEMASET );
    }
    strcpy( parschemaset, "" );
    for ( jj = 0; jj < colcount; jj++ ) {
        if ( jj == 0 ) {
            sprintf( tmpStr, " '%s'", cval[jj] );
        }
        else {
            sprintf( tmpStr, ", '%s'", cval[jj] );
        }
        strcat( parschemaset, tmpStr );
    }
    return( RCAT_SUCCESS );

}


mcatContextDesc
initializeMcatContext( char *schemaset ) {

    char parschemaset[HUGE_STRING];
    int i;

    i =   getParentSchemaSet( parschemaset, schemaset );

    if ( i != RCAT_SUCCESS ) {
        return( i );
    }
    return( initializeMcatContextInternal( parschemaset, schemaset ) );
}


mcatContextDesc
initializeMcatContextInternal( char *schemaset, char *userschemaset ) {

    int i, cnta, cntf, cntg, cd;
    rcat_metadata_attr_entry *attr_info;
    rcat_metadata_fkrel_entry *fkrel_info;
    rcat_metadata_graph_entry *graph_info;

    cnta = getAttrCountinSchemaSet( schemaset, "all" );
    if ( cnta < 0 ) {
        return ( cnta );
    }
    i = allocAttrEntrySet( &attr_info,  cnta );
    if ( i < 0 )  {
        free( attr_info ); // JMC cppcheck - leak
        return ( i );
    }
    i = getAttrEntrySet( schemaset, attr_info, cnta, "all" );
    if ( i < 0 ) {
        freeAttrEntrySet( attr_info, cnta );
        return ( i );
    }
    /*****
    cntf = getFkrelCountinSchemaSet(schemaset);
    if (cntf < 0) return (cntf);
    i = allocFkrelEntrySet(&fkrel_info,  cntf);
    if (i < 0)  return (i);
    i = getFkrelEntrySet(schemaset, fkrel_info, cntf);
    if (i < 0) {
      freeFkrelEntrySet(fkrel_info, cntf);
      freeAttrEntrySet(attr_info, cnta);
      return (i);
    }
    ****/
    cntg = getGraphPathCountinSchemaSet( schemaset );
    if ( cntg < 0 ) {
        free( attr_info );    // JMC cppcheck - leak
        return ( cntg );
    }
    i = allocGraphPathEntrySet( &graph_info,  cntg );
    if ( i < 0 )  {
        free( attr_info );    // JMC cppcheck - leak
        free( graph_info );
        return ( i );
    }
    i = getGraphPathEntrySet( schemaset, graph_info, cntg );
    if ( i < 0 ) {
        freeGraphPathEntrySet( graph_info, cntg );
        freeAttrEntrySet( attr_info, cnta );
        return ( i );
    }

    cd = getFreeMcatContxtDesc();
    if ( cd < 0 ) {
        free( attr_info );    // JMC cppcheck - leak
        free( graph_info );
        return( cd );
    }

    svrMcatContext[cd].userContext = ( char * )
                                     malloc( sizeof( char ) * ( strlen( userschemaset ) + 1 ) );
    if ( svrMcatContext[cd].userContext == NULL ) {
        freeGraphPathEntrySet( graph_info, cntg );
        freeAttrEntrySet( attr_info, cnta );
        return ( MEMORY_ALLOCATION_ERROR );
    }
    svrMcatContext[cd].serverContext = ( char * )
                                       malloc( sizeof( char ) * ( strlen( schemaset ) + 1 ) );
    if ( svrMcatContext[cd].serverContext == NULL ) {
        freeGraphPathEntrySet( graph_info, cntg );
        freeAttrEntrySet( attr_info, cnta );
        free( svrMcatContext[cd].userContext );
        return ( MEMORY_ALLOCATION_ERROR );
    }



    svrMcatContext[cd].used = 1;
    svrMcatContext[cd].attr_info = attr_info;
    svrMcatContext[cd].max_attr_entries = cnta;
    /*****
    svrMcatContext[cd].fkrel_info = fkrel_info;
    svrMcatContext[cd].max_fkrel_entries = cntf;
    *****/
    svrMcatContext[cd].graph_info = graph_info;
    svrMcatContext[cd].max_graph_entries = cntg;
    strcpy( svrMcatContext[cd].userContext, userschemaset );
    strcpy( svrMcatContext[cd].serverContext, schemaset );

    return ( cd );
}

void
freeMcatContext( mcatContextDesc cd ) {
    freeAttrEntrySet( svrMcatContext[cd].attr_info,
                      svrMcatContext[cd].max_attr_entries );
    freeFkrelEntrySet( svrMcatContext[cd].fkrel_info,
                       svrMcatContext[cd].max_fkrel_entries );
    freeGraphPathEntrySet( svrMcatContext[cd].graph_info,
                           svrMcatContext[cd].max_graph_entries );
    free( svrMcatContext[cd].userContext );
    free( svrMcatContext[cd].serverContext );
    svrMcatContext[cd].used = 0;


}



struct varlena *
getAttrEntrySetSmallfromContext( mcatContextDesc cd ) {
    /*attr_id,schema_name, ext_attr_name */

    int resLen = SIZEOF32;  /* The length required for max_entries*/
    int i, status;
    char *ptr;          /* a temp char pointer */
    struct varlena *retVal;     /* the return pointer */
#if defined(PORTNAME_c90)
    struct int32Array *iptr;
#endif

    resLen = 0;
    for ( i = 0; i < svrMcatContext[cd].max_attr_entries ; i++ ) {
        resLen += SIZEOF32;  /* The length required  to attr_id */
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].schema_name ) + 1 );
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].ext_attr_name ) + 1 );
    }

    retVal = ( struct varlena * )malloc( VAROUTHDRSZ + resLen );
    VARSIZE( retVal ) = resLen + VAROUTHDRSZ;
    VAROUTSTAT( retVal ) = 0;           /* status */
    ptr = VAROUTDATA( retVal );         /* the beginning of the data */
#if defined(PORTNAME_c90)
    iptr = ( struct int32Array * ) VAROUTDATA( retVal );
#endif

    /* Now fill in the data */
    ptr = VARDATA( retVal );    /* the beginning of the data section */

#if defined(PORTNAME_c90)
    iptr->myInt0 = htonl( svrMcatContext[cd].max_attr_entries );
    ptr += SIZEOF32;
#else
    *( int32 * ) ptr = htonl( svrMcatContext[cd].max_attr_entries );
    ptr += SIZEOF32;
#endif



    for ( i = 0; i < svrMcatContext[cd].max_attr_entries ; i =  i + 2 ) {
#if defined(PORTNAME_c90)
        iptr->myInt1 = htonl( svrMcatContext[cd].attr_info[i].attr_id );
        ptr += SIZEOF32;
        iptr = ( struct int32Array * ) ptr;
        if ( i + 1 <  svrMcatContext[cd].max_attr_entries ) {
            iptr->myInt0 = htonl( svrMcatContext[cd].attr_info[i + 1].attr_id );
            ptr += SIZEOF32;
        }
#else
        *( int32 * ) ptr = htonl( svrMcatContext[cd].attr_info[i].attr_id );
        ptr += sizeof( int32 );
        if ( i + 1 <  svrMcatContext[cd].max_attr_entries ) {
            *( int32 * ) ptr = htonl( svrMcatContext[cd].attr_info[i + 1].attr_id );
            ptr += sizeof( int32 );
        }
#endif
    }
    for ( i = 0; i < svrMcatContext[cd].max_attr_entries ; i++ ) {
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].schema_name );
        ptr += strlen( svrMcatContext[cd].attr_info[i].schema_name );
        *ptr = '\0';
        ptr ++;
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].ext_attr_name );
        ptr += strlen( svrMcatContext[cd].attr_info[i].ext_attr_name );
        *ptr = '\0';
        ptr ++;
    }
    return retVal;
}


struct varlena *
getAttrEntrySetLargefromContext( mcatContextDesc cd ) {
    /*attr_id,schema_name, ext_attr_name ext_attr_name,attr_data_type,
      presentation, attr_dimension, default_value*/

    int resLen = SIZEOF32;  /* The length required for max_entries*/
    int i, status;
    char *ptr;          /* a temp char pointer */
    struct varlena *retVal;     /* the return pointer */
#if defined(PORTNAME_c90)
    struct int32Array *iptr;
#endif

    resLen = 0;
    for ( i = 0; i < svrMcatContext[cd].max_attr_entries ; i++ ) {
        resLen += SIZEOF32;  /* The length required  to attr_id */
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].schema_name ) + 1 );
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].ext_attr_name ) + 1 );
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].attr_data_type ) + 1 );
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].presentation ) + 1 );
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].attr_dimension ) + 1 );
        resLen += ( strlen( svrMcatContext[cd].attr_info[i].default_value ) + 1 );

    }

    retVal = ( struct varlena * )malloc( VAROUTHDRSZ + resLen );
    VARSIZE( retVal ) = resLen + VAROUTHDRSZ;
    VAROUTSTAT( retVal ) = 0;           /* status */
    ptr = VAROUTDATA( retVal );         /* the beginning of the data */
#if defined(PORTNAME_c90)
    iptr = ( struct int32Array * ) VAROUTDATA( retVal );
#endif

    /* Now fill in the data */
    ptr = VARDATA( retVal );    /* the beginning of the data section */

#if defined(PORTNAME_c90)
    iptr->myInt0 = htonl( svrMcatContext[cd].max_attr_entries );
    ptr += SIZEOF32;
#else
    *( int32 * ) ptr = htonl( svrMcatContext[cd].max_attr_entries );
    ptr += SIZEOF32;
#endif



    for ( i = 0; i < svrMcatContext[cd].max_attr_entries ; i =  i + 2 ) {
#if defined(PORTNAME_c90)
        iptr->myInt1 = htonl( svrMcatContext[cd].attr_info[i].attr_id );
        ptr += SIZEOF32;
        iptr = ( struct int32Array * ) ptr;
        if ( i + 1 <  svrMcatContext[cd].max_attr_entries ) {
            iptr->myInt0 = htonl( svrMcatContext[cd].attr_info[i + 1].attr_id );
            ptr += SIZEOF32;
        }
#else
        *( int32 * ) ptr = htonl( svrMcatContext[cd].attr_info[i].attr_id );
        ptr += sizeof( int32 );
        if ( i + 1 <  svrMcatContext[cd].max_attr_entries ) {
            *( int32 * ) ptr = htonl( svrMcatContext[cd].attr_info[i + 1].attr_id );
            ptr += sizeof( int32 );
        }
#endif
    }


    for ( i = 0; i < svrMcatContext[cd].max_attr_entries ; i++ ) {
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].schema_name );
        ptr += strlen( svrMcatContext[cd].attr_info[i].schema_name );
        *ptr = '\0';
        ptr ++;
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].ext_attr_name );
        ptr += strlen( svrMcatContext[cd].attr_info[i].ext_attr_name );
        *ptr = '\0';
        ptr ++;
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].attr_data_type );
        ptr += strlen( svrMcatContext[cd].attr_info[i].attr_data_type );
        *ptr = '\0';
        ptr ++;
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].presentation );
        ptr += strlen( svrMcatContext[cd].attr_info[i].presentation );
        *ptr = '\0';
        ptr ++;
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].attr_dimension );
        ptr += strlen( svrMcatContext[cd].attr_info[i].attr_dimension );
        *ptr = '\0';
        ptr ++;
        strcpy( ptr,  svrMcatContext[cd].attr_info[i].default_value );
        ptr += strlen( svrMcatContext[cd].attr_info[i].default_value );
        *ptr = '\0';
        ptr ++;
    }
    return retVal;
}

int
updateInSchemaByUser( int  cd,
                      int     *attrId,
                      char    **attrValue,
                      int     attrCnt,
                      int    *qattrval,
                      char   **qval,
                      int      qcount,
                      char *userName, char *userDomain ) {
    int i, user_id;

    user_id = get_user_id_in_domain( userName, userDomain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    i = updateInSchema( cd, attrId, attrValue, attrCnt,
                        qattrval, qval, qcount, user_id );
    return( i );
}


int
updateInSchema( int  cd,
                int     *attrId,
                char    **attrValue,
                int     attrCnt,
                int    *qattrval,
                char   **qval,
                int      qcount,
                int               userId ) {
    return( OPERATION_NOT_IMPLEMENTED );
}

int deleteFromClusterByUser( int  cd,
                             char   *clusterName,
                             int   *qattrval,
                             char  **qval,
                             int   qcount,
                             char *userName, char *userDomain, char *delDimen[] ) {
    int i, user_id;

    user_id = get_user_id_in_domain( userName, userDomain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    i = deleteFromCluster( cd, clusterName,
                           qattrval, qval, qcount, user_id, delDimen );
    return( i );
}
int deleteFromCluster( int  cd,
                       char   *clusterName,
                       int   *qattrval,
                       char  **qval,
                       int   qcount,
                       int     userId,
                       char  *delDimen[] ) {
    return( OPERATION_NOT_IMPLEMENTED );
}
int
updateAttrValueByUser( int cd, char *userName, char *userDomain,
                       int   *attr_id,
                       char **attr_value,
                       int    attr_cnt,
                       char *updDimen[] ) {
    int i, user_id;

    user_id = get_user_id_in_domain( userName, userDomain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    i = updateAttrValue( cd,
                         attr_id, attr_value, attr_cnt, user_id, updDimen );
    return( i );
}

int
updateAttrValue( int cd,
                 int   *attr_id,
                 char **attr_value,
                 int    attr_cnt, int    real_user_id,
                 char *updDimen[] ) {
    int i, ii, j, k;
    char **attr_name;
    char **table_name;
    int  *in_rsrcid;
    int    tabcnt;
    int    tabflag;
    char  qs[HUGE_STRING];
    char *iisAttrVal[MAX_QUERY_LIMIT];

    strcpy( qs, "" );


    attr_name = malloc( sizeof( char * ) * attr_cnt );
    table_name = malloc( sizeof( char * ) * attr_cnt );
    in_rsrcid = malloc( sizeof( int )  * attr_cnt );

    i = checkAccessMetaDataAttr( real_user_id, attr_id, attr_cnt );
    if ( i != 0 ) {
        free( in_rsrcid );    // JMC cppcheck - leak
        return( i );
    }
    tabcnt = 0;
    for ( i = 0; i < attr_cnt; i++ )  {
        for ( j = 0; j < svrMcatContext[cd].max_attr_entries; j++ ) {
            if ( svrMcatContext[cd].attr_info[j].attr_id == attr_id[i] ) {
                attr_name[i] = svrMcatContext[cd].attr_info[j].attr_name;
                tabflag = 0;
                for ( k = 0 ; k < tabcnt; k++ ) {
                    if ( table_name[k] == svrMcatContext[cd].attr_info[j].extended_table_name ) {
                        tabflag = 1;
                        break;
                    }
                }
                if ( tabflag == 0 ) {
                    table_name[tabcnt] = svrMcatContext[cd].attr_info[j].extended_table_name;
                    in_rsrcid[tabcnt] = svrMcatContext[cd].attr_info[j].rsrc_id;
                    tabcnt++;
                }
                break;
            }
        }
    }
    if ( debugging4 ) {
        printf( "Calling convert_to_updatesql\n" );
        fflush( stdout );
    }


    for ( i = 0; i < attr_cnt; i++ ) {
        iisAttrVal[i] = malloc( sizeof( char ) * ( strlen( attr_value[i] ) * 4 + 3 ) );
        if ( iisAttrVal[i] == NULL ) {
            free( in_rsrcid );    // JMC cppcheck
            return( MEMORY_ALLOCATION_ERROR );
        }
        strcpy( iisAttrVal[i], attr_value[i] );
    }
    ii = castDimension( cd, updDimen, iisAttrVal, attr_id, attr_cnt );
    if ( ii < 0 ) {
        for ( i = 0; i < attr_cnt; i++ ) {
            free( iisAttrVal[i] );
        }
        free( in_rsrcid ); // JMC cppcheck - leak
        return( ii );
    }
    ii = convert_to_updatesql( cd, tabcnt, table_name, in_rsrcid,
                               attr_id, iisAttrVal, attr_cnt, qs );
    strcpy( GENERATEDSQL, qs );
    if ( ii < 0 ) {
        for ( i = 0; i < attr_cnt; i++ ) {
            free( iisAttrVal[i] );
        }
        return ( ii );
    }
    i = ExecSqlDb( henv, hdbc, qs );

    if ( i < 0 ) {
        strcpy( GENERATEDSQL, qs );
        return( i );
    }
    return( 0 );
}

int
insertIntoSchemaByUser( int cd, char *userName, char *userDomain,
                        int   *attr_id,
                        char **attr_value,
                        int    attr_cnt,
                        char *insDimen[] ) {
    int i, user_id;

    user_id = get_user_id_in_domain( userName, userDomain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    i = insertIntoSchema( cd,
                          attr_id, attr_value, attr_cnt, user_id, insDimen );
    return( i );
}

int
insertIntoSchema( int cd,
                  int   *attr_id,
                  char **attr_value,
                  int    attr_cnt, int    real_user_id,
                  char *insDimen[] ) {
    int i, ii, j, k;
    char **attr_name;
    char **table_name;
    int  *in_rsrcid;
    int    tabcnt;
    int    tabflag;
    int qsptr, query_stringptr;
    char  query_string[HUGE_STRING];
    char  qs[HUGE_STRING];
    char *iisAttrVal[MAX_QUERY_LIMIT];
    strcpy( query_string, "" );


    attr_name = malloc( sizeof( char * ) * attr_cnt );
    table_name = malloc( sizeof( char * ) * attr_cnt );
    in_rsrcid = malloc( sizeof( int )  * attr_cnt );

    i = checkAccessMetaDataAttr( real_user_id, attr_id, attr_cnt );
    if ( i != 0 ) {
        free( in_rsrcid );
        return( i );
    } // JMC cppcheck - leak

    tabcnt = 0;

    for ( i = 0; i < attr_cnt; i++ )  {
        for ( j = 0; j < svrMcatContext[cd].max_attr_entries; j++ ) {
            if ( svrMcatContext[cd].attr_info[j].attr_id == attr_id[i] ) {

                tabflag = 0;
                attr_name[i] = svrMcatContext[cd].attr_info[j].attr_name;

                for ( k = 0 ; k < tabcnt; k++ ) {
                    if ( table_name[k] == svrMcatContext[cd].attr_info[j].extended_table_name ) {
                        tabflag = 1;
                        break;
                    }
                }

                if ( tabflag == 0 ) {
                    table_name[tabcnt] = svrMcatContext[cd].attr_info[j].extended_table_name;
                    in_rsrcid[tabcnt] = svrMcatContext[cd].attr_info[j].rsrc_id;
                    tabcnt++;
                }

                break;
            } // if attr_id
        } // for j
    } // for i

    if ( debugging4 ) {
        printf( "Calling convert_to_insertsql\n" );
    }
    fflush( stdout );

    for ( i = 0; i < attr_cnt; i++ ) {
        iisAttrVal[i] = malloc( sizeof( char ) * ( strlen( attr_value[i] ) * 4 + 3 ) );
        if ( iisAttrVal[i] == NULL ) {
            free( in_rsrcid );    // JMC cppcheck - leak
            return( MEMORY_ALLOCATION_ERROR );
        }
        strcpy( iisAttrVal[i], attr_value[i] );
    }
    ii = castDimension( cd, insDimen, iisAttrVal, attr_id, attr_cnt );
    if ( ii < 0 ) {
        for ( i = 0; i < attr_cnt; i++ ) {
            free( iisAttrVal[i] );
        }
        free( in_rsrcid ); // JMC cppcheck - leak
        return( ii );
    }

    ii = convert_to_insertsql( cd, tabcnt, table_name, in_rsrcid, attr_id,
                               iisAttrVal, attr_cnt, query_string );
    strcpy( GENERATEDSQL, query_string );
    if ( ii < 0 ) {
        for ( i = 0; i < attr_cnt; i++ ) {
            free( iisAttrVal[i] );
        }
        return( ii );
    }
    /*  printf("QS:%s\n", query_string);*/
    qsptr = 0;
    query_stringptr = 0;
    while ( query_string[query_stringptr] != '\n'
            && query_stringptr < strlen( query_string ) ) {
        qs[qsptr] = query_string[query_stringptr];
        qsptr++;
        query_stringptr++;
    }
    qs[qsptr] = '\0';
    if ( query_string[query_stringptr] == '\n' ) {
        query_stringptr++;
    }
    while ( strlen( qs ) > 0 ) {
        i = ExecSqlDb( henv, hdbc, qs );
        if ( i < 0 ) {
            strcpy( GENERATEDSQL, qs );
            return( i );
        }
        qsptr = 0;
        while ( query_string[query_stringptr] != '\n'
                && query_stringptr < strlen( query_string ) ) {
            qs[qsptr] = query_string[query_stringptr];
            qsptr++;
            query_stringptr++;
        }
        qs[qsptr] = '\0';
        if ( query_string[query_stringptr] == '\n' ) {
            query_stringptr++;
        }
    }
    return( 0 );
}


// =-=-=-=-=-=-=-
// JMC :: build query for physical table using STL string vs char* & sprintf
const char* buildPhyiscalTableQuery( char **attr_name,
                                     char **attr_val,
                                     char  *dbschema_name,
                                     char  *table_name,
                                     int    attr_count ) {
    std::string sqlq = "insert into " +
                       string( dbschema_name ) +
                       string( table_name ) +
                       string( " ( " ) +
                       string( attr_name[0] );

    for ( i = 1; i < attr_count; i++ ) {
        sqlq += ", ";
        sqlq += string( attr_name[i] );
    }

    sqlq += " ) values ( ";
    sqlq += string( attr_val[0] );

    for ( i = 1; i < attr_count; i++ ) {
        sqlq += ", ";
        sqlq += string( attr_val[i] );
    }

    sqlq += ")";


    char* ret = new char[ sqlq.len() + 1 ];
    strncpy( ret, sqlq.c_str(), sqlq.len() );
    ret[ sqlq.len() + 1 ] = '\0';

    return ret;

} // buildPhyiscalTableQuery

// =-=-=-=-=-=-=-

insertIntoPhysicalTable( char **attr_name,
                         char **attr_val,
                         char  *dbschema_name,
                         char  *table_name,
                         int    attr_count ) {
#if 0 // JMC cppcheck - src & dest sprintf
    char sqlq[HUGE_STRING];
    int i;

    sprintf( sqlq, "insert into %s%s (", dbschema_name, table_name );
    sprintf( sqlq, "%s  %s", sqlq, attr_name[0] );

    for ( i = 1; i < attr_count; i++ ) {
        sprintf( sqlq, "%s , %s", sqlq, attr_name[i] );
    }
    sprintf( sqlq, "%s ) values (", sqlq );
    sprintf( sqlq, "%s %s", sqlq, attr_val[0] );
    for ( i = 1; i < attr_count; i++ ) {
        sprintf( sqlq, "%s , %s", sqlq, attr_val[i] );
    }
    sprintf( sqlq, "%s )", sqlq );
#else
    const char* sqlq = buildPhyiscalTableQuery( attr_name, attr_val, dbschema_name, table_name, attr_count );
#endif
    i = ExecSqlDb( henv, hdbc,  sqlq );
    if ( i != 0 ) {
        sprintf( errmess, "Data Insertion Error: %s", sqlq );
        error_exit( errmess );
        failure = METADATA_INSERTION_ERROR;
        return( METADATA_INSERTION_ERROR );
    }

    delete sqlq;
    return( RCAT_SUCCESS );
}
insertIntoPhysicalTableDontCare( char **attr_name,
                                 char **attr_val,
                                 char  *dbschema_name,
                                 char  *table_name,
                                 int    attr_count ) {
#if 0 // JMC cppcheck - src & dest sprintf
    char sqlq[HUGE_STRING];
    int i;

    sprintf( sqlq, "insert into %s%s (", dbschema_name, table_name );
    sprintf( sqlq, "%s  %s", sqlq, attr_name[0] );
    for ( i = 1; i < attr_count; i++ ) {
        sprintf( sqlq, "%s , %s", sqlq, attr_name[i] );
    }
    sprintf( sqlq, "%s ) values (", sqlq );
    sprintf( sqlq, "%s %s", sqlq, attr_val[0] );
    for ( i = 1; i < attr_count; i++ ) {
        sprintf( sqlq, "%s , %s", sqlq, attr_val[i] );
    }
    sprintf( sqlq, "%s )", sqlq );
#else
    const char* sqlq = buildPhyiscalTableQuery( attr_name, attr_val, dbschema_name, table_name, attr_count );
#endif
    i = ExecSqlDb( henv, hdbc,  sqlq );
    if ( i != 0 ) {
        return( METADATA_INSERTION_ERROR );
    }
    delete sqlq;
    return( RCAT_SUCCESS );
}

// =-=-=-=-=-=-=-
// JMC :: build query for physical table using STL string vs char* & sprintf
const char* buildDeleteFRomPhysicalTableQuery( char **attr_name,
        char **attr_val,
        char  *dbschema_name,
        char  *table_name,
        int    attr_count ) {
    string sqlq = "delete from " + string( dbschema_name ) + string( table_name );
    for ( i = 0; i < attr_count; i++ ) {
        if ( i > 0 ) {
            sqlq += string( " AND " ) +
                    string( attr_name[i] ) +
                    string( " = " ) +
                    string( attr_val[i] );
        }
        else {
            sqlq += " WHERE " + string( attr_name[i] ) + string( " = " ) + string( attr_val[i] );
        }
    }
} // buildDeleteFRomPhysicalTableQuery

// =-=-=-=-=-=-=-

int deleteFromPhysicalTable( char **attr_name,
                             char **attr_val,
                             char  *dbschema_name,
                             char  *table_name,
                             int    attr_count ) {
#if 0
    char sqlq[HUGE_STRING];
    int i;

    sprintf( sqlq, "delete from %s%s ", dbschema_name, table_name );
    for ( i = 0; i < attr_count; i++ ) {
        if ( i > 0 ) {
            sprintf( sqlq, "%s AND %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
        else {
            sprintf( sqlq, "%s WHERE %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
    }
#else
    const char* sqlq = buildDeleteFRomPhysicalTableQuery( attr_name, attr_val, dbschema_name, table_name, attr_count );
#endif

    i = ExecSqlDb( henv, hdbc, sqlq );
    if ( i != 0 ) {
        sprintf( errmess, "Data Deletion  Error: %s", sqlq );
        error_exit( errmess );
        failure = METADATA_DELETION_ERROR;
        return( METADATA_DELETION_ERROR );
    }
    return( RCAT_SUCCESS );
}
int deleteFromPhysicalTableDontCare( char **attr_name,
                                     char **attr_val,
                                     char  *dbschema_name,
                                     char  *table_name,
                                     int    attr_count ) {
#if 0
    char sqlq[HUGE_STRING];
    int i;

    sprintf( sqlq, "delete from %s%s ", dbschema_name, table_name );
    for ( i = 0; i < attr_count; i++ ) {
        if ( i > 0 ) {
            sprintf( sqlq, "%s AND %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
        else {
            sprintf( sqlq, "%s WHERE %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
    }
#else
    const char* sqlq = buildDeleteFRomPhysicalTableQuery( attr_name, attr_val, dbschema_name, table_name, attr_count );
#endif
    i = ExecSqlDb( henv, hdbc, sqlq );
    if ( i != 0 ) {
        return( METADATA_DELETION_ERROR );
    }
    return( RCAT_SUCCESS );
}

int changeAttrValueInPhysicalTable( char **attr_name,
                                    char **attr_val,
                                    char  *dbschema_name,
                                    char  *table_name,
                                    int    attr_count ) {

    sprintf( sqlq, "update %s%s set %s = %s ", dbschema_name, table_name,
             attr_name[0], attr_val[0] );
    for ( i = 1; i < attr_count; i++ ) {
        if ( i > 1 ) {
            sprintf( sqlq, "%s AND %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
        else {
            sprintf( sqlq, "%s WHERE %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
    }
    i = ExecSqlDb( henv, hdbc, sqlq );
    if ( i != 0 ) {
        sprintf( errmess, "Data Update Error: %s", sqlq );
        error_exit( errmess );
        failure = METADATA_UPDATE_ERROR;
        return( METADATA_UPDATE_ERROR );
    }
    return( RCAT_SUCCESS );
}

int changeAttrValueInPhysicalTableDontCare( char **attr_name,
        char **attr_val,
        char  *dbschema_name,
        char  *table_name,
        int    attr_count ) {

    sprintf( sqlq, "update %s%s set %s = %s ", dbschema_name, table_name,
             attr_name[0], attr_val[0] );
    for ( i = 1; i < attr_count; i++ ) {
        if ( i > 1 ) {
            sprintf( sqlq, "%s AND %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
        else {
            sprintf( sqlq, "%s WHERE %s = %s", sqlq, attr_name[i], attr_val[i] );
        }
    }
    i = ExecSqlDb( henv, hdbc, sqlq );
    if ( i != 0 ) {
        return( METADATA_UPDATE_ERROR );
    }
    return( RCAT_SUCCESS );
}
/*** template
sprintf(qval[qcount]," = '%s'",argv[2]);
	        qattrval[qcount++] = ;
                selval[selcount++] = ;

 ***/


rcatC_sql_result_struct *
queryMcatInfo( mcatContextDesc incd,
               int *selval, int *aggrval,
               int *qattrval, char **qval,
               int selcount, int qcount, int numrows,
               int OuterJoin, void *qDimen, void *selDimen
             ) {
    int x;
    int i, j;
    rcatC_sql_query_struct* info1;
    char *in_dti;
    char *qMIqval[MAX_QUERY_LIMIT];
    rcatC_sql_result_struct* result1;

    failure = 0;
    info1 = ( rcatC_sql_query_struct * )
            malloc( sizeof( rcatC_sql_query_struct ) );
    if ( info1 == NULL ) {
        failure = MEMORY_ALLOCATION_ERROR;
        return( NULL );
    }
    result1 = ( rcatC_sql_result_struct * )
              malloc( sizeof( rcatC_sql_result_struct ) );
    if ( result1 == NULL ) {
        free( info1 );
        failure = MEMORY_ALLOCATION_ERROR;
        return( NULL );
    }

    for ( i = 0; i < qcount; i++ ) {
        qMIqval[i] = malloc( sizeof( char ) * ( strlen( qval[i] ) * 4 + 3 ) );
        if ( qMIqval[i] == NULL ) {
            for ( j = 0; j < i ; j++ ) {
                free( qMIqval[j] );
            }
            free( info1 );
            free( result1 );
            failure = MEMORY_ALLOCATION_ERROR;
            return( NULL );
        }
        strcpy( qMIqval[i], qval[i] );
    }

    i = translateArraysQToSqlQStruct( incd, selval, aggrval,
                                      qattrval, qMIqval,
                                      selcount, qcount, numrows,
                                      info1, result1, qDimen, selDimen );
    if ( i != 0 ) {
        for ( j = 0; j < qcount; j++ ) {
            free( qMIqval[j] );
        }
        free( info1 );
        free( result1 );
        failure = i;
        return( NULL );
    }
    outer_join_flag = OuterJoin;
    rcatC_db_info_inquire( &serverhndl[0], ( rcatC_infoh* ) info1 ,
                           ( rcatC_infoh* )  result1,  status, numrows, incd );
    distinct_flag = 1;
    outer_join_flag = 0;
    best_response_flag = 0;


    for ( i = 0; i < qcount; i++ ) {
        free( qMIqval[i] );
    }
    if ( status[2] != RCAT_SUCCESS ) {
#ifdef DEBUGON
        rodsLog( NOTICE, "Error in Inquire Status:%i\n", status[2] );
        rodsLog( NOTICE, "Failed SQL:%s\n", GENERATEDSQL );
#endif /* DEBUGON */
        free( info1 );
        free( result1 );
        failure = MCAT_INQUIRE_ERROR;
        return( NULL );
    }
    free( info1 );
    return ( result1 );
}

int queryMcatInfoArrays( mcatContextDesc incd,
                         int *selval, int *aggrval,
                         int *qattrval, char **qval,
                         int selcount, int qcount, int numrows,
                         int *colcount, int *rowcount,
                         int **rattrval, char **resval, int **rattrvalsize,
                         int *continuation_index, int OuterJoin,
                         void *qDimen, void *selDimen ) {
    int i, j;
    rcatC_sql_result_struct* result1;

    result1 = queryMcatInfo( incd, selval, aggrval, qattrval, qval,
                             selcount, qcount, numrows, OuterJoin,
                             qDimen, selDimen );
    if ( result1 == NULL ) {
        return( failure );
    }

    i = translateSqlRStructToArraysR( result1, colcount, rowcount,
                                      rattrval, resval, rattrvalsize,
                                      continuation_index );

    free( result1 );
    return ( i );
}

int
findAttrIndexFromId( mcatContextDesc cd, int attr_id ) {
    int ii;
    for ( ii = 0; ii < svrMcatContext[cd].max_attr_entries ; ii++ ) {
        if ( svrMcatContext[cd].attr_info[ii].attr_id == attr_id ) {
            return( ii );
        }
    }
    printf( "wrong id:%i\n", attr_id );
    return( UNKNOWN_ATTRIBUTE_ID );
}

int
getAttributesfromMcatContext( mcatContextDesc cd, int *counta,
                              rcat_metadata_attr_entry **AttrEntries ) {

    int i, cnta;

    cnta = getAttrCountinSchemaSet( svrMcatContext[cd].userContext, "known" );
    if ( cnta < 0 ) {
        return ( cnta );
    }
    i = allocAttrEntrySet( AttrEntries,  cnta );
    if ( i < 0 ) {
        return ( i );
    }
    i = getAttrEntrySet( svrMcatContext[cd].userContext,
                         *AttrEntries, cnta, "known" );
    if ( i < 0 ) {
        freeAttrEntrySet( *AttrEntries, cnta );
        return ( i );
    }
    *counta = cnta;
    return( RCAT_SUCCESS );
}

mcatAttrPropSet *
getAttrPropertiesfromMcat( mcatContextDesc cd ) {
    mcatAttrPropSet *mAPS;
    mAPS = ( mcatAttrPropSet * ) malloc( sizeof( mcatAttrPropSet ) );
    mAPS->attr_info = svrMcatContext[cd].attr_info;
    mAPS->max_attr_entries = svrMcatContext[cd].max_attr_entries;
    return( mAPS );
}

int
checkforownershipofschema( char *schemaName, int realUserId ) {
    int i, user_id;
    failure = 0;


    sprintf( sqlq, "select schema_name from %sRCORE_SCHEMAS  where \
schema_name =  '%s' and user_id =  %i",
             RCATSCHEMENAME, schemaName, realUserId );
    if ( check_exists( sqlq ) != 0 ) {
        if ( debugging4 ) printf( "ACTION_NOT_ALLOWED_FOR_USER:%s,%i\n",
                                      schemaName, realUserId );
        return ( ACTION_NOT_ALLOWED_FOR_USER );
    }
    return( RCAT_SUCCESS );

}

int
checkforuniqueschemaname( char *schemaName ) {
    int i;
    failure = 0;
    sprintf( sqlq, "select schema_name from %sRCORE_SCHEMAS  where \
schema_name =  '%s'",
             RCATSCHEMENAME, schemaName );
    if ( check_exists( sqlq ) != 0 ) {
        return ( UNKNOWN_SCHEMA_NAME );
    }
    return( RCAT_SUCCESS );

}

unsigned long int
getMaxSize( char *attrDataType ) {
    int i = 0;
    int j;
    int k = 0;
    unsigned long int m = 1;
    unsigned long int n = 0;
    char numb[20];
    j = strlen( attrDataType );
    if ( strstr( attrDataType, "int" ) != NULL ) {
        return ( DEFAULT_INTEGER_SPACE_SIZE );
    }
    if ( strstr( attrDataType, "integer" ) != NULL ) {
        return ( DEFAULT_INTEGER_SPACE_SIZE );
    }
    if ( strstr( attrDataType, "INT" ) != NULL ) {
        return ( DEFAULT_INTEGER_SPACE_SIZE );
    }
    if ( strstr( attrDataType, "INTEGER" ) != NULL ) {
        return ( DEFAULT_INTEGER_SPACE_SIZE );
    }
    if ( strstr( attrDataType, "float" ) != NULL ) {
        return ( DEFAULT_FLOAT_SPACE_SIZE );
    }
    if ( strstr( attrDataType, "FLOAT" ) != NULL ) {
        return ( DEFAULT_FLOAT_SPACE_SIZE );
    }
    if ( strstr( attrDataType, "date" ) != NULL ) {
        return ( DEFAULT_DATE_SPACE_SIZE );
    }
    if ( strstr( attrDataType, "DATE" ) != NULL ) {
        return ( DEFAULT_DATE_SPACE_SIZE );
    }

    if ( !strcmp( RCATDBTYPE, "oracle" ) ) {
        if ( ( strstr( attrDataType, "blob" ) != NULL ) ||
                ( strstr( attrDataType, "BLOB" ) != NULL ) ) {
            strcpy( attrDataType, "LONG RAW" );
            return ( DEFAULT_ORACLE_BLOB_SPACE_SIZE );
        }
        else if ( ( strstr( attrDataType, "clob" ) != NULL ) ||
                  ( strstr( attrDataType, "CLOB" ) != NULL ) ) {
            strcpy( attrDataType, "LONG" );
            return ( DEFAULT_ORACLE_CLOB_SPACE_SIZE );
        }
    }
    while ( i < j && attrDataType[i] != '(' ) {
        i++;
    }
    if ( i == j ) {
        return ( SIZE_OF_ATTRIBUTE_UNKNOWN );
    }
    i++;
    while ( attrDataType[i] != ')'
            && attrDataType[i] != 'K'
            && attrDataType[i] != 'M'
            && attrDataType[i] != 'G' ) {
        if ( attrDataType[i] < '0' || attrDataType[i] > '9' ) {
            return ( SIZE_OF_ATTRIBUTE_UNKNOWN );
        }
        numb[k] = attrDataType[i];
        i++;
        k++;
    }
    if ( k == 0 ) {
        return ( SIZE_OF_ATTRIBUTE_UNKNOWN );
    }
    for ( i = k - 1; i >= 0; i -- ) {
        n = n + ( m * ( int )( numb[i] - '0' ) );
        m = m * 10;
    }
    if ( attrDataType[i] == 'K' ) {
        n = n * 1024 ;
    }
    else if ( attrDataType[i] == 'M' ) {
        n = n * 1024 * 1024 ;
    }
    else if ( attrDataType[i] == 'G' ) {
        n = n * 1024 * 1024 * 1024 ;
    }
    return ( n );

}


int
getAttrIdForName( char *attrName, char *schemaName ) {
    int i, j, k, attrId;
    char mySchemaName[MAX_TOKEN];
    char myAttrName[MAX_TOKEN];
    char sqlq[HUGE_STRING];
    char *tmpStr;

    if ( ( tmpStr = strstr( attrName, "." ) ) != NULL ) {
        strcpy( mySchemaName, attrName );
        mySchemaName[ tmpStr - attrName ] = '\0';
        strcpy( myAttrName, ( char * )( tmpStr + 1 ) );
    }
    else {
        strcpy( mySchemaName, schemaName );
        strcpy( myAttrName, attrName );
    }

    failure = 0;
    sprintf( sqlq, "select attr_id from %sRCORE_ATTRIBUTES t1, %sRCORE_TABLES t2 \
where t1.external_attr_name  =  '%s' and t1.table_id = t2.table_id and \
t2.schema_name  =  '%s' ",
             RCATSCHEMENAME, RCATSCHEMENAME, myAttrName, mySchemaName );
    attrId = get_itype_value_from_query( sqlq );
    return( attrId );
}

int
getAttrInfoForToken( char *attrName, char *schemaName,
                     char *outAttrName, int *attrTokenId,
                     int *tabTokenId, char *mySchemaName,
                     char *attrDataType, unsigned long int *maxsize,
                     char *outExterAttrName, int *expose,
                     char *presentation, char *default_value,
                     int *exter_attrib_id, char *attr_dimension ) {
    int i, j, k, attrId;
    char myAttrName[MAX_TOKEN];
    char sqlq[HUGE_STRING];
    char cval[15][MAX_TOKEN];
    int colcount;
    char *tmpStr;
    char attrIdenType[SMALL_TOKEN];

    if ( ( tmpStr = strstr( attrName, "." ) ) != NULL ) {
        strcpy( mySchemaName, attrName );
        mySchemaName[ tmpStr - attrName ] = '\0';
        strcpy( myAttrName, ( char * )( tmpStr + 1 ) );
    }
    else {
        strcpy( mySchemaName, schemaName );
        strcpy( myAttrName, attrName );
    }



    failure = 0;
    sprintf( sqlq, "select attr_val_type, attr_iden_type, attr_id from %sRCORE_ATTRIBUTES t1, %sRCORE_TABLES t2 \
where t1.external_attr_name  =  '%s' and t1.table_id = t2.table_id and \
t2.schema_name  =  '%s' ",
             RCATSCHEMENAME, RCATSCHEMENAME, myAttrName, mySchemaName );
    colcount = 3;
    i = get_row_from_query( cval, &colcount, sqlq );
    /*get_itype_value_from_query(sqlq); */
    if ( i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( INVALID_TOKEN_NAME );
    }
    strcpy( attrIdenType, cval[1] );
    if ( !strcmp( attrIdenType, "SR" ) ||
            !strcmp( attrIdenType, "LS" ) ||
            !strcmp( attrIdenType, "HR" ) ||
            !strcmp( attrIdenType, "PR" ) ||
            !strcmp( attrIdenType, "UR" ) ||
            !strcmp( attrIdenType, "ST" ) ||
            !strcmp( attrIdenType, "LT" ) ||
            !strcmp( attrIdenType, "HT" ) ||
            !strcmp( attrIdenType, "UT" ) ) {
        attrId = atoi( cval[0] );
    }
    else {
        attrId = atoi( cval[2] );
    }

    sprintf( sqlq, "select attr_name, table_id, attr_data_type, maxsize, \
external_attr_name, expose, presentation, default_value, exter_attrib_id, \
  attr_dimension from %sRCORE_ATTRIBUTES t1 where \
t1.attr_id = %i ", RCATSCHEMENAME, attrId );
    colcount = 10;
    i = get_row_from_query( cval, &colcount, sqlq );
    if ( failure != 0 || i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( INVALID_TOKEN_NAME );
    }
    strcpy( outAttrName, cval[0] );
    *tabTokenId = atoi( cval[1] );
    strcpy( attrDataType, cval[2] );
    *maxsize =  strtoul( cval[3], ( char ** )NULL, 10 );
    strcpy( outExterAttrName, cval[4] );
    *attrTokenId = attrId;
    *expose = atoi( cval[5] );
    strcpy( presentation, cval[6] );
    strcpy( default_value, cval[7] );
    *exter_attrib_id = atoi( cval[8] );
    strcpy( attr_dimension, cval[9] );
    return( RCAT_SUCCESS );
}



int
getNewAttrName( char *attrExterName, char *tableName,
                char *attrName ) {
    int i, k;
    int j = 1;
    char newattrName[MAX_TOKEN];
    char *tmpSTr;
    char sqlq[HUGE_STRING];

    strcpy( newattrName, attrExterName );
    if ( strlen( attrExterName ) < MAX_SQL_NAME_LIMIT ) {
        for ( i = 0; i < strlen( newattrName ) ; i++ ) {
            if ( attrExterName[i] == ' ' ) {
                attrName[i] = '_';
            }
            else {
                attrName[i] = attrExterName[i];
            }
        }
        attrName[i] = '\0';
        return( RCAT_SUCCESS );
    }
    for ( i = 0, j = 0; i < strlen( newattrName ) ; i++ ) {
        if ( newattrName[i] == ' ' || newattrName[i] == '_' ) {
            continue;
        }
        newattrName[j] = newattrName[i];
        j++;
    }
    newattrName[j] = '\0';
    strncpy( attrName, newattrName, MAX_SQL_NAME_LIMIT );
    failure = 0;
    sprintf( sqlq, "select attr_name from %sRCORE_ATTRIBUTES t1, %sRCORE_TABLES t2 where \
t1.attr_name = '%s' and t2.table_name = '%s' and t1.table_id = t2.table_id",
             RCATSCHEMENAME, RCATSCHEMENAME, attrName, tableName );
    while ( check_exists( sqlq ) == RCAT_SUCCESS ) {
        if ( strlen( attrName ) < 1 ) {
            return ( UNABLE_TO_MAKE_UNIQUE_ATTRIBUTE_NAME );
        }
        failure = 0;
        i = random() % strlen( attrName );
        strcpy( ( char * ) &attrName[i], ( char * ) &attrName[i + 1] );
        sprintf( sqlq, "select attr_name from %sRCORE_ATTRIBUTES t1, %sRCORE_TABLES t2 where \
t1.attr_name = '%s' and t2.table_name = '%s' and t1.table_id = t2.table_id",
                 RCATSCHEMENAME, RCATSCHEMENAME, attrName, tableName );
    }
    for ( i = 0; i < strlen( attrName ) ; i++ ) {
        if ( ( attrName[i] == '_' && attrName[i + 1] == '_' ) ) {
            strcpy( ( char * ) &attrName[i], ( char * ) &attrName[i + 1] );
            i++;
        }
    }
    return( RCAT_SUCCESS );
}
int
getNewTableName( char *schemaName,
                 char *attrIdenType,
                 char *attrName,
                 char *tableName ) {
    int i, k;
    int j = 1;
    char newattrName[MAX_TOKEN];
    char *tmpStr, *tmpPtr;
    char sqlq[HUGE_STRING];
    strcpy( newattrName, attrName );
    tableName[0] = schemaName[0];
    k = strlen( schemaName );


    for ( i = 1; ( i < k && j < 4 ) ; i++ ) {
        if ( schemaName[i] == '_' || schemaName[i] == ' ' ) {
            tableName[j++] = schemaName[i + 1];
        }
    }
    if ( j < 4 ) {
        strncpy( tableName, schemaName, 4 );
        tableName[4] = '\0';
        j = strlen( tableName );
    }
    tableName[j] = '\0';

    if ( !strcmp( attrIdenType, "SR" ) ||
            !strcmp( attrIdenType, "LS" ) ||
            !strcmp( attrIdenType, "HR" ) ||
            !strcmp( attrIdenType, "PR" ) ||
            !strcmp( attrIdenType, "UR" ) ||
            !strcmp( attrIdenType, "ST" ) ||
            !strcmp( attrIdenType, "LT" ) ||
            !strcmp( attrIdenType, "HT" ) ||
            !strcmp( attrIdenType, "UT" ) ) {
        sprintf( ( char * ) &tableName[j], "%s", "_TD_" );
    }
    else {
        sprintf( ( char * ) &tableName[j], "%s", "_AD_" );
    }
    j =  j + 4;



    if ( ( tmpStr = strstr( newattrName, "Name" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 4 ) );
    }
    if ( ( tmpStr = strstr( newattrName, "NAME" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 4 ) );
    }
    if ( ( tmpStr = strstr( newattrName, "type" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 4 ) );
    }
    if ( ( tmpStr = strstr( newattrName, "TYPE" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 4 ) );
    }
    if ( ( tmpStr = strstr( newattrName, "Identifier" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 10 ) );
    }
    if ( ( tmpStr = strstr( newattrName, "IDENTIFIER" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 10 ) );
    }
    if ( ( tmpStr = strstr( newattrName, "Id" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 2 ) );
    }
    if ( ( tmpStr = strstr( newattrName, "ID" ) ) != NULL ) {
        strcpy( tmpStr, ( char * )( tmpStr + 2 ) );
    }
    for ( i = 0; i < strlen( newattrName ) ; i++ ) {
        if ( newattrName[i] == ' ' ) {
            newattrName[i] = '_';
        }
        if ( ( newattrName[i] == '_' && newattrName[i + 1] == '_' ) ||
                ( newattrName[i] == '_' && newattrName[i + 1] == ' ' ) ) {
            strcpy( ( char * ) &newattrName[i], ( char * ) &newattrName[i + 1] );
            i++;
        }
    }


    if ( strlen( newattrName ) < 9 ) {
        strcat( tableName, newattrName );
    }
    else {
        if ( ( tmpStr = strstr( newattrName, "_" ) ) != NULL ) {
            strncat( tableName, newattrName, 4 );
            strcat( tableName, "_" );
            strncat( tableName, tmpStr, 3 );
        }
        else {
            strncat( tableName, newattrName, 8 );
        }
    }

    while ( tableName[strlen( tableName ) - 1] == '_' ) {
        tableName[strlen( tableName ) - 1] = '\0';
    }

    failure = 0;
    sprintf( sqlq, "select table_name from %sRCORE_TABLES t1 where \
t1.table_name = '%s' and t1.schema_name =  '%s'",
             RCATSCHEMENAME, tableName, schemaName );
    while ( check_exists( sqlq ) == RCAT_SUCCESS ) {
        if ( strlen( tableName ) < 1 ) {
            return ( UNABLE_TO_MAKE_UNIQUE_TABLE_NAME );
        }
        failure = 0;

        if ( ( ( tmpPtr = strstr( tableName, "_TD_" ) ) != NULL ) ||
                ( ( tmpPtr = strstr( tableName, "_TD_" ) ) != NULL ) ) {
            i = ( random() % ( strlen( tableName ) -
                               ( ( int )tmpPtr - ( int )tableName + 4 ) ) ) +
                ( ( int )tmpPtr - ( int )tableName + 4 );
        }
        else {
            i = random() % strlen( tableName );
        }
        strcpy( ( char * ) &tableName[i], ( char * ) &tableName[i + 1] );
        sprintf( sqlq, "select table_name from %sRCORE_TABLES t1 where \
t1.table_name = '%s' and t1.schema_name =  '%s'",
                 RCATSCHEMENAME, tableName, schemaName );
    }
    for ( i = 0; i < strlen( tableName ) ; i++ ) {
        if ( ( tableName[i] == '_' && tableName[i + 1] == '_' ) ) {
            strcpy( ( char * ) &tableName[i], ( char * ) &tableName[i + 1] );
            i++;
        }
    }

    return( RCAT_SUCCESS );
}
int
insert_into_rcore_fk_relations( int attrcnt,
                                int tabList[], int attrList[],
                                char *funcIn,  char *funcOut,
                                char *relOp,
                                char *schemaIn, char *schemaOut ) {
    /*
    insert into rcore_fk_relations values (44,54,11,54,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,'','','=','mcatcore','mcatcore');
    */

    int res, i, j;
    int tabId;
    char sqlq[HUGH_HUGE_STRING];
    failure = 0;
    tabId =  get_next_counter_value( "TABLE_ID" );
    if ( failure != 0 ) {
        return ( TABLE_ID_COUNTER_ERROR );
    }
    sprintf( sqlq,
             "insert into %s%s (", RCATSCHEMENAME, "RCORE_FK_RELATIONS" );
    for ( i = 0; i <  MAX_FK_REL_PAIRS ; i++ ) {
        sprintf( &sqlq[strlen( sqlq )],
                 "table_id_in%i , attr_id_in%i ,table_id_out%i , attr_id_out%i , ",
                 i + 1, i + 1, i + 1, i + 1 );
    }
    strcat( sqlq, " func_in, func_out, attr_rel_name, schema_name_in, schema_name_out ) values ( " );
    for ( i = 0; i < attrcnt ; i++ ) {
        sprintf( &sqlq[strlen( sqlq )], "%i, %i, %i, %i, ",
                 tabList[2 * i], attrList[2 * i],
                 tabList[( 2 * i ) + 1], attrList[( 2 * i ) + 1] );
    }
    for ( i = attrcnt; i < MAX_FK_REL_PAIRS; i++ ) {
        sprintf( &sqlq[strlen( sqlq )], "-1, -1, -1, -1, " );

    }
    sprintf( &sqlq[strlen( sqlq )], " '%s', '%s',  '%s',  '%s',  '%s' )",
             funcIn, funcOut, relOp, schemaIn, schemaOut );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( FKREL_METADATA_INSERTION_ERROR );
    }
    else {
        return( RCAT_SUCCESS );
    }
}


int
insert_into_rcore_tables( char *table_name,       char *database_name,
                          char *schema_name,      char *cluster_name,
                          char *dbschema_name,
                          char *tablespace_name,  int   user_id,
                          int   rsrc_id,		int   repl_enum,
                          char *subject,		char *comments,
                          char *constraints,	int sameas ) {
    int res;
    int tabId;
    char sqlq[HUGH_HUGE_STRING];
    failure = 0;
    tabId =  get_next_counter_value( "TABLE_ID" );
    if ( failure != 0 ) {
        return ( TABLE_ID_COUNTER_ERROR );
    }
    sprintf( sqlq,
             "insert into %s%s (table_id,table_name,database_name,schema_name, \
dbschema_name,tablespace_name,user_id,rsrc_id,repl_enum,subject,comments, \
constraints,sameas, cluster_name) values (%i,'%s','%s','%s','%s','%s',%i,%i,%i,'%s','%s','%s',%i,'%s')", RCATSCHEMENAME, "RCORE_TABLES",
             tabId, table_name, database_name, schema_name, dbschema_name,
             tablespace_name, user_id, rsrc_id, repl_enum, subject, comments,
             constraints, sameas, cluster_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( TABLE_METADATA_INSERTION_ERROR );
    }
    else {
        return( tabId );
    }
}

int
insert_into_rcore_attributes( char *schema_name,
                              int table_id,         char *attr_name,
                              char *attr_data_type,     char *attr_iden_type,
                              char *external_attr_name, char *default_value,
                              int   attr_val_type,	  int   sameas,
                              char *at_comments,        char *presentation,
                              int   primary_key,	  int   expose,
                              int   exter_attrib_id,    int   maxsize,
                              char *attr_dimension ) {
    int res;
    int attrId;
    char sqlq[HUGH_HUGE_STRING];


    if ( debugging4 ) {
        printf( "Entering insert_into_rcore_attributes\n" );
        fflush( stdout );
    }

    failure = 0;
    attrId =  get_next_counter_value( "ATTRIBUTE_ID" );
    if ( failure != 0 ) {
        return ( ATTRIBUTE_ID_COUNTER_ERROR );
    }

    sprintf( sqlq,
             "insert into %s%s (attr_id, table_id,attr_name,attr_data_type, \
attr_iden_type,external_attr_name,default_value,attr_val_type,sameas, \
at_comments,presentation,primary_key,expose,exter_attrib_id,maxsize, attr_dimension) \
 values (%i,%i,'%s','%s','%s','%s','%s',%i,%i,'%s','%s',%i,%i,%i,%i,'%s')",
             RCATSCHEMENAME, "RCORE_ATTRIBUTES",
             attrId, table_id, attr_name, attr_data_type, attr_iden_type,
             external_attr_name, default_value, attr_val_type, sameas,
             at_comments, presentation, primary_key, expose, exter_attrib_id, maxsize,
             attr_dimension );
    if ( debugging4 ) {
        printf( "Insert Statement:%s\n", sqlq );
        fflush( stdout );
    }

    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( ATTRIBUTE_METADATA_INSERTION_ERROR );
    }
    sprintf( sqlq,
             "insert into %s%s (user_schema_name,attr_id,expose) values ('%s',%i,%i)",
             RCATSCHEMENAME, "RCORE_USCHEMA_ATTR",
             schema_name, attrId, expose );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( ATTRIBUTE_METADATA_INSERTION_ERROR );
    }
    return( attrId );
}
int
createNewTable( int numattrs,            char *tableName,
                char *Lattrnames[],      char *Lattrdatatypes[],
                char *otherString,       char *database_name,
                char *dbschema_name,     char *tablespace_name,
                int   user_id, 	        int   rsrc_id ) {
    int i, j;
    char sqlq[HUGH_HUGE_STRING];
    char tmpStr[MAX_TOKEN];
    if ( !strcmp( database_name, RCATDBNAME ) &&
            !strcmp( dbschema_name, RCATSCHEMENAME ) &&
            !strcmp( tablespace_name, RCATTABLESPACENAME ) &&
            user_id == MCATUSERID &&
            rsrc_id == RCATRESOURCEID ) {
        sprintf( sqlq, "create table %s%s ( ", dbschema_name, tableName );
        for ( i = 0; i <  numattrs ; i++ ) {
            if ( i != 0 ) {
                sprintf( tmpStr, ", %s %s ", Lattrnames[i], Lattrdatatypes[i] );
            }
            else {
                sprintf( tmpStr, " %s %s ", Lattrnames[i], Lattrdatatypes[i] );
            }
            strcat( sqlq, tmpStr );
        }
        if ( strlen( otherString )  > 0 ) {
            strcat( sqlq, ", " );
            strcat( sqlq, otherString );
        }
        strcat( sqlq, " ) " );
        if ( strlen( tablespace_name ) > 0 ) {
            strcat( sqlq, " tablespace " );
            strcat( sqlq, tablespace_name );
        }

        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res < 0 ) {
            strcpy( GENERATEDSQL, sqlq );
            return( MCAT_TABLE_CREATION_ERROR );
        }
        else {
            return( RCAT_SUCCESS );
        }
    }
    else  {
        return ( INSERT_INTO_NONLOCAL_MCAT_DATABASE );
    }
}


int
addTokenAttrInSchemaByUser( char *schemaName, int attrCount,
                            char *attrExtrName[], char *attrDataType[],
                            char *attrIdenType[], char *defaultVal[],
                            char *attrComments[], char *attrPresentation[],
                            char *subjects,	char *constraints,
                            char *user, char *domain, char *attrDimension[] ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( addTokenAttrInSchema( schemaName, attrCount, attrExtrName,
                                  attrDataType, attrIdenType, defaultVal,
                                  attrComments, attrPresentation, subjects,
                                  constraints, user_id, attrDimension ) );

}

int
addTokenAttrInSchema( char *schemaName, int attrCount,
                      char *attrExtrName[], char *attrDataType[],
                      char *attrIdenType[], char *defaultVal[],
                      char *attrComments[], char *attrPresentation[],
                      char *subjects,	char *constraints,
                      int  realUserId, char *attrDimension[] ) {
    int i, j, k, tabid, attrid1, attrid2, startk, tokenId, user_id;
    unsigned long int maxsize;
    int  numattrs = 1;
    char *attrName[MAX_TABLE_COLS];
    char *LocattrDataType[MAX_TABLE_COLS];
    char tableName[MAX_TOKEN];
    char idAttrDataType[MAX_TOKEN], idAttrIdenType[MAX_TOKEN];
    /* register a table */

    i = checkforuniqueschemaname( schemaName ) ;
    if ( i != 0 ) {
        return ( i );
    }

    i = checkforownershipofschema( schemaName, realUserId );
    if ( i != 0 ) {
        return ( i );
    }


    i = getNewTableName( schemaName, attrIdenType[0],
                         attrExtrName[0], tableName );
    if ( i < 0 ) {
        return( i );
    }

    tabid = insert_into_rcore_tables( tableName, RCATDBNAME, schemaName,
                                      attrExtrName[0],
                                      RCATSCHEMENAME, RCATTABLESPACENAME,
                                      MCATUSERID , RCATRESOURCEID,
                                      0, subjects, attrComments[0], constraints, 0 );
    if ( tabid < 0 ) {
        return( tabid );
    }

    startk = 0;

    if ( !strcmp( attrIdenType[0], "SR" ) ||
            !strcmp( attrIdenType[0], "LS" ) ||
            !strcmp( attrIdenType[0], "UR" ) ) {
        strcpy( idAttrIdenType, "ST" );
        maxsize = DEFAULT_INTEGER_SPACE_SIZE;
        strcpy( idAttrDataType, "int" );
    }
    else if ( !strcmp( attrIdenType[0], "HR" ) ) {
        strcpy( idAttrIdenType, "HT" );
        maxsize = DEFAULT_TOKEN_ID_MAXSIZE;
        sprintf( idAttrDataType, "char(%i)", DEFAULT_TOKEN_ID_MAXSIZE );
    }
    else {
        return ( TOKEN_IDEN_TYPE_ERROR );
    }
    attrName[0] = ( char * )malloc( sizeof( char ) * MAX_TOKEN );
    if ( attrName[0] == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );
    }
    startk = 1;
    i = getNewAttrName( attrExtrName[0], tableName, attrName[0] );
    if ( i < 0 ) {
        free( attrName[0] );
        return( i );
    }
    if ( strlen( attrName[0] ) < ( MAX_SQL_NAME_LIMIT - 4 ) ) {
        strcat( attrName[0], "_mid" );
    }
    else {
        strcat( ( char * )( &attrName[0] + MAX_SQL_NAME_LIMIT - 4 ),  "_mid" );
    }
    tokenId = insert_into_rcore_attributes( schemaName, tabid, attrName[0],
                                            idAttrDataType,
                                            idAttrIdenType, attrName[0],
                                            "",
                                            -1, -1, "MCAT created token identifier",
                                            "",
                                            1, 0, -1, maxsize, "" );

    if ( tokenId < 0 ) {
        free( attrName[0] );
        return( tokenId );
    }
    strcat( idAttrDataType, " not null " );
    LocattrDataType[0] = idAttrDataType;




    for ( k = 0; k < attrCount ; k++ ) {
        maxsize = getMaxSize( attrDataType[k] );
        if ( maxsize < 0 ) {
            for ( i = 0; i <= ( k + startk ); i++ ) {
                free( attrName[i] );
            }
            return( maxsize );
        }
        attrName[k + startk] = ( char * )malloc( sizeof( char ) * MAX_TOKEN );
        if ( attrName[k + startk] == NULL ) {
            return ( MEMORY_ALLOCATION_ERROR );
        }
        LocattrDataType[k + startk] = attrDataType[k];
        j = getNewAttrName( attrExtrName[k], tableName, attrName[k + startk] );
        if ( i < 0 ) {
            for ( i = 0; i <= ( k + startk ); i++ ) {
                free( attrName[i] );
            }
            return( j );
        }
        attrid1 = insert_into_rcore_attributes( schemaName, tabid, attrName[k + startk],
                                                attrDataType[k],
                                                attrIdenType[k], attrExtrName[k],
                                                defaultVal[k],
                                                tokenId, -1, attrComments[k],
                                                attrPresentation[k],
                                                0, 1, -1, maxsize, attrDimension[k] );

        if ( attrid1 < 0 ) {
            for ( i = 0; i <= ( k + startk ); i++ ) {
                free( attrName[i] );
            }
            return( attrid1 );
        }
    }
    i = createNewTable( attrCount + startk, tableName, attrName,
                        LocattrDataType, "",
                        RCATDBNAME, RCATSCHEMENAME,
                        RCATTABLESPACENAME, MCATUSERID , RCATRESOURCEID );
    for ( k = 0; k < ( attrCount + startk ) ; k++ ) {
        free( attrName[k] );
    }
    return ( i );

}

int
registerLinkInSchemaByUser( char *schemaName, char *clusterName,
                            char *rfschemaName, char *rfCluster,
                            char *rfdbSchema, char *rftabName,
                            char *forKeyAttr[], char *referAttr[],
                            int rfCount, char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( registerLinkInSchema( schemaName, clusterName, rfschemaName,
                                  rfCluster, rfdbSchema, rftabName,
                                  forKeyAttr, referAttr, rfCount, user_id ) );
}

int
registerLinkInSchema( char *schemaName, char *clusterName,
                      char *rfschemaName, char *rfCluster,
                      char *rfdbSchema, char *rftabName,
                      char *forKeyAttr[], char *referAttr[],
                      int rfCount, int  realUserId ) {
    int i, j, k;
    int tabIdList[MAX_TABLE_COLS];
    int attrIdList[MAX_TABLE_COLS];
    char  exterSchemaName[MAX_TOKEN];

    i = checkforownershipofschema( schemaName, realUserId );
    if ( i != 0 ) {
        return ( i );
    }

    tabIdList[0] = getTabIdFromSchemaCluster( schemaName, clusterName );
    if ( tabIdList[0] < 0 ) {
        return ( tabIdList[0] );
    }


    if ( rfCluster != NULL && strlen( rfCluster ) > 0 ) {
        tabIdList[1] = getTabIdFromSchemaCluster( rfschemaName, rfCluster );
        if ( tabIdList[1] < 0 ) {
            return ( tabIdList[1] );
        }
    }
    else if ( rftabName != NULL && strlen( rftabName ) > 0 ) {
        tabIdList[1] = getTabIdFromDBSchemaTabName( rfdbSchema, rftabName );
        if ( tabIdList[1] < 0 ) {
            return ( tabIdList[1] );
        }
    }
    else {
        return( FK_REL_TABLE_ID_ERROR );
    }
    i = getSchemaNameFromTabId( exterSchemaName, tabIdList[1] );
    if ( i != 0 ) {
        return ( i );
    }

    for ( i = 1; i < rfCount; i++ ) {
        tabIdList[2 * i] = tabIdList[0];
        tabIdList[( 2 * i ) + 1] = tabIdList[1];
    }

    for ( i = 0; i < rfCount; i++ ) {
        attrIdList[2 * i] =  getAttrIdFromTabIdAttrName( tabIdList[2 * i],
                             forKeyAttr[i] );
        if ( attrIdList[2 * i] < 0 ) {
            return ( attrIdList[2 * i] );
        }
        if ( rftabName != NULL && strlen( rftabName ) > 0 )
            attrIdList[( 2 * i ) + 1] = getAttrIdFromTabIdAttrName( tabIdList[( 2 * i ) + 1],
                                        referAttr[i] );
        else
            attrIdList[( 2 * i ) + 1] = getAttrIdFromTabIdExterAttrName( tabIdList[( 2 * i ) + 1],
                                        referAttr[i] );

        if ( attrIdList[( 2 * i ) + 1] < 0 ) {
            return ( attrIdList[( 2 * i ) + 1] );
        }
    }

    j = insert_into_rcore_fk_relations( rfCount, tabIdList, attrIdList, "", "",
                                        "=", exterSchemaName, schemaName );
    return( j );
}

int
registerTokenAttrInSchemaByUser( char *resourceName, char *dbSchemeName,
                                 char *tableSpaceName, char *tableName,
                                 char *schemaName, char *clusterName, int attrCount,
                                 char *attrExtrName[], char *attrDataType[],
                                 char *attrIdenType[], char *defaultVal[],
                                 char *attrComments[], char *attrPresentation[],
                                 char *subjects,	char *constraints,
                                 char *user, char *domain, char *attrDimension[] ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( registerTokenAttrInSchema( resourceName, dbSchemeName,
                                       tableSpaceName, tableName,
                                       schemaName, clusterName, attrCount, attrExtrName,
                                       attrDataType, attrIdenType, defaultVal,
                                       attrComments, attrPresentation, subjects,
                                       constraints, user_id, attrDimension ) );

}

int
registerTokenAttrInSchema( char *resourceName, char *dbSchemeName,
                           char *tableSpaceName, char *tableName,
                           char *schemaName, char *clusterName, int attrCount,
                           char *attrName[], char *attrDataType[],
                           char *attrIdenType[], char *defaultVal[],
                           char *attrComments[], char *attrPresentation[],
                           char *subjects,	char *constraints,
                           int  realUserId, char *attrDimension[] ) {
    int i, j, k, tabid, attrid1, attrid2, startk, tokenId, user_id;
    unsigned long int maxsize;
    int  numattrs = 1;
    char *LocattrDataType[MAX_TABLE_COLS];
    char  myAttrPresentation[MAX_TOKEN];
    char idAttrDataType[MAX_TOKEN], idAttrIdenType[MAX_TOKEN];
    /* register a table */

    char databaseName[MAX_TOKEN];
    int  resourceId;

    i = checkforuniqueschemaname( schemaName ) ;
    if ( i != 0 ) {
        return ( i );
    }

    i = checkforownershipofschema( schemaName, realUserId );
    if ( i != 0 ) {
        return ( i );
    }

    i = getDBResourceInfo( resourceName, &resourceId,
                           databaseName );
    if ( i != 0 ) {
        return ( i );
    }

    i = checkforexistsclusterNameinSchemaResourceId( clusterName, schemaName,
            resourceId );
    if ( i != 0 ) {
        return ( i );
    }



    tabid = insert_into_rcore_tables( tableName, databaseName, schemaName,
                                      clusterName,
                                      dbSchemeName, tableSpaceName,
                                      realUserId, resourceId,
                                      0, subjects, attrComments[0], constraints, 0 );
    if ( tabid < 0 ) {
        return( tabid );
    }

    startk = 0;

    if ( strstr( attrDataType[0], "int" ) != NULL ) {
        if ( !strcmp( attrIdenType[0], "SR" ) ||
                !strcmp( attrIdenType[0], "LS" ) ||
                !strcmp( attrIdenType[0], "UR" ) ||
                !strcmp( attrIdenType[0], "ST" ) ||
                !strcmp( attrIdenType[0], "LT" ) ||
                !strcmp( attrIdenType[0], "UT" ) ||
                !strcmp( attrIdenType[0], "US" ) ) {
            strcpy( idAttrIdenType, "ST" );
            maxsize = DEFAULT_INTEGER_SPACE_SIZE;
            strcpy( idAttrDataType, "int" );
        }
        else {
            return ( TOKEN_IDEN_TYPE_ERROR );
        }
    }
    else if ( strstr( attrDataType[0], "char" ) != NULL ) {
        if ( !strcmp( attrIdenType[0], "HR" ) ||
                !strcmp( attrIdenType[0], "HT" ) ) {
            strcpy( idAttrIdenType, "HT" );
            maxsize = DEFAULT_TOKEN_ID_MAXSIZE;
            sprintf( idAttrDataType, "char(%i)", DEFAULT_TOKEN_ID_MAXSIZE );
        }
        else {
            strcpy( idAttrIdenType, attrIdenType[0] );
            strcpy( idAttrDataType, attrDataType[0] );
            maxsize =  getMaxSize( attrDataType[0] );
        }
    }
    else {
        return ( TOKEN_IDEN_TYPE_ERROR );
    }


    tokenId = insert_into_rcore_attributes( schemaName, tabid, attrName[0],
                                            idAttrDataType,
                                            idAttrIdenType, attrName[0],
                                            "",
                                            -1, -1, "MCAT registered token identifier",
                                            "",
                                            1, 0, -1, maxsize, "" );

    if ( tokenId < 0 ) {
        return( tokenId );
    }
    strcat( idAttrDataType, " not null " );
    LocattrDataType[0] = idAttrDataType;




    for ( k = 1; k < attrCount ; k++ ) {
        maxsize = getMaxSize( attrDataType[k] );
        if ( maxsize < 0 ) {
            return( maxsize );
        }
        if ( attrPresentation[k] == NULL || strlen( attrPresentation[k] ) == 0 ) {
            strcpy( myAttrPresentation, "html:in:single_select_list" );
        }
        else {
            strcpy( myAttrPresentation, attrPresentation[k] );
        }
        attrid1 = insert_into_rcore_attributes( schemaName, tabid, attrName[k],
                                                attrDataType[k],
                                                attrIdenType[k], attrName[k],
                                                defaultVal[k],
                                                tokenId, -1, attrComments[k],
                                                myAttrPresentation,
                                                0, 1, -1, maxsize, attrDimension[k] );

        if ( attrid1 < 0 ) {
            return( attrid1 );
        }
    }
    return ( RCAT_SUCCESS );
}



int
addAttrClusterInSchemaByUser( char *schemaName, char *clusterName,
                              int attrCount,
                              char *attrExtrName[], char *attrDataType[],
                              char *attrIdenType[], char *defaultVal[],
                              char *attrComments[], char *attrPresentation[],
                              char *sameas[], int primaryKey[],
                              char *subjects,	char *constraints,
                              char *user, char *domain, char *attrDimension[] ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( addAttrClusterInSchema( schemaName, clusterName,
                                    attrCount, attrExtrName,
                                    attrDataType, attrIdenType, defaultVal,
                                    attrComments, attrPresentation, sameas,
                                    primaryKey, subjects,
                                    constraints, user_id, attrDimension ) );

}


int
addAttrClusterInSchema( char *schemaName, char *clusterName, int attrCount,
                        char *attrExtrName[], char *attrDataType[],
                        char *attrIdenType[], char *defaultVal[],
                        char *attrComments[], char *attrPresentation[],
                        char *sameas[], int primaryKey[],
                        char *subjects,	char *constraints,
                        int realUserId, char *attrDimension[] ) {
    int i, j, k, tabid, attrid1, attrid2, startk, attrTokenId, tabTokenId;
    unsigned long int maxsize;
    int  numattrs = 1;
    char *attrName[MAX_TABLE_COLS];
    char *LocattrDataType[MAX_TABLE_COLS];
    char tableName[MAX_TOKEN];
    char  exterSchemaName[MAX_TOKEN];
    char idAttrDataType[MAX_TOKEN], idAttrIdenType[MAX_TOKEN];
    int tabIdList[15];
    int attrIdList[15];
    char tokenExterAttrName[MAX_TOKEN];
    int expose, exter_attrib_id;
    char presentation[MAX_TOKEN];
    char dimension[MAX_TOKEN];
    char default_value[MAX_TOKEN];

    /* register a table */

    i = checkforuniqueschemaname( schemaName ) ;
    if ( i != 0 ) {
        return ( i );
    }

    i = checkforownershipofschema( schemaName, realUserId );
    if ( i != 0 ) {
        return ( i );
    }
    if ( debugging4 ) {
        printf( "Calling getNewTableName:%s:%s\n", schemaName, clusterName );
        fflush( stdout );
    }

    i = getNewTableName( schemaName, attrIdenType[0],
                         clusterName, tableName );
    if ( i < 0 ) {
        return( i );
    }
    if ( debugging4 ) {
        printf( "Calling insert_into_rcore_tables:%s\n", tableName );
        fflush( stdout );
    }

    tabid = insert_into_rcore_tables( tableName, RCATDBNAME, schemaName,
                                      clusterName,
                                      RCATSCHEMENAME, RCATTABLESPACENAME,
                                      MCATUSERID , RCATRESOURCEID,
                                      0, subjects, attrComments[0], constraints, 0 );

    if ( tabid < 0 ) {
        return( tabid );
    }


    startk = 0;
    strcpy( idAttrIdenType, "SD" );
    maxsize = DEFAULT_INTEGER_SPACE_SIZE;
    strcpy( idAttrDataType, "int" );
    attrName[0] = ( char * )malloc( sizeof( char ) * MAX_TOKEN );
    if ( attrName[0] == NULL ) {
        return ( MEMORY_ALLOCATION_ERROR );
    }
    startk = 1;
    i = getNewAttrName( attrExtrName[0], tableName, attrName[0] );

    if ( i < 0 ) {
        free( attrName[0] );
        return( i );
    }
    if ( strlen( attrName[0] ) < ( MAX_SQL_NAME_LIMIT - 4 ) ) {
        strcat( attrName[0], "_mid" );
    }
    else {
        strcat( ( char * )( &attrName[0] + MAX_SQL_NAME_LIMIT - 4 ),  "_mid" );
    }
    if ( debugging4 ) {
        printf( "Calling insert_into_rcore_attributes:%s\n", attrName[0] );
    }
    fflush( stdout );

    attrid1 = insert_into_rcore_attributes( schemaName, tabid, attrName[0],
                                            idAttrDataType,
                                            idAttrIdenType, attrName[0],
                                            "0",
                                            -1, -1, "MCAT created token identifier",
                                            "",
                                            1, 0, -1, maxsize, "" );

    if ( attrid1 < 0 ) {
        free( attrName[0] );
        return( attrid1 );
    }
    strcat( idAttrDataType, " not null " );
    LocattrDataType[0] = idAttrDataType;




    for ( k = 0; k < attrCount ; k++ ) {


        attrName[k + startk] = ( char * )malloc( sizeof( char ) * MAX_TOKEN );
        if ( attrName[k + startk] == NULL ) {
            return ( MEMORY_ALLOCATION_ERROR );
        }
        LocattrDataType[k + startk] = attrDataType[k];



        if ( !strcmp( attrIdenType[k], "SR" ) ||
                !strcmp( attrIdenType[k], "LS" ) ||
                !strcmp( attrIdenType[k], "HR" ) ||
                !strcmp( attrIdenType[k], "PR" ) ||
                !strcmp( attrIdenType[k], "UR" ) ||
                !strcmp( attrIdenType[k], "FK" ) ) {


            LocattrDataType[k + startk] = ( char * )malloc( sizeof( char ) * MAX_TOKEN );
            if ( LocattrDataType[k + startk] == NULL ) {
                return ( MEMORY_ALLOCATION_ERROR );
            }
            if ( debugging4 ) {
                printf( "Calling getAttrInfoForToken:%s<BR>\n", attrName[k + startk] );
                fflush( stdout );
            }
            j = getAttrInfoForToken( attrExtrName[k], schemaName,
                                     attrName[k + startk],
                                     &attrTokenId, &tabTokenId, exterSchemaName,
                                     LocattrDataType[k + startk], &maxsize,
                                     tokenExterAttrName, &expose,
                                     presentation, default_value,
                                     &exter_attrib_id, dimension );

            if ( j < 0 ) {
                for ( i = 0; i <= ( k + startk ); i++ ) {
                    free( attrName[i] );
                }
                return( j );
            }
            if ( !strcmp( exterSchemaName, schemaName ) ) {
                expose = 0;
            }

            if ( attrPresentation[k] != NULL && strlen( attrPresentation[k] ) != 0 ) {
                strcpy( presentation, attrPresentation[k] );
            }
            if ( attrDimension[k] != NULL && strlen( attrDimension[k] ) != 0 ) {
                strcpy( dimension, attrDimension[k] );
            }
            if ( debugging4 ) {
                printf( "Calling insert_into_rcore_attributes:%s\n", attrName[k + startk] );
                fflush( stdout );
            }
            attrid1 = insert_into_rcore_attributes( schemaName, tabid,
                                                    attrName[k + startk], LocattrDataType[k + startk], "FK",
                                                    tokenExterAttrName, default_value, -1, attrTokenId ,
                                                    attrComments[k], presentation, primaryKey[k],
                                                    expose, exter_attrib_id, maxsize, dimension );

            if ( attrid1 < 0 ) {
                for ( i = 0; i <= ( k + startk ); i++ ) {
                    free( attrName[i] );
                }
                return( attrid1 );
            }
            tabIdList[0] = tabid;
            attrIdList[0] = attrid1;
            tabIdList[1] = tabTokenId;
            attrIdList[1] = attrTokenId;

            j = insert_into_rcore_fk_relations( 1, tabIdList, attrIdList, "", "",
                                                "=", exterSchemaName, schemaName );

            if ( j < 0 ) {
                for ( i = 0; i <= ( k + startk ); i++ ) {
                    free( attrName[i] );
                }
                return( j );
            }
            /*
            insert into rcore_fk_relations values (44,54,11,54,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,'','','=','mcatcore','mcatcore');
            */
        }
        else {


            maxsize = getMaxSize( attrDataType[k] );
            if ( maxsize < 0 ) {
                for ( i = 0; i <= ( k + startk ); i++ ) {
                    free( attrName[i] );
                }
                return( maxsize );
            }

            attrTokenId =  0;
            if ( sameas[k] != NULL && strlen( sameas[k] ) != 0 ) {
                attrTokenId = getAttrIdForName( sameas[k], schemaName );

                if ( attrTokenId < 0 ) {
                    for ( i = 0; i <= ( k + startk ); i++ ) {
                        free( attrName[i] );
                    }
                    return( attrTokenId );
                }
            }
            i = getNewAttrName( attrExtrName[k], tableName, attrName[k + startk] );

            if ( i < 0 ) {
                for ( j = 0; j <= ( k + startk ); j++ ) {
                    free( attrName[j] );
                }
                return( i );
            }
            if ( debugging4 ) {
                printf( "Calling insert_into_rcore_attributes:%s\n", attrName[k + startk] );
                fflush( stdout );
            }
            attrid1 = insert_into_rcore_attributes( schemaName, tabid,
                                                    attrName[k + startk], attrDataType[k], attrIdenType[k],
                                                    attrExtrName[k], defaultVal[k], -1, attrTokenId,
                                                    attrComments[k], attrPresentation[k], primaryKey[k],
                                                    1, -1, maxsize, attrDimension[k] );

            if ( attrid1 < 0 ) {
                for ( i = 0; i <= ( k + startk ); i++ ) {
                    free( attrName[i] );
                }
                return( attrid1 );
            }

        }

    }

    i = createNewTable( attrCount + startk, tableName, attrName,
                        LocattrDataType, "",
                        RCATDBNAME, RCATSCHEMENAME,
                        RCATTABLESPACENAME, MCATUSERID , RCATRESOURCEID );

    for ( k = 0; k < ( attrCount + startk ) ; k++ ) {
        free( attrName[k] );
    }
    return ( i );

}





int
registerClusterInSchemaByUser( char *resourceName, char *dbSchemeName,
                               char *tableSpaceName, char *tableName,
                               char *schemaName,
                               char *clusterName, int attrCount,
                               char *attrExtrName[], char *attrDataType[],
                               char *attrIdenType[], char *defaultVal[],
                               char *attrComments[], char *attrPresentation[],
                               char *sameas[], int primaryKey[],
                               char *subjects,	char *constraints,
                               char *user, char *domain, char *attrDimension[],
                               int expose[] ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( registerClusterInSchema( resourceName, dbSchemeName,
                                     tableSpaceName, tableName, schemaName,
                                     clusterName, attrCount, attrExtrName,
                                     attrDataType, attrIdenType, defaultVal,
                                     attrComments, attrPresentation, sameas,
                                     primaryKey, subjects,
                                     constraints, user_id, attrDimension,
                                     expose ) );

}


int
registerClusterInSchema( char *resourceName, char *dbSchemeName,
                         char *tableSpaceName, char *tableName,
                         char *schemaName,
                         char *clusterName, int attrCount,
                         char *attrExtrName[], char *attrDataType[],
                         char *attrIdenType[], char *defaultVal[],
                         char *attrComments[], char *attrPresentation[],
                         char *sameas[], int primaryKey[],
                         char *subjects,	char *constraints,
                         int realUserId, char *attrDimension[],
                         int expose[] ) {
    int i, j, k, tabid, attrid1, attrid2, startk, attrTokenId, tabTokenId;
    unsigned long int maxsize;
    int  numattrs = 1;
    char *attrName[MAX_TABLE_COLS];
    char *LocattrDataType[MAX_TABLE_COLS];
    char  exterSchemaName[MAX_TOKEN];
    char idAttrDataType[MAX_TOKEN], idAttrIdenType[MAX_TOKEN];
    int tabIdList[15];
    int attrIdList[15];
    char tokenExterAttrName[MAX_TOKEN];
    int exter_attrib_id;
    char presentation[MAX_TOKEN];
    char dimension[MAX_TOKEN];
    char default_value[MAX_TOKEN];

    char databaseName[MAX_TOKEN];
    int  resourceId;

    if ( debugging4 ) {
        printf( "RN=%s:\n", resourceName );
        fflush( stdout );
        printf( "DSN=%s:\n", dbSchemeName );
        fflush( stdout );
        printf( "TSN=%s:\n", tableSpaceName );
        fflush( stdout );
        printf( "TN=%s:\n", tableName );
        fflush( stdout );
        printf( "SN=%s:\n", schemaName );
        fflush( stdout );
        printf( "CN=%s:\n", clusterName );
        fflush( stdout );
        printf( "ACN=%i:\n", attrCount );
        fflush( stdout );
        printf( "AEN0=%s:\n", attrExtrName[0] );
        fflush( stdout );
        printf( "ADT0=%s:\n", attrDataType[0] );
        fflush( stdout );
        printf( "AEN1=%s:\n", attrExtrName[1] );
        fflush( stdout );
        printf( "ADT1=%s:\n", attrDataType[1] );
        fflush( stdout );
        printf( "UI=%i\n", realUserId );
        fflush( stdout );
    }
    /* register a table */
    i = checkforuniqueschemaname( schemaName ) ;
    if ( i != 0 ) {
        return ( i );
    }

    i = checkforownershipofschema( schemaName, realUserId );
    if ( i != 0 ) {
        return ( i );
    }
    if ( debugging4 ) {
        printf( "Calling getDBResourceInfo:%s\n", resourceName );
        fflush( stdout );
    }

    i = getDBResourceInfo( resourceName, &resourceId,
                           databaseName );
    if ( i != 0 ) {
        return ( i );
    }

    i = checkforexistsclusterNameinSchemaResourceId( clusterName, schemaName,
            resourceId );
    if ( i != 0 ) {
        return ( i );
    }


    if ( debugging4 ) {
        printf( "Calling insert_into_rcore_tables:%s\n", tableName );
        fflush( stdout );
    }
    tabid = insert_into_rcore_tables( tableName, databaseName, schemaName,
                                      clusterName,
                                      dbSchemeName, tableSpaceName,
                                      realUserId, resourceId,
                                      0, subjects, attrComments[0], constraints, 0 );


    if ( tabid < 0 ) {
        return( tabid );
    }
    for ( k = 0; k < attrCount ; k++ ) {
        maxsize = getMaxSize( attrDataType[k] );
        if ( maxsize < 0 ) {
            return( maxsize );
        }
        attrTokenId =  0;
        if ( sameas[k] != NULL && strlen( sameas[k] ) != 0 ) {
            attrTokenId = getAttrIdForName( sameas[k], schemaName );
            if ( attrTokenId < 0 ) {
                return( attrTokenId );
            }
        }
        if ( debugging4 ) {
            printf( "Calling insert_into_rcore_attributes:%s\n", attrExtrName[k] );
            fflush( stdout );
        }
        if ( debugging4 ) {
            printf( "Calling insert_into_rcore_attributes:%s\n", attrExtrName[k] );
            fflush( stdout );
        }

        attrid1 = insert_into_rcore_attributes( schemaName, tabid,
                                                attrExtrName[k], attrDataType[k], attrIdenType[k],
                                                attrExtrName[k], defaultVal[k], -1, attrTokenId,
                                                attrComments[k], attrPresentation[k], primaryKey[k],
                                                expose[k], -1, maxsize, attrDimension[k] );

        if ( attrid1 < 0 ) {
            return( attrid1 );
        }
    }
    return( RCAT_SUCCESS );

}


int
createNewSchemaByUser( char *schemaName,
                       char *subjects,	char *comments,
                       char *constraints, char *similarto,
                       char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( createNewSchema( schemaName, subjects, comments,
                             constraints, similarto, user_id ) );

}



int
createNewSchema( char *schema_name,
                 char *subject,
                 char *comments,
                 char *constraints,
                 char *similarto, int user_id ) {
    int i, j;
    char sqlq[HUGE_STRING];
    i = checkforuniqueschemaname( schema_name );
    if ( i == RCAT_SUCCESS ) {
        return ( SCHEMA_NAME_NOT_UNIQUE_IN_CAT );
    }

    i = checkifallowedtocreateschema( user_id );
    if ( i != RCAT_SUCCESS ) {
        return ( ACTION_NOT_ALLOWED_FOR_USER );
    }

    sprintf( sqlq,
             "insert into %s%s (schema_name,subject,comments, \
constraints,similarto, user_id) values ('%s','%s','%s','%s','%s',%i)",
             RCATSCHEMENAME, "RCORE_SCHEMAS",
             schema_name, subject, comments,
             constraints, similarto, user_id );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS )  {
        strcpy( GENERATEDSQL, sqlq );
        return( MCAT_SCHEMA_CREATION_ERROR );
    }

    sprintf( sqlq,
             "insert into %s%s (user_schema_name,comments,parent_schema) \
values ('%s','%s','%s')",
             RCATSCHEMENAME, "RCORE_USER_SCHEMAS",
             schema_name, "original schema", schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( MCAT_SCHEMA_CREATION_ERROR );
    }
    return( RCAT_SUCCESS );

}



int
createVuSchemaByUser( int cnt, int *AttrList, char *par_schema_list,
                      char *schema_name, char *subject, char *comments,
                      char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( createVuSchema( cnt, AttrList, par_schema_list,
                            schema_name, subject, comments, user_id ) );

}
int
createVuSchema( int cnt, int *AttrList, char *par_schema_list,
                char *schema_name, char *subject, char *comments,
                int user_id ) {
    int i, j;
    char sqlq[HUGE_STRING];
    char scList[SMALL_TOKEN][MAX_TOKEN];
    i = checkforuniqueschemaname( schema_name );
    if ( i == RCAT_SUCCESS ) {
        return ( SCHEMA_NAME_NOT_UNIQUE_IN_CAT );
    }


    sprintf( sqlq,
             "insert into %s%s (schema_name,subject,comments, \
constraints,similarto, user_id) values ('%s','%s','%s','','',%i)",
             RCATSCHEMENAME, "RCORE_SCHEMAS",
             schema_name, subject, comments, user_id );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS )  {
        strcpy( GENERATEDSQL, sqlq );
        return( MCAT_SCHEMA_CREATION_ERROR );
    }

    for ( i = 0; i < cnt ; i++ ) {
        sprintf( sqlq,
                 "insert into %s%s (user_schema_name,attr_id, expose) values ('%s', %i, 1)",
                 RCATSCHEMENAME, "RCORE_USCHEMA_ATTR", schema_name, AttrList[i] );

        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res != RCAT_SUCCESS )  {
            strcpy( GENERATEDSQL, sqlq );
            return( MCAT_SCHEMA_CREATION_ERROR );
        }
    }
    j = splitListString( par_schema_list, scList, ',' );
    for ( i = 0; i < j; i++ ) {
        sprintf( sqlq,
                 "insert into %s%s (user_schema_name,comments,parent_schema) \
values ('%s','%s',%s)",
                 RCATSCHEMENAME, "RCORE_USER_SCHEMAS",
                 schema_name, "sub schema", scList[i] );
        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res != RCAT_SUCCESS ) {
            strcpy( GENERATEDSQL, sqlq );
            return( MCAT_SCHEMA_CREATION_ERROR );
        }
    }
    return( RCAT_SUCCESS );
}


int
dropClusterinCoreSchemaByUser( char  *clusterName, char *schema_name,
                               char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( dropClusterinCoreSchema( clusterName, schema_name, user_id ) );
}

int dropClusterinCoreSchema( char  *clusterName,
                             char  *schemaName,
                             int  userId ) {
    return( OPERATION_NOT_IMPLEMENTED );
}

int
dropAttrinCoreSchemaByUser( int AttrId, char *schema_name,
                            char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( dropAttrinCoreSchema( AttrId, schema_name, user_id ) );
}

int
dropAttrinCoreSchema( int AttrId, char *schema_name, int  user_id ) {
    int i, j;
    char sqlq[HUGE_STRING];
    int  colcount, jj;

    i = checkforownershipofschema( schema_name, user_id ) ;
    if ( i != 0 ) {
        return ( i );
    }


    if ( strstr( user, "rods" ) != NULL ) {
        return( INNER_CORE_SCHEMA_CANNOT_BE_DELETED );
    }
    if ( strstr( schema_name, "mcat" ) != NULL ||
            strstr( schema_name, "core" ) != NULL  ||
            strstr( schema_name, "MCAT" ) != NULL  ||
            strstr( schema_name, "CORE" ) != NULL ) {
        return( INNER_CORE_SCHEMA_CANNOT_BE_DELETED );
    }


    sprintf( sqlq,
             "delete from %s%s where  attr_id =%i",
             RCATSCHEMENAME, "RCORE_USCHEMA_ATTR", AttrId );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( DELETE_ATTR_ERROR );
    }
    sprintf( sqlq,
             "delete from %s%s where  attr_id =%i",
             RCATSCHEMENAME, "RCORE_ATTRIBUTES", AttrId );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( DELETE_ATTR_ERROR );
    }
    return( RCAT_SUCCESS );
}

int
dropCoreSchemaByUser( char *schema_name,
                      char *user, char *domain ) {
    int user_id;
    if ( strstr( user, "rods" ) != NULL ) {
        return( INNER_CORE_SCHEMA_CANNOT_BE_DELETED );
    }
    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( dropCoreSchema( schema_name, user_id ) );
}

int
dropCoreSchema( char *schema_name, int user_id ) {
    int i, j;
    char sqlq[HUGE_STRING];
    char tmpStr[MAX_TOKEN];
    int  colcount, jj;
    char  cval[MAX_NO_OF_TABLES][MAX_TOKEN];
    char  cval2[MAX_NO_OF_TABLES][MAX_TOKEN];
    int noTablesFound = 0;


    if ( user_id < 3 ) {
        return( INNER_CORE_SCHEMA_CANNOT_BE_DELETED );
    }


    if ( strstr( schema_name, "mcat" ) != NULL ||
            strstr( schema_name, "core" ) != NULL  ||
            strstr( schema_name, "MCAT" ) != NULL  ||
            strstr( schema_name, "CORE" ) != NULL ) {
        return( INNER_CORE_SCHEMA_CANNOT_BE_DELETED );
    }

    i = checkforownershipofschema( schema_name, user_id );
    if ( i != 0 ) {
        return ( i );
    }


    /*
    TABLES IN THE SCHEMA
    RCORE_USCHEMA_ATTR
    RCORE_USER_SCHEMAS
    RCORE_FK_RELATIONS
    RCORE_ATTRIBUTES
    RCORE_TABLES
    RCORE_SCHEMAS
    */
    sprintf( sqlq, "select dbschema_name, table_name from %s%s where schema_name ='%s'",
             RCATSCHEMENAME, "RCORE_TABLES", schema_name );
    i = get_2ctype_list_from_query( cval, cval2, &colcount, sqlq );
    if ( i != RCAT_SUCCESS ) {
        if ( i == MCAT_NO_DATA_FOUND ) {
            noTablesFound = 1;
        }
        else {
            return( i );
        }
    }
    if ( noTablesFound == 0 ) {
        if ( colcount == 0 ) {
            return ( UNKNOWN_USER_SCHEMASET );
        }
        for ( jj = 0; jj < colcount; jj++ ) {
            sprintf( sqlq, "drop table %s%s ", cval[jj], cval2[jj] );
            res = ExecSqlDb( henv, hdbc, sqlq );
            if ( res != RCAT_SUCCESS ) {
                printf( "Problem with Dropping Table:%s%s\n", cval[jj], cval2[jj] );
            }
        }

        sprintf( sqlq,
                 "delete from %s%s where  user_schema_name ='%s'",
                 RCATSCHEMENAME, "RCORE_USCHEMA_ATTR", schema_name );
        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res != RCAT_SUCCESS ) {
            printf( "Problem with Deleting from RCORE_USCHEMA_ATTR\n" );
            return( DELETE_SCHEMA_ERROR );
        }
    } /*    (noTablesFound == 0)*/
    sprintf( sqlq,
             "delete from %s%s where  user_schema_name ='%s'",
             RCATSCHEMENAME, "RCORE_USER_SCHEMAS", schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        printf( "Problem with Deleting from RCORE_USER_SCHEMAS\n" );
        return( DELETE_SCHEMA_ERROR );
    }

    sprintf( sqlq,
             "delete from %s%s where  parent_schema ='%s'",
             RCATSCHEMENAME, "RCORE_USER_SCHEMAS", schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        printf( "Problem with Deleting from RCORE_USER_SCHEMAS\n" );
        return( DELETE_SCHEMA_ERROR );
    }

    if ( noTablesFound == 0 ) {
        sprintf( sqlq,
                 "delete from %s%s t0 where t0.table_id in \
( select table_id from RCORE_TABLES where schema_name ='%s')",
                 RCATSCHEMENAME, "RCORE_ATTRIBUTES", schema_name );
        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res != RCAT_SUCCESS ) {
            printf( "Problem with Deleting from RCORE_ATTRIBUTES\n" );
            return( DELETE_SCHEMA_ERROR );
        }

        sprintf( sqlq,
                 "delete from %s%s t0 where \
t0.table_id_in1 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_out1 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_in2 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_out2 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_in3 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_out3 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_in4 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_out4 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_in5 in ( select table_id from RCORE_TABLES where schema_name ='%s') \
or t0.table_id_out5 in ( select table_id from RCORE_TABLES where schema_name ='%s') ",
                 RCATSCHEMENAME, "RCORE_FK_RELATIONS", schema_name, schema_name,
                 schema_name, schema_name, schema_name, schema_name, schema_name,
                 schema_name, schema_name, schema_name );
        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res != RCAT_SUCCESS ) {
            printf( "Problem with Deleting from RCORE_FK_RELATIONS\n" );
            return( DELETE_SCHEMA_ERROR );
        }

        sprintf( sqlq,
                 "delete from %s%s  where schema_name ='%s'",
                 RCATSCHEMENAME, "RCORE_TABLES", schema_name );
        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res != RCAT_SUCCESS ) {
            printf( "Problem with Deleting from RCORE_TABLES\n" );
            return( DELETE_SCHEMA_ERROR );
        }
    } /*(noTablesFound == 0) */

    sprintf( sqlq,
             "delete from %s%s where  schema_name ='%s'",
             RCATSCHEMENAME, "RCORE_SCHEMAS", schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        printf( "Problem with Deleting from RCORE_SCHEMAS\n" );
        return( DELETE_SCHEMA_ERROR );
    }

    return( RCAT_SUCCESS );
}



int
dropVuSchemaByUser( char *schema_name,
                    char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( dropVuSchema( schema_name, user_id ) );
}


int
dropVuSchema( char *schema_name, int user_id ) {
    int i, j;
    char sqlq[HUGE_STRING];
    char attrSet[HUGE_STRING];

    i = checkforownershipofschema( schema_name, user_id ) ;
    if ( i != 0 ) {
        return ( i );
    }

    sprintf( sqlq,
             "delete from %s%s where  user_schema_name ='%s'",
             RCATSCHEMENAME, "RCORE_USCHEMA_ATTR", schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        return( DELETE_SCHEMA_ERROR );
    }
    sprintf( sqlq,
             "delete from %s%s where  user_schema_name ='%s'",
             RCATSCHEMENAME, "RCORE_USER_SCHEMAS", schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        return( DELETE_SCHEMA_ERROR );
    }
    sprintf( sqlq,
             "delete from %s%s where  schema_name ='%s'",
             RCATSCHEMENAME, "RCORE_SCHEMAS", schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( DELETE_SCHEMA_ERROR );
    }
    return( RCAT_SUCCESS );
}




int
dropAttrinVuSchemaByUser( int cnt, int *AttrList, char *schema_name,
                          char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( dropAttrinVuSchema( cnt, AttrList, schema_name, user_id ) );
}

int
dropAttrinVuSchema( int cnt, int *AttrList, char *schema_name,
                    int user_id ) {
    int i, j;
    char sqlq[HUGE_STRING];
    char attrSet[HUGE_STRING];


    i = checkforownershipofschema( schema_name, user_id ) ;
    if ( i != 0 ) {
        return ( i );
    }

    sprintf( attrSet, "%i", AttrList[0] );

    for ( i = 1; i < cnt; i++ ) {
        sprintf( &attrSet[strlen( attrSet )], " , %i", AttrList[i] );
    }

    sprintf( sqlq,
             "delete from %s%s where attr_id in ( %s ) and user_schema_name ='%s'",
             RCATSCHEMENAME, "RCORE_USCHEMA_ATTR", attrSet, schema_name );
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( DELETE_ATTR_ERROR );
    }
    return( RCAT_SUCCESS );
}

int
addAttrinVuSchemaByUser( int cnt, int *AttrList, char *schema_name,
                         char *user, char *domain ) {
    int user_id;

    user_id = get_user_id_in_domain( user, domain );
    if ( user_id < 0 ) {
        return ( USER_NOT_IN_DOMN );
    }
    return( addAttrinVuSchema( cnt, AttrList, schema_name, user_id ) );
}
int
addAttrinVuSchema( int cnt, int *AttrList, char *schema_name,
                   int user_id ) {
    int i, j;
    char sqlq[HUGE_STRING];
    char attrSet[HUGE_STRING];

    i = checkforownershipofschema( schema_name, user_id ) ;
    if ( i != 0 ) {
        return ( i );
    }


    for ( i = 0; i < cnt ; i++ ) {
        sprintf( sqlq,
                 "insert into %s%s (user_schema_name,attr_id, expose) values ('%s', %i, 1)",
                 RCATSCHEMENAME, "RCORE_USCHEMA_ATTR", schema_name, AttrList[i] );
        res = ExecSqlDb( henv, hdbc, sqlq );
        if ( res != RCAT_SUCCESS ) {
            strcpy( GENERATEDSQL, sqlq );
            return( INSERT_ATTR_ERROR );
        }
    }
    return( RCAT_SUCCESS );
}


int
getOutDimensions( char *inDim, char outDim[][MAX_TOKEN], int *rowcount ) {
    char sqlq[2 * MAX_TOKEN];
    int  i, j;

    sprintf( sqlq, "select distinct OUT_ATTR_DIMEN from %sRCORE_ATTRDIMN11_FN where IN_ATTR_DIMEN = '%s'", RCATSCHEMENAME, inDim );
    i = get_ctype_list_from_query( outDim, &j, sqlq );
    if ( i != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( i );
    }
    if ( j == 0 ) {
        return ( UNKNOWN_USER_SCHEMASET );
    }
    *rowcount = j;
    return( RCAT_SUCCESS );
}

int
getSchemaNames( char *type, char cval[][], int *rowcount ) {
    char sqlq[HUGE_STRING];
    int  i, j;


    if ( !strcmp( type, "vu" ) ) {
        sprintf( sqlq, "select distinct USER_SCHEMA_NAME from %sRCORE_USER_SCHEMAS where PARENT_SCHEMA <> user_schema_name ", RCATSCHEMENAME );
    }
    else if ( !strcmp( type, "core" ) ) {
        sprintf( sqlq, "select distinct USER_SCHEMA_NAME from %sRCORE_USER_SCHEMAS where PARENT_SCHEMA = user_schema_name ", RCATSCHEMENAME );
    }
    else if ( !strcmp( type, "all" ) ) {
        sprintf( sqlq, "select distinct USER_SCHEMA_NAME from %sRCORE_USER_SCHEMAS", RCATSCHEMENAME );
    }
    else {
        return ( UNKNOWN_USER_SCHEMASET );
    }

    i = get_ctype_list_from_query( cval, &j, sqlq );
    if ( i != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( i );
    }
    if ( j == 0 ) {
        return ( UNKNOWN_USER_SCHEMASET );
    }
    *rowcount = j;

    return( RCAT_SUCCESS );
}


int
getRelatedAttr( char *AttrListStr,
                char AttrOutList[][MAX_TOKEN] ) {
    int i, j, k, jj;
    char sqlq[HUGE_STRING];
    char tmpStr[MAX_TOKEN];
    int  colcount;
    char  cval[MAX_TOKEN][MAX_TOKEN];
    char  cval2[MAX_TOKEN][MAX_TOKEN];
    char  cval3[MAX_TOKEN][MAX_TOKEN];
    char  cval4[MAX_TOKEN][MAX_TOKEN];
    char  cval5[MAX_TOKEN][MAX_TOKEN];



    sprintf( sqlq, "select distinct t0.attr_id, t1.schema_name, t0.EXTERNAL_attr_name, t0.presentation, t0.attr_dimension \
from rcore_attributes t0, rcore_tables t1,  rcore_attributes t2 \
where t2.attr_id in ( %s ) and \
t2.table_id = t0.table_id and t1.table_id = t0.table_id and t0.expose = 1 \
 union \
select  distinct t10.attr_id, t11.schema_name, t10.attr_name, t10.presentation, t10.attr_dimension  \
from rcore_attributes t20, rcore_attributes t22 , rcore_attributes t10, rcore_tables t11, rcore_attributes t12 \
where t22.attr_id in ( %s ) and \
t22.table_id = t20.table_id and t20.attr_iden_type = 'FK' and t12.attr_id = t20.sameas and \
t12.table_id = t10.table_id and t11.table_id = t10.table_id and t10.expose = 1\
 union \
select  distinct t40.attr_id, t41.schema_name, t40.attr_name,t40.presentation, t40.attr_dimension \
from rcore_attributes t60, rcore_attributes t62 , rcore_attributes t50, rcore_attributes t52 , rcore_attributes t40, rcore_tables t41, rcore_attributes t42 \
where t52.attr_id in ( %s ) and \
t52.table_id = t50.table_id and t50.attr_iden_type = 'FK' and t62.attr_id = t50.sameas and \
t62.table_id = t60.table_id and t60.attr_iden_type = 'FK' and t42.attr_id = t60.sameas and \
t42.table_id = t40.table_id and t41.table_id = t40.table_id and t40.expose = 1",
             AttrListStr, AttrListStr, AttrListStr );

    i = get_5ctype_list_from_query( cval, cval2, cval3, cval4, cval5, &colcount, sqlq );
    if ( i != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( i );
    }
    if ( colcount == 0 ) {
        return ( ATTRIBUTE_SET_EMPTY );
    }
    for ( jj = 0; jj < colcount; jj++ ) {
        sprintf( AttrOutList[jj], "%s#%s.%s###%s####%s",
                 cval[jj], cval2[jj], cval3[jj], cval4[jj], cval5[jj] );
    }
    return( colcount );

}


int
getStylesForSchema( char *schemaset, char **styleName,
                    int *rowcount ) {
    char sqlq[HUGE_STRING];
    int  i, j;

    sprintf( sqlq, "select distinct STYLE_NAME from %sRCORE_SCHEMA_STYLES where SCHEMA_NAME in %s ", RCATSCHEMENAME, schemaset );

    i = get_ctype_list_from_query( styleName, &j, sqlq );

    if ( i != RCAT_SUCCESS ) {
        strcpy( GENERATEDSQL, sqlq );
        return( i );
    }
    /*printf("GG:%s<BR>\n",sqlq);
      printf("RR:%i:%s:%s:%s<BR>", styleName[0],styleName[1],styleName[2]);
    */

    if ( j == 0 ) {
        return ( MCAT_NO_DATA_FOUND );
    }
    *rowcount = j;

    return( RCAT_SUCCESS );
}

int
getProcForStyle( char *styleName, char *schemaName, char *styleProc ) {
    char sqlq[MAX_TOKEN];

    failure = 0;
    sprintf( sqlq, "select distinct STYLE_PROC from %sRCORE_SCHEMA_STYLES where SCHEMA_NAME = %s  and STYLE_NAME = '%s' ",
             RCATSCHEMENAME,  schemaName, styleName );

    get_ctype_value_from_query( styleProc, sqlq );
    return( failure );

}



int
getRelatedContexts( char *idList, char *context ) {
    char  cval[MAX_TOKEN][MAX_TOKEN];
    int i, jj;
    char sqlq[HUGE_STRING];
    char tmpStr[MAX_TOKEN];
    int  colcount;

    sprintf( sqlq, "select distinct t0.schema_name from %srcore_tables t0, %srcore_attributes t1 where t1.attr_id in (%s) and t0.table_id = t1.table_id",
             RCATSCHEMENAME, RCATSCHEMENAME, idList );
    i = get_ctype_list_from_query( cval, &colcount, sqlq );
    if ( i != RCAT_SUCCESS ) {
        return( i );
    }
    if ( colcount == 0 ) {
        return ( UNKNOWN_USER_SCHEMASET );
    }
    strcpy( context, "" );
    for ( jj = 0; jj < colcount; jj++ ) {
        if ( jj == 0 ) {
            sprintf( tmpStr, " '%s'", cval[jj] );
        }
        else {
            sprintf( tmpStr, ", '%s'", cval[jj] );
        }
        strcat( context, tmpStr );
    }
    return( RCAT_SUCCESS );
}

int
getTabIdFromDBSchemaTabName( char *dbSchema, char *tableName ) {
    int i;
    char sqlq[HUGE_STRING];

    sprintf( sqlq, "select table_id from %srcore_tables where DBSCHEMA_NAME = '%s' and TABLE_NAME ='%s'", RCATSCHEMENAME, dbSchema, tableName );

    i = get_itype_value_from_query( sqlq );
    if ( i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
    }
    return( i );
}

int
getAttrIdFromExterAttrNameInCD( char *attrName,
                                rcat_metadata_attr_entry *AttrEntries,
                                int countAttrs ) {
    int i;
    for ( i = 0; i < countAttrs ; i++ ) {
        if ( !strcmp( AttrEntries[i].ext_attr_name, attrName ) ) {
            return( AttrEntries[i].attr_id );
        }
    }
    return( UNKNOWN_ATTRIBUTE_NAME );
}
int
getExternalAttrName( char *extAttrName,
                     int attrId,
                     rcat_metadata_attr_entry *AttrEntries,
                     int countAttrs ) {
    int i;

    for ( i = 0 ; i < countAttrs; i++ ) {
        if ( AttrEntries[i].attr_id == attrId ) {
            strcpy( extAttrName, AttrEntries[i].ext_attr_name );
            return( 0 );
        }
    }
    return ( UNKNOWN_ATTRIBUTE_ID );

}



int
getAttrIdFromTabIdAttrName( int tableId, char *AttrName ) {
    int i;
    char sqlq[HUGE_STRING];

    sprintf( sqlq, "select attr_id from %srcore_attributes where TABLE_ID  = %i and ATTR_NAME ='%s'", RCATSCHEMENAME, tableId, AttrName );

    i = get_itype_value_from_query( sqlq );
    if ( i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
    }
    return( i );
}

int
getAttrIdFromTabIdExterAttrName( int tableId, char *AttrName ) {
    int i;
    char sqlq[HUGE_STRING];

    sprintf( sqlq, "select attr_id from %srcore_attributes where TABLE_ID  = %i and EXTERNAL_ATTR_NAME ='%s'", RCATSCHEMENAME, tableId, AttrName );

    i = get_itype_value_from_query( sqlq );
    if ( i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
    }
    return( i );
}

int
getTabIdFromSchemaCluster( char * schemaName, char *clusterName ) {
    int i;
    char sqlq[HUGE_STRING];

    sprintf( sqlq, "select table_id from %srcore_tables where SCHEMA_NAME = '%s' and CLUSTER_NAME ='%s'", RCATSCHEMENAME, schemaName, clusterName );

    i = get_itype_value_from_query( sqlq );
    if ( i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
    }
    return( i );
}

int
getSchemaNameFromTabId( char *schemaName, int tableId ) {
    int i;
    char sqlq[HUGE_STRING];
    char cval[2][MAX_TOKEN];
    int colcount;

    sprintf( sqlq, "select schema_name  from %srcore_tables where TABLE_ID  = %i ", RCATSCHEMENAME, tableId );

    colcount = 1;
    i = get_row_from_query( cval, &colcount, sqlq );
    if ( i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
    }
    if ( i == 0 ) {
        strcpy( schemaName, cval[0] );
    }
    return( i );
}

int
checkforexistsclusterNameinSchemaResourceId( char *clusterName,
        char *schemaName, int resourceId ) {
    int i;

    sprintf( sqlq, "select cluster_name from %sRCORE_TABLES  where \
schema_name =  '%s' and cluster_name = '%s' and rsrc_id = %i",
             RCATSCHEMENAME, schemaName, clusterName, resourceId );
    strcpy( GENERATEDSQL, sqlq );
    if ( check_exists( sqlq ) != 0 ) {
        return ( RCAT_SUCCESS );
    }

    return( CLUSTER_SCHEMA_EXISTS );

}

int
getDBResourceInfo( char *resourceName,
                   int *resourceId,
                   char *databaseName ) {
    int i, j, k, jj;
    char sqlq[HUGE_STRING];
    char tmpStr[MAX_TOKEN];
    int  colcount;
    char  cval[MAX_TOKEN][MAX_TOKEN];

    if ( strlen( resourceName ) == 0 || !strcmp( resourceName, RCATRESOURCE ) ) {
        *resourceId = RCATRESOURCEID;
        strcpy( databaseName, RCATDBNAME );
        return( RCAT_SUCCESS );
    }

    sprintf( sqlq, "select distinct t0.rsrc_id, t0.database_name from %s rcore_ar_db_rsrc t0 where t0.rsrc_name = '%s'",
             databaseName, resourceName ); // JMC cppcheck - missing first parameter ( presumed db name ? )
    colcount = 2;
    i = get_row_from_query( cval, &colcount, sqlq );
    if ( failure != 0 || i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( RESOURCE_NOT_IN_CAT );
    }
    *resourceId = atoi( cval[0] );
    strcpy( databaseName, cval[1] );
    return( RCAT_SUCCESS );

}

int
registerDbResourceInfo( char *resourceName,
                        char *databaseName, char *dbLinkName,
                        char *registrarName, char *registrarDomain,
                        char *registrarPassword,
                        char *userName, char *userPass,
                        char *connectString ) {
    char sqlq[HUGE_STRING];
    int i;
    int  colcount;
    char  cval[MAX_TOKEN][MAX_TOKEN];

    strcpy( reg_registrar_name, registrarName );
    strcpy( reg_obj_name, resourceName );
    strcpy( reg_obj_code, "RCORE_CD_RSRC" );
    strcpy( reg_action_name, "create resource" );
    reg_obj_repl_enum = 0;
    failure = 0;

    i = web_rcore_sys_authorization( registrarName, registrarPassword,
                                     registrarDomain );
    if ( i != 0 ) {
        sprintf( sqlq,
                 "resource registry SYSAPI: Failed Password:%s,%s\n",
                 registrarPassword, registrarName );
        error_exit( query_string );
        return( SYS_USER_AUTHORIZATION_ERROR );
    }

    sprintf( sqlq, "select distinct t0.rsrc_id, t0.rsrc_typ_id from %srcore_ar_repl t0 where t0.rsrc_name = '%s'", RCATSCHEMENAME,
             resourceName );
    colcount = 2;
    i = get_row_from_query( cval, &colcount, sqlq );
    if ( failure != 0 || i < 0 ) {
        strcpy( GENERATEDSQL, sqlq );
        return( RESOURCE_NOT_IN_CAT );
    }
    sprintf( sqlq,
             "insert into %s%s (rsrc_id,repl_enum,rsrc_name,rsrc_typ_id,rsrc_database_name,db_link_name,db_user_name,db_pass,db_connect_string) values (%i,%i,'%s','%s','%s','%s','%s','%s','%s')",
             RCATSCHEMENAME, "RCORE_AR_DB_RSRC",
             atoi( cval[0] ), 0, resourceName, cval[1], databaseName,
             dbLinkName, userName, userPass, connectString );
    /*printf("rR1:%s\n", sqlq);*/
    res = ExecSqlDb( henv, hdbc, sqlq );
    if ( res != 0 ) {
        error_exit( "user registry API: CD resource insertion error" );
        strcpy( GENERATEDSQL, sqlq );
        return( RCORE_AR_DB_RSRC_INSERTION_ERROR );
    }

    nice_exit( "resource registered by SYSAPI" );
    return( RCAT_SUCCESS );
}






int
checkifallowedtocreateschema( int user_id ) {
    return( RCAT_SUCCESS );
}
int
checkAccessMetaDataAttr( int  real_user_id,
                         int *attr_id,
                         int  attr_cnt ) {
    return( RCAT_SUCCESS );
}



/*
typedef struct {
   int attr_id;
   int tab_id;
   char *schema_name;
   char *dbschema_name;
   char *database_name;
   char *table_name;
   char *attr_name;
   char *ext_attr_name;
   char *default_value;
   char *attr_data_type;
   char *attr_iden_type;
   char *presentation;
   int   attr_val_type;
   int   sameas;
   int   maxsize;
   int   primary_key;
   int   expose;
   int   exter_attrib_id;
   int   rsrc_id;
   char *attr_dimension;
   char *at_comments;
   char *cluster_name;
} rcat_metadata_attr_entry;


  sprintf(sqlq, "select attr_id, schema_name, dbschema_name, database_name, table_name, attr_name, external_attr_name, attr_data_type, attr_iden_type, attr_val_type, t0.table_id,  presentation,  default_value, t0.sameas, t0.maxsize from %srcore_attributes t0,  %srcore_tables t1 where t0.table_id = t1.table_id and t1.schema_name in (%s) order by 2,1",

mcatContext svrMcatContext[MAX_NO_OF_MCAT_CONTEXTS];
mcatContextDesc
*/

#endif

