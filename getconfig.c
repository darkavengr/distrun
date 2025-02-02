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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include "defines.h"

VIRTUALHOSTLIST *hostlist;
VIRTUALHOSTLIST *hostlist_end;
int number_of_nodes=0;

/*
 * Get configuration
 *
 * In: Nothing
 *
 * Returns: Nothing
 *
 */
void getconfig(void) {
FILE *handle;
char *buf[BUF_SIZE];
char *next_token;
int count;
int lc=1;
char *b;
char *nodename[BUF_SIZE];
VIRTUALHOSTLIST *next=hostlist;
IPLIST *iplist;

handle=fopen(distrun_client_config_file,"r"); 	                /* open file */
if(!handle) {						/* exit on error */
	printf("distrun: Error opening configuration file %s\n",distrun_client_config_file); 
 	exit(CONFIG_ERROR);
}
 
/* process file */
 
while(!feof(handle)) {
	fgets(buf,BUF_SIZE,handle);			/* get line */

	lc++;
	
//	if((*buf == '\n') || (*buf == '#')) continue;		/* skip empty newline and comments */
  
	b=buf;
	b += strlen(buf)-1;

	if(*b == '\n') *b=0;		/* remove newline from line if found */
	if(*b == '\r') *b=0;		

 	/* client node declaration */
	
	b=buf;

	if((char) *b == '[') {		
		b += (strlen(buf)-1);

		if((char) *b == ']') {
			/* save node name */
			b=buf;
			b++;

			strncpy(nodename,b,BUF_SIZE);

			b=nodename;
			b += strlen(nodename)-1;		/* remove the ] at end */
			*b=0;

			/* add to virtual host list */

			if(hostlist == NULL) {			/* first in list */
				hostlist=malloc(sizeof(VIRTUALHOSTLIST));
				
				if(hostlist == NULL) {
					perror("distrun:");
					exit(CONFIG_ERROR);
				}

				hostlist_end=hostlist;
			}
			else
			{
				hostlist_end->next=malloc(sizeof(VIRTUALHOSTLIST));
				
				if(hostlist_end->next == NULL) {
					perror("distrun:");
					exit(CONFIG_ERROR);
				}				
		
				hostlist_end=hostlist_end->next;
			}

			hostlist_end->number_of_nodes=0;

			strncpy(hostlist_end->virtualhostname,nodename,BUF_SIZE);		/* copy host name */

			hostlist_end->iplist=NULL;

			while(!feof(handle)) {
				fgets(buf,BUF_SIZE,handle);			/* get line */

				b=buf;
				if(*b == '[') break;		/* at end */

				lc++;
				if(memcmp(buf,"\015",1) == 0 || memcmp(buf,"#",1) == 0) continue;			/* comment */
  
				b=buf;
				b += strlen(buf)-1;

				if(*b == '\n') *b=0;		/* remove newline from line if found */
				if(*b == '\r') *b=0;		

				next_token=strtok(buf," ");				/* get first part */
				if(next_token == NULL) break;				/* exit if no more */

				/* add ip list entry */
				
				if(hostlist_end->iplist == NULL) {			/* first in list */
					hostlist_end->iplist=malloc(sizeof(hostlist_end->iplist));
				
					if(hostlist_end->iplist == NULL) {
						perror("distrun:");
						exit(CONFIG_ERROR);
					}

					hostlist_end->iplist_end=hostlist_end->iplist;
				}
				else
				{
					hostlist_end->iplist_end->next=malloc(sizeof(hostlist_end->iplist));
				
					if(hostlist_end->iplist_end->next == NULL) {
						perror("distrun:");
						exit(CONFIG_ERROR);
					}	

					hostlist_end->iplist_end=hostlist_end->iplist_end->next;
				}

				address_lookup(next_token,hostlist_end->iplist_end->ipaddress);		/* get ip address */

				next_token=strtok(NULL," ");				/* get next part */
				if(next_token == NULL) {			/* no next token */
					printf("distrun: %d: Port missing for node\n" ,lc);
					exit(CONFIG_ERROR);
				}

				hostlist_end->iplist_end->port=atoi(next_token);		/* get port number */	
				hostlist_end->number_of_nodes++;
			}
		}	
	}

}

fclose(handle);

return(0);
}

/* TODO: find better way to find host */

int find_host_to_run_on(char *virtualhost,IPLIST *buffer) {
int whichhost;
int count=0;
VIRTUALHOSTLIST *next;
IPLIST *iplist;

next=hostlist;

while(next != NULL) {
	if(strncmp(next->virtualhostname,virtualhost,BUF_SIZE) == 0) {		/* found virtual host */
		iplist=next->iplist;		/* point to IP address list */
		whichhost=rand() % next->number_of_nodes;

		/* search list for hostname */

		while(iplist != NULL) {

			if(count++ == whichhost) {		/* found name */
				memcpy(buffer,iplist,sizeof(IPLIST));
				return(0);
			}

			iplist=iplist->next;
		}
	}

	next=next->next;
}

return(-1);
}
