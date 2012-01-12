/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* phpTest.c - test the high level api */

#include "rodsClient.h" 
#include <php_embed.h>
#include <php.h>
#include <SAPI.h>
#include <php_main.h>
#include <php_variables.h>
#include <php_ini.h>
#include <zend_ini.h>
#include <signal.h>

int 
php_seek_file_begin(zend_file_handle *file_handle, char *script_file, 
int *lineno TSRMLS_DC);
int
execPhpScript (char *scrFile, int scrArgc, char **scrArgv);
int
phpShutdown ();

int
main(int argc, char **argv)
{
    char *scrFile;
    int scrArgc;
    int status;
    int numLoop;
    int i;

    if (argc < 3) {
	fprintf (stderr, "usage: phptest n scriptFile argv ...\n");
	exit (1);
    }

    numLoop = atoi (argv[1]);
    if (numLoop <=0 || numLoop > 10000) {
        fprintf (stderr, "usage: phptest n scriptFile argv ...\n");
        exit (1);
    }

    scrFile = argv[2];
    /* the count includes the script file */
    scrArgc = argc - 2;
    static char *myargv[2] = {"irodsPhp", NULL};
    if (php_embed_init(1, myargv PTSRMLS_CC) == FAILURE) {
	fprintf (stderr, "php_embed_init failed\n");
	exit (1);
    }

    php_embed_module.executable_location = argv[0];
#if 0
    if (php_embed_module.startup(&php_embed_module)==FAILURE) {
        fprintf (stderr, "startup failed\n");
        exit (1);
    }
#endif

    for (i = 0; i < numLoop; i++) {
        status = execPhpScript (scrFile, scrArgc, &argv[2]);
        if (status < 0) {
            fprintf (stderr, "execPhpScript of %s error, status = %d\n",
	      argv[1], status);
	    break;
        }
    }

    phpShutdown ();

    exit (status);
}

int
execPhpScript (char *scrFile, int scrArgc, char **scrArgv)
{ 
    zend_file_handle file_handle;
    int status = 0;
    int lineno = 0;

    if ((status = php_seek_file_begin(&file_handle, scrFile, &lineno TSRMLS_CC))
       < 0) { 
	rodsLog (LOG_ERROR,
	  "execPhpScript: php_seek_file_begin error for %s", scrFile);
	return (status);
    }

    file_handle.type = ZEND_HANDLE_FP;
    file_handle.opened_path = NULL;
    file_handle.free_filename = 0;
    SG(request_info).argc = scrArgc;

    SG(request_info).path_translated = file_handle.filename;
    SG(request_info).argv = scrArgv;

    if (php_request_startup(TSRMLS_C)==FAILURE) {
        rodsLog (LOG_ERROR, 
          "execPhpScript: php_request_startup error for %s", scrFile);
        fclose(file_handle.handle.fp);
        return (PHP_REQUEST_STARTUP_ERR);
    }
    CG(start_lineno) = lineno;
    zend_is_auto_global("_SERVER", sizeof("_SERVER")-1 TSRMLS_CC);
    PG(during_request_startup) = 0;
    php_execute_script (&file_handle TSRMLS_CC);
    status = EG(exit_status);

    if (status != SUCCESS) {
        rodsLog (LOG_ERROR,
          "execPhpScript: php_execute_script error for %s", scrFile);
	status = PHP_EXEC_SCRIPT_ERR;
    }

    php_request_shutdown ((void *) 0);

    return (status);
}

int
phpShutdown ()
{
    if (php_embed_module.php_ini_path_override) {
        free(php_embed_module.php_ini_path_override);
    }

    if (php_embed_module.ini_entries) {
        free(php_embed_module.ini_entries);
    }

    php_module_shutdown(TSRMLS_C);
    sapi_shutdown();

    return (0);
}

/* {{{ php_seek_file_begin
 */
int 
php_seek_file_begin(zend_file_handle *file_handle, char *script_file, 
int *lineno TSRMLS_DC)
{
    int c;

    *lineno = 1;

    if (!(file_handle->handle.fp = VCWD_FOPEN(script_file, "rb"))) {
        rodsLog (LOG_ERROR,
          "php_seek_file_begin: VCWD_FOPEN error for %s", script_file);
        return PHP_OPEN_SCRIPT_FILE_ERR - errno;
    }
    file_handle->filename = script_file;
    /* #!php support */
    c = fgetc(file_handle->handle.fp);
    if (c == '#') {
        while (c != '\n' && c != '\r') {
            c = fgetc(file_handle->handle.fp);  /* skip to end of line */
        }
        /* handle situations where line is terminated by \r\n */
        if (c == '\r') {
            if (fgetc(file_handle->handle.fp) != '\n') {
                long pos = ftell(file_handle->handle.fp);
                fseek(file_handle->handle.fp, pos - 1, SEEK_SET);
            }
        }
        *lineno = 2;
    } else {
        rewind(file_handle->handle.fp);
    }
    return 0;
}

