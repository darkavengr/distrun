/*  Distrun Version 1.0
    (C) Matthew Boote 2023

    This file is part of Distrun.

    Distrun is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Distrun is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Distrun.  If not, see <https:www.gnu.org/licenses/>.
*/

/*
 *
 * Distrun - Distributed execution program
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include "defines.h"

void signalhandler(int sig,siginfo_t *info,void *ucontext);

struct {
	char *name;
	int method_id;
} authentication_methods[] = { { "any",0 },
			      { "publickey",SSH_AUTH_METHOD_PUBLICKEY },
			      { "password",SSH_AUTH_METHOD_PASSWORD },
			      { "interactive",SSH_AUTH_METHOD_INTERACTIVE },
			      { "questions",SSH_AUTH_INFO },
			      { NULL,0 },
};

int main(int argc, char *argv[])
{
int count;
char *b;
char c;
char *pipeinput[BUF_SIZE];
char *pipeoutput[BUF_SIZE];
int commandend;
int returnvalue;
int flags;
char *toofewargs="distrun: Too few arguments to %s\n";
char *command[BUF_SIZE];
char *virtualhost=NULL;
IPLIST runhost;
ssh_session client;
char *errormessage[BUF_SIZE];
bool havecommand=FALSE;
struct sigaction signalaction;
int authmethod=0;

if(argc < 2) {			  	  /* if no arguments */
	printf("Distributed execution program\n\n");
	printf("Usage:\n");
	printf("distrun [OPTIONS] -c [command1] ! [command2] ! [commandn]\n\n");
	printf("-a			Run commands asynchronously\n");
	printf("-h [hostname]		Virtual host to run job on\n");
	printf("-i [filenames]		Submit input files\n");	
	printf("\n");
	printf("-m [method]		Authenticate using [method]:\n");
	printf("			any 		Authenticate using every method (default)\n");
	printf("			publickey	Authenticate using public key\n");	
	printf("			password	Authenticate using username and password\n");
	printf("			interactive	Authenticate using host-based interactive method\n");
	printf("			questions	Authenticate by asking user questions\n");
	printf("\n");
	printf("-o [filenames]		Receive output files after execution\n");
	printf("-p			Redirect console output from command to input of next command\n");
	printf("-v			Explain what is being done\n");
	printf("\n");

	exit(0);
}

signalaction.sa_sigaction=&signalhandler;
signalaction.sa_flags=SA_NODEFER;

if(sigaction(SIGINT,&signalaction,NULL) == -1) {		/* set signal handler */
	perror("distrun:");
	exit(1);
}

getconfig();				/* get configuration */

/* process arguments */

for(count=1;count<argc;count++) {
	b=argv[count];

	if((char) *b == '-') {
		b++;

		if((char) *b == 'c') {				/* run command */
			if((count+1) > argc) {
				printf(toofewargs,"-c");
				exit(NO_ARGS);
			}

			if(virtualhost == NULL) {		/* no virtual host name */
				printf("distrun: No virtual hostname given\n");
				exit(NO_ARGS);
			}

			havecommand=TRUE;			/* have passed -c */
			commandend=1;

			/* get command and arguments */

			while(commandend < argc) {
				strncpy(command,argv[count+1],BUF_SIZE);		/* command */

				/* arguments */

				for(commandend=count+2;commandend<argc;commandend++) {
					c=*argv[commandend];

					if(c == '!') break;			/* end if separator found */
	
					strncat(command," ",BUF_SIZE);
					strncat(command,argv[commandend],BUF_SIZE);
				}
						
				if(find_host_to_run_on(virtualhost,&runhost) == -1) {		/* find hostname */
					printf("distrun: Virtual host %s not found\n",virtualhost);
					exit(CONFIG_ERROR);
				}

				if(tmpnam(pipeoutput) == NULL) {	/* create output file */
					printf("distrun: local error: Can't create temporary file %s\n",pipeoutput);
					exit(errno);
				}
	
				client=OpenSSHConnection(authmethod,runhost.ipaddress,runhost.port);	/* create SSL connection */
				if(client == -1) {
						GetErrorMessage(errormessage);

						printf("distrun: local error: %s\n",errormessage);

						exit(SSH_ERROR);
				}				

				/* run command on host */
				
				if(flags & FLAGS_VERBOSE) printf("distrun: running remote command %s on %s port %d\n",command,runhost.ipaddress,runhost.port);
	
				returnvalue=-2;			/* run synchronously by default */

				if(flags & FLAGS_ASYNC) {
					returnvalue=fork();	/* create new process to run command if asynchronous */

					if(returnvalue == -1) {		/* fork error */
						perror("distrun: local error:");

						SSHCloseSession(client);	/* close SSH session */
						exit(errno);
					}
				}

				if((returnvalue == -2) || (returnvalue == 0)) {	/* if not running async or running in forked child process */																																								
					if(run_remote_command(client,command,flags,pipeinput,pipeoutput,GetInputFileList(),GetOutputFileList()) == -1) {
						printf("distrun: Job failed with the following error: ");	/* error occurred */

						GetErrorMessage(errormessage);		/* Get error message */

						printf("%s",errormessage);

						SSHCloseSession(client);	/* close SSH session */
						return(-1);
					}
				}
				
				/* if not piped output, display output for each command */
				if((flags & FLAGS_PIPE) == 0) {
					printf("distrun: command %s returned the following output:\n",command);

					displayoutput(pipeoutput); 
				}

				if(flags & FLAGS_PIPE) {
					if(flags & FLAGS_VERBOSE) printf("distrun: copying output to input\n");

					if(strlen(pipeinput) > 0) unlink(pipeinput);	/* remove previous input file */

					strncpy(pipeinput,pipeoutput,BUF_SIZE);	/* if piping output, copy output to input */
				}
			
				count=commandend;
			}

			SSHCloseSession(client);	/* close SSH session */
			
		}
		else if((char) *b == 'v') {			/* verbose */
			flags |= FLAGS_VERBOSE;	
		}
		else if((char) *b == 'p') {			/* pipe output */
			if(flags & FLAGS_ASYNC) {	/* can't use -a and -p together */
				printf("distrun: Can't use -p with -a\n");
				exit(CONFIG_ERROR);
			}

			flags |= FLAGS_PIPE;
		}	
		else if((char) *b == 'h') {			/* virtual host name */
			virtualhost=argv[count+1];
		}
		else if((char) *b == 'a') {			/* run asynchronously */
			if(flags & FLAGS_PIPE) {	/* can't use -a and -p together */
				printf("distrun: Can't use -a with -p\n");
				exit(CONFIG_ERROR);
			}

			flags |= FLAGS_ASYNC;
		}
		else if((char) *b == 'i') {				/* input files */
			if((count+1) > argc) {
				printf(toofewargs,"-i");
				exit(NO_ARGS);
			}

			for(commandend=count+1;commandend<argc;commandend++) {
				if(*argv[commandend] == '-') break;		/* found option */

				AddInputFileToList(argv[commandend]);  /* add link to list */
			}

			count=commandend-1;
		}
		else if((char) *b == 'o') {				/* output files */
			if((count+1) > argc) {
				printf(toofewargs,"-o");
				exit(NO_ARGS);
			}

			for(commandend=count+1;commandend<argc;commandend++) {
				c=*argv[commandend];

				if((char) *b == '-') break;		/* found option */

				AddOutputFileToList(argv[commandend]);  /* add link to list */			

				printf("Added output file %s to list\n",argv[commandend]);
			}

			count=commandend-1;
		}
		else if((char) *b == 'm') {			/* authentication method */
			commandend=0;

			while(authentication_methods[commandend].name != NULL) {
				if(authentication_methods[commandend].name == NULL) break;

				if(strcmp(argv[count+1],authentication_methods[commandend].name) == 0) {	/* found method */
					authmethod=authentication_methods[commandend].method_id;
					break;
				}

				commandend++;
			}

			count++;
		}
		else if((char) *b == 'p') {			/* pipe output */
			if(flags & FLAGS_ASYNC) {	/* can't use -a and -p together */
				printf("distrun: Can't use -p with -a\n");
				exit(CONFIG_ERROR);
			}

			flags |= FLAGS_PIPE;
		}	
		else
		{
			printf("distrun: Invalid option -%c\n",*b);

			exit(INVALID_OPTION);
		}	 		 	
	}
}

if(havecommand == FALSE) {			/* no -c */
	printf("distrun: No command given\n");
	exit(INVALID_OPTION);
}

/*
 * OUTPUT PHASE:
 *
 * Collect any output generated by the job
 *
 */

if((flags & FLAGS_PIPE)) {			/* display final output */
	printf("distrun: job returned the following output:\n");

	displayoutput(pipeoutput);			/* display final output */
}

unlink(pipeoutput);			/* delete temporary file */

return(0);
}


void PromptUsernamePassword(char *username,char *password) {
printf("Enter username:");
fgets(username,BUF_SIZE,stdin);

removecrlf(username);			/* remove newline */

strncpy(password,getpass("Enter password:"),BUF_SIZE);
removecrlf(password);
}

void PromptKeyPassword(char *keyname,char *password) {
printf("distrun: A password is required for public key %s\n",keyname);

strncpy(password,getpass("Enter password:"),BUF_SIZE);
removecrlf(password);
}

void GetInteractiveResponse(char *prompt,char *response,bool echo) {

if(echo == TRUE) {			/* show input */
	printf("%s",prompt);

	fgets(response,BUF_SIZE,stdin);
}
else
{
	strncpy(response,getpass(prompt),BUF_SIZE);
}

}

void displayoutput(char *filename) {
FILE *handle;
char *buffer[BUF_SIZE];

handle=fopen(filename,"r");
if(handle != NULL) {

	while(fgets(buffer,BUF_SIZE,handle)) {		
		printf("%s",buffer);
	}

	fclose(handle);
}

}

void signalhandler(int sig,siginfo_t *info,void *ucontext) {
if((sig == SIGINT) || (sig == SIGTERM) || (sig == SIGKILL)) {		/* if terminated */
	printf("distrun: signal %d received. Terminating\n",sig);
}

}

