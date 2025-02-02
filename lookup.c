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

#ifdef __linux__
 #include <netdb.h>
 #include <sys/socket.h>
 #include <sys/types.h> 
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
#endif


#ifdef _WIN32
 #include <winsock2.h>
#endif

#include "defines.h"

/*
 * lookup address
 */

int address_lookup(char *address,char *buf) {
struct hostent *he;
struct in_addr **addr_list;

he=gethostbyname(address);				/* get ip address */
if(he == NULL) {
	strncpy(buf,address,BUF_SIZE);
	return(0);
}

addr_list=(struct in_addr **) he->h_addr_list;

strcpy(buf,inet_ntoa(*addr_list[0]));

return(0);
}

