#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "defines.h"
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdbool.h>

char *username[BUF_SIZE];
char *password[BUF_SIZE];
char *LastError[BUF_SIZE];

ssh_session OpenSSHConnection(int method,char *ipaddress,int port) {
ssh_session new_ssh_session;
int returncode;
ssh_key privkey;
char *keynames[] = { "id_rsa","id_ecdsa","id_ecdsa_sk","id_ed25519","id_ed25519_sk","id_dsa",NULL };
int count;
char *temp[BUF_SIZE];
char *homedir;
bool showinput;
char *questionptr;
bool usingmultiplemethods=FALSE;

new_ssh_session=ssh_new();			/* create SSH session */

if(new_ssh_session == NULL) {
	SetErrorMessage(new_ssh_session,new_ssh_session,TRUE);

	ssh_free(new_ssh_session);
	return(-1);
}

ssh_options_set(new_ssh_session,SSH_OPTIONS_HOST,ipaddress);		/* set ipaddress and port */
ssh_options_set(new_ssh_session,SSH_OPTIONS_PORT,&port);

if(ssh_connect(new_ssh_session) == SSH_ERROR) {
	SetErrorMessage(new_ssh_session,TRUE);

	ssh_free(new_ssh_session);
	return(-1);
}

/* authenticate the user */

returncode=ssh_userauth_none(new_ssh_session,NULL);
if(returncode == SSH_AUTH_ERROR) {
	SetErrorMessage(new_ssh_session,TRUE);
	return(returncode);
}

if(method == 0) {
	method=ssh_userauth_list(new_ssh_session,NULL);		/* get authentication methods */
	usingmultiplemethods=TRUE;
}

if(method & SSH_AUTH_METHOD_PUBLICKEY) {	/* authenticate using public key */
	/* try default key names */

	count=0;

	while(keynames[count] != NULL) {
		homedir=getenv("HOME");			/* get home directory */

		if(homedir == NULL) {			/* could not be found */
			strncpy(LastError,"distrun: local error: Could not get user's home directory\n",BUF_SIZE);
	
			return(-1);
		}

		snprintf(temp,BUF_SIZE,"%s/.ssh/%s",homedir,keynames[count]);

		if(ssh_pki_import_privkey_file(temp,NULL,NULL,NULL,&privkey) == SSH_OK) {		/* import private key */
			returncode=ssh_userauth_publickey(new_ssh_session,NULL,privkey); 	/* authenticate using private key */

			if(returncode == SSH_AUTH_ERROR) {			/* error ocurred */
				SetErrorMessage(new_ssh_session,TRUE);

				ssh_key_free(privkey);
				ssh_free(new_ssh_session);
				return(-1);
			}

			if(returncode == SSH_OK) return(new_ssh_session);

			if(returncode == SSH_AUTH_PARTIAL) {			/* password required */
				PromptKeyPassword(keynames[count],password);

				returncode=ssh_userauth_publickey(new_ssh_session,password,privkey); 	/* authenticate using private key */

				if(returncode == SSH_OK) {
					ssh_key_free(privkey);
					return(new_ssh_session);
				}

				if(returncode == SSH_AUTH_ERROR) {			/* error ocurred */
					SetErrorMessage(new_ssh_session,TRUE);

					ssh_free(new_ssh_session);
					return(-1);
				}
			}
		}
	
		count++;
	}


	if(usingmultiplemethods == FALSE) {		/* using a single authentication method */
		SetErrorMessage(new_ssh_session,TRUE);

		ssh_key_free(privkey);
		ssh_free(new_ssh_session);
		return(-1);
	}

	printf("distrun: SSH_AUTH_METHOD_PUBLICKEY failed, trying SSH_AUTH_METHOD_PASSWORD\n");
}

if(method & SSH_AUTH_METHOD_PASSWORD) { 			/* authenticate using username and password */	
	/* if first time running OpenSSHConnection(), prompt for username and password */

	if((strlen(username) == 0) && (strlen(password) == 0)) PromptUsernamePassword(username,password);	/* get username and password */

	/* authenticate using username and password */

	if(ssh_userauth_password(new_ssh_session,username,password) == SSH_AUTH_SUCCESS) return(new_ssh_session);

	if(usingmultiplemethods == FALSE) {		/* using a single authentication method */
		SetErrorMessage(new_ssh_session,TRUE);

		ssh_key_free(privkey);
		ssh_free(new_ssh_session);
		return(-1);
	}
}

if(method & SSH_AUTH_METHOD_INTERACTIVE) {	/* authenticate using interactive method */

	returncode=ssh_userauth_kbdint(new_ssh_session,NULL,NULL);

	if(returncode == SSH_AUTH_ERROR) {			/* error ocurred */
		SetErrorMessage(new_ssh_session,TRUE);

		ssh_free(new_ssh_session);
		return(-1);
	}
	else if(returncode == SSH_AUTH_INFO) {		/* authenticate using questions */
		for(count=0;count<ssh_userauth_kbdint_getnprompts(new_ssh_session);count++) {

			questionptr=ssh_userauth_kbdint_getprompt(new_ssh_session,count,&showinput);	/* get question */

			GetInteractiveResponse(questionptr,temp,showinput);		/* get user response */

			if(ssh_userauth_kbdint_setanswer(new_ssh_session,count,temp) == 0) return(new_ssh_session);	/* send response */
		}
	}

	if(usingmultiplemethods == FALSE) {		/* using a single authentication method */
		SetErrorMessage(new_ssh_session,TRUE);

		ssh_key_free(privkey);
		ssh_free(new_ssh_session);
		return(-1);
	}
}

strcpy(LastError,"distrun: remote error: Unable to use any authentication method\n");

return(-1);
}

ssh_channel OpenSSHChannel(ssh_session session) {
ssh_channel channel;
int returncode;

channel=ssh_channel_new(session);
if(channel == NULL) {
	SetErrorMessage(session,TRUE);

	return(-1);
}

if(ssh_channel_open_session(channel) != SSH_OK) {
	ssh_channel_free(channel);

	SetErrorMessage(session,TRUE);
	return(-1);
}

return(channel);
}

int SSHExecuteCommand(ssh_session session,ssh_channel channel,char *command,int flags,char *pinput,char *poutput) {
FILE *handle;
int filesize;
char *buffer[BUF_SIZE];
int bytesread;
int returncode;
sftp_session sftp;
char *cmdptr;
char *tempfile[BUF_SIZE];

if((flags & FLAGS_PIPE) && (strlen(pinput) > 0)) {			/* send input command if there is piped input */
	if(flags & FLAGS_VERBOSE) printf("distrun: Sending input from previous command to next command\n");
	
	/* upload pipe input and pipe it to command remotely
	   TODO: find a beter way of sending input to a command
	 */

	sftp=SSHInitSFTP(session);		/* start SFTP session */
	if(sftp == SSH_ERROR) return(-1);
	
	tmpnam(tempfile);		/* create temporary filename */

	if(SendInputFile(session,sftp,pinput,tempfile) == -1) {		/* send file */
		SSHCLoseSFTP(sftp);
		return(-1);
	}

	SSHCLoseSFTP(sftp);

	snprintf(buffer,BUF_SIZE,"cat %s | %s\n",tempfile,command);

	cmdptr=buffer;
}
else
{
	cmdptr=command;
}

if(ssh_channel_request_exec(channel,cmdptr) != SSH_OK) {
	SetErrorMessage(session,TRUE);
	return(-1);
}

returncode=-1;

if((flags & FLAGS_ASYNC) == 0) {	/* if not asynchronus, wait for process to finish */
	do {
		returncode=ssh_channel_get_exit_status(channel);

		if(returncode == SSH_ERROR) return(-1);

	} while(returncode == -1);

if((flags & FLAGS_PIPE) && (strlen(pinput) > 0)) unlink(tempfile);		/* delete temporary file */
}

if(returncode == 127) {				/* command not found */
	snprintf(LastError,BUF_SIZE,"distrun: remote error: command %s not found\n",cmdptr);
	return(-1);
}
else if(returncode > 0) {
	snprintf(LastError,BUF_SIZE,"distrun: remote error: command returned with error: %s\n",strerror(errno));
	return(-1);
}


if(strlen(poutput) > 0) {		/* if there is piped output file */
	if(flags & FLAGS_VERBOSE) printf("distrun: Getting output from command\n");

	handle=fopen(poutput,"w");		/* open output file */
	if(handle == NULL) {
		SetErrorMessage(session,FALSE);

		if((flags & FLAGS_PIPE) && (strlen(pinput) > 0)) unlink(tempfile);		/* delete temporary file */
		return(-1);
	}
	
	/* get console output from the job */
	
	do {
		bytesread=ssh_channel_read(channel,buffer,BUF_SIZE,0);			/* read output */
		if(bytesread == SSH_ERROR) {
			SetErrorMessage(session,TRUE);

			fclose(handle);
			if((flags & FLAGS_PIPE) && (strlen(pinput) > 0)) unlink(tempfile);		/* delete temporary file */

			return(-1);
		}

		fwrite(buffer,1,bytesread,handle);		
		
		memset(buffer,0,BUF_SIZE);
	} while(bytesread > 0);

	fclose(handle);
}


return(0);
}

void SSHCloseSession(ssh_session session) {
	ssh_free(session);
}

void SSHCloseChannel(ssh_channel channel) {
  ssh_channel_send_eof(channel);
  ssh_channel_close(channel);
  ssh_channel_free(channel);
}

sftp_session SSHInitSFTP(ssh_session session) {
sftp_session sftp;

sftp=sftp_new(session);
if(sftp == NULL) {
	SetErrorMessage(session,TRUE);
	return(-1);
}

if(sftp_init(sftp) != SSH_OK) {
	SetErrorMessage(session,TRUE);
	return(-1);
}

return(sftp);
}

void SSHCLoseSFTP(sftp_session sftp) {
sftp_free(sftp);
}

int SendInputFile(ssh_session session,sftp_session sftp,char *filename,char *remotename) {
FILELIST *next;
sftp_file localfile;
FILE *handle;
char *outputbuffer;
int bytesreceived;

outputbuffer=malloc(IO_FILE_BUFFER_SIZE);
if(outputbuffer == NULL) {
	SetErrorMessage(session,FALSE);
	return(-1);
}

handle=fopen(filename,"r");					/* open local file */
if(handle == NULL) {
	SetErrorMessage(session,FALSE);

	free(outputbuffer);
	return(-1);
}				

localfile=sftp_open(sftp,remotename,O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);	/* create remote file */
if(localfile == NULL) {
	SetErrorMessage(session,TRUE);

	free(outputbuffer);
	fclose(handle);
	return(-1);
}
	
/* read file in blocks and send them */

do {
	bytesreceived=fread(outputbuffer,1,IO_FILE_BUFFER_SIZE,handle);	
	if(bytesreceived == -1) {
		SetErrorMessage(session,FALSE);

		free(outputbuffer);
		fclose(handle);
		return(-1);
	}
		
	if(sftp_write(localfile,outputbuffer,bytesreceived) != bytesreceived) {
		SetErrorMessage(session,TRUE);

		free(outputbuffer);
		fclose(handle);
		return(-1);
	}

} while(!feof(handle));

free(outputbuffer);
fclose(handle);
return(0);
}

int ReceiveOutputFile(ssh_session session,sftp_session sftp,char *filename,char *localname) {
FILELIST *next;
sftp_file remotefile;
FILE *handle;
char *inputbuffer;
int inputsize;
int bytesread;

inputbuffer=malloc(IO_FILE_BUFFER_SIZE);
if(inputbuffer == NULL) {
	SetErrorMessage(session,FALSE);

	return(-1);
}

handle=fopen(localname,"w");				/* open local file */
if(handle == NULL) {
	SetErrorMessage(session,FALSE);

	free(inputbuffer);
	return(-1);
}
					
remotefile=sftp_open(sftp,filename,O_RDONLY,S_IRUSR | S_IWUSR );	/* create remote file */
if(handle == NULL) {
	SetErrorMessage(session,TRUE);

	free(inputbuffer);
	return(-1);
}
	

/* read file in blocks and send them */

do {
	bytesread=fread(inputbuffer,1,IO_FILE_BUFFER_SIZE,handle);	
	if(bytesread == -1) {
		SetErrorMessage(session,FALSE);

		free(inputbuffer);
		fclose(handle);

		return(-1);
	}

	if(sftp_write(remotefile,inputbuffer,bytesread) != bytesread) {
		SetErrorMessage(session,TRUE);

		free(inputbuffer);
		fclose(handle);
		return(-1);
	}

} while(!feof(handle));

free(inputbuffer);
fclose(handle);

return(0);
}

void GetErrorMessage(char *buffer) {
strncpy(buffer,LastError,BUF_SIZE);
}

void SetErrorMessage(ssh_session session,bool IsRemote) {

if(IsRemote == TRUE) {
	snprintf(LastError,BUF_SIZE,"distrun: remote error: %s\n",ssh_get_error(session));
}
else
{
	snprintf(LastError,BUF_SIZE,"distrun: local error: %s\n",strerror(errno));
}

}


