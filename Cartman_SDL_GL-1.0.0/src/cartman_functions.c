#include "cartman_functions.h"

#include "cartman.h"
#include "cartman_mpd.h"

#include "cartman_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

enum
{
    CORNER_TL,
    CORNER_TR,
    CORNER_BL,
    CORNER_BR,
    CORNER_COUNT
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int     numselect_verts = 0;
static Uint32  select_verts[MAXSELECT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float dist_from_border( float x, float y )
{
    float x_dst, y_dst;

    if ( x < 0.0f ) x_dst = 0.0f;
    else if ( x > mesh.edgex * 0.5f ) x_dst = mesh.edgex - x;
    else if ( x > mesh.edgex ) x_dst = 0.0f;
    else x_dst = x;

    if ( y < 0.0f ) y_dst = 0.0f;
    else if ( y > mesh.edgey * 0.5f ) y_dst = mesh.edgey - y;
    else if ( y > mesh.edgey ) y_dst = 0.0f;
    else y_dst = x;

    return ( x < y ) ? x : y;
}

//--------------------------------------------------------------------------------------------
int dist_from_edge( int x, int y )
{
    if ( x > ( mesh.tiles_x >> 1 ) )
        x = mesh.tiles_x - x - 1;
    if ( y > ( mesh.tiles_y >> 1 ) )
        y = mesh.tiles_y - y - 1;
    if ( x < y )
        return x;
    return y;
}

//--------------------------------------------------------------------------------------------
void fix_mesh( void )
{
    // ZZ> This function corrects corners across entire mesh
    int x, y;

    for ( y = 0; y < mesh.tiles_y; y++ )
    {

        for ( x = 0; x < mesh.tiles_x; x++ )
        {
            //      fix_corners(x, y);
            fix_vertices( x, y );
        }
    }
}

//--------------------------------------------------------------------------------------------
void fix_vertices( int x, int y )
{
    int fan;
    int cnt;

    fix_corners( x, y );
    fan = mesh_get_fan( x, y );
    if ( -1 != fan )
    {
        cnt = 4;
        while ( cnt < mesh.command[mesh.fantype[fan]].numvertices )
        {
            weld_cnt( x, y, cnt, fan );
            cnt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void fix_corners( int x, int y )
{
    int fan;

    fan = mesh_get_fan( x, y );
    if ( -1 != fan )
    {
        weld_0( x, y );
        weld_1( x, y );
        weld_2( x, y );
        weld_3( x, y );
    }
}

//--------------------------------------------------------------------------------------------
void weld_0( int x, int y )
{
    select_clear();
    select_add( get_vertex( x, y, 0 ) );
    select_add( get_vertex( x - 1, y, 1 ) );
    select_add( get_vertex( x, y - 1, 3 ) );
    select_add( get_vertex( x - 1, y - 1, 2 ) );
    weld_select();
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_1( int x, int y )
{
    select_clear();
    select_add( get_vertex( x, y, 1 ) );
    select_add( get_vertex( x + 1, y, 0 ) );
    select_add( get_vertex( x, y - 1, 2 ) );
    select_add( get_vertex( x + 1, y - 1, 3 ) );
    weld_select();
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_2( int x, int y )
{
    select_clear();
    select_add( get_vertex( x, y, 2 ) );
    select_add( get_vertex( x + 1, y, 3 ) );
    select_add( get_vertex( x, y + 1, 1 ) );
    select_add( get_vertex( x + 1, y + 1, 0 ) );
    weld_select();
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_3( int x, int y )
{
    select_clear();
    select_add( get_vertex( x, y, 3 ) );
    select_add( get_vertex( x - 1, y, 2 ) );
    select_add( get_vertex( x, y + 1, 0 ) );
    select_add( get_vertex( x - 1, y + 1, 1 ) );
    weld_select();
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_cnt( int x, int y, int cnt, Uint32 fan )
{
    if ( mesh.command[mesh.fantype[fan]].x[cnt] < NEARLOW + 1 ||
         mesh.command[mesh.fantype[fan]].y[cnt] < NEARLOW + 1 ||
         mesh.command[mesh.fantype[fan]].x[cnt] > NEARHI - 1 ||
         mesh.command[mesh.fantype[fan]].y[cnt] > NEARHI - 1 )
    {
        select_clear();
        select_add( get_vertex( x, y, cnt ) );
        if ( mesh.command[mesh.fantype[fan]].x[cnt] < NEARLOW + 1 )
            select_add( nearest_vertex( x - 1, y, NEARHI, mesh.command[mesh.fantype[fan]].y[cnt] ) );
        if ( mesh.command[mesh.fantype[fan]].y[cnt] < NEARLOW + 1 )
            select_add( nearest_vertex( x, y - 1, mesh.command[mesh.fantype[fan]].x[cnt], NEARHI ) );
        if ( mesh.command[mesh.fantype[fan]].x[cnt] > NEARHI - 1 )
            select_add( nearest_vertex( x + 1, y, NEARLOW, mesh.command[mesh.fantype[fan]].y[cnt] ) );
        if ( mesh.command[mesh.fantype[fan]].y[cnt] > NEARHI - 1 )
            select_add( nearest_vertex( x, y + 1, mesh.command[mesh.fantype[fan]].x[cnt], NEARLOW ) );
        weld_select();
        select_clear();
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void select_add( int vert )
{
    // ZZ> This function highlights a vertex
    int cnt, found;

    if ( vert < 0 || vert >= MAXTOTALMESHVERTICES ) return;

    if ( numselect_verts >= MAXSELECT ) return;

    found = bfalse;
    for ( cnt = 0; cnt < numselect_verts; cnt++ )
    {
        if ( select_verts[cnt] == vert )
        {
            found = btrue;
            break;
        }
    }

    if ( !found )
    {
        select_verts[numselect_verts] = vert;
        numselect_verts++;
    }
}

//--------------------------------------------------------------------------------------------
void select_clear( void )
{
    // ZZ> This function unselects all vertices
    numselect_verts = 0;
}

//--------------------------------------------------------------------------------------------
int select_has_vert( int vert )
{
    // ZZ> This function returns btrue if the vertex has been highlighted by user
    int cnt;

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        if ( vert == select_verts[cnt] )
        {
            return btrue;
        }
        cnt++;
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void select_remove( int vert )
{
    // ZZ> This function makes sure the vertex is not highlighted
    int cnt;
    bool_t stillgoing;

    stillgoing = btrue;
    for ( cnt = 0; cnt < numselect_verts; cnt++ )
    {
        if ( vert == select_verts[cnt] )
        {
            stillgoing = bfalse;
            break;
        }
    }

    if ( !stillgoing )
    {
        for ( /* nothing */; cnt < numselect_verts; cnt++ )
        {
            select_verts[cnt-1] = select_verts[cnt];
        }
        numselect_verts--;
    }
}

//--------------------------------------------------------------------------------------------
int select_count()
{
    return numselect_verts;
}

//--------------------------------------------------------------------------------------------
void select_add_rect( float tlx, float tly, float brx, float bry, int mode )
{
    // ZZ> This function checks the rectangular selection

    Uint32 vert;

    if ( tlx > brx )  { float tmp = tlx;  tlx = brx;  brx = tmp; }
    if ( tly > bry )  { float tmp = tly;  tly = bry;  bry = tmp; }

    if ( mode == WINMODE_VERTEX )
    {
        for ( vert = 0; vert < MAXTOTALMESHVERTICES; vert++ )
        {
            if ( VERTEXUNUSED == mesh.vrta[vert] ) continue;

            if ( mesh.vrtx[vert] >= tlx &&
                 mesh.vrtx[vert] <= brx &&
                 mesh.vrty[vert] >= tly &&
                 mesh.vrty[vert] <= bry )
            {
                select_add( vert );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
        for ( vert = 0; vert < MAXTOTALMESHVERTICES; vert++ )
        {
            if ( VERTEXUNUSED == mesh.vrta[vert] ) continue;

            if ( mesh.vrtx[vert] >= tlx &&
                 mesh.vrtx[vert] <= brx &&
                 mesh.vrtz[vert] >= tly &&
                 mesh.vrtz[vert] <= bry )
            {
                select_add( vert );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void select_remove_rect( float tlx, float tly, float brx, float bry, int mode )
{
    // ZZ> This function checks the rectangular selection, and removes any fans
    //     in the selection area

    Uint32 vert;

    if ( tlx > brx )  { float tmp = tlx;  tlx = brx;  brx = tmp; }
    if ( tly > bry )  { float tmp = tly;  tly = bry;  bry = tmp; }

    if ( mode == WINMODE_VERTEX )
    {
        for ( vert = 0; vert < MAXTOTALMESHVERTICES; vert++ )
        {
            if ( VERTEXUNUSED == mesh.vrta[vert] ) continue;

            if ( mesh.vrtx[vert] >= tlx &&
                 mesh.vrtx[vert] <= brx &&
                 mesh.vrty[vert] >= tly &&
                 mesh.vrty[vert] <= bry )
            {
                select_remove( vert );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
        for ( vert = 0; vert < MAXTOTALMESHVERTICES; vert++ )
        {
            if ( VERTEXUNUSED == mesh.vrta[vert] ) continue;

            if ( mesh.vrtx[vert] >= tlx &&
                 mesh.vrtx[vert] <= brx &&
                 mesh.vrtz[vert] >= tly &&
                 mesh.vrtz[vert] <= bry )
            {
                select_remove( vert );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int nearest_vertex( int x, int y, float nearx, float neary )
{
    // ZZ> This function gets a vertex number or -1
    int vert, bestvert, cnt;
    int fan;
    int num;
    float prox, proxx, proxy, bestprox;

    bestvert = -1;
    fan = mesh_get_fan( x, y );
    if ( -1 != fan )
    {
        num = mesh.command[mesh.fantype[fan]].numvertices;
        vert = mesh.vrtstart[fan];
        vert = mesh.vrtnext[vert];
        vert = mesh.vrtnext[vert];
        vert = mesh.vrtnext[vert];
        vert = mesh.vrtnext[vert];
        bestprox = 9000;
        cnt = 4;
        while ( cnt < num )
        {
            proxx = mesh.command[mesh.fantype[fan]].x[cnt] - nearx;
            proxy = mesh.command[mesh.fantype[fan]].y[cnt] - neary;
            if ( proxx < 0 ) proxx = -proxx;
            if ( proxy < 0 ) proxy = -proxy;
            prox = proxx + proxy;
            if ( prox < bestprox )
            {
                bestvert = vert;
                bestprox = prox;
            }
            vert = mesh.vrtnext[vert];
            cnt++;
        }
    }
    return bestvert;
}

//--------------------------------------------------------------------------------------------
void move_select( int x, int y, int z )
{
    int vert, cnt, newx, newy, newz;

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        newx = mesh.vrtx[vert] + x;
        newy = mesh.vrty[vert] + y;
        newz = mesh.vrtz[vert] + z;
        if ( newx < 0 )  x = 0 - mesh.vrtx[vert];
        if ( newx > mesh.edgex ) x = mesh.edgex - mesh.vrtx[vert];
        if ( newy < 0 )  y = 0 - mesh.vrty[vert];
        if ( newy > mesh.edgey ) y = mesh.edgey - mesh.vrty[vert];
        if ( newz < 0 )  z = 0 - mesh.vrtz[vert];
        if ( newz > mesh.edgez ) z = mesh.edgez - mesh.vrtz[vert];
        cnt++;
    }

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        newx = mesh.vrtx[vert] + x;
        newy = mesh.vrty[vert] + y;
        newz = mesh.vrtz[vert] + z;

        if ( newx < 0 )  newx = 0;
        if ( newx > mesh.edgex )  newx = mesh.edgex;
        if ( newy < 0 )  newy = 0;
        if ( newy > mesh.edgey )  newy = mesh.edgey;
        if ( newz < 0 )  newz = 0;
        if ( newz > mesh.edgez )  newz = mesh.edgez;

        mesh.vrtx[vert] = newx;
        mesh.vrty[vert] = newy;
        mesh.vrtz[vert] = newz;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void set_select_no_bound_z( int z )
{
    int vert, cnt;

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        mesh.vrtz[vert] = z;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void jitter_select()
{
    int cnt;
    Uint32 vert;

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        move_vert( vert, ( rand() % 3 ) - 1, ( rand() % 3 ) - 1, 0 );
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void select_verts_connected()
{
    int vert, cnt, tnc, x, y, totalvert = 0;
    int fan;
    Uint8 found, select_vertsfan;

    y = 0;
    while ( y < mesh.tiles_y )
    {
        x = 0;
        while ( x < mesh.tiles_x )
        {
            fan = mesh_get_fan( x, y );
            select_vertsfan = bfalse;
            if ( -1 != fan )
            {
                totalvert = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                vert = mesh.vrtstart[fan];
                while ( cnt < totalvert )
                {

                    found = bfalse;
                    tnc = 0;
                    while ( tnc < numselect_verts && !found )
                    {
                        if ( select_verts[tnc] == vert )
                        {
                            found = btrue;
                        }
                        tnc++;
                    }
                    if ( found ) select_vertsfan = btrue;
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }

            if ( select_vertsfan )
            {
                cnt = 0;
                vert = mesh.vrtstart[fan];
                while ( cnt < totalvert )
                {
                    select_add( vert );
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
}

//--------------------------------------------------------------------------------------------
void weld_select()
{
    // ZZ> This function welds the highlighted vertices
    int cnt, x, y, z, a;
    Uint32 vert;

    if ( numselect_verts > 1 )
    {
        x = 0;
        y = 0;
        z = 0;
        a = 0;
        cnt = 0;
        while ( cnt < numselect_verts )
        {
            vert = select_verts[cnt];
            x += mesh.vrtx[vert];
            y += mesh.vrty[vert];
            z += mesh.vrtz[vert];
            a += mesh.vrta[vert];
            cnt++;
        }
        x += cnt >> 1;  y += cnt >> 1;
        x = x / numselect_verts;
        y = y / numselect_verts;
        z = z / numselect_verts;
        a = a / numselect_verts;
        cnt = 0;
        while ( cnt < numselect_verts )
        {
            vert = select_verts[cnt];
            mesh.vrtx[vert] = x;
            mesh.vrty[vert] = y;
            mesh.vrtz[vert] = z;
            mesh.vrta[vert] = CLIP( a, 1, 255 );
            cnt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_tile( Uint16 tiletoset, Uint8 upper, Uint16 presser, Uint8 tx )
{
    // ZZ> This function sets one tile type to another
    int fan;
    int x, y;

    for ( y = 0; y < mesh.tiles_y; y++ )
    {
        for ( x = 0; x < mesh.tiles_x; x++ )
        {
            fan = mesh_get_fan( x, y );
            if ( -1 == fan ) continue;

            if ( TILE_IS_FANOFF( mesh.tx_bits[fan] ) ) continue;

            if ( tiletoset == mesh.tx_bits[fan] )
            {
                int tx_bits;

                tx_bits = TILE_SET_UPPER_BITS( upper );
                switch ( presser )
                {
                    case 0:
                        tx_bits |= tx & 0xFF;
                        break;

                    case 1:
                        tx_bits |= ( tx & 0xFE ) + ( rand() & 1 );
                        break;

                    case 2:
                        tx_bits |= ( tx & 0xFC ) + ( rand() & 3 );
                        break;

                    case 3:
                        tx_bits |= ( tx & 0xF8 ) + ( rand() & 7 );
                        break;

                    default:
                        tx_bits = mesh.tx_bits[fan];
                }
                mesh.tx_bits[fan] = tx_bits;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_mesh_z( int z, Uint16 tiletype, Uint16 tileand )
{
    int vert, cnt, newz, x, y, totalvert;
    int fan;

    tiletype = tiletype & tileand;

    for ( y = 0; y < mesh.tiles_y; y++ )
    {
        for ( x = 0; x < mesh.tiles_x; x++ )
        {
            fan = mesh_get_fan( x, y );
            if ( -1 == fan ) continue;

            if ( tiletype == ( mesh.tx_bits[fan]&tileand ) )
            {
                vert = mesh.vrtstart[fan];
                totalvert = mesh.command[mesh.fantype[fan]].numvertices;

                for ( cnt = 0; cnt < totalvert; cnt++ )
                {
                    newz = mesh.vrtz[vert] + z;
                    if ( newz < 0 )  newz = 0;
                    if ( newz > mesh.edgez ) newz = mesh.edgez;
                    mesh.vrtz[vert] = newz;
                    vert = mesh.vrtnext[vert];
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_vert( int vert, float x, float y, float z )
{
    int newx, newy, newz;

    newx = mesh.vrtx[vert] + x;
    newy = mesh.vrty[vert] + y;
    newz = mesh.vrtz[vert] + z;

    if ( newx < 0 )  newx = 0;
    if ( newx > mesh.edgex )  newx = mesh.edgex;

    if ( newy < 0 )  newy = 0;
    if ( newy > mesh.edgey )  newy = mesh.edgey;

    if ( newz < -mesh.edgez )  newz = -mesh.edgez;
    if ( newz > mesh.edgez )  newz = mesh.edgez;

    mesh.vrtx[vert] = newx;
    mesh.vrty[vert] = newy;
    mesh.vrtz[vert] = newz;
}

//--------------------------------------------------------------------------------------------
void raise_mesh( Uint32 point_lst[], size_t point_cnt, float x, float y, int amount, int size )
{
    int disx, disy, dis, cnt, newamount;
    Uint32 vert;

    if ( NULL == point_lst || 0 == point_cnt ) return;

    for ( cnt = 0; cnt < point_cnt; cnt++ )
    {
        vert = point_lst[cnt];
        if ( vert >= MAXTOTALMESHVERTICES ) break;

        disx = mesh.vrtx[vert] - x;
        disy = mesh.vrty[vert] - y;
        dis = sqrt(( float )( disx * disx + disy * disy ) );

        newamount = abs( amount ) - (( dis << 1 ) >> size );
        if ( newamount < 0 ) newamount = 0;
        if ( amount < 0 )  newamount = -newamount;

        move_vert( vert, 0, 0, newamount );
    }
}

//--------------------------------------------------------------------------------------------
void level_vrtz()
{
    int mapx, mapy, fan, num, cnt;
    Uint32 vert;

    mapy = 0;
    while ( mapy < mesh.tiles_y )
    {
        mapx = 0;
        while ( mapx < mesh.tiles_x )
        {
            fan = mesh_get_fan( mapx, mapy );
            if ( -1 != fan )
            {
                vert = mesh.vrtstart[fan];
                num = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                while ( cnt < num )
                {
                    mesh.vrtz[vert] = 0;
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            mapx++;
        }
        mapy++;
    }
}

//--------------------------------------------------------------------------------------------
void jitter_mesh()
{
    int x, y, fan, num, cnt;
    Uint32 vert;

    y = 0;
    while ( y < mesh.tiles_y )
    {
        x = 0;
        while ( x < mesh.tiles_x )
        {
            fan = mesh_get_fan( x, y );
            if ( -1 != fan )
            {
                vert = mesh.vrtstart[fan];
                num = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                while ( cnt < num )
                {
                    select_clear();
                    select_add( vert );
                    //        srand(mesh.vrtx[vert]+mesh.vrty[vert]+dunframe);
                    move_select(( rand()&7 ) - 3, ( rand()&7 ) - 3, ( rand()&63 ) - 32 );
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
    select_clear();
}

//--------------------------------------------------------------------------------------------
void flatten_mesh( int y0 )
{
    int x, y, fan, num, cnt;
    Uint32 vert;
    int height;

    height = ( 780 - ( y0 ) ) * 4;
    if ( height < 0 )  height = 0;
    if ( height > mesh.edgez ) height = mesh.edgez;
    y = 0;
    while ( y < mesh.tiles_y )
    {
        x = 0;
        while ( x < mesh.tiles_x )
        {
            fan = mesh_get_fan( x, y );
            if ( -1 != fan )
            {
                vert = mesh.vrtstart[fan];
                num = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                while ( cnt < num )
                {
                    if ( mesh.vrtz[vert] > height - 50 )
                        if ( mesh.vrtz[vert] < height + 50 )
                            mesh.vrtz[vert] = height;
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
    select_clear();
}

//--------------------------------------------------------------------------------------------
void clear_mesh( Uint8 upper, Uint16 presser, Uint8 tx, Uint8 type )
{
    int x, y;
    int fan;

    int loc_type = type;

    if ( !TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {
        y = 0;
        while ( y < mesh.tiles_y )
        {
            x = 0;
            while ( x < mesh.tiles_x )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 != fan )
                {
                    int tx_bits;

                    remove_fan( fan );

                    tx_bits = TILE_SET_UPPER_BITS( upper );
                    switch ( presser )
                    {
                        case 0:
                            tx_bits |= tx & 0xFF;
                            break;
                        case 1:
                            tx_bits |= ( tx & 0xFE ) | ( rand() & 1 );
                            break;
                        case 2:
                            tx_bits |= ( tx & 0xFC ) | ( rand() & 3 );
                            break;
                        case 3:
                            tx_bits |= ( tx & 0xF8 ) | ( rand() & 7 );
                            break;
                        default:
                            tx_bits = mesh.tx_bits[fan];
                            break;
                    }
                    mesh.tx_bits[fan] = tx_bits;

                    loc_type = type;
                    if ( loc_type <= 1 ) loc_type = rand() & 1;
                    if ( loc_type == 32 || loc_type == 33 ) loc_type = 32 + ( rand() & 1 );
                    mesh.fantype[fan] = loc_type;

                    add_fan( fan, x * TILE_SIZE, y * TILE_SIZE );
                }
                x++;
            }
            y++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void three_e_mesh( Uint8 upper, Uint8 tx )
{
    // ZZ> Replace all 3F tiles with 3E tiles...

    int x, y;
    int fan;

    if ( TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {
        return;
    }

    for ( y = 0; y < mesh.tiles_y; y++ )
    {
        for ( x = 0; x < mesh.tiles_x; x++ )
        {
            fan = mesh_get_fan( x, y );
            if ( -1 == fan ) continue;

            if ( 0x3F == mesh.tx_bits[fan] )
            {
                mesh.tx_bits[fan] = 0x3E;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t fan_is_floor( int x, int y )
{
    int fan = mesh_get_fan( x, y );
    if ( -1 == fan ) return bfalse;

    return !HAS_BITS( mesh.fx[fan], ( MPDFX_WALL | MPDFX_IMPASS ) );
}

//--------------------------------------------------------------------------------------------
void set_barrier_height( int x, int y, int bits )
{
    Uint32 fantype, fan, vert, vert_count;
    int cnt, noedges;
    float bestprox, prox, tprox, scale, max_height, min_height;

    bool_t floor_mx, floor_px, floor_my, floor_py;
    bool_t floor_mxmy, floor_mxpy, floor_pxmy, floor_pxpy;

    fan = mesh_get_fan( x, y );
    if ( -1 == fan ) return;

    // bust be a MPDFX_WALL
    if ( !HAS_BITS( mesh.fx[fan], bits ) ) return;

    floor_mx   = fan_is_floor( x - 1, y );
    floor_px   = fan_is_floor( x + 1, y );
    floor_my   = fan_is_floor( x, y - 1 );
    floor_py   = fan_is_floor( x, y + 1 );
    noedges = !( floor_mx || floor_px || floor_my || floor_py );

    floor_mxmy = fan_is_floor( x - 1, y - 1 );
    floor_mxpy = fan_is_floor( x - 1, y + 1 );
    floor_pxmy = fan_is_floor( x + 1, y - 1 );
    floor_pxpy = fan_is_floor( x + 1, y + 1 );

    fantype    = mesh.fantype[fan];
    vert_count = mesh.command[fantype].numvertices;

    vert       = mesh.vrtstart[fan];

    min_height = mesh.vrtz[vert];
    max_height = mesh.vrtz[vert];
    vert       = mesh.vrtnext[vert];
    for ( cnt = 1; cnt < vert_count; cnt++ )
    {
        min_height = MIN( min_height, mesh.vrtz[vert] );
        max_height = MAX( max_height, mesh.vrtz[vert] );
        vert       = mesh.vrtnext[vert];
    }

    vert = mesh.vrtstart[fan];
    for ( cnt = 0; cnt < vert_count; cnt++ )
    {
        float ftmp;

        bestprox = NEARHI; // 2.0f / 3.0f * (NEARHI - NEARLOW);
        if ( floor_px )
        {
            prox = NEARHI - mesh.command[fantype].x[cnt];
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_py )
        {
            prox = NEARHI - mesh.command[fantype].y[cnt];
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_mx )
        {
            prox = mesh.command[fantype].x[cnt] - NEARLOW;
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_my )
        {
            prox = mesh.command[fantype].y[cnt] - NEARLOW;
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( noedges )
        {
            // Surrounded by walls on all 4 sides, but it may be a corner piece
            if ( floor_pxpy )
            {
                prox  = NEARHI - mesh.command[fantype].x[cnt];
                tprox = NEARHI - mesh.command[fantype].y[cnt];
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_pxmy )
            {
                prox = NEARHI - mesh.command[fantype].x[cnt];
                tprox = mesh.command[fantype].y[cnt] - NEARLOW;
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_mxpy )
            {
                prox = mesh.command[fantype].x[cnt] - NEARLOW;
                tprox = NEARHI - mesh.command[fantype].y[cnt];
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_mxmy )
            {
                prox = mesh.command[fantype].x[cnt] - NEARLOW;
                tprox = mesh.command[fantype].y[cnt] - NEARLOW;
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
        }
        //scale = window_lst[mdata.window_id].surfacey - (mdata.rect_y0);
        //bestprox = bestprox * scale * BARRIERHEIGHT / window_lst[mdata.window_id].surfacey;

        //if (bestprox > mesh.edgez) bestprox = mesh.edgez;
        //if (bestprox < 0) bestprox = 0;

        ftmp = bestprox / 128.0f;
        ftmp = 1.0f - ftmp;
        ftmp *= ftmp * ftmp;
        ftmp = 1.0f - ftmp;

        mesh.vrtz[vert] = ftmp * ( max_height - min_height ) + min_height;
        vert = mesh.vrtnext[vert];
    }
}

//--------------------------------------------------------------------------------------------
void fix_walls()
{
    int x, y;

    for ( y = 0; y < mesh.tiles_y; y++ )
    {
        for ( x = 0; x < mesh.tiles_x; x++ )
        {
            set_barrier_height( x, y, MPDFX_WALL | MPDFX_IMPASS );
        }
    }
}

//--------------------------------------------------------------------------------------------
void impass_edges( int amount )
{
    int x, y;
    int fan;

    for ( y = 0; y < mesh.tiles_y; y++ )
    {
        for ( x = 0; x < mesh.tiles_x; x++ )
        {
            if ( dist_from_edge( x, y ) < amount )
            {
                fan = mesh_get_fan( x, y );
                if ( -1 == fan ) continue;

                mesh.fx[fan] |= MPDFX_IMPASS;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_replace_fx( Uint16 fx_bits, Uint16 fx_mask, Uint8 fx_new )
{
    // ZZ> This function sets the fx for a group of tiles

    int mapx, mapy;

    // trim away any bits that can't be matched
    fx_bits &= fx_mask;

    for ( mapy = 0; mapy < mesh.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < mesh.tiles_x; mapx++ )
        {
            int fan = mesh_get_fan( mapx, mapy );
            if ( -1 == fan ) continue;

            if ( fx_bits == ( mesh.tx_bits[fan]&fx_mask ) )
            {
                mesh.fx[fan] = fx_new;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 tile_is_different( int fan_x, int fan_y, Uint16 fx_bits, Uint16 fx_mask )
{
    // ZZ> bfalse if of same set, btrue if different
    int fan;

    fan = mesh_get_fan( fan_x, fan_y );
    if ( -1 == fan )
    {
        return bfalse;
    }

    if ( fx_mask == 0xC0 )
    {
        if ( mesh.tx_bits[fan] >= ( MPDFX_WALL | MPDFX_IMPASS ) )
        {
            return bfalse;
        }
    }

    if ( fx_bits == ( mesh.tx_bits[fan]&fx_mask ) )
    {
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 trim_code( int fan_x, int fan_y, Uint16 fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    Uint16 code;

    if ( tile_is_different( fan_x, fan_y - 1, fx_bits, 0xF0 ) )
    {
        // Top
        code = 0;
        if ( tile_is_different( fan_x - 1, fan_y, fx_bits, 0xF0 ) )
        {
            // Left
            code = 8;
        }
        if ( tile_is_different( fan_x + 1, fan_y, fx_bits, 0xF0 ) )
        {
            // Right
            code = 9;
        }
        return code;
    }

    if ( tile_is_different( fan_x, fan_y + 1, fx_bits, 0xF0 ) )
    {
        // Bottom
        code = 1;
        if ( tile_is_different( fan_x - 1, fan_y, fx_bits, 0xF0 ) )
        {
            // Left
            code = 10;
        }
        if ( tile_is_different( fan_x + 1, fan_y, fx_bits, 0xF0 ) )
        {
            // Right
            code = 11;
        }
        return code;
    }

    if ( tile_is_different( fan_x - 1, fan_y, fx_bits, 0xF0 ) )
    {
        // Left
        code = 2;
        return code;
    }
    if ( tile_is_different( fan_x + 1, fan_y, fx_bits, 0xF0 ) )
    {
        // Right
        code = 3;
        return code;
    }

    if ( tile_is_different( fan_x + 1, fan_y + 1, fx_bits, 0xF0 ) )
    {
        // Bottom Right
        code = 4;
        return code;
    }
    if ( tile_is_different( fan_x - 1, fan_y + 1, fx_bits, 0xF0 ) )
    {
        // Bottom Left
        code = 5;
        return code;
    }
    if ( tile_is_different( fan_x + 1, fan_y - 1, fx_bits, 0xF0 ) )
    {
        // Top Right
        code = 6;
        return code;
    }
    if ( tile_is_different( fan_x - 1, fan_y - 1, fx_bits, 0xF0 ) )
    {
        // Top Left
        code = 7;
        return code;
    }

    // unknown
    code = 255;

    return code;
}

//--------------------------------------------------------------------------------------------
Uint16 wall_code( int fan_x, int fan_y, Uint16 fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    Uint16 code;

    if ( tile_is_different( fan_x, fan_y - 1, fx_bits, 0xC0 ) )
    {
        // Top
        code = ( rand() & 2 ) + 20;
        if ( tile_is_different( fan_x - 1, fan_y, fx_bits, 0xC0 ) )
        {
            // Left
            code = 48;
        }
        if ( tile_is_different( fan_x + 1, fan_y, fx_bits, 0xC0 ) )
        {
            // Right
            code = 50;
        }

        return code;
    }

    if ( tile_is_different( fan_x, fan_y + 1, fx_bits, 0xC0 ) )
    {
        // Bottom
        code = ( rand() & 2 );
        if ( tile_is_different( fan_x - 1, fan_y, fx_bits, 0xC0 ) )
        {
            // Left
            code = 52;
        }
        if ( tile_is_different( fan_x + 1, fan_y, fx_bits, 0xC0 ) )
        {
            // Right
            code = 54;
        }
        return code;
    }

    if ( tile_is_different( fan_x - 1, fan_y, fx_bits, 0xC0 ) )
    {
        // Left
        code = ( rand() & 2 ) + 16;
        return code;
    }

    if ( tile_is_different( fan_x + 1, fan_y, fx_bits, 0xC0 ) )
    {
        // Right
        code = ( rand() & 2 ) + 4;
        return code;
    }

    if ( tile_is_different( fan_x + 1, fan_y + 1, fx_bits, 0xC0 ) )
    {
        // Bottom Right
        code = 32;
        return code;
    }
    if ( tile_is_different( fan_x - 1, fan_y + 1, fx_bits, 0xC0 ) )
    {
        // Bottom Left
        code = 34;
        return code;
    }
    if ( tile_is_different( fan_x + 1, fan_y - 1, fx_bits, 0xC0 ) )
    {
        // Top Right
        code = 36;
        return code;
    }
    if ( tile_is_different( fan_x - 1, fan_y - 1, fx_bits, 0xC0 ) )
    {
        // Top Left
        code = 38;
        return code;
    }

    // unknown
    code = 255;

    return code;
}

//--------------------------------------------------------------------------------------------
void trim_mesh_tile( Uint16 fx_bits, Uint16 fx_mask )
{
    // ZZ> This function trims walls and floors and tops automagically
    int fan;
    int fan_x, fan_y, code;

    fx_bits = fx_bits & fx_mask;

    for ( fan_y = 0; fan_y < mesh.tiles_y; fan_y++ )
    {
        for ( fan_x = 0; fan_x < mesh.tiles_x; fan_x++ )
        {
            fan = mesh_get_fan( fan_x, fan_y );
            if ( -1 == fan ) continue;

            if ( fx_bits == ( mesh.tx_bits[fan]&fx_mask ) )
            {
                if ( fx_mask == 0xC0 )
                {
                    code = wall_code( fan_x, fan_y, fx_bits );
                }
                else
                {
                    code = trim_code( fan_x, fan_y, fx_bits );
                }

                if ( code != 255 )
                {
                    mesh.tx_bits[fan] = fx_bits + code;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_replace_tile( int _xfan, int _yfan, int _onfan, Uint8 _tx, Uint8 _upper, Uint8 _fx, Uint8 _type, Uint16 _presser, bool_t tx_only, bool_t at_floor_level )
{
    cart_vec_t pos[CORNER_COUNT];
    int tx_bits;
    int vert;

    if ( _onfan < 0 || _onfan >= MAXMESHFAN ) return;

    if ( !tx_only )
    {
        if ( !at_floor_level )
        {
            // Save corner positions
            vert = mesh.vrtstart[_onfan];
            pos[CORNER_TL][kX] = mesh.vrtx[vert];
            pos[CORNER_TL][kY] = mesh.vrty[vert];
            pos[CORNER_TL][kZ] = mesh.vrtz[vert];

            vert = mesh.vrtnext[vert];
            pos[CORNER_TR][kX] = mesh.vrtx[vert];
            pos[CORNER_TR][kY] = mesh.vrty[vert];
            pos[CORNER_TR][kZ] = mesh.vrtz[vert];

            vert = mesh.vrtnext[vert];
            pos[CORNER_BL][kX] = mesh.vrtx[vert];
            pos[CORNER_BL][kY] = mesh.vrty[vert];
            pos[CORNER_BL][kZ] = mesh.vrtz[vert];

            vert = mesh.vrtnext[vert];
            pos[CORNER_BR][kX] = mesh.vrtx[vert];
            pos[CORNER_BR][kY] = mesh.vrty[vert];
            pos[CORNER_BR][kZ] = mesh.vrtz[vert];
        }
        remove_fan( _onfan );
    }

    tx_bits = TILE_SET_UPPER_BITS( _upper );
    switch ( _presser )
    {
        case 0:
            tx_bits |= _tx & 0xFF;
            break;
        case 1:
            tx_bits |= ( _tx & 0xFE ) | ( rand() & 1 );
            break;
        case 2:
            tx_bits |= ( _tx & 0xFC ) | ( rand() & 3 );
            break;
        case 3:
            tx_bits |= ( _tx & 0xF8 ) | ( rand() & 7 );
            break;
        default:
            tx_bits = mesh.tx_bits[_onfan];
            break;
    };
    mesh.tx_bits[_onfan] = tx_bits;

    if ( !tx_only )
    {
        mesh.fantype[_onfan] = _type;
        add_fan( _onfan, _xfan * TILE_SIZE, _yfan * TILE_SIZE );
        mesh.fx[_onfan] = _fx;
        if ( !at_floor_level )
        {
            // Return corner positions
            vert = mesh.vrtstart[_onfan];
            mesh.vrtx[vert] = pos[CORNER_TL][kX];
            mesh.vrty[vert] = pos[CORNER_TL][kY];
            mesh.vrtz[vert] = pos[CORNER_TL][kZ];

            vert = mesh.vrtnext[vert];
            mesh.vrtx[vert] = pos[CORNER_TR][kX];
            mesh.vrty[vert] = pos[CORNER_TR][kY];
            mesh.vrtz[vert] = pos[CORNER_TR][kZ];

            vert = mesh.vrtnext[vert];
            mesh.vrtx[vert] = pos[CORNER_BL][kX];
            mesh.vrty[vert] = pos[CORNER_BL][kY];
            mesh.vrtz[vert] = pos[CORNER_BL][kZ];

            vert = mesh.vrtnext[vert];
            mesh.vrtx[vert] = pos[CORNER_BR][kX];
            mesh.vrty[vert] = pos[CORNER_BR][kY];
            mesh.vrtz[vert] = pos[CORNER_BR][kZ];
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_fx( int fan, Uint8 fx )
{
    if ( fan >= 0 && fan < MAXMESHFAN )
    {
        mesh.fx[fan] = fx;
    }
}

void mesh_move( float dx, float dy, float dz )
{
    int fan;
    Uint32 vert;
    int mapx, mapy, cnt;

    for ( mapy = 0; mapy < mesh.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < mesh.tiles_x; mapx++ )
        {
            int type, count;

            fan = mesh_get_fan( mapx, mapy );
            if ( -1 == fan ) continue;

            type = mesh.fantype[fan];
            if ( type >= MAXMESHTYPE ) continue;

            count = mesh.command[type].numvertices;

            for ( cnt = 0, vert = mesh.vrtstart[fan];
                  cnt < count && CHAINEND != vert;
                  cnt++, vert = mesh.vrtnext[vert] )
            {
                move_vert( vert, dx, dy, dz );
            }
        }
    }
}

