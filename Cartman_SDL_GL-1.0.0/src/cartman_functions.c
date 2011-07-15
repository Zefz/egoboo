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
float dist_from_border( cartman_mpd_t * pmesh, float x, float y )
{
    float x_dst, y_dst;

    if ( x < 0.0f ) x_dst = 0.0f;
    else if ( x > pmesh->info.edgex * 0.5f ) x_dst = pmesh->info.edgex - x;
    else if ( x > pmesh->info.edgex ) x_dst = 0.0f;
    else x_dst = x;

    if ( y < 0.0f ) y_dst = 0.0f;
    else if ( y > pmesh->info.edgey * 0.5f ) y_dst = pmesh->info.edgey - y;
    else if ( y > pmesh->info.edgey ) y_dst = 0.0f;
    else y_dst = x;

    return ( x < y ) ? x : y;
}

//--------------------------------------------------------------------------------------------
int dist_from_edge( cartman_mpd_t * pmesh, int x, int y )
{
    if ( x > ( pmesh->info.tiles_x >> 1 ) )
        x = pmesh->info.tiles_x - x - 1;
    if ( y > ( pmesh->info.tiles_y >> 1 ) )
        y = pmesh->info.tiles_y - y - 1;
    if ( x < y )
        return x;
    return y;
}

//--------------------------------------------------------------------------------------------
void fix_mesh( cartman_mpd_t * pmesh )
{
    // ZZ> This function corrects corners across entire mesh
    int x, y;

    for ( y = 0; y < pmesh->info.tiles_y; y++ )
    {

        for ( x = 0; x < pmesh->info.tiles_x; x++ )
        {
            //      fix_corners( pmesh, pmesh, x, y);
            fix_vertices( pmesh, x, y );
        }
    }
}

//--------------------------------------------------------------------------------------------
void fix_vertices( cartman_mpd_t * pmesh, int x, int y )
{
    int fan;
    int cnt;

    fix_corners( pmesh,  x, y );
    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( -1 != fan )
    {
        cnt = 4;
        while ( cnt < tile_dict[pmesh->fan[fan].type].numvertices )
        {
            weld_cnt( pmesh,  x, y, cnt, fan );
            cnt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void fix_corners( cartman_mpd_t * pmesh, int x, int y )
{
    int fan;

    if ( NULL == pmesh ) pmesh = pmesh;

    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( -1 != fan )
    {
        weld_0( pmesh,  x, y );
        weld_1( pmesh,  x, y );
        weld_2( pmesh,  x, y );
        weld_3( pmesh, x, y );
    }
}

//--------------------------------------------------------------------------------------------
void weld_0( cartman_mpd_t * pmesh, int x, int y )
{
    if ( NULL == pmesh ) pmesh = pmesh;

    select_clear();
    select_add( cartman_mpd_get_vertex( pmesh, x, y, 0 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x - 1, y, 1 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x, y - 1, 3 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x - 1, y - 1, 2 ) );
    weld_select( pmesh );
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_1( cartman_mpd_t * pmesh, int x, int y )
{
    if ( NULL == pmesh ) pmesh = pmesh;

    select_clear();
    select_add( cartman_mpd_get_vertex( pmesh, x, y, 1 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x + 1, y, 0 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x, y - 1, 2 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x + 1, y - 1, 3 ) );
    weld_select( pmesh );
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_2( cartman_mpd_t * pmesh, int x, int y )
{
    if ( NULL == pmesh ) pmesh = pmesh;

    select_clear();
    select_add( cartman_mpd_get_vertex( pmesh, x, y, 2 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x + 1, y, 3 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x, y + 1, 1 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x + 1, y + 1, 0 ) );
    weld_select( pmesh );
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_3( cartman_mpd_t * pmesh, int x, int y )
{
    if ( NULL == pmesh ) pmesh = pmesh;

    select_clear();
    select_add( cartman_mpd_get_vertex( pmesh, x, y, 3 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x - 1, y, 2 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x, y + 1, 0 ) );
    select_add( cartman_mpd_get_vertex( pmesh, x - 1, y + 1, 1 ) );
    weld_select( pmesh );
    select_clear();
}

//--------------------------------------------------------------------------------------------
void weld_cnt( cartman_mpd_t * pmesh, int x, int y, int cnt, Uint32 fan )
{
    if ( NULL == pmesh ) pmesh = pmesh;

    if (( int )( tile_dict[pmesh->fan[fan].type].u[cnt] * 128 ) < NEARLOW + 1 ||
        ( int )( tile_dict[pmesh->fan[fan].type].v[cnt] * 128 ) < NEARLOW + 1 ||
        ( int )( tile_dict[pmesh->fan[fan].type].u[cnt] * 128 ) > NEARHI - 1 ||
        ( int )( tile_dict[pmesh->fan[fan].type].v[cnt] * 128 ) > NEARHI - 1 )
    {
        select_clear();
        select_add( cartman_mpd_get_vertex( pmesh, x, y, cnt ) );
        if (( int )( tile_dict[pmesh->fan[fan].type].u[cnt] * 128 ) < NEARLOW + 1 )
            select_add( nearest_vertex( pmesh,  x - 1, y, NEARHI, ( int )( tile_dict[pmesh->fan[fan].type].v[cnt] * 128 ) ) );
        if (( int )( tile_dict[pmesh->fan[fan].type].v[cnt] * 128 ) < NEARLOW + 1 )
            select_add( nearest_vertex( pmesh,  x, y - 1, ( int )( tile_dict[pmesh->fan[fan].type].u[cnt] * 128 ), NEARHI ) );
        if (( int )( tile_dict[pmesh->fan[fan].type].u[cnt] * 128 ) > NEARHI - 1 )
            select_add( nearest_vertex( pmesh,  x + 1, y, NEARLOW, ( int )( tile_dict[pmesh->fan[fan].type].v[cnt] * 128 ) ) );
        if (( int )( tile_dict[pmesh->fan[fan].type].v[cnt] * 128 ) > NEARHI - 1 )
            select_add( nearest_vertex( pmesh,  x, y + 1, ( int )( tile_dict[pmesh->fan[fan].type].u[cnt] * 128 ), NEARLOW ) );
        weld_select( pmesh );
        select_clear();
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void select_add( int vert )
{
    // ZZ> This function highlights a vertex
    int cnt, found;

    if ( vert < 0 || vert >= MPD_VERTICES_MAX ) return;

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
void select_add_rect( cartman_mpd_t * pmesh, float tlx, float tly, float brx, float bry, int mode )
{
    // ZZ> This function checks the rectangular selection

    Uint32 vert;

    if ( tlx > brx )  { float tmp = tlx;  tlx = brx;  brx = tmp; }
    if ( tly > bry )  { float tmp = tly;  tly = bry;  bry = tmp; }

    if ( mode == WINMODE_VERTEX )
    {
        for ( vert = 0; vert < MPD_VERTICES_MAX; vert++ )
        {
            if ( VERTEXUNUSED == pmesh->vrt[vert].a ) continue;

            if ( pmesh->vrt[vert].x >= tlx &&
                 pmesh->vrt[vert].x <= brx &&
                 pmesh->vrt[vert].y >= tly &&
                 pmesh->vrt[vert].y <= bry )
            {
                select_add( vert );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
        for ( vert = 0; vert < MPD_VERTICES_MAX; vert++ )
        {
            if ( VERTEXUNUSED == pmesh->vrt[vert].a ) continue;

            if ( pmesh->vrt[vert].x >= tlx &&
                 pmesh->vrt[vert].x <= brx &&
                 pmesh->vrt[vert].z >= tly &&
                 pmesh->vrt[vert].z <= bry )
            {
                select_add( vert );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void select_remove_rect( cartman_mpd_t * pmesh, float tlx, float tly, float brx, float bry, int mode )
{
    // ZZ> This function checks the rectangular selection, and removes any fans
    //     in the selection area

    Uint32 vert;

    if ( tlx > brx )  { float tmp = tlx;  tlx = brx;  brx = tmp; }
    if ( tly > bry )  { float tmp = tly;  tly = bry;  bry = tmp; }

    if ( mode == WINMODE_VERTEX )
    {
        for ( vert = 0; vert < MPD_VERTICES_MAX; vert++ )
        {
            if ( VERTEXUNUSED == pmesh->vrt[vert].a ) continue;

            if ( pmesh->vrt[vert].x >= tlx &&
                 pmesh->vrt[vert].x <= brx &&
                 pmesh->vrt[vert].y >= tly &&
                 pmesh->vrt[vert].y <= bry )
            {
                select_remove( vert );
            }
        }
    }
    else if ( mode == WINMODE_SIDE )
    {
        for ( vert = 0; vert < MPD_VERTICES_MAX; vert++ )
        {
            if ( VERTEXUNUSED == pmesh->vrt[vert].a ) continue;

            if ( pmesh->vrt[vert].x >= tlx &&
                 pmesh->vrt[vert].x <= brx &&
                 pmesh->vrt[vert].z >= tly &&
                 pmesh->vrt[vert].z <= bry )
            {
                select_remove( vert );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int nearest_vertex( cartman_mpd_t * pmesh, int x, int y, float nearx, float neary )
{
    // ZZ> This function gets a vertex number or -1
    int vert, bestvert, cnt;
    int fan;
    int num;
    float prox, proxx, proxy, bestprox;

    if ( NULL == pmesh ) pmesh = pmesh;

    bestvert = -1;
    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( -1 != fan )
    {
        num = tile_dict[pmesh->fan[fan].type].numvertices;
        vert = pmesh->fan[fan].vrtstart;
        vert = pmesh->vrt[vert].next;
        vert = pmesh->vrt[vert].next;
        vert = pmesh->vrt[vert].next;
        vert = pmesh->vrt[vert].next;
        bestprox = 9000;
        cnt = 4;
        while ( cnt < num )
        {
            proxx = ( int )( tile_dict[pmesh->fan[fan].type].u[cnt] * 128 ) - nearx;
            proxy = ( int )( tile_dict[pmesh->fan[fan].type].v[cnt] * 128 ) - neary;
            if ( proxx < 0 ) proxx = -proxx;
            if ( proxy < 0 ) proxy = -proxy;
            prox = proxx + proxy;
            if ( prox < bestprox )
            {
                bestvert = vert;
                bestprox = prox;
            }
            vert = pmesh->vrt[vert].next;
            cnt++;
        }
    }
    return bestvert;
}

//--------------------------------------------------------------------------------------------
void move_select( cartman_mpd_t * pmesh, float x, float y, float z )
{
    int vert, cnt, newx, newy, newz;

    if ( NULL == pmesh ) pmesh = pmesh;

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        newx = pmesh->vrt[vert].x + x;
        newy = pmesh->vrt[vert].y + y;
        newz = pmesh->vrt[vert].z + z;
        if ( newx < 0 )  x = 0 - pmesh->vrt[vert].x;
        if ( newx > pmesh->info.edgex ) x = pmesh->info.edgex - pmesh->vrt[vert].x;
        if ( newy < 0 )  y = 0 - pmesh->vrt[vert].y;
        if ( newy > pmesh->info.edgey ) y = pmesh->info.edgey - pmesh->vrt[vert].y;
        if ( newz < 0 )  z = 0 - pmesh->vrt[vert].z;
        if ( newz > pmesh->info.edgez ) z = pmesh->info.edgez - pmesh->vrt[vert].z;
        cnt++;
    }

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        newx = pmesh->vrt[vert].x + x;
        newy = pmesh->vrt[vert].y + y;
        newz = pmesh->vrt[vert].z + z;

        if ( newx < 0 )  newx = 0;
        if ( newx > pmesh->info.edgex )  newx = pmesh->info.edgex;
        if ( newy < 0 )  newy = 0;
        if ( newy > pmesh->info.edgey )  newy = pmesh->info.edgey;
        if ( newz < 0 )  newz = 0;
        if ( newz > pmesh->info.edgez )  newz = pmesh->info.edgez;

        pmesh->vrt[vert].x = newx;
        pmesh->vrt[vert].y = newy;
        pmesh->vrt[vert].z = newz;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void set_select_no_bound_z( cartman_mpd_t * pmesh, float z )
{
    int vert, cnt;

    if ( NULL == pmesh ) pmesh = pmesh;

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        pmesh->vrt[vert].z = z;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void jitter_select( cartman_mpd_t * pmesh )
{
    int cnt;
    Uint32 vert;

    cnt = 0;
    while ( cnt < numselect_verts )
    {
        vert = select_verts[cnt];
        move_vert( pmesh,  vert, ( rand() % 3 ) - 1, ( rand() % 3 ) - 1, 0 );
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void select_verts_connected( cartman_mpd_t * pmesh )
{
    int vert, cnt, tnc, x, y, totalvert = 0;
    int fan;
    Uint8 found, select_vertsfan;

    if ( NULL == pmesh ) pmesh = pmesh;

    y = 0;
    while ( y < pmesh->info.tiles_y )
    {
        x = 0;
        while ( x < pmesh->info.tiles_x )
        {
            fan = cartman_mpd_get_fan( pmesh, x, y );
            select_vertsfan = bfalse;
            if ( -1 != fan )
            {
                totalvert = tile_dict[pmesh->fan[fan].type].numvertices;
                cnt = 0;
                vert = pmesh->fan[fan].vrtstart;
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
                    vert = pmesh->vrt[vert].next;
                    cnt++;
                }
            }

            if ( select_vertsfan )
            {
                cnt = 0;
                vert = pmesh->fan[fan].vrtstart;
                while ( cnt < totalvert )
                {
                    select_add( vert );
                    vert = pmesh->vrt[vert].next;
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
}

//--------------------------------------------------------------------------------------------
void weld_select( cartman_mpd_t * pmesh )
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
            x += pmesh->vrt[vert].x;
            y += pmesh->vrt[vert].y;
            z += pmesh->vrt[vert].z;
            a += pmesh->vrt[vert].a;
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
            pmesh->vrt[vert].x = x;
            pmesh->vrt[vert].y = y;
            pmesh->vrt[vert].z = z;
            pmesh->vrt[vert].a = CLIP( a, 1, 255 );
            cnt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_tile( cartman_mpd_t * pmesh, Uint16 tiletoset, Uint8 upper, Uint16 presser, Uint8 tx )
{
    // ZZ> This function sets one tile type to another
    int fan;
    int x, y;

    if ( NULL == pmesh ) pmesh = pmesh;

    for ( y = 0; y < pmesh->info.tiles_y; y++ )
    {
        for ( x = 0; x < pmesh->info.tiles_x; x++ )
        {
            fan = cartman_mpd_get_fan( pmesh, x, y );
            if ( -1 == fan ) continue;

            if ( TILE_IS_FANOFF( pmesh->fan[fan].tx_bits ) ) continue;

            if ( tiletoset == pmesh->fan[fan].tx_bits )
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
                        tx_bits = pmesh->fan[fan].tx_bits;
                }
                pmesh->fan[fan].tx_bits = tx_bits;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_mesh_z( cartman_mpd_t * pmesh, int z, Uint16 tiletype, Uint16 tileand )
{
    int vert, cnt, newz, x, y, totalvert;
    int fan;

    if ( NULL == pmesh ) pmesh = pmesh;

    tiletype = tiletype & tileand;

    for ( y = 0; y < pmesh->info.tiles_y; y++ )
    {
        for ( x = 0; x < pmesh->info.tiles_x; x++ )
        {
            fan = cartman_mpd_get_fan( pmesh, x, y );
            if ( -1 == fan ) continue;

            if ( tiletype == ( pmesh->fan[fan].tx_bits&tileand ) )
            {
                vert = pmesh->fan[fan].vrtstart;
                totalvert = tile_dict[pmesh->fan[fan].type].numvertices;

                for ( cnt = 0; cnt < totalvert; cnt++ )
                {
                    newz = pmesh->vrt[vert].z + z;
                    if ( newz < 0 )  newz = 0;
                    if ( newz > pmesh->info.edgez ) newz = pmesh->info.edgez;
                    pmesh->vrt[vert].z = newz;
                    vert = pmesh->vrt[vert].next;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_vert( cartman_mpd_t * pmesh, int vert, float x, float y, float z )
{
    int newx, newy, newz;

    newx = pmesh->vrt[vert].x + x;
    newy = pmesh->vrt[vert].y + y;
    newz = pmesh->vrt[vert].z + z;

    if ( newx < 0 )  newx = 0;
    if ( newx > pmesh->info.edgex )  newx = pmesh->info.edgex;

    if ( newy < 0 )  newy = 0;
    if ( newy > pmesh->info.edgey )  newy = pmesh->info.edgey;

    if ( newz < -pmesh->info.edgez )  newz = -pmesh->info.edgez;
    if ( newz > pmesh->info.edgez )  newz = pmesh->info.edgez;

    pmesh->vrt[vert].x = newx;
    pmesh->vrt[vert].y = newy;
    pmesh->vrt[vert].z = newz;
}

//--------------------------------------------------------------------------------------------
void raise_mesh( cartman_mpd_t * pmesh, Uint32 point_lst[], size_t point_cnt, float x, float y, int amount, int size )
{
    int disx, disy, dis, cnt, newamount;
    Uint32 vert;

    if ( NULL == point_lst || 0 == point_cnt ) return;

    for ( cnt = 0; cnt < point_cnt; cnt++ )
    {
        vert = point_lst[cnt];
        if ( vert >= MPD_VERTICES_MAX ) break;

        disx = pmesh->vrt[vert].x - x;
        disy = pmesh->vrt[vert].y - y;
        dis = sqrt(( float )( disx * disx + disy * disy ) );

        newamount = abs( amount ) - (( dis << 1 ) >> size );
        if ( newamount < 0 ) newamount = 0;
        if ( amount < 0 )  newamount = -newamount;

        move_vert( pmesh,  vert, 0, 0, newamount );
    }
}

//--------------------------------------------------------------------------------------------
void level_vrtz( cartman_mpd_t * pmesh )
{
    int mapx, mapy, fan, num, cnt;
    Uint32 vert;

    if ( NULL == pmesh ) pmesh = pmesh;

    mapy = 0;
    while ( mapy < pmesh->info.tiles_y )
    {
        mapx = 0;
        while ( mapx < pmesh->info.tiles_x )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );
            if ( -1 != fan )
            {
                vert = pmesh->fan[fan].vrtstart;
                num = tile_dict[pmesh->fan[fan].type].numvertices;
                cnt = 0;
                while ( cnt < num )
                {
                    pmesh->vrt[vert].z = 0;
                    vert = pmesh->vrt[vert].next;
                    cnt++;
                }
            }
            mapx++;
        }
        mapy++;
    }
}

//--------------------------------------------------------------------------------------------
void jitter_mesh( cartman_mpd_t * pmesh )
{
    int x, y, fan, num, cnt;
    Uint32 vert;

    if ( NULL == pmesh ) pmesh = pmesh;

    y = 0;
    while ( y < pmesh->info.tiles_y )
    {
        x = 0;
        while ( x < pmesh->info.tiles_x )
        {
            fan = cartman_mpd_get_fan( pmesh, x, y );
            if ( -1 != fan )
            {
                vert = pmesh->fan[fan].vrtstart;
                num = tile_dict[pmesh->fan[fan].type].numvertices;
                cnt = 0;
                while ( cnt < num )
                {
                    select_clear();
                    select_add( vert );
                    //        srand(pmesh->vrt[vert].x+pmesh->vrt[vert].y+dunframe);
                    move_select( pmesh, ( rand()&7 ) - 3, ( rand()&7 ) - 3, ( rand()&63 ) - 32 );
                    vert = pmesh->vrt[vert].next;
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
void flatten_mesh( cartman_mpd_t * pmesh, int y0 )
{
    int x, y, fan, num, cnt;
    Uint32 vert;
    int height;

    if ( NULL == pmesh ) pmesh = pmesh;

    height = ( 780 - ( y0 ) ) * 4;
    if ( height < 0 )  height = 0;
    if ( height > pmesh->info.edgez ) height = pmesh->info.edgez;
    y = 0;
    while ( y < pmesh->info.tiles_y )
    {
        x = 0;
        while ( x < pmesh->info.tiles_x )
        {
            fan = cartman_mpd_get_fan( pmesh, x, y );
            if ( -1 != fan )
            {
                vert = pmesh->fan[fan].vrtstart;
                num = tile_dict[pmesh->fan[fan].type].numvertices;
                cnt = 0;
                while ( cnt < num )
                {
                    if ( pmesh->vrt[vert].z > height - 50 )
                        if ( pmesh->vrt[vert].z < height + 50 )
                            pmesh->vrt[vert].z = height;
                    vert = pmesh->vrt[vert].next;
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
void clear_mesh( cartman_mpd_t * pmesh, Uint8 upper, Uint16 presser, Uint8 tx, Uint8 type )
{
    int x, y;
    int fan;

    int loc_type = type;

    if ( !TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {
        y = 0;
        while ( y < pmesh->info.tiles_y )
        {
            x = 0;
            while ( x < pmesh->info.tiles_x )
            {
                fan = cartman_mpd_get_fan( pmesh, x, y );
                if ( -1 != fan )
                {
                    int tx_bits;

                    cartman_mpd_remove_fan( pmesh, fan );

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
                            tx_bits = pmesh->fan[fan].tx_bits;
                            break;
                    }
                    pmesh->fan[fan].tx_bits = tx_bits;

                    loc_type = type;
                    if ( loc_type <= 1 ) loc_type = rand() & 1;
                    if ( loc_type == 32 || loc_type == 33 ) loc_type = 32 + ( rand() & 1 );
                    pmesh->fan[fan].type = loc_type;

                    cartman_mpd_add_fan( pmesh, fan, x * TILE_SIZE, y * TILE_SIZE );
                }
                x++;
            }
            y++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void three_e_mesh( cartman_mpd_t * pmesh, Uint8 upper, Uint8 tx )
{
    // ZZ> Replace all 3F tiles with 3E tiles...

    int x, y;
    int fan;

    if ( TILE_IS_FANOFF( TILE_SET_BITS( upper, tx ) ) )
    {
        return;
    }

    for ( y = 0; y < pmesh->info.tiles_y; y++ )
    {
        for ( x = 0; x < pmesh->info.tiles_x; x++ )
        {
            fan = cartman_mpd_get_fan( pmesh, x, y );
            if ( -1 == fan ) continue;

            if ( 0x3F == pmesh->fan[fan].tx_bits )
            {
                pmesh->fan[fan].tx_bits = 0x3E;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t fan_is_floor( cartman_mpd_t * pmesh, int x, int y )
{
    int fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( -1 == fan ) return bfalse;

    return !HAS_BITS( pmesh->fan[fan].fx, ( MPDFX_WALL | MPDFX_IMPASS ) );
}

//--------------------------------------------------------------------------------------------
void set_barrier_height( cartman_mpd_t * pmesh, int x, int y, int bits )
{
    Uint32 fantype, fan, vert, vert_count;
    int cnt, noedges;
    float bestprox, prox, tprox, max_height, min_height;

    bool_t floor_mx, floor_px, floor_my, floor_py;
    bool_t floor_mxmy, floor_mxpy, floor_pxmy, floor_pxpy;
    tile_definition_t * pdef;

    fan = cartman_mpd_get_fan( pmesh, x, y );
    if ( -1 == fan ) return;

    // bust be a MPDFX_WALL
    if ( !HAS_BITS( pmesh->fan[fan].fx, bits ) ) return;

    floor_mx   = fan_is_floor( pmesh,  x - 1, y );
    floor_px   = fan_is_floor( pmesh,  x + 1, y );
    floor_my   = fan_is_floor( pmesh,  x, y - 1 );
    floor_py   = fan_is_floor( pmesh,  x, y + 1 );
    noedges = !( floor_mx || floor_px || floor_my || floor_py );

    floor_mxmy = fan_is_floor( pmesh,  x - 1, y - 1 );
    floor_mxpy = fan_is_floor( pmesh,  x - 1, y + 1 );
    floor_pxmy = fan_is_floor( pmesh,  x + 1, y - 1 );
    floor_pxpy = fan_is_floor( pmesh,  x + 1, y + 1 );

    // valid fantype?
    fantype = pmesh->fan[fan].type;
    if ( fantype >= MPD_FAN_TYPE_MAX ) return;

    // fan is defined?
    pdef = tile_dict + fantype;
    if ( 0 == pdef->numvertices ) return;

    vert_count = pdef->numvertices;

    vert       = pmesh->fan[fan].vrtstart;

    min_height = pmesh->vrt[vert].z;
    max_height = pmesh->vrt[vert].z;
    vert       = pmesh->vrt[vert].next;
    for ( cnt = 1; cnt < vert_count; cnt++ )
    {
        min_height = MIN( min_height, pmesh->vrt[vert].z );
        max_height = MAX( max_height, pmesh->vrt[vert].z );
        vert       = pmesh->vrt[vert].next;
    }

    vert = pmesh->fan[fan].vrtstart;
    for ( cnt = 0; cnt < vert_count; cnt++ )
    {
        float ftmp;

        bestprox = NEARHI; // 2.0f / 3.0f * (NEARHI - NEARLOW);
        if ( floor_px )
        {
            prox = NEARHI - ( int )( pdef->u[cnt] * 128 );
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_py )
        {
            prox = NEARHI - ( int )( pdef->v[cnt] * 128 );
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_mx )
        {
            prox = ( int )( pdef->u[cnt] * 128 ) - NEARLOW;
            if ( prox < bestprox ) bestprox = prox;
        }
        if ( floor_my )
        {
            prox = ( int )( pdef->v[cnt] * 128 ) - NEARLOW;
            if ( prox < bestprox ) bestprox = prox;
        }

        if ( noedges )
        {
            // Surrounded by walls on all 4 sides, but it may be a corner piece
            if ( floor_pxpy )
            {
                prox  = NEARHI - ( int )( pdef->u[cnt] * 128 );
                tprox = NEARHI - ( int )( pdef->v[cnt] * 128 );
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_pxmy )
            {
                prox = NEARHI - ( int )( pdef->u[cnt] * 128 );
                tprox = ( int )( pdef->v[cnt] * 128 ) - NEARLOW;
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_mxpy )
            {
                prox = ( int )( pdef->u[cnt] * 128 ) - NEARLOW;
                tprox = NEARHI - ( int )( pdef->v[cnt] * 128 );
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
            if ( floor_mxmy )
            {
                prox = ( int )( pdef->u[cnt] * 128 ) - NEARLOW;
                tprox = ( int )( pdef->v[cnt] * 128 ) - NEARLOW;
                if ( tprox > prox ) prox = tprox;
                if ( prox < bestprox ) bestprox = prox;
            }
        }
        //scale = window_lst[mdata.window_id].surfacey - (mdata.rect_y0);
        //bestprox = bestprox * scale * BARRIERHEIGHT / window_lst[mdata.window_id].surfacey;

        //if (bestprox > pmesh->info.edgez) bestprox = pmesh->info.edgez;
        //if (bestprox < 0) bestprox = 0;

        ftmp = bestprox / 128.0f;
        ftmp = 1.0f - ftmp;
        ftmp *= ftmp * ftmp;
        ftmp = 1.0f - ftmp;

        pmesh->vrt[vert].z = ftmp * ( max_height - min_height ) + min_height;
        vert = pmesh->vrt[vert].next;
    }
}

//--------------------------------------------------------------------------------------------
void fix_walls( cartman_mpd_t * pmesh )
{
    int x, y;

    for ( y = 0; y < pmesh->info.tiles_y; y++ )
    {
        for ( x = 0; x < pmesh->info.tiles_x; x++ )
        {
            set_barrier_height( pmesh,  x, y, MPDFX_WALL | MPDFX_IMPASS );
        }
    }
}

//--------------------------------------------------------------------------------------------
void impass_edges( cartman_mpd_t * pmesh, int amount )
{
    int x, y;
    int fan;

    for ( y = 0; y < pmesh->info.tiles_y; y++ )
    {
        for ( x = 0; x < pmesh->info.tiles_x; x++ )
        {
            if ( dist_from_edge( pmesh,  x, y ) < amount )
            {
                fan = cartman_mpd_get_fan( pmesh, x, y );
                if ( -1 == fan ) continue;

                pmesh->fan[fan].fx |= MPDFX_IMPASS;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_replace_fx( cartman_mpd_t * pmesh, Uint16 fx_bits, Uint16 fx_mask, Uint8 fx_new )
{
    // ZZ> This function sets the fx for a group of tiles

    int mapx, mapy;

    // trim away any bits that can't be matched
    fx_bits &= fx_mask;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            int fan = cartman_mpd_get_fan( pmesh, mapx, mapy );
            if ( -1 == fan ) continue;

            if ( fx_bits == ( pmesh->fan[fan].tx_bits&fx_mask ) )
            {
                pmesh->fan[fan].fx = fx_new;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 tile_is_different( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits, Uint16 fx_mask )
{
    // ZZ> bfalse if of same set, btrue if different
    int fan;

    fan = cartman_mpd_get_fan( pmesh, mapx, mapy );
    if ( -1 == fan )
    {
        return bfalse;
    }

    if ( fx_mask == 0xC0 )
    {
        if ( pmesh->fan[fan].tx_bits >= ( MPDFX_WALL | MPDFX_IMPASS ) )
        {
            return bfalse;
        }
    }

    if ( fx_bits == ( pmesh->fan[fan].tx_bits&fx_mask ) )
    {
        return bfalse;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 trim_code( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    Uint16 code;

    if ( tile_is_different( pmesh,  mapx, mapy - 1, fx_bits, 0xF0 ) )
    {
        // Top
        code = 0;
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xF0 ) )
        {
            // Left
            code = 8;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xF0 ) )
        {
            // Right
            code = 9;
        }
        return code;
    }

    if ( tile_is_different( pmesh,  mapx, mapy + 1, fx_bits, 0xF0 ) )
    {
        // Bottom
        code = 1;
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xF0 ) )
        {
            // Left
            code = 10;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xF0 ) )
        {
            // Right
            code = 11;
        }
        return code;
    }

    if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xF0 ) )
    {
        // Left
        code = 2;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xF0 ) )
    {
        // Right
        code = 3;
        return code;
    }

    if ( tile_is_different( pmesh,  mapx + 1, mapy + 1, fx_bits, 0xF0 ) )
    {
        // Bottom Right
        code = 4;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy + 1, fx_bits, 0xF0 ) )
    {
        // Bottom Left
        code = 5;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx + 1, mapy - 1, fx_bits, 0xF0 ) )
    {
        // Top Right
        code = 6;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy - 1, fx_bits, 0xF0 ) )
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
Uint16 wall_code( cartman_mpd_t * pmesh, int mapx, int mapy, Uint16 fx_bits )
{
    // ZZ> This function returns the standard tile set value thing...  For
    //     Trimming tops of walls and floors

    Uint16 code;

    if ( tile_is_different( pmesh,  mapx, mapy - 1, fx_bits, 0xC0 ) )
    {
        // Top
        code = ( rand() & 2 ) + 20;
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xC0 ) )
        {
            // Left
            code = 48;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xC0 ) )
        {
            // Right
            code = 50;
        }

        return code;
    }

    if ( tile_is_different( pmesh,  mapx, mapy + 1, fx_bits, 0xC0 ) )
    {
        // Bottom
        code = ( rand() & 2 );
        if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xC0 ) )
        {
            // Left
            code = 52;
        }
        if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xC0 ) )
        {
            // Right
            code = 54;
        }
        return code;
    }

    if ( tile_is_different( pmesh,  mapx - 1, mapy, fx_bits, 0xC0 ) )
    {
        // Left
        code = ( rand() & 2 ) + 16;
        return code;
    }

    if ( tile_is_different( pmesh,  mapx + 1, mapy, fx_bits, 0xC0 ) )
    {
        // Right
        code = ( rand() & 2 ) + 4;
        return code;
    }

    if ( tile_is_different( pmesh,  mapx + 1, mapy + 1, fx_bits, 0xC0 ) )
    {
        // Bottom Right
        code = 32;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy + 1, fx_bits, 0xC0 ) )
    {
        // Bottom Left
        code = 34;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx + 1, mapy - 1, fx_bits, 0xC0 ) )
    {
        // Top Right
        code = 36;
        return code;
    }
    if ( tile_is_different( pmesh,  mapx - 1, mapy - 1, fx_bits, 0xC0 ) )
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
void trim_mesh_tile( cartman_mpd_t * pmesh, Uint16 fx_bits, Uint16 fx_mask )
{
    // ZZ> This function trims walls and floors and tops automagically
    int fan;
    int mapx, mapy, code;

    fx_bits = fx_bits & fx_mask;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );
            if ( -1 == fan ) continue;

            if ( fx_bits == ( pmesh->fan[fan].tx_bits&fx_mask ) )
            {
                if ( fx_mask == 0xC0 )
                {
                    code = wall_code( pmesh,  mapx, mapy, fx_bits );
                }
                else
                {
                    code = trim_code( pmesh,  mapx, mapy, fx_bits );
                }

                if ( code != 255 )
                {
                    pmesh->fan[fan].tx_bits = fx_bits + code;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_replace_tile( cartman_mpd_t * pmesh, int _xfan, int _yfan, int _onfan, Uint8 _tx, Uint8 _upper, Uint8 _fx, Uint8 _type, Uint16 _presser, bool_t tx_only, bool_t at_floor_level )
{
    cart_vec_t pos[CORNER_COUNT];
    int tx_bits;
    int vert;

    if ( _onfan < 0 || _onfan >= MPD_TILE_MAX ) return;

    if ( !tx_only )
    {
        if ( !at_floor_level )
        {
            // Save corner positions
            vert = pmesh->fan[_onfan].vrtstart;
            pos[CORNER_TL][kX] = pmesh->vrt[vert].x;
            pos[CORNER_TL][kY] = pmesh->vrt[vert].y;
            pos[CORNER_TL][kZ] = pmesh->vrt[vert].z;

            vert = pmesh->vrt[vert].next;
            pos[CORNER_TR][kX] = pmesh->vrt[vert].x;
            pos[CORNER_TR][kY] = pmesh->vrt[vert].y;
            pos[CORNER_TR][kZ] = pmesh->vrt[vert].z;

            vert = pmesh->vrt[vert].next;
            pos[CORNER_BL][kX] = pmesh->vrt[vert].x;
            pos[CORNER_BL][kY] = pmesh->vrt[vert].y;
            pos[CORNER_BL][kZ] = pmesh->vrt[vert].z;

            vert = pmesh->vrt[vert].next;
            pos[CORNER_BR][kX] = pmesh->vrt[vert].x;
            pos[CORNER_BR][kY] = pmesh->vrt[vert].y;
            pos[CORNER_BR][kZ] = pmesh->vrt[vert].z;
        }
        cartman_mpd_remove_fan( pmesh, _onfan );
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
            tx_bits = pmesh->fan[_onfan].tx_bits;
            break;
    };
    pmesh->fan[_onfan].tx_bits = tx_bits;

    if ( !tx_only )
    {
        pmesh->fan[_onfan].type = _type;
        cartman_mpd_add_fan( pmesh, _onfan, _xfan * TILE_SIZE, _yfan * TILE_SIZE );
        pmesh->fan[_onfan].fx = _fx;
        if ( !at_floor_level )
        {
            // Return corner positions
            vert = pmesh->fan[_onfan].vrtstart;
            pmesh->vrt[vert].x = pos[CORNER_TL][kX];
            pmesh->vrt[vert].y = pos[CORNER_TL][kY];
            pmesh->vrt[vert].z = pos[CORNER_TL][kZ];

            vert = pmesh->vrt[vert].next;
            pmesh->vrt[vert].x = pos[CORNER_TR][kX];
            pmesh->vrt[vert].y = pos[CORNER_TR][kY];
            pmesh->vrt[vert].z = pos[CORNER_TR][kZ];

            vert = pmesh->vrt[vert].next;
            pmesh->vrt[vert].x = pos[CORNER_BL][kX];
            pmesh->vrt[vert].y = pos[CORNER_BL][kY];
            pmesh->vrt[vert].z = pos[CORNER_BL][kZ];

            vert = pmesh->vrt[vert].next;
            pmesh->vrt[vert].x = pos[CORNER_BR][kX];
            pmesh->vrt[vert].y = pos[CORNER_BR][kY];
            pmesh->vrt[vert].z = pos[CORNER_BR][kZ];
        }
    }
}

//--------------------------------------------------------------------------------------------
void mesh_set_fx( cartman_mpd_t * pmesh, int fan, Uint8 fx )
{
    if ( fan >= 0 && fan < MPD_TILE_MAX )
    {
        pmesh->fan[fan].fx = fx;
    }
}

//--------------------------------------------------------------------------------------------
void mesh_move( cartman_mpd_t * pmesh, float dx, float dy, float dz )
{
    int fan;
    Uint32 vert;
    int mapx, mapy, cnt;

    for ( mapy = 0; mapy < pmesh->info.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < pmesh->info.tiles_x; mapx++ )
        {
            int type, count;

            fan = cartman_mpd_get_fan( pmesh, mapx, mapy );
            if ( -1 == fan ) continue;

            type = pmesh->fan[fan].type;
            if ( type >= MPD_FAN_TYPE_MAX ) continue;

            count = tile_dict[type].numvertices;

            for ( cnt = 0, vert = pmesh->fan[fan].vrtstart;
                  cnt < count && CHAINEND != vert;
                  cnt++, vert = pmesh->vrt[vert].next )
            {
                move_vert( pmesh,  vert, dx, dy, dz );
            }
        }
    }
}

