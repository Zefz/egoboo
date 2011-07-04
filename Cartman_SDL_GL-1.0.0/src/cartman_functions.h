#pragma once

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define NEARLOW  0.0f //16.0f     // For autoweld
#define NEARHI 128.0f //112.0f        //
#define BARRIERHEIGHT 14.0f      //

#define MAXSELECT 2560          // Max points that can be select_vertsed

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

float dist_from_border( float x, float y );
int dist_from_edge( int x, int y );
int nearest_vertex( int x, int y, float nearx, float neary );

void fix_mesh( void );
void fix_vertices( int x, int y );
void fix_corners( int x, int y );

void weld_0( int x, int y );
void weld_1( int x, int y );
void weld_2( int x, int y );
void weld_3( int x, int y );
void weld_cnt( int x, int y, int cnt, Uint32 fan );

// selection functions
void weld_select();
void move_select( int x, int y, int z );
void set_select_no_bound_z( int z );;
void jitter_select();
void select_verts_connected();

// vertex selection functions
void select_add( int vert );
void select_clear( void );
void select_remove( int vert );
void select_add_rect( float tlx, float tly, float brx, float bry, int mode );
void select_remove_rect( float tlx, float tly, float brx, float bry, int mode );
int  select_count();
int  select_has_vert( int vert );

void mesh_set_tile( Uint16 tiletoset, Uint8 upper, Uint16 presser, Uint8 tx );
void move_mesh_z( int z, Uint16 tiletype, Uint16 tileand );
void move_vert( int vert, float x, float y, float z );
void raise_mesh( Uint32 point_lst[], size_t point_count, float x, float y, int amount, int size );
void level_vrtz();
void jitter_mesh();
void flatten_mesh( int y0 );
void clear_mesh( Uint8 upper, Uint16 presser, Uint8 tx, Uint8 type );
void three_e_mesh( Uint8 upper, Uint8 tx );

int  fan_is_floor( int x, int y );
void set_barrier_height( int x, int y, int bits );
void fix_walls();
void impass_edges( int amount );

void mesh_replace_fx( Uint16 fx_bits, Uint16 fx_mask, Uint8 fx_new );
void mesh_replace_tile( int xfan, int yfan, int onfan, Uint8 tx, Uint8 upper, Uint8 fx, Uint8 type, Uint16 presser, bool_t tx_only, bool_t at_floor_level );
void mesh_set_fx( int fan, Uint8 fx );
void mesh_move( float dx, float dy, float dz );

// indecipherable legacy code
Uint8  tile_is_different( int fan_x, int fan_y, Uint16 fx_bits, Uint16 fx_mask );
Uint16 trim_code( int x, int y, Uint16 fx_bits );
Uint16 wall_code( int x, int y, Uint16 fx_bits );
void   trim_mesh_tile( Uint16 fx_bits, Uint16 fx_mask );
