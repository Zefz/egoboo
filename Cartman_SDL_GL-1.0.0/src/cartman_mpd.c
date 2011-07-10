#include "cartman_mpd.h"

#include <egolib.h>

#include "cartman.h"
#include "cartman_math.inl"

#include <assert.h>
#include <math.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static Uint32  atvertex = 0;           // Current vertex check for new

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

mesh_t mesh;
size_t numwritten = 0;
size_t numattempt = 0;
Uint32 numfreevertices = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void load_mesh_fans()
{
    // ZZ> This function loads fan types for the mesh...  Starting vertex
    //     positions and number of vertices
    int cnt, entry;
    int numfantype, fantype, bigfantype, vertices;
    int numcommand, command, command_size;
    int itmp;
    float ftmp;
    vfs_FILE* fileread;
    STRING fname;

    // Initialize all mesh types to 0
    for ( entry = 0; entry < MAXMESHTYPE; entry++ )
    {
        mesh.numline[entry] = 0;
        mesh.command[entry].numvertices = 0;
    }

    // Open the file and go to it
    sprintf( fname, "%s" SLASH_STR "basicdat" SLASH_STR "fans.txt", egoboo_path );
    fileread = vfs_openRead( fname );
    if ( NULL == fileread )
    {
        log_error( "load_mesh_fans() - Cannot find fans.txt file\n" );
    }

    goto_colon( NULL, fileread, bfalse );
    vfs_scanf( fileread, "%d", &numfantype );

    for ( fantype = 0, bigfantype = MAXMESHTYPE / 2; fantype < numfantype; fantype++, bigfantype++ )
    {
        goto_colon( NULL, fileread, bfalse );
        vfs_scanf( fileread, "%d", &vertices );
        mesh.command[fantype].numvertices = vertices;
        mesh.command[fantype+MAXMESHTYPE/2].numvertices = vertices;  // DUPE

        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            // lighting "ref" data
            goto_colon( NULL, fileread, bfalse );
            vfs_scanf( fileread, "%d", &itmp );
            mesh.command[fantype].ref[cnt] = itmp;
            mesh.command[fantype+MAXMESHTYPE/2].ref[cnt] = itmp;  // DUPE

            // texure u data and mesh x data
            goto_colon( NULL, fileread, bfalse );
            vfs_scanf( fileread, "%f", &ftmp );

            mesh.command[fantype].u[cnt] = ftmp;
            mesh.command[fantype+MAXMESHTYPE/2].u[cnt] = ftmp;  // DUPE

            mesh.command[fantype].x[cnt] = ( ftmp ) * 128;
            mesh.command[fantype+MAXMESHTYPE/2].x[cnt] = ( ftmp ) * 128;  // DUPE

            // texure v data and mesh y data
            goto_colon( NULL, fileread, bfalse );
            vfs_scanf( fileread, "%f", &ftmp );
            mesh.command[fantype].v[cnt] = ftmp;
            mesh.command[fantype+MAXMESHTYPE/2].v[cnt] = ftmp;  // DUPE

            mesh.command[fantype].y[cnt] = ( ftmp ) * 128;
            mesh.command[fantype+MAXMESHTYPE/2].y[cnt] = ( ftmp ) * 128;  // DUPE
        }

        // Get the vertex connections
        goto_colon( NULL, fileread, bfalse );
        vfs_scanf( fileread, "%d", &numcommand );
        mesh.command[fantype].count = numcommand;
        mesh.command[bigfantype].count = numcommand;  // Dupe

        entry = 0;
        for ( command = 0; command < numcommand; command++ )
        {
            // grab the fan vertex data
            goto_colon( NULL, fileread, bfalse );
            vfs_scanf( fileread, "%d", &command_size );
            mesh.command[fantype].size[command] = command_size;
            mesh.command[bigfantype].size[command] = command_size;  // Dupe

            for ( cnt = 0; cnt < command_size; cnt++ )
            {
                goto_colon( NULL, fileread, bfalse );
                vfs_scanf( fileread, "%d", &itmp );
                mesh.command[fantype].vrt[entry] = itmp;
                mesh.command[bigfantype].vrt[entry] = itmp;  // Dupe
                entry++;
            }
        }
    }
    vfs_close( fileread );

    for ( fantype = 0, bigfantype = MAXMESHTYPE / 2; fantype < numfantype; fantype++, bigfantype++ )
    {
        int inow, ilast, fancenter;

        entry = 0;
        numcommand = mesh.command[fantype].count;
        for ( command = 0; command < numcommand; command++ )
        {
            command_size = mesh.command[fantype].size[command];

            // convert the fan data into lines representing the fan edges
            fancenter = mesh.command[fantype].vrt[entry++];
            inow      = mesh.command[fantype].vrt[entry++];
            for ( cnt = 2; cnt < command_size; cnt++, entry++ )
            {
                ilast = inow;
                inow = mesh.command[fantype].vrt[entry];

                add_line( fantype, fancenter, inow );
                add_line( fantype, fancenter, ilast );
                add_line( fantype, ilast,     inow );

                add_line( bigfantype, fancenter, inow );
                add_line( bigfantype, fancenter, ilast );
                add_line( bigfantype, ilast,     inow );
            }
        }
    }

    // Correct all of them silly texture positions for seamless tiling
    for ( entry = 0; entry < MAXMESHTYPE / 2; entry++ )
    {
        for ( cnt = 0; cnt < mesh.command[entry].numvertices; cnt++ )
        {
            mesh.command[entry].u[cnt] = ( 0.6f / SMALLXY ) + ( mesh.command[entry].u[cnt] * ( SMALLXY - 2 * 0.6f ) / SMALLXY );
            mesh.command[entry].v[cnt] = ( 0.6f / SMALLXY ) + ( mesh.command[entry].v[cnt] * ( SMALLXY - 2 * 0.6f ) / SMALLXY );
        }
    }

    // Do for big tiles too
    for ( /*nothing*/; entry < MAXMESHTYPE; entry++ )
    {
        for ( cnt = 0; cnt < mesh.command[entry].numvertices; cnt++ )
        {
            mesh.command[entry].u[cnt] = ( 0.6f / BIGXY ) + ( mesh.command[entry].u[cnt] * ( BIGXY - 2 * 0.6f ) / BIGXY );
            mesh.command[entry].v[cnt] = ( 0.6f / BIGXY ) + ( mesh.command[entry].v[cnt] * ( BIGXY - 2 * 0.6f ) / BIGXY );
        }
    }
}

//--------------------------------------------------------------------------------------------
void make_fanstart()
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    cnt = 0;
    while ( cnt < mesh.tiles_y )
    {
        mesh.fanstart[cnt] = mesh.tiles_x * cnt;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void make_twist()
{
    Uint32 fan, numfan;

    numfan = mesh.tiles_x * mesh.tiles_y;
    fan = 0;
    while ( fan < numfan )
    {
        mesh.twist[fan] = get_fan_twist( fan );
        fan++;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void create_mesh( mesh_info_t * pinfo )
{
    // ZZ> This function makes the mesh
    int mapx, mapy, fan, tile;
    int x, y;

    free_vertices();

    if ( NULL == pinfo ) return;

    mesh.tiles_x = pinfo->tiles_x;
    mesh.tiles_y = pinfo->tiles_y;

    mesh.edgex = mesh.tiles_x * TILE_SIZE;
    mesh.edgey = mesh.tiles_y * TILE_SIZE;
    mesh.edgez = 180 << 4;

    fan = 0;
    tile = 0;
    for ( mapy = 0; mapy < mesh.tiles_y; mapy++ )
    {
        y = mapy * TILE_SIZE;
        for ( mapx = 0; mapx < mesh.tiles_x; mapx++ )
        {
            x = mapx * TILE_SIZE;

            mesh.fantype[fan] = 0;
            mesh.tx_bits[fan] = ((( mapx & 1 ) + ( mapy & 1 ) ) & 1 ) + DEFAULT_TILE;

            fan++;
        }
    }

    make_fanstart();
}

//--------------------------------------------------------------------------------------------
int load_mesh( const char *modname )
{
    vfs_FILE* fileread;
    STRING  newloadname;
    Uint32  uiTmp32;
    Sint32   iTmp32;
    Uint8  uiTmp8;
    int num, cnt;
    float ftmp;
    int fan;
    Uint32 numvert, numfan;
    Uint32 vert;
    int x, y;

    sprintf( newloadname, "%s" SLASH_STR "gamedat" SLASH_STR "level.mpd", modname );

    fileread = vfs_openReadB( newloadname );
    if ( NULL == fileread )
    {
        log_warning( "load_mesh() - Cannot find mesh for module \"%s\"\n", modname );
    }

    if ( fileread )
    {
        free_vertices();

        vfs_read( &uiTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32( uiTmp32 ); if ( uiTmp32 != MAPID ) return bfalse;
        vfs_read( &uiTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32( iTmp32 ); numvert = uiTmp32;
        vfs_read( &iTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32( iTmp32 ); mesh.tiles_x = iTmp32;
        vfs_read( &iTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32( iTmp32 ); mesh.tiles_y = iTmp32;

        numfan = mesh.tiles_x * mesh.tiles_y;
        mesh.edgex = mesh.tiles_x * TILE_SIZE;
        mesh.edgey = mesh.tiles_y * TILE_SIZE;
        mesh.edgez = 180 << 4;
        numfreevertices = MAXTOTALMESHVERTICES - numvert;

        // Load fan data
        fan = 0;
        while ( fan < numfan )
        {
            vfs_read( &uiTmp32, 4, 1, fileread );
            uiTmp32 = ENDIAN_INT32( uiTmp32 );

            mesh.fantype[fan] = ( uiTmp32 >> 24 ) & 0x00FF;
            mesh.fx[fan]      = ( uiTmp32 >> 16 ) & 0x00FF;
            mesh.tx_bits[fan] = ( uiTmp32 >>  0 ) & 0xFFFF;

            fan++;
        }

        // Load normal data
        // Load fan data
        fan = 0;
        while ( fan < numfan )
        {
            vfs_read( &uiTmp8, 1, 1, fileread );
            mesh.twist[fan] = uiTmp8;

            fan++;
        }

        // Load vertex x data
        cnt = 0;
        while ( cnt < numvert )
        {
            vfs_read( &ftmp, 4, 1, fileread ); ftmp = ENDIAN_FLOAT( ftmp );
            mesh.vrtx[cnt] = ftmp;
            cnt++;
        }

        // Load vertex y data
        cnt = 0;
        while ( cnt < numvert )
        {
            vfs_read( &ftmp, 4, 1, fileread ); ftmp = ENDIAN_FLOAT( ftmp );
            mesh.vrty[cnt] = ftmp;
            cnt++;
        }

        // Load vertex z data
        cnt = 0;
        while ( cnt < numvert )
        {
            vfs_read( &ftmp, 4, 1, fileread ); ftmp = ENDIAN_FLOAT( ftmp );
            mesh.vrtz[cnt] = ftmp;

            if ( ftmp > 0 &&  mesh.edgez < 2.0f * ftmp )
            {
                mesh.edgez = 2.0f * ftmp;
            }
            else if ( ftmp < 0 &&  mesh.edgez < -2.0f * ftmp )
            {
                mesh.edgez = -2.0f * ftmp;
            }

            cnt++;
        }

        // Load vertex a data
        cnt = 0;
        while ( cnt < numvert )
        {
            vfs_read( &uiTmp8, 1, 1, fileread );
            mesh.vrta[cnt] = CLIP( uiTmp8, 1, 255 );  // VERTEXUNUSED == unused
            cnt++;
        }

        make_fanstart();

        // store the vertices in the vertex chain for editing
        vert = 0;
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    int type = mesh.fantype[fan];
                    if ( type >= 0 && type < MAXMESHTYPE )
                    {
                        num = mesh.command[type].numvertices;
                        mesh.vrtstart[fan] = vert;
                        cnt = 0;
                        while ( cnt < num )
                        {
                            mesh.vrtnext[vert] = vert + 1;
                            vert++;
                            cnt++;
                        }
                    }
                    else
                    {
                        assert( 0 );
                    }
                }
                mesh.vrtnext[vert-1] = CHAINEND;
                x++;
            }
            y++;
        }
        return btrue;
    }
    return bfalse;
}

//--------------------------------------------------------------------------------------------
void save_mesh( const char *modname )
{
#define ISAVE(VAL) numwritten += vfs_write(&VAL, sizeof(Uint32), 1, filewrite); numattempt++
#define FSAVE(VAL) numwritten += vfs_write(&VAL, sizeof(float), 1, filewrite); numattempt++

    vfs_FILE* filewrite;
    STRING newloadname;
    int itmp;
    float ftmp;
    int fan, x, y, cnt, num;
    Uint32 vert;
    Uint8 ctmp;

    make_twist();

    sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "level.mpd", egoboo_path, modname );

    filewrite = vfs_openWriteB( newloadname );
    if ( filewrite )
    {
        itmp = MAPID;             ISAVE( itmp );
        itmp = count_vertices();  ISAVE( itmp );
        itmp = mesh.tiles_x;      ISAVE( itmp );
        itmp = mesh.tiles_y;      ISAVE( itmp );

        // Write tile data
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    itmp = ( mesh.fantype[fan] << 24 ) + ( mesh.fx[fan] << 16 ) + mesh.tx_bits[fan];  ISAVE( itmp );
                }
                x++;
            }
            y++;
        }

        // Write twist data
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    ctmp = mesh.twist[fan];
                    numwritten += vfs_write( &ctmp, 1, 1, filewrite );
                }
                numattempt++;
                x++;
            }
            y++;
        }

        // Write x vertices
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;

                    for ( cnt = 0, vert = mesh.vrtstart[fan];
                          cnt < num && CHAINEND != vert ;
                          cnt++, vert = mesh.vrtnext[vert] )
                    {
                        ftmp = mesh.vrtx[vert];  FSAVE( ftmp );
                    }
                }
                x++;
            }
            y++;
        }

        // Write y vertices
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;

                    for ( cnt = 0, vert = mesh.vrtstart[fan];
                          cnt < num && CHAINEND != vert ;
                          cnt++, vert = mesh.vrtnext[vert] )
                    {
                        ftmp = mesh.vrty[vert];  FSAVE( ftmp );
                    }
                }
                x++;
            }
            y++;
        }

        // Write z vertices
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;

                    for ( cnt = 0, vert = mesh.vrtstart[fan];
                          cnt < num && CHAINEND != vert ;
                          cnt++, vert = mesh.vrtnext[vert] )
                    {
                        ftmp = mesh.vrtz[vert];  FSAVE( ftmp );
                    }
                }
                x++;
            }
            y++;
        }

        // Write a vertices
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;

                    for ( cnt = 0, vert = mesh.vrtstart[fan];
                          cnt < num && CHAINEND != vert;
                          cnt++, vert = mesh.vrtnext[vert] )
                    {
                        ctmp = mesh.vrta[vert];
                        numwritten += vfs_write( &ctmp, 1, 1, filewrite );
                        numattempt++;
                    }
                }
                x++;
            }
            y++;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int mesh_get_fan( int x, int y )
{
    int fan = -1;
    if ( y >= 0 && y < MAXMESHTILEY && y < mesh.tiles_y )
    {
        if ( x >= 0 && x < MAXMESHTILEY && x < mesh.tiles_x )
        {
            fan = mesh.fanstart[y] + x;
        }
    }
    return fan;
};

//--------------------------------------------------------------------------------------------
void num_free_vertex()
{
    // ZZ> This function counts the unused vertices and sets numfreevertices
    int cnt, num;

    num = 0;
    for ( cnt = 0; cnt < MAXTOTALMESHVERTICES; cnt++ )
    {
        if ( VERTEXUNUSED == mesh.vrta[cnt] )
        {
            num++;
        }
    }

    numfreevertices = num;
}

//--------------------------------------------------------------------------------------------
int count_vertices()
{
    int fan, x, y, cnt, num, totalvert;
    Uint32 vert;

    totalvert = 0;
    y = 0;
    while ( y < mesh.tiles_y )
    {
        x = 0;
        while ( x < mesh.tiles_x )
        {
            fan = mesh_get_fan( x, y );
            if ( -1 != fan )
            {
                num = mesh.command[mesh.fantype[fan]].numvertices;

                for ( cnt = 0, vert = mesh.vrtstart[fan];
                      cnt < num && CHAINEND != vert ;
                      cnt++, vert = mesh.vrtnext[vert] )
                {
                    totalvert++;
                }
            }
            x++;
        }
        y++;
    }
    return totalvert;
}

//--------------------------------------------------------------------------------------------
void add_line( int fantype, int start, int end )
{
    // ZZ> This function adds a line to the vertex schematic
    int cnt;

    if ( fantype < 0 || fantype >= MAXMESHTYPE ) return;

    if ( mesh.numline[fantype] >= MAXMESHTYPE ) return;

    // Make sure line isn't already in list
    for ( cnt = 0; cnt < mesh.numline[fantype]; cnt++ )
    {
        if (( mesh.line[fantype].start[cnt] == start && mesh.line[fantype].end[cnt] == end ) ||
            ( mesh.line[fantype].end[cnt] == start && mesh.line[fantype].start[cnt] == end ) )
        {
            return;
        }
    }

    // Add it in
    cnt = mesh.numline[fantype];
    mesh.line[fantype].start[cnt] = start;
    mesh.line[fantype].end[cnt]   = end;
    mesh.numline[fantype]++;
}

//--------------------------------------------------------------------------------------------
void free_vertices()
{
    // ZZ> This function sets all vertices to unused
    int cnt;

    cnt = 0;
    while ( cnt < MAXTOTALMESHVERTICES )
    {
        mesh.vrta[cnt] = VERTEXUNUSED;
        cnt++;
    }
    atvertex = 0;
    numfreevertices = MAXTOTALMESHVERTICES;
}

//--------------------------------------------------------------------------------------------
int get_free_vertex()
{
    // ZZ> This function returns btrue if it can find an unused vertex, and it
    // will set atvertex to that vertex index.  bfalse otherwise.
    int cnt;

    if ( numfreevertices != 0 )
    {
        for ( cnt = 0;
              cnt < MAXTOTALMESHVERTICES && VERTEXUNUSED != mesh.vrta[atvertex];
              cnt++ )
        {
            atvertex++;
            if ( atvertex == MAXTOTALMESHVERTICES )
            {
                atvertex = 0;
            }

        }

        if ( VERTEXUNUSED == mesh.vrta[atvertex] )
        {
            mesh.vrta[atvertex] = 60;
            return btrue;
        }
    }
    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint8 get_twist( int x, int y )
{
    Uint8 twist;

    // x and y should be from -7 to 8
    if ( x < -7 ) x = -7;
    if ( x > 8 ) x = 8;
    if ( y < -7 ) y = -7;
    if ( y > 8 ) y = 8;

    // Now between 0 and 15
    x = x + 7;
    y = y + 7;
    twist = ( y << 4 ) + x;

    return twist;
}

//--------------------------------------------------------------------------------------------
Uint8 get_fan_twist( Uint32 fan )
{
    int zx, zy, vt0, vt1, vt2, vt3;
    Uint8 twist;

    vt0 = mesh.vrtstart[fan];
    vt1 = mesh.vrtnext[vt0];
    vt2 = mesh.vrtnext[vt1];
    vt3 = mesh.vrtnext[vt2];

    zx = ( mesh.vrtz[vt0] + mesh.vrtz[vt3] - mesh.vrtz[vt1] - mesh.vrtz[vt2] ) / SLOPE;
    zy = ( mesh.vrtz[vt2] + mesh.vrtz[vt3] - mesh.vrtz[vt0] - mesh.vrtz[vt1] ) / SLOPE;

    twist = get_twist( zx, zy );

    return twist;
}

//--------------------------------------------------------------------------------------------
int get_level( int x, int y )
{
    int fan;
    int z0, z1, z2, z3;         // Height of each fan corner
    int zleft, zright, zdone;   // Weighted height of each side

    zdone = 0;
    fan = mesh_get_fan( floor( x / ( float )TILE_SIZE ), floor( y / ( float )TILE_SIZE ) );
    if ( -1 != fan )
    {
        x &= 127;
        y &= 127;

        z0 = mesh.vrtz[mesh.vrtstart[fan] + 0];
        z1 = mesh.vrtz[mesh.vrtstart[fan] + 1];
        z2 = mesh.vrtz[mesh.vrtstart[fan] + 2];
        z3 = mesh.vrtz[mesh.vrtstart[fan] + 3];

        zleft = ( z0 * ( 128 - y ) + z3 * y ) / 128;
        zright = ( z1 * ( 128 - y ) + z2 * y ) / 128;
        zdone = ( zleft * ( 128 - x ) + zright * x ) / 128;
    }

    return ( zdone );
}

//--------------------------------------------------------------------------------------------
int add_fan( int fan, float x, float y )
{
    // ZZ> This function allocates the vertices needed for a fan
    int cnt;
    int numvert;
    Uint32 vertex;
    Uint32 vertexlist[17];

    numvert = mesh.command[mesh.fantype[fan]].numvertices;
    if ( numfreevertices >= numvert )
    {
        mesh.fx[fan] = MPDFX_SHA;
        cnt = 0;
        while ( cnt < numvert )
        {
            if ( get_free_vertex() == bfalse )
            {
                // Reset to unused
                numvert = cnt;
                cnt = 0;
                while ( cnt < numvert )
                {
                    mesh.vrta[vertexlist[cnt]] = 60;
                    cnt++;
                }
                return bfalse;
            }
            vertexlist[cnt] = atvertex;
            cnt++;
        }
        vertexlist[cnt] = CHAINEND;

        cnt = 0;
        while ( cnt < numvert )
        {
            vertex = vertexlist[cnt];
            mesh.vrtx[vertex] = x + ( mesh.command[mesh.fantype[fan]].x[cnt] >> 2 );
            mesh.vrty[vertex] = y + ( mesh.command[mesh.fantype[fan]].y[cnt] >> 2 );
            mesh.vrtz[vertex] = 0;
            mesh.vrtnext[vertex] = vertexlist[cnt+1];
            cnt++;
        }
        mesh.vrtstart[fan] = vertexlist[0];
        numfreevertices -= numvert;
        return btrue;
    }
    return bfalse;
}

//--------------------------------------------------------------------------------------------
void remove_fan( int fan )
{
    // ZZ> This function removes a fan's vertices from usage and sets the fan
    //     to not be drawn
    int cnt, vert;
    Uint32 numvert;

    numvert = mesh.command[mesh.fantype[fan]].numvertices;

    for ( cnt = 0, vert = mesh.vrtstart[fan];
          cnt < numvert && CHAINEND != vert;
          cnt++, vert = mesh.vrtnext[vert] )
    {
        mesh.vrta[vert] = VERTEXUNUSED;
        numfreevertices++;
    }

    mesh.fantype[fan] = 0;
    mesh.fx[fan] = MPDFX_SHA;
}

//--------------------------------------------------------------------------------------------
int get_vertex( int x, int y, int num )
{
    // ZZ> This function gets a vertex number or -1
    int vert, cnt;
    int fan;

    vert = -1;
    fan = mesh_get_fan( x, y );
    if ( -1 != fan )
    {
        if ( mesh.command[mesh.fantype[fan]].numvertices > num )
        {
            vert = mesh.vrtstart[fan];
            cnt = 0;
            while ( cnt < num )
            {
                vert = mesh.vrtnext[vert];
                if ( vert == -1 )
                {
                    return vert;
                }
                cnt++;
            }
        }
    }

    return vert;
}

//--------------------------------------------------------------------------------------------
int fan_at( int x, int y )
{
    return mesh_get_fan( x, y );
}