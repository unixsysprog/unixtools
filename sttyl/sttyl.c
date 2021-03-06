/*
* 
* 			sttyl  (stty-lite)
* 
*  Purpose: 
*	   This is a unix commandline utility used to view and modify the terminal control settings 
*	   for your console. In other words, stty allows you to determine how your 
*	   current login session (for example) processes input and output, and to 
*	   control these parameters.
* 
*  Arguments (commandline):
*		sttyl [erase <char>] [kill <char>] {[[-]flag]}*
*
*		where <char> represents a character. 
*
*		flag: 
*			This is used to toggle on or off a setting. As denoted in the format above, 
*			multiple flags/settings can be specified. When the '-' is placed in front of a 
*			flag, the corresponding setting is turned off and on otherwise. 
*
*			Supported flags include:
*				icrnl 
*				onlcr
*				echo
*				echoe
*				olcuc
*				icanon
*				isig
*
*		For example, 
*				$ sttyl kill=k erase=c -echo isig -icrnl
*	
*
*
*   Author: Osazuwa Omigie	
*   Date: March 9,2014.		
*   CSCI-E28
*   Harvard University		
*		
*/


#include	<stdio.h>
#include	<stdlib.h>
#include	<termios.h>
#include 	<string.h>
#include	<fcntl.h>
#include	<unistd.h>


/* TABLE IDs */
#define 	IFLAG_TABLE_ID			1
#define 	OFLAG_TABLE_ID			2
#define 	LFLAG_TABLE_ID			3
#define		CCSETTINGS_TABLE_ID		4


#define 	FALSE 		0
#define		TRUE		1

// struct flaginfo { tcflag_t fl_value; char * name; };
struct settingsinfo{tcflag_t value; char * name; };


/* 	
	Each entry/struct contains a tcflag_t property and a string property.
	Where the tcflag_t property would be used as an index to identify the setting in 
	the termios.c_cc array. And the name of the setting as the string property.
	*/
struct settingsinfo settings_table[] = {
	{VERASE	,	"erase"},
	{VKILL	,	"kill"},
	{VINTR	,	"intr"},
	{0		,	NULL}
}; 

struct settingsinfo input_flags[] = {		
		{IGNBRK	,	"ignbrk"},
		{BRKINT	,	"brkint"},
		{IGNPAR	,	"ignpar"},
		{INLCR	,	"inlcr"},
		{ICRNL	,	"icrnl"},
		{IXON	,	"ixon"},
		{0		,	NULL} 
};

struct settingsinfo local_flags[] = {
		{ISIG	,	"isig"},
		{ICANON	,	"icanon"},
		{ECHO	,	"echo"},
		{ECHOE	,	"echoe"},
		{ECHOK	,	"echok"},
		{0		,	NULL} 
};

struct settingsinfo output_flags[] = {
		{OLCUC	,	"olcuc"}, 
		{ONLCR	,	"onlcr"},
		{0		,	NULL}
};



void showbaud(int thespeed);
void show_flagset( int thevalue, struct settingsinfo thebitnames[] );
void show_some_flags( struct termios *ttyp );
void show_erase(struct termios *ttyp);
void show_other_settings(struct termios *ttyp, struct settingsinfo thesettings[]);
int table_lookup(struct settingsinfo thesettings[], char *name);
void process_args(struct termios * ttyinfo, int argc, char * argv[]);
int fl_table_lookup(char * cmdline_arg, struct termios * ttyp);

int main(int argc, char * argv[])
{	
	struct	termios ttyinfo;	/* this struct holds tty info */
	
	/* get settings for current std in */
	if ( tcgetattr( STDIN_FILENO , &ttyinfo ) == -1 ){   
		perror( "Could not get params about stdin");
		exit(1);
	}

	process_args(&ttyinfo,argc, argv); /*modify tty info based on the commandline arguments*/

	/* write/strore modified stdin settings */
	if ( tcsetattr(STDIN_FILENO, TCSANOW, &ttyinfo) == -1 ){   
		perror( "Could not write new settings to file");
		exit(1);
	}
	
	return 0;
}


void process_args(struct termios * ttyp,int argc, char * argv[]){
	int i;
	int table_index;
	char * cmdline_arg;
	char newchar;
	
	if (argc==1){ 	
		/*show info*/
		showbaud(cfgetospeed(ttyp)); /*get and display baud rate */
		show_other_settings(ttyp, settings_table); 
		show_some_flags(ttyp);
	}
	else if (argc>=2){		
		for (i=1;i<argc;i++){
			cmdline_arg = argv[i];		
			if((table_index=table_lookup(settings_table,cmdline_arg)) >= 0 ){
				if((i+1) >= argc) {
					fprintf(stderr, "Expected a char after %s\n",cmdline_arg);
					//show usage and exit 
					exit(1); 
				}
				newchar = argv[i+1][0]; //set new char to the next argument, TODO: error check
				// printf("the new char is %c\n",newchar);
				printf("cc_index: %d ; erase_index: %d \n",settings_table[table_index].value, VERASE);
				ttyp->c_cc[settings_table[table_index].value] = newchar;
				printf("the new char is %c\n",ttyp->c_cc[settings_table[table_index].value]);
				i++; //skip next value 
				continue;
			}else{ //check flag tables
				if(fl_table_lookup(cmdline_arg,ttyp) == FALSE){
					fprintf(stderr, "Invalid commandline argument %s\n",argv[i]);
					exit(1);
				}
			}
		}//end for loop
	} //end else-if
}

/*
This is utility funtion for process_args that helps in looking up arguments
on the flag tables. And if found, turns on or off the appropriate bit on the
termios struct passed by reference.
It also depends on the table_lookup function for table look ups. 

Returns TRUE/1 if found or FALSE/0 otherwise.
**/
int fl_table_lookup(char * cmdline_arg, struct termios * ttyp){
	struct settingsinfo * table;
	tcflag_t * flag; /* will hold a reference to the flag that should be modified*/
	tcflag_t fl_value, turn_off, turn_on; 
	int table_index; /* hold the index to the entry on the table*/

	/* */
	if((table_index=table_lookup(input_flags,cmdline_arg)) >= 0 ){
		table = input_flags;	//keep a reference to the table
		flag = &ttyp->c_iflag; //keep a reference to the flag
	}else if((table_index=table_lookup(output_flags,cmdline_arg)) >= 0 ){
		table = output_flags;
		flag = &ttyp->c_oflag; 
	}else if((table_index=table_lookup(local_flags,cmdline_arg)) >= 0 ){
		table = local_flags;
		flag = &ttyp->c_lflag; 
	}else{
		return FALSE; /* argument name not found in any table*/
	}

	/*argument has been found in a flag table*/	
	fl_value = table[table_index].value;	/* Get the corresponing bit mask*/
	turn_on = ((*flag) | (fl_value));		/* logic to turn on bit */
	turn_off = ((*flag) & (~fl_value));		/* logic to turn off bit */

	/*update the appropriate ttyp flag accordingly*/
	*flag = (cmdline_arg[0]=='-')?turn_off:turn_on;

	return TRUE; 
}



/*
	This function does a table lookup on a given table. 
	Returns the position in the array where the value was found.
**/
int table_lookup(struct settingsinfo thesettings[], char *name){
	int i,j,start_index=0;
	char * setting_name;
	char * tmpname = malloc(sizeof(name)-1);

	if (name[0] == '-'){
		/* strip off '-' and store result in tmpname */
		for(j=0;j<strlen(name);j++){
			if(j==0)continue; /*dont copy the '-' character */
			tmpname[j-1] = name[j];
		}
	}else{
		tmpname = name;
	}

	for(i=0;(setting_name=thesettings[i].name);i++){
		if(!strcmp(setting_name,tmpname)){ //name found
			// printf("%s found in   \n");
			return i;
		}
	}
	return -1;
}

void show_other_settings(struct termios *ttyp, struct settingsinfo thesettings[]){
	int i,index;
	char special_char;
	char * name;

	for ( i=0; thesettings[i].name ; i++ ) {
		index=thesettings[i].value; 		/*get the cc_index from the table*/
		special_char = ttyp->c_cc[index]; 	/*get the control character */
		name = thesettings[i].name;
		// is_null_cc = (special_char == '\0')?TRUE:FALSE
		// if(special_char == 127){
		// 	printf("%s = ^?; ", special_char-1+'A');		
		// }else{
		// 	printf("%s = %c; ", name, special_char);	
		// }
		printf("%s = %c; ",name,special_char);
		// printf("%s = %c; ",thesettings[i].name, 
			// ((special_char == NULL)?(char)(special_char-1+'A'):((char)special_char)));
	}
	printf("\n");
}

/*
 *	show the values of two of the flag sets_: c_iflag and c_lflag
 *	adding c_oflag and c_cflag is pretty routine - just add new
 *	tables above and a bit more code below.
 */
void show_some_flags( struct termios *ttyp )
{
	show_flagset( ttyp->c_iflag, input_flags );
	show_flagset( ttyp->c_lflag, local_flags );
}


/*
 * Check each bit pattern and display descriptive title
 */
void show_flagset( int thevalue, struct settingsinfo thebitnames[] )
{
	int	i;
	for ( i=0; thebitnames[i].value ; i++ ) {
		if (!(thevalue & thebitnames[i].value)) 
			printf("-"); /*place a '-'' in front to indicate off status*/
		printf( "%s; ", thebitnames[i].name);
	}
	printf("\n");
}


/*
 *	prints the speed in english
 */
void showbaud( int thespeed ){
	printf("speed ");
	switch ( thespeed ){
		case B300:	printf("300");	break;
		case B600:	printf("600"); 	break;
		case B1200:	printf("1200"); 	break;
		case B1800:	printf("1800"); 	break;
		case B2400:	printf("2400"); 	break;
		case B4800:	printf("4800"); 	break;
		case B9600:	printf("9600"); 	break;
		case B38400:	printf("38400"); 	break;
		case B57600:	printf("57600"); 	break;
		case B115200:	printf("115200"); 	break;
		case B230400:	printf("230400"); 	break;
		default:	printf("n/a");	break;
	}
	printf(" baud; ");
}
