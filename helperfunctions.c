#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "defines.h"

FILELIST *inputfilelist=NULL;
FILELIST *inputfilelist_end=NULL;
FILELIST *outputfilelist=NULL;
FILELIST *outputfilelist_end=NULL;

int AddInputFileToList(char *filename) {
struct stat filestat;

if(inputfilelist == NULL) {		/* first in list */
	inputfilelist=malloc(sizeof(FILELIST));
	if(inputfilelist == NULL) return(ENOMEM);
		
	inputfilelist_end=inputfilelist;
}
else
{
	inputfilelist_end->next=malloc(sizeof(FILELIST));
	if(inputfilelist_end->next == NULL) return(ENOMEM);
		
	inputfilelist_end=inputfilelist_end->next;
}

stat(filename,&filestat);			/* get file information */
		
strncpy(inputfilelist_end->filename,filename,BUF_SIZE);		/* get filename */
inputfilelist_end->size=filestat.st_size;				/* get size */

return(0);
}

int AddOutputFileToList(char *filename) {
struct stat filestat;

if(outputfilelist == NULL) {		/* first in list */
	outputfilelist=malloc(sizeof(FILELIST));
	if(outputfilelist == NULL) return(-1);
		
	outputfilelist_end=outputfilelist;
}
else
{
	outputfilelist_end->next=malloc(sizeof(FILELIST));
	if(outputfilelist_end->next == NULL) return(-1);
		
	outputfilelist_end=outputfilelist_end->next;
}

stat(filename,&filestat);			/* get file information */
		
strncpy(outputfilelist_end->filename,filename,BUF_SIZE);		/* get filename */
outputfilelist_end->size=filestat.st_size;				/* get size */

return(0);
}

FILELIST *GetOutputFileList(void) {
return(outputfilelist);
}

FILELIST *GetOutputFileListEnd(void) {
return(outputfilelist_end);
}

FILELIST *GetInputFileList(void) {
return(inputfilelist);
}

FILELIST *GetInputFileListEnd(void) {
return(inputfilelist_end);
}

void removecrlf(char *command) {
char *b;

b=strchr(command,'\r');
if(b != NULL) *b=0;

b=strchr(command,'\n');
if(b != NULL) *b=0;
}

char *GetFilenameFromPath(char *path) {
char *aptr;

aptr=path;

while(*aptr++ != 0) ;;
while(*--aptr != '/') ;;

return(++aptr);
}


