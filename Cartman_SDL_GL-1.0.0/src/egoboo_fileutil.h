#pragma once

#include "egoboo_typedef.h"

#define MAXCAPNAMESIZE      32                      // Character class names

// For damage/stat pair reads/writes
extern int   pairbase, pairrand;
extern float pairfrom, pairto;

char   get_first_letter( FILE* fileread );
void   goto_colon( FILE* fileread );
bool_t goto_colon_yesno( FILE* fileread );
void   read_pair( FILE* fileread );

Sint32 fget_int( FILE* fileread );
void   get_name( FILE* fileread, char *szName );
