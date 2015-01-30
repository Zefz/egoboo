//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/collision.h

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

class chr_t;
struct prt_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct CoNode_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// element for storing pair-wise "collision" data
/// @note this does not use the "standard" method of inheritance from hash_node_t, where an
/// instance of hash_node_t is embedded inside CoNode_t as CoNode_t::base or something.
/// Instead, a separate lists of free hash_nodes and free CoNodes are kept, and they are
/// associated through the hash_node_t::data pointer when the hash node is added to the
/// hash_list_t
struct CoNode_t
{
    // the "colliding" objects
    CHR_REF chra;
    PRT_REF prta;

    // the "collided with" objects
    CHR_REF chrb;
    PRT_REF prtb;
    Uint32  tileb;

    // some information about the estimated collision
    float    tmin, tmax;
    oct_bb_t cv;
};

CoNode_t * CoNode_ctor( CoNode_t * );
Uint8      CoNode_generate_hash( CoNode_t * coll );
int        CoNode_cmp( const void * pleft, const void * pright );

//--------------------------------------------------------------------------------------------

/// a useful re-typing of the CHashList_t, in case we need to add more variables or functionality later
typedef hash_list_t CoHashList_t;

/// Insert a collision into a collision hash list if it does not exist yet.
/// @param self the collision hash list
/// @param collision the collision
/// @param collisions the list of collisions
/// @param hashNodes the list of hash nodes
bool CoHashList_insert_unique(CoHashList_t *coHashList, CoNode_t *coNode, Ego::DynamicArray<CoNode_t> *coNodeAry, Ego::DynamicArray<hash_node_t> *hashNodeAry);

CoHashList_t *CoHashList_getInstance(size_t capacity);

//--------------------------------------------------------------------------------------------
extern int CoHashList_inserted;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// global functions

bool collision_system_begin();
void collision_system_end();

void bump_all_objects();
