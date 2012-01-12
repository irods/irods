#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 This C program does a subset of m2icat.pl, the data-objects.
 m2icat.pl can take a long time to generate the SQL for ingesting
 data-objects (5 days for 6 million) so instead you can use this.
 Initial testing indicates that it is many thousands of times faster.

 This is still under development and, very likely, has some bugs.  So
 you will need to work with it, verify the SQL, and help us make it
 work properly.

 In m2icat.pl, we usually set a flag ($bypass_time_for_speed) to use
 the current time for time values (instead of the actual create time
 and access time) so it won't be extremely slow.  In this tho, we can
 imbed some C code to do the conversions so those are included by
 default.

 To use this:
 1) comment out  processLogFile($logData); in m2icat.pl before you run that.
 2) set the values below (see the comments).
 3) build with 'gcc -g m2icatd.c'
 4) run with 'a.out Spull.log.data outFile'.  The outFile (or whatever
    you call it) will then contain the SQL needed to ingest the
    data-objects.

 Note that this C program does not determine the datatypes that are
 being used, so you may need to set those by hand.  We can add code to
 do that, if needed.

 */


/* ----------------------------------------------
   Beginning of user settable of configuration options
   Please check and change the options below:
   ---------------------------------------------- */
char cv_srb_zone[] = "galvin";       /* Your SRB zone name */
char cv_irods_zone[] = "tempZone";   /* Your iRODS zone name */

/* You need to set these resource names to use before compiling, the
   srb resources (only data-objects on these resources will be
   processed) and the corresponding irods resource name: */
char *cv_srb_resources[]={"sdsc-mda18-fs","silas"};
char *cv_irods_resources[]={"fs1", "i-silas"};
int nResc=2;   /* set this to the number of resources in the above arrays */


/* Define the type of ICAT DB you have: */
#define CAT_POSTGRES 1
/*#define CAT_ORACLE 1 */
/*#define CAT_MYSQL 1 */

/* Special usernames to convert: */
char *cv_srb_usernames[]={"srbAdmin", "vidarch"}; /* username conversion table*/
                                                  /* srb form */
char *cv_irods_usernames[]={"rods", "nvidar"};    /* irods form for */
                                                  /* corresponding names. */
int nUserNames=2;                      /* number of names in the above table */
char *cv_srb_userdomains[]={"sils"};   /* Used for user.domain situations */
int nUserDomains=1;                    /* number of items in above table */

/* ----------------------------------------------
   End of configuration options
   ---------------------------------------------- */

#ifdef CAT_POSTGRES
char nextValStr[]="select nextval('R_ObjectID')"; /* Postgres format */
char currValStr[]="select currval('R_ObjectID')"; /* Postgres format */
#endif
#ifdef CAT_ORACLE
char nextValStr[]="R_ObjectID.nextval"; /* Oracle format */
char currValStr[]="R_ObjectID.currval"; /*/ Oracle format */
#endif
#ifdef CAT_MYSQL
char nextValStr[]="%s_nextval()"; /* MySQL format */
char currValStr[]="%s_currval()"; /* MySQL format */
#endif


/* The following value is May 6 2010 but gets overwritten with the
 * current time*/
char nowTime[]="01273184051";

/* Indexes to the fields in the input file: */
int g_data_name_ix=-1;
int g_resc_name_ix=-1;
int g_data_type_name_ix=-1;
int g_path_name_ix=-1;
int g_size_ix=-1;
int g_data_owner_ix=-1;
int g_data_owner_domain_ix=-1;
int g_data_create_timestamp_ix=-1;
int g_data_last_access_timestamp_ix=-1;
int g_container_repl_enum_ix=-1;
int g_collection_cont_name_ix=-1;
int g_max_ix=-1;

void
getNowStr(char *timeStr);

void
setNowTime() {
   getNowStr(nowTime);
   printf("Current time as iRODS integer time: %s\n", nowTime);
}

int
checkDateFormat(char *s);

int
findIndex(char *inLine, char *inItem) {
   char *cp1, *cp2;
   char matchStr[200];
   int ix;
   strncpy(matchStr, inItem, sizeof(matchStr));
   matchStr[ sizeof( matchStr )-1 ] = '\0'; // JMC cppcheck - dangerous use of strncpy
   strncat(matchStr, "|", sizeof(matchStr));
   cp1 = strstr(inLine, matchStr);
   if (cp1==NULL) {
      printf("Needed item %s not found, exiting\n",inItem);
      exit(-5);
   }
   ix=0;
   for (cp2=inLine;cp2<cp1;cp2++) {
      if (*cp2=='|') ix++;
   }
   printf("%d %s\n",ix,inItem);
   if (ix > g_max_ix) g_max_ix=ix;
   return(ix);
}

void
setGlobalIndexes(char *inLine) {
   g_max_ix=-1;
   g_data_name_ix=findIndex(inLine,"DATA_NAME");
   g_resc_name_ix=findIndex(inLine,"RSRC_NAME");
   g_path_name_ix=findIndex(inLine,"PATH_NAME");
   g_data_type_name_ix=findIndex(inLine,"DATA_TYP_NAME");
   g_size_ix=findIndex(inLine,"SIZE");
   g_data_owner_ix=findIndex(inLine,"DATA_OWNER");
   g_data_owner_domain_ix=findIndex(inLine,"DATA_OWNER_DOMAIN");
   g_data_create_timestamp_ix=findIndex(inLine,"DATA_CREATE_TIMESTAMP");
   g_data_last_access_timestamp_ix=
      findIndex(inLine,"DATA_LAST_ACCESS_TIMESTAMP");
   g_container_repl_enum_ix=findIndex(inLine,"CONTAINER_REPL_ENUM");
   g_collection_cont_name_ix=findIndex(inLine,"COLLECTION_CONT_NAME");
}

/* Check if we should handle this collection */
int
checkDoCollection(char *inColl) {
   char testColl[2000];
   if (strstr(inColl, "/container")==inColl) {
      return(0); /* don't convert those starting with /container */
   }
   if (strstr(inColl, "/home/")==inColl) {
      return(1); /* do convert /home/* collections */
   }

   testColl[0]='/';
   testColl[1]='\0';
   strcat(testColl, cv_srb_zone);
   strcat(testColl, "/container");
   if (strstr(inColl, testColl)==inColl) {
      return(0); /* don't convert these */
   }

   testColl[0]='/';
   testColl[1]='\0';
   strcat(testColl, cv_srb_zone);
   strcat(testColl, "/trash");
   if (strstr(inColl, testColl)==inColl) {
      return(1); /*  convert these */
   }
   testColl[0]='/';
   testColl[1]='\0';
   strcat(testColl, cv_srb_zone);
   strcat(testColl, "/");
   if (strstr(inColl, testColl)==inColl) {
      char *cp2, *cp3;
      cp2 = rindex(inColl, '/');
      cp3 = inColl + strlen(testColl);
      if (cp2 < cp3) {
	 /* and there are no additional subdirectories */
	 return(0); 
      }
   }
   testColl[0]='/';
   testColl[1]='\0';
   strcat(testColl, cv_srb_zone);
   strcat(testColl, "/");
   if (strstr(inColl, testColl)!=inColl) {
      /* if it doesn't start w /zone/ */
      return(0);       /* skip it */
   }
   return(1);
}

char * 
convertUser(char *inUser, char *inDomain) {
   int i;
   static char newUserName[100];
   for (i=0;i<nUserNames;i++) {
      if (strcmp(cv_srb_usernames[i],inUser)==0) {
	 return(cv_irods_usernames[i]);
      }
   }
   strcpy(newUserName, inUser);
   strcat(newUserName, "#");
   strcat(newUserName, inDomain);
   return(newUserName);
}

char *
convertTime(char *inTime) {
   static char myTime[50];
   int status;
/*   printf("inTime=%s\n", inTime); */
   strncpy(myTime, inTime, sizeof(myTime));
   if (myTime[10]=='-') {
      myTime[10]='.';
   }
   if (myTime[13]=='.') {
      myTime[13]=':';
   }
   if (myTime[16]=='.') {
      myTime[16]=':';
   }
/*   printf("myTime=%s\n", myTime); */
   status=checkDateFormat(myTime);
   if (status) {
      printf("convertTime checkDateFormat error");
   }
/*   printf("Converted to local iRODS integer time: %s\n", myTime); */
   return(myTime);
}

char *
convertCollection(char *inColl) {
   static char outColl[2000];
   char outColl2[2000];
   char testStr[50];
   char *cp;
   int i,j;
   strncpy(outColl, inColl, sizeof(outColl));

   if (strstr(inColl, "/home/")==inColl) {
      strcpy(outColl, cv_irods_zone);
      strcat(outColl, inColl);
   }

   strcpy(testStr, "/");
   strcat(testStr, cv_srb_zone);
   strcat(testStr, "/");
   cp=strstr(outColl, testStr);
   if (cp != NULL) {
      int count;
      outColl2[0]='\0';
      count = cp-(char*)&outColl;
      strncpy(outColl2, outColl, count);
      outColl2[count]='\0';
      strcat(outColl2, "/");
      strcat(outColl2, cv_irods_zone);
      strcat(outColl2, cp+strlen(cv_srb_zone)+1);
      strcpy(outColl, outColl2);
   }
   for (i=0;i<nUserDomains;i++) {
      for (j=0;j<nUserNames;j++) {
	 strcpy(testStr, "/");
	 strcat(testStr, cv_srb_usernames[j]);
	 strcat(testStr, ".");
	 strcat(testStr, cv_srb_userdomains[i]);
	 cp=strstr(outColl, testStr);
	 if (cp != NULL) {
	    int count;
	    outColl2[0]='\0';
	    count = cp-(char*)&outColl;
	    strncpy(outColl2, outColl, count);
	    outColl2[count]='\0';
	    strcat(outColl2, "/");
	    strcat(outColl2, cv_irods_usernames[j]);
	    strcat(outColl2, cp+strlen(testStr));
	    strcpy(outColl, outColl2);
	 }
      }
   }
/*   printf("inColl=%s outColl=%s\n",inColl,outColl); */
   return(outColl);
}

int
main(argc, argv)
int argc;
char **argv;
{
    FILE *FI, *FO;
    int wval;
    char *rchar;
    char buf[1024];
    int nInLines=0;
    int nOutItems=0;
    int i;
    char *ixFTL[50];  /* Indexes For This Line */
    int maxIxFTL=50;
    int nIxFTL=0;
    int doDataInsert;
    char *v_collection;
    char *v_create_time;
    char *v_access_time;
    char *v_owner;
    char *v_owner_domain;
    char *newResource;
 
    setNowTime();

    if (argc < 3) {
      printf("a.out file-in file-out\n");
      return(-1);
    }

    FI = fopen(argv[1],"r");
    if (FI==0)
    {
        fprintf(stderr,"can't open input file %s\n",argv[1]);
        return(-2);
    }

    FO = fopen(argv[2],"w");
    if (FO==0)
    {
        fprintf(stderr,"can't open output file %s\n",argv[2]);
		fclose( FI ); // JMC cppcheck - resource
        return(-3);
    }

    memset(&buf, 0, (size_t)sizeof(buf));
    rchar='\0';
    do  {
       rchar = fgets(&buf[0], sizeof(buf), FI);
       if (rchar=='\0') {
	  break;
       }
       nInLines++;
       if (nInLines==1) {
	  if (strstr(buf, "GET_CHANGED_DATA_CORE_INFO")==0) {
	     printf("This program only handles Spull.log.data type files.\n");
	     return(-4);
	  }
       }
       if (nInLines==2) {
	  setGlobalIndexes(buf);
       }
       if (nInLines > 2) {
	  for (i=0;i<maxIxFTL;i++) {
	     ixFTL[i]=0;
	  }
	  ixFTL[0]=&buf[0];
	  nIxFTL=1;
	  for (i=0;i<sizeof(buf);i++) {
	     if (buf[i]=='\0') break;
	     if (buf[i]=='\n') {
		buf[i]='\0';
		break;
	     }
	     if (buf[i]=='|') {
		buf[i]='\0';
		ixFTL[nIxFTL++]=&buf[i+1];
	     }
	  }

	  if (nIxFTL < g_max_ix) {
	     printf("Missing item(s) from line %d, exiting\n", nInLines);
	     exit(-6);
	  }
/*	  printf("g_resc_name_ix=%d\n",g_resc_name_ix); */
/*	  printf("Resc:%s:\n",ixFTL[g_resc_name_ix]); */

	  doDataInsert=0;
	  for (i=0;i<nResc;i++) {
	     if (strcmp(cv_srb_resources[i],
			ixFTL[g_resc_name_ix])==0) {
		newResource=cv_irods_resources[i];
		doDataInsert=1;
	     }
	  }
	  if (doDataInsert==0) {
	     printf("Not inserting data-object on resource %s\n",
		    ixFTL[g_resc_name_ix]);
	  }
/*	wval = fprintf(FO, "%s",buf); */

/* Perl version has a begin and commit around these, but now I'm
 thinking it is better not to do that and just do a commit at the
 end. */
	  if (doDataInsert) {
	     if (checkDoCollection(ixFTL[g_collection_cont_name_ix])==1) {
		
		v_collection = convertCollection(
		   ixFTL[g_collection_cont_name_ix]);
		v_create_time = convertTime(
		   ixFTL[g_data_create_timestamp_ix]);
		v_access_time = convertTime(
		   ixFTL[g_data_last_access_timestamp_ix]);
		v_owner_domain = ixFTL[g_data_owner_domain_ix];
		v_owner = convertUser(ixFTL[g_data_owner_ix],
				      v_owner_domain);

		wval = fprintf(FO, "insert into R_DATA_MAIN (data_id, coll_id, data_name, data_repl_num, data_version, data_type_name, data_size, resc_name, data_path, data_owner_name, data_owner_zone, data_is_dirty, create_ts, modify_ts) values ((%s), (select coll_id from R_COLL_MAIN where coll_name ='%s'), '%s', '%s', ' ', '%s', '%s', '%s', '%s', '%s', '%s', '1', '%s', '%s');\n",
			       nextValStr,
			       v_collection,
			       ixFTL[g_data_name_ix],
			       ixFTL[g_container_repl_enum_ix],
			       ixFTL[g_data_type_name_ix], 
			       ixFTL[g_size_ix],
			       newResource,
			       ixFTL[g_path_name_ix],
			       v_owner,
			       cv_irods_zone,
			       v_create_time,
			       v_access_time
		   );

		wval = fprintf(FO, 
			       "insert into R_OBJT_ACCESS ( object_id, user_id, access_type_id , create_ts,  modify_ts) values ( (%s), (select user_id from R_USER_MAIN where user_name = '%s'), '1200', '%s', '%s');\n",
			       currValStr,
			       v_owner,
			       nowTime,
			       nowTime);

		if (wval < 0) {
		   perror("fwriting data:");
		}
		nOutItems++;
	     }
	  }
       }
    } while (rchar!='\0');
    if (nOutItems > 0) {
       wval = fprintf(FO, "commit;\n");
    }

    printf("Processed %d input lines\n",nInLines);
    printf("Wrote output lines for %d items\n",nOutItems);
    fclose(FI);
    fclose(FO);
    return(0);
}


#define DATE_FORMAT_ERR -10
#define TIME_LEN        32
typedef long long rodsLong_t;
/*
 The following functions copied from rcMisc.c and have minor
 modifications to easily run as part of this program.
 */

/*
 Return an integer string of the current time in the Unix Time
 format (integer seconds since 1970).  This is the same
 in all timezones (sort of CUT) and is converted to local time
 for display.
 */
void
getNowStr(char *timeStr) 
{
    time_t myTime;

    myTime = time(NULL);
    snprintf(timeStr, 15, "%011d", (uint) myTime);
}

int
isInteger (char *inStr)
{
    int i;
    int len;

    len = strlen(inStr);
    /* see if it is all digit */
    for (i = 0; i < len; i++) {
        if (!isdigit(inStr[i])) {
	    return (0);
       }
    }
    return (1);
}

int
localToUnixTime (char *localTime, char *unixTime)
{
    time_t myTime;
    struct tm *mytm;
    time_t newTime;
    char s[TIME_LEN];

    myTime = time(NULL);
    mytm = localtime (&myTime);

    strncpy(s,localTime, TIME_LEN);

    s[19] = '\0';
    mytm->tm_sec = atoi(&s[17]);
    s[16] = '\0';
    mytm->tm_min = atoi(&s[14]);
    s[13] = '\0';
    mytm->tm_hour = atoi(&s[11]);
    s[10] = '\0';
    mytm->tm_mday = atoi(&s[8]);
    s[7] = '\0';
    mytm->tm_mon = atoi(&s[5]) - 1;
    s[4] = '\0';
    mytm->tm_year = atoi(&s[0]) - 1900;

    newTime = mktime(mytm);
    if (sizeof (newTime) == 64) {
      snprintf (unixTime, TIME_LEN, "%lld", (rodsLong_t) newTime);
    } else {
      snprintf (unixTime, TIME_LEN, "%d", (uint) newTime);
    }
    return (0);
}


/* checkDateFormat - convert the string given in s and output the time
 * in sec of unix time in the same string s
 * The input can be incremental time given in :
 *     nnnn - an integer. assumed to be in sec
 *     nnnns - an integer followed by 's' ==> in sec
 *     nnnnm - an integer followed by 'm' ==> in min
 *     nnnnh - an integer followed by 'h' ==> in hours
 *     nnnnd - an integer followed by 'd' ==> in days
 *     nnnnd - an integer followed by 'y' ==> in years
 *     dd.hh:mm:ss - where dd, hh, mm and ss are 2 digits integers representing
 *       days, hours minutes and seconds, repectively. Truncation from the
 *       end is allowed. e.g. 20:40 means mm:ss
 * The input can also be full calander time in the form:
 *    YYYY-MM-DD.hh:mm:ss  - Truncation from the beginnning is allowed.
 *       e.g., 2007-07-29.12 means noon of July 29, 2007.
 * 
 */
  
int
checkDateFormat(char *s)
{
  /* Note. The input *s is assumed to be TIME_LEN long */
  int len;
  char t[] = "0000-00-00.00:00:00";
  char outUnixTime[TIME_LEN]; 
  int status;
  int offset = 0;

  if (isInteger (s))
    return (0);

  len = strlen(s);

  if (s[len - 1] == 's') {
    /* in sec */
    s[len - 1] = '\0';
    offset = atoi (s);
    snprintf (s, 19, "%d", offset);
    return 0;
  } else if (s[len - 1] == 'm') {
    /* in min */
    s[len - 1] = '\0';
    offset = atoi (s) * 60;
    snprintf (s, 19, "%d", offset);
    return 0;
  } else if (s[len - 1] == 'h') {
    /* in hours */
    s[len - 1] = '\0';
    offset = atoi (s) * 3600;
    snprintf (s, 19, "%d", offset);
    return 0;
  } else if (s[len - 1] == 'd') {
    /* in days */
    s[len - 1] = '\0';
    offset = atoi (s) * 3600 * 24;
    snprintf (s, 19, "%d", offset);
    return 0;
  } else if (s[len - 1] == 'y') {
    /* in days */
    s[len - 1] = '\0';
    offset = atoi (s) * 3600 * 24 * 365;
    snprintf (s, 19, "%d", offset);
    return 0;
  } else if (len < 19) {
    /* not a full date. */
    if (isdigit(s[0]) && isdigit(s[1]) && isdigit(s[2]) && isdigit(s[3])) {
	/* start with year, fill in the rest */
        strcat(s,(char *)&t[len]);
    } else {
	/* must be offset */
	int mypos;
	
	/* sec */
	mypos = len - 1;
	while (mypos >= 0) {
	    if (isdigit (s[mypos]))
	        offset += s[mypos] - 48;
	    else
		return (DATE_FORMAT_ERR);

            mypos--;
            if (mypos >= 0)
                if (isdigit (s[mypos])) 
                    offset += 10 * (s[mypos] - 48);
                else 
                    return (DATE_FORMAT_ERR);
            else 
		break;

            mypos--;
            if (mypos >= 0)
	        if (s[mypos] != ':') return (DATE_FORMAT_ERR);

            /* min */
            mypos--;
            if (mypos >= 0) 
                if (isdigit (s[mypos]))
                    offset += 60 * (s[mypos] - 48);
                else
                    return (DATE_FORMAT_ERR);
            else
                break;

            mypos--;
            if (mypos >= 0)
                if (isdigit (s[mypos]))
                    offset += 10 * 60 * (s[mypos] - 48);
                else
                    return (DATE_FORMAT_ERR);
            else
		break;

            mypos--;
            if (mypos >= 0)
                if (s[mypos] != ':') return (DATE_FORMAT_ERR);

	    /* hour */
            mypos--;
            if (mypos >= 0)
                if (isdigit (s[mypos]))
                    offset += 3600 * (s[mypos] - 48);
                else
                    return (DATE_FORMAT_ERR);
            else
		break;

            mypos--;
            if (mypos >= 0)
                if (isdigit (s[mypos]))
                    offset += 10 * 3600 * (s[mypos] - 48);
                else
                    return (DATE_FORMAT_ERR);
            else
		break;

            mypos--;
            if (mypos >= 0)
                if (s[mypos] != '.') return (DATE_FORMAT_ERR);

	    /* day */
	
            mypos--;
            if (mypos >= 0)
                if (isdigit (s[mypos]))
                    offset += 24 * 3600 * (s[mypos] - 48);
                else
                    return (DATE_FORMAT_ERR);
            else
		break;

            mypos--;
            if (mypos >= 0)
                if (isdigit (s[mypos]))
                    offset += 10 * 24 * 3600 * (s[mypos] - 48);
                else
                    return (DATE_FORMAT_ERR);
            else
		break;
	}
	snprintf (s, 19, "%d", offset);
	return (0);
    }
  }

  if (isdigit(s[0]) && isdigit(s[1]) && isdigit(s[2]) && isdigit(s[3]) && 
      isdigit(s[5]) && isdigit(s[6]) && isdigit(s[8]) && isdigit(s[9]) && 
      isdigit(s[11]) && isdigit(s[12]) && isdigit(s[14]) && isdigit(s[15]) && 
      isdigit(s[17]) && isdigit(s[18]) && 
      s[4] == '-' && s[7] == '-' && s[10] == '.' && 
      s[13] == ':' && s[16] == ':' ) {
    status = localToUnixTime (s, outUnixTime);
    if (status >= 0) {
       strncpy (s, outUnixTime, TIME_LEN);
    }
    return(status);
  } else {
    return(DATE_FORMAT_ERR);
  }
}
