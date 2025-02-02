#include <stdint.h>
#include <errno.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

#define TRUE			1
#define FALSE			0

#define	distrun_client_config_file "distrun.conf"

#define FLAGS_ASYNC	1
#define FLAGS_PIPE	2
#define FLAGS_VERBOSE	4

#define ERROR_START		4096

#define CONFIG_ERROR		ERROR_START+1
#define NO_ARGS			ERROR_START+2
#define NOT_VALID_NODE		ERROR_START+3
#define INVALID_OPTION		ERROR_START+4

#define TIMEOUT 255
#define BUF_SIZE 256
#define IO_FILE_BUFFER_SIZE	1024

#define LIBSSH_STATIC 1

typedef struct {
	char *ipaddress[BUF_SIZE];
	int port;
	struct IPLIST *next;
} IPLIST;

typedef struct {
	char *virtualhostname[BUF_SIZE];
	IPLIST *iplist;
	int number_of_nodes;
	IPLIST *iplist_end;
	struct VIRTUALHOSTNAME *next;
} VIRTUALHOSTLIST;

typedef struct {
	char *filename[BUF_SIZE];
	unsigned int size;
	struct FILELIST *next;
} FILELIST;

void PromptUsernamePassword(char *username,char *password);
void getconfig_client(void);
int find_host_to_run_on(char *virtualhost,IPLIST *buffer);
int AddInputFileToList(char *filename);
int AddOutputFileToList(char *filename);
FILELIST *GetOutputFileList(void);
FILELIST *GetOutputFileListEnd(void);
FILELIST *GetInputFileList(void);
FILELIST *GetInputFileListEnd(void);
int address_lookup(char *address,char *buf);
ssh_session OpenSSHConnection(int method,char *hostname,int port);
ssh_channel OpenSSHChannel(ssh_session session);
int SSHExecuteCommand(ssh_session session,ssh_channel channel,char *command,int flags,char *pinput,char *poutput);
void SSHCloseChannel(ssh_channel channel);
int SendInputFile(ssh_session session,sftp_session sftp,char *filename,char *remotename);
int ReceiveOutputFile(ssh_session session,sftp_session sftp,char *filename,char *remotename);
int run_remote_command(ssh_session client,char *command,int flags,char *pinput,char *poutput,FILELIST *inputfilelist,FILELIST *outputfilelist);
char *GetFilenameFromPath(char *path);
sftp_session SSHInitSFTP(ssh_session session);
void SSHCLoseSFTP(sftp_session sftp);

