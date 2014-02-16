#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <utmp.h>
#include <utmpx.h>
#include <fcntl.h>
#include <time.h>
#include "utmplib.h" /* include interface def for utmplib.c */

void show_info( struct utmp *utbufp );
void show_usage();
double get_connect_time(char *username, char *filename);
time_t login_time(char * username);
time_t logout_time(char * username); 
void dump_output(double connect_time,char * username);

#define DEBUG 1 /*Debug mode. Enable:1,Disable:0*/

int main(int argc, char * argv[]){
	//< > =
	char * username = NULL; //= malloc(UT_NAMESIZE * sizeof(char));
	char * filename = NULL; //= malloc(32 * sizeof(char));
	//float connect_time;

	if(argc==2){
		/* # aac <username>*/
		//strcpy(username,argv[1]);
		username = argv[1];

		dump_output(get_connect_time(username,filename),username);
	}else if(argc==4){
		//check for flag
		if(strcmp(argv[1],"-f")==0){
			//printf("wtmp file %s",argv[2]);
			filename = argv[2];
			username = argv[3];
		}else if(strcmp(argv[2],"-f")==0){
			//printf("wtmp file %s",argv[3]);
			filename = argv[3];
			username = argv[1];
		}else{
			show_usage();
			return 0;
		}
		dump_output(get_connect_time(username,filename),username);
		
	}else{
		show_usage();
		return 0;
	}
	return 0;
}

/*
* Gets the connection duration in seconds, converts
* it to decimal time and outputs it to the console
* 
*/
void dump_output(double connect_time,char * username){
	if(connect_time >= 0){
		/*convert seconds to decimal hours*/
		connect_time = connect_time/3600; 

		printf("\t total \t%.2f\n",connect_time);		
	}else{
		if(DEBUG){
			printf("connect time:%.2f\n",connect_time);
		}
		printf("ERROR: Could not determine connect time for the username,%s. \n",username);
		
	}
}

/**
* Returns the connect time of the  
*
*/
double get_connect_time(char *username, char *filename){
	if(username==NULL){
		printf("\nget_connect_time: username is NULL \n");
		return -1;
	}	
	if(filename!=NULL){
		utmpname(filename); /*set the utmp file to be accessed*/
	}
	
	setutxent(); /*set file pointer at the beginning*/
	//calculate duration
	time_t login = login_time(username);
	//time_t login_val = *login;

	if(login==0) {
		if(DEBUG){
			printf("\nget_connect_time: could not find login for user \n");
		}
		return -1;
	}
	time_t logout = logout_time(username);

	if(logout==0){
		/*use current time, if a logout entry is not found*/
		time(&logout);
	}else if(login > logout){
		if(DEBUG){
			printf("\nget_connect_time: -1 \n");
			printf("login:%f\tlogout:%f\n",(float)login,(float)logout);
		}
		return -1;		
	}
	endutxent(); /*close utmpx file*/
	
	return difftime(logout,login);
	
}

/*
* Given a specific username, searches the utmp file for user's initial login time 
* Returns 0 if not found
*/
time_t login_time(char * username){
	struct utmpx * entry; //= malloc(sizeof(struct utmp));
	time_t login_time ; //= malloc(sizeof(time_t)); 
	while((entry=getutxent())!=NULL){
		//check the username and entry type
		int same_username = (strcmp(entry->ut_user,username)==0);
		
		if((entry->ut_type==USER_PROCESS) && same_username){
			login_time = entry->ut_time;
			return login_time;
		}
	}
	return 0;
}

/*
* Given a specific username, searches the utmp file for user's initial logout time
* Returns 0 if not found
*/
time_t logout_time(char * username){
	struct utmpx * entry; //= malloc(sizeof(struct utmp));
	time_t logout_time ; //= malloc(sizeof(time_t)); 
	while((entry=getutxent())!=NULL){
		//check the username and entry type
		int same_username = (strcmp(entry->ut_user,username)==0);
		
		if((entry->ut_type==DEAD_PROCESS) && same_username){
			logout_time = entry->ut_time;
			return logout_time;
		}
	}
	return 0;
}



void show_usage() {
	printf("\nUsage:\n\taac <username>\n");
	printf("\taac -f <filename> <username>\n");
	printf("\taac <username> -f <filename>\n");
}
/*
* * show info()
* * displays the contents of the utmp struct
* * in human readable form
* * * displays nothing if record has no user name
* */
void show_info( struct utmp *utbufp ) {
 	void showtime( time_t , char *);
 	if ( utbufp->ut_type != USER_PROCESS )
 		return;
 	printf("%-8.8s", utbufp->ut_name); /* the logname */
 	printf(" "); /* a space */
 	printf("%-12.12s", utbufp->ut_line); /* the tty */
	printf(" "); /* a space */
	//printf("%-13s",utbufp->ut_time); 	
	//showtime( utbufp->ut_time, DATE_FMT ); display time
 	if (utbufp->ut_host[0] != '\0') 
 		printf(" (%s)", utbufp->ut_host); /* the host */
 	printf("\n"); /* newline */
 }
