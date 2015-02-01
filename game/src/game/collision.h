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
#include "game/PrtList.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

class GameObject;
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
public:
    CoNode_t();

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

    bool operator == (const CoNode_t &pright) const
    {
        // don't compare the times
        int itmp = ( int32_t )REF_TO_INT( this->chra ) - ( int32_t )REF_TO_INT( pright.chra );
        if ( 0 != itmp ) return false;

        itmp = ( int32_t )REF_TO_INT( this->prta ) - ( int32_t )REF_TO_INT( pright.prta );
        if ( 0 != itmp ) return false;

        itmp = ( int32_t )REF_TO_INT( this->chrb ) - ( int32_t )REF_TO_INT( pright.chrb );
        if ( 0 != itmp ) return false;

        itmp = ( int32_t )REF_TO_INT( this->prtb ) - ( int32_t )REF_TO_INT( pright.prtb );
        if ( 0 != itmp ) return false;

        itmp = ( int32_t )this->tileb - ( int32_t )pright.tileb;
        if ( 0 != itmp ) return false;

        return true;
    }
};

//--------------------------------------------------------------------------------------------
/**
* Hash algorithm for collision nodes
**/

#define MAKE_HASH(AA,BB)         CLIP_TO_08BITS( ((AA) * 0x0111 + 0x006E) + ((BB) * 0x0111 + 0x006E) )

struct CNodeHash {
   size_t operator() (const CoNode_t &coll) const {
        Uint32 AA, BB;

        AA = ( Uint32 )( ~(( Uint32 )0 ) );
        if ( coll.chra < MAX_CHR && coll.chra != INVALID_CHR_REF )
        {
            AA = REF_TO_INT( coll.chra );
        }
        else if ( _VALID_PRT_RANGE( coll.prta ) )
        {
            AA = REF_TO_INT( coll.prta );
        }

        BB = ( Uint32 )( ~(( Uint32 )0 ) );
        if ( coll.chrb < MAX_CHR && coll.chrb != INVALID_CHR_REF )
        {
            BB = REF_TO_INT( coll.chrb );
        }
        else if ( _VALID_PRT_RANGE( coll.prtb ) )
        {
            BB = REF_TO_INT( coll.prtb );
        }
        else if ( MAP_FANOFF != coll.tileb )
        {
            BB = coll.tileb;
        }

        return MAKE_HASH( AA, BB );
   }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// global functions

void collision_system_begin();
void collision_system_end();

void bump_all_objects();
