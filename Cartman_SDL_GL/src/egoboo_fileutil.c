#include "egoboo_fileutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int pairbase, pairrand;
float pairfrom, pairto;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
char get_first_letter( FILE* fileread )
{
    // ZZ> This function returns the next non-whitespace character
    char cTmp;
    fscanf( fileread, "%c", &cTmp );

    while ( isspace( cTmp ) )
    {
        fscanf( fileread, "%c", &cTmp );
    }

    return cTmp;
}

//------------------------------------------------------------------------------
void goto_colon(FILE* fileread)
{
    // ZZ> This function moves a file read pointer to the next colon
    char cTmp;

    fscanf(fileread, "%c", &cTmp);
    while (cTmp != ':')
    {
        fscanf(fileread, "%c", &cTmp);
    }
}

//--------------------------------------------------------------------------------------------
bool_t goto_colon_yesno( FILE* fileread )
{
    // ZZ> This function moves a file read pointer to the next colon, or it returns
    //     bfalse if there are no more
    char cTmp;

    do
    {
        if ( fscanf( fileread, "%c", &cTmp ) == EOF )
        {
            return bfalse;
        }
    }
    while ( cTmp != ':' );

    return btrue;
}

//--------------------------------------------------------------------------------------------
void get_name( FILE* fileread, char *szName )
{
    // ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
    //     it for underscores.  The szName argument is rewritten with the null terminated
    //     string
    int cnt;
    char cTmp;
    char szTmp[256];

    fscanf( fileread, "%s", szTmp );
    cnt = 0;

    while ( cnt < MAXCAPNAMESIZE - 1 )
    {
        cTmp = szTmp[cnt];
        if ( cTmp == '_' )  cTmp = ' ';

        szName[cnt] = cTmp;
        cnt++;
    }

    szName[cnt] = 0;
}


//--------------------------------------------------------------------------------------------
void read_pair( FILE* fileread )
{
    // ZZ> This function reads a damage/stat pair ( eg. 5-9 )
    char cTmp;
    float  fBase, fRand;

    fscanf( fileread, "%f", &fBase );  // The first number
    pairbase = fBase * 256;
    cTmp = get_first_letter( fileread );  // The hyphen
    if ( cTmp != '-' )
    {
        // Not in correct format, so fail
        pairrand = 1;
        return;
    }

    fscanf( fileread, "%f", &fRand );  // The second number
    pairrand = fRand * 256;
    pairrand = pairrand - pairbase;
    if ( pairrand < 1 )
        pairrand = 1;
}



//--------------------------------------------------------------------------------------------
int fget_int( FILE* fileread )
{
    int iTmp = 0;
    if ( feof( fileread ) ) return iTmp;

    fscanf( fileread, "%d", &iTmp );
    return iTmp;
}

