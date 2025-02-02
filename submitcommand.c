#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include "defines.h"

int run_remote_command(ssh_session client,char *command,int flags,char *pinput,char *poutput,FILELIST *inputfilelist,FILELIST *outputfilelist) {
FILELIST *inlist;
FILELIST *outlist;
char *consoleoutputbuffer[BUF_SIZE];
ssh_channel channel;
char *copycommand[BUF_SIZE];
char *token;
char *tempdir[BUF_SIZE];
char *tempfile[BUF_SIZE];
sftp_session sftp;
char *temp[BUF_SIZE];

channel=OpenSSHChannel(client);		/* open SSH channel */
if(channel == -1) {
	SetErrorMessage(TRUE);
	return(-1);			
}

/* if there are input or output files, start SFTP session and create temporary directory */

if((inputfilelist != NULL) || (outputfilelist != NULL)) {
	sftp=SSHInitSFTP(client);			/* start SFTP session */
	if(sftp == -1) {		
		SetErrorMessage(TRUE);

		SSHCloseChannel(channel);
		return(-1);
	}

	tmpnam(tempdir);			/* create temporary directory name */

	if(sftp_mkdir(sftp,tempdir,0700) < 0) {			/* create temporary directory */
		SetErrorMessage(TRUE);

		SSHCloseChannel(channel);
		return(-1);
	}

}

/* send inputfile command if there are input files */

if(inputfilelist != NULL) {
	memset(copycommand,0,BUF_SIZE);

	token=strtok(command," ");

	do {
		inlist=inputfilelist;

		while(inlist != NULL) {
			/* if file is used by this command */	

			if(strcmp(token,inputfilelist->filename) == 0) {
				/* get path to destination */

				snprintf(tempfile,BUF_SIZE,"%s/%s",tempdir,GetFilenameFromPath(inlist->filename));

				if(SendInputFile(client,sftp,inlist->filename,tempfile) == -1) return(-1);

				strncat(copycommand,tempfile,BUF_SIZE);		/* add modified filename */
			}
			else
			{
				strncat(copycommand,token,BUF_SIZE);
			}

			strncat(copycommand," ",BUF_SIZE);

			inlist=inlist->next;
		}

		token=strtok(NULL," ");	
	} while(token != NULL);
}
else
{
	strncpy(copycommand,command,BUF_SIZE);
}

if(SSHExecuteCommand(client,channel,copycommand,flags,pinput,poutput) == -1) return(-1);		/* run command */

/* process output file list and receive any output files */

if(outputfilelist != NULL) {
	
	outlist=outputfilelist;

	while(outlist != NULL) {
		snprintf(tempfile,BUF_SIZE,"%s/%s",tempdir,GetFilenameFromPath(outlist->filename));	/* get path to destoutation */

		if(sftp_stat(client,tempfile) != NULL) {		/* if output file exists */
			if(ReceiveOutputFile(client,sftp,outlist->filename,tempfile) == -1) return(-1);
		}

		outlist=outlist->next;
	}

}

/* clean up temporary files */

if((inputfilelist != NULL) || (outputfilelist != NULL)) {
	snprintf(tempfile,BUF_SIZE,"%s/*",tempdir);

	sftp_unlink(sftp,tempfile);
	sftp_rmdir(sftp,tempdir);

	SSHCLoseSFTP(sftp);		/* end SFTP session */
}

SSHCloseChannel(channel);

return(0);
}


