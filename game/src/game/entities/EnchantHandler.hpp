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

/// @file  game/entities/EnchantManager.hpp
/// @brief Manager of enchantment entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/entities/_Include.hpp` instead)
#endif


#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/LockableList.hpp"
#include "game/entities/Enchant.hpp"

//Forward declarations
struct enc_t;

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the EncList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.
#define ENC_BEGIN_LOOP_ACTIVE(IT, PENC) \
    { \
        int IT##_internal; \
        int enc_loop_start_depth = EncList.getLockCount(); \
        EncList.lock(); \
        for(IT##_internal=0;IT##_internal<EncList.getUsedCount();IT##_internal++) \
        { \
            ENC_REF IT; \
            enc_t * PENC = NULL; \
            IT = (ENC_REF)EncList.used_ref[IT##_internal]; \
            if(!ACTIVE_ENC(IT)) continue; \
            PENC =  EncList.get_ptr(IT);

#define ENC_END_LOOP() \
        } \
        EncList.unlock(); \
        EGOBOO_ASSERT(enc_loop_start_depth == EncList.getLockCount()); \
        EncList.maybeRunDeferred(); \
    }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

struct EnchantManager : public _LockableList < enc_t, ENC_REF, INVALID_ENC_REF, MAX_ENC, BSP_LEAF_ENC>
{
    EnchantManager() :
        _LockableList()
    {
    }

public:
    static EnchantManager& getSingleton();
    void maybeRunDeferred();
    ENC_REF allocate(const ENC_REF override);
    bool free_one(const ENC_REF ref);
    void update_used();
    bool push_free(const ENC_REF ref);
    void prune_used_list();
    void prune_free_list();
};

extern EnchantManager EncList;

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------
bool VALID_ENC_RANGE(const ENC_REF ref);
bool DEFINED_ENC(const ENC_REF ref);
bool ALLOCATED_ENC(const ENC_REF ref);
bool ACTIVE_ENC(const ENC_REF ref);
bool WAITING_ENC(const ENC_REF ref);
bool TERMINATED_ENC(const ENC_REF ref);
ENC_REF GET_REF_PENC(const enc_t *ptr);
bool DEFINED_PENC(const enc_t *ptr);
bool VALID_ENC_PTR(const enc_t *ptr);
bool ALLOCATED_PENC(const enc_t *ptr);
bool ACTIVE_PENC(const enc_t *ptr);
bool WAITIN_PENC(const enc_t *ptr);
bool TERMINATED_PENC(const enc_t *ptr);
bool INGAME_ENC_BASE(const ENC_REF ref);
bool INGAME_PENC_BASE(const enc_t *ptr);
bool INGAME_ENC(const ENC_REF ref);
bool INGAME_PENC(const enc_t *ptr);