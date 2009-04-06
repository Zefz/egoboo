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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egoboo_setup.h"

#include "log.h"
#include "configfile.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Macros for reading values from a ConfigFile
//   - Must have a valid ConfigFilePtr_t named lConfigSetup
//   - Must have a string named lCurSectionName to define the section
//   - Must have temporary variables defined of the correct type (lTempBool, lTempInt, and lTempStr)

#define GetKey_bool(label, var, default) \
    { \
        if ( ConfigFile_GetValue_Boolean( cfg_file, lCurSectionName, (label), &lTempBool ) == 0 ) \
        { \
            lTempBool = (default); \
        } \
        (var) = lTempBool; \
    }

#define GetKey_int(label, var, default) \
    { \
        if ( ConfigFile_GetValue_Int( cfg_file, lCurSectionName, (label), &lTempInt ) == 0 ) \
        { \
            lTempInt = (default); \
        } \
        (var) = lTempInt; \
    }

// Don't make len larger than 64
#define GetKey_string(label, var, len, default) \
    { \
        if ( ConfigFile_GetValue_String( cfg_file, lCurSectionName, (label), lTempStr, sizeof( lTempStr ) / sizeof( *lTempStr ) ) == 0 ) \
        { \
            strncpy( lTempStr, (default), sizeof( lTempStr ) / sizeof( *lTempStr ) ); \
        } \
        strncpy( (var), lTempStr, (len) ); \
        (var)[(len) - 1] = '\0'; \
    }

#define SetKey_bool(label, var)     ConfigFile_SetValue_Boolean( cfg_file, lCurSectionName, (label), (var) )
#define SetKey_int(label, var)      ConfigFile_SetValue_Int( cfg_file, lCurSectionName, (label), (var) )
#define SetKey_string( label, var ) ConfigFile_SetValue_String( cfg_file, lCurSectionName, (label), (var) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static ConfigFilePtr_t lConfigSetup = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t setup_quit( ConfigFilePtr_t cfg_file )
{
    return ConfigFile_succeed == ConfigFile_destroy( &cfg_file );
}

//--------------------------------------------------------------------------------------------
ConfigFilePtr_t setup_read( char* filename )
{
    // BB> read the setup file

    return LoadConfigFile( filename );
}

//--------------------------------------------------------------------------------------------
bool_t setup_write( ConfigFilePtr_t cfg_file )
{
    // BB> save the current setup file

    return ConfigFile_succeed == SaveConfigFile( cfg_file );
}

//--------------------------------------------------------------------------------------------
bool_t setup_download( ConfigFilePtr_t cfg_file, config_data_t * pc )
{
    // BB > download the ConfigFile_t keys into game variables
    //      use default values to fill in any missing keys

    char  *lCurSectionName;
    bool_t lTempBool;
    Sint32 lTempInt;
    char   lTempStr[256];
    if (NULL == cfg_file) return bfalse;

    // *********************************************
    // * GRAPHIC Section
    // *********************************************

    lCurSectionName = "GRAPHIC";

    // Draw z reflection?
    GetKey_bool( "Z_REFLECTION", pc->zreflect, bfalse );

    // Max number of vertrices (Should always be 100!)
    GetKey_int( "MAX_NUMBER_VERTICES", pc->maxtotalmeshvertices, 100 );
    pc->maxtotalmeshvertices *= 1024;

    // Do fullscreen?
    GetKey_bool( "FULLSCREEN", pc->fullscreen, bfalse );

    // Screen Size
    GetKey_int( "SCREENSIZE_X", pc->scr.x, 640 );
    GetKey_int( "SCREENSIZE_Y", pc->scr.y, 480 );

    // Color depth
    GetKey_int( "COLOR_DEPTH", pc->scr.d, 32 );

    // The z depth
    GetKey_int( "Z_DEPTH", pc->scr.z, 8 );

    // Max number of messages displayed
    GetKey_int( "MAX_TEXT_MESSAGE", pc->maxmessage, 6 );

    // Max number of messages displayed
    GetKey_int( "MESSAGE_DURATION", pc->messagetime, 50 );

    // Show status bars? (Life, mana, character icons, etc.)
    GetKey_bool( "STATUS_BAR", pc->staton, btrue );
    pc->wraptolerance = 32;
    if ( pc->staton )
    {
        pc->wraptolerance = 90;
    }

    // Perspective correction
    GetKey_bool( "PERSPECTIVE_CORRECT", pc->perspective, bfalse );

    // Enable dithering? (Reduces quality but increases preformance)
    GetKey_bool( "DITHERING", pc->dither, bfalse );

    // Reflection fadeout
    GetKey_int( "FLOOR_REFLECTION_FADEOUT", pc->reffadeor, bfalse );

    // Draw Reflection?
    GetKey_bool( "REFLECTION", pc->refon, bfalse );

    // Draw shadows?
    GetKey_bool( "SHADOWS", pc->shaon, bfalse );

    // Draw good shadows?
    GetKey_bool( "SHADOW_AS_SPRITE", pc->shasprite, btrue );

    // Draw phong mapping?
    GetKey_bool( "PHONG", pc->phongon, btrue );

    // Draw water with more layers?
    GetKey_bool( "MULTI_LAYER_WATER", pc->twolayerwateron, bfalse );

    // TODO: This is not implemented
    GetKey_bool( "OVERLAY", pc->overlayvalid, bfalse );

    // Allow backgrounds?
    GetKey_bool( "BACKGROUND", pc->backgroundvalid, bfalse );

    // Enable fog?
    GetKey_bool( "FOG", pc->fogallowed, bfalse );

    // Do gourad shading?
    GetKey_bool( "GOURAUD_SHADING", lTempBool, btrue );
    pc->shading = lTempBool ? GL_SMOOTH : GL_FLAT;

    // Enable antialiasing?
    GetKey_bool( "ANTIALIASING", pc->antialiasing, bfalse );

    // Do we do texture filtering?
    GetKey_string( "TEXTURE_FILTERING", lTempStr, 24, "LINEAR" );
    if ( lTempStr[0] == 'U' || lTempStr[0] == 'u' )  pc->texturefilter = TX_UNFILTERED;
    if ( lTempStr[0] == 'L' || lTempStr[0] == 'l' )  pc->texturefilter = TX_LINEAR;
    if ( lTempStr[0] == 'M' || lTempStr[0] == 'm' )  pc->texturefilter = TX_MIPMAP;
    if ( lTempStr[0] == 'B' || lTempStr[0] == 'b' )  pc->texturefilter = TX_BILINEAR;
    if ( lTempStr[0] == 'T' || lTempStr[0] == 't' )  pc->texturefilter = TX_TRILINEAR_1;
    if ( lTempStr[0] == '2'                       )  pc->texturefilter = TX_TRILINEAR_2;
    if ( lTempStr[0] == 'A' || lTempStr[0] == 'a' )  pc->texturefilter = TX_ANISOTROPIC;

    // Max number of lights
    GetKey_int( "MAX_DYNAMIC_LIGHTS", pc->maxlights, 12 );

    // Get the FPS limit
    GetKey_int( "MAX_FPS_LIMIT", pc->frame_limit, 30 );

    // Get the particle limit
    GetKey_int( "MAX_PARTICLES", pc->maxparticles, 512 );

    // *********************************************
    // * SOUND Section
    // *********************************************

    lCurSectionName = "SOUND";

    // Enable sound
    GetKey_bool( "SOUND", pc->soundvalid, bfalse );

    // Enable music
    GetKey_bool( "MUSIC", pc->musicvalid, bfalse );

    // Music volume
    GetKey_int( "MUSIC_VOLUME", pc->musicvolume, 50 );

    // Sound volume
    GetKey_int( "SOUND_VOLUME", pc->soundvolume, 75 );

    // Max number of sound channels playing at the same time
    GetKey_int( "MAX_SOUND_CHANNEL", pc->maxsoundchannel, 16 );

    // The output buffer size
    GetKey_int( "OUTPUT_BUFFER_SIZE", pc->buffersize, 2048 );

    // *********************************************
    // * CONTROL Section
    // *********************************************

    lCurSectionName = "CONTROL";

    // Camera control mode
    GetKey_string( "AUTOTURN_CAMERA", lTempStr, 24, "GOOD" );
    if ( lTempStr[0] == 'G' || lTempStr[0] == 'g' )  pc->camautoturn = 255;
    if ( lTempStr[0] == 'T' || lTempStr[0] == 't' )  pc->camautoturn = btrue;
    if ( lTempStr[0] == 'F' || lTempStr[0] == 'f' )  pc->camautoturn = bfalse;

    // *********************************************
    // * NETWORK Section
    // *********************************************

    lCurSectionName = "NETWORK";

    // Enable networking systems?
    GetKey_bool( "NETWORK_ON", pc->networkon, bfalse );

    // Max lag
    GetKey_int( "LAG_TOLERANCE", pc->lag, 2 );

    // Name or IP of the host or the target to join
    GetKey_string( "HOST_NAME", pc->nethostname, 64, "no host" );

    // Multiplayer name
    GetKey_string( "MULTIPLAYER_NAME", pc->netmessagename, 64, "little Raoul" );

    // *********************************************
    // * DEBUG Section
    // *********************************************

    lCurSectionName = "DEBUG";

    // Some special debug settings
    GetKey_bool( "DISPLAY_FPS", pc->fpson, btrue );
    GetKey_bool( "HIDE_MOUSE", pc->HideMouse, btrue );
    GetKey_bool( "GRAB_MOUSE", pc->GrabMouse, btrue );
    GetKey_bool( "DEV_MODE", pc->DevMode, btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t setup_upload( ConfigFilePtr_t cfg_file, config_data_t * pc )
{
    // BB > upload game variables into the ConfigFile_t keys

    char  *lCurSectionName;
    if (NULL == cfg_file) return bfalse;

    // *********************************************
    // * GRAPHIC Section
    // *********************************************

    lCurSectionName = "GRAPHIC";

    // Draw z reflection?
    SetKey_bool( "Z_REFLECTION", pc->zreflect );

    // Max number of vertrices (Should always be 100!)
    SetKey_int( "MAX_NUMBER_VERTICES", pc->maxtotalmeshvertices / 1024 );

    // Do fullscreen?
    SetKey_bool( "FULLSCREEN", pc->fullscreen );

    // Screen Size
    SetKey_int( "SCREENSIZE_X", pc->scr.x );
    SetKey_int( "SCREENSIZE_Y", pc->scr.y );

    // Color depth
    SetKey_int( "COLOR_DEPTH", pc->scr.d );

    // The z depth
    SetKey_int( "Z_DEPTH", pc->scr.z );

    // Max number of messages displayed
    SetKey_int( "MAX_TEXT_MESSAGE", pc->messageon ? pc->maxmessage : 0 );

    // Max number of messages displayed
    SetKey_int( "MESSAGE_DURATION", pc->messagetime );

    // Show status bars? (Life, mana, character icons, etc.)
    SetKey_bool( "STATUS_BAR", pc->staton );

    // Perspective correction
    SetKey_bool( "PERSPECTIVE_CORRECT", pc->perspective );

    // Enable dithering? (Reduces quality but increases preformance)
    SetKey_bool( "DITHERING", pc->dither );

    // Reflection fadeout
    SetKey_bool( "FLOOR_REFLECTION_FADEOUT", pc->reffadeor != 0 );

    // Draw Reflection?
    SetKey_bool( "REFLECTION", pc->refon );

    // Draw shadows?
    SetKey_bool( "SHADOWS", pc->shaon );

    // Draw good shadows?
    SetKey_bool( "SHADOW_AS_SPRITE", pc->shasprite );

    // Draw phong mapping?
    SetKey_bool( "PHONG", pc->phongon );

    // Draw water with more layers?
    SetKey_bool( "MULTI_LAYER_WATER", pc->twolayerwateron );

    // TODO: This is not implemented
    SetKey_bool( "OVERLAY", pc->overlayvalid );

    // Allow backgrounds?
    SetKey_bool( "BACKGROUND", pc->backgroundvalid );

    // Enable fog?
    SetKey_bool( "FOG", pc->fogallowed );

    // Do gourad shading?
    SetKey_bool( "GOURAUD_SHADING", pc->shading == GL_SMOOTH );

    // Enable antialiasing?
    SetKey_bool( "ANTIALIASING", pc->antialiasing );

    // Do we do texture filtering?
    switch ( pc->texturefilter )
    {
        case TX_UNFILTERED:  SetKey_string( "TEXTURE_FILTERING", "UNFILTERED" ); break;
        case TX_MIPMAP:      SetKey_string( "TEXTURE_FILTERING", "MIPMAP" ); break;
        case TX_BILINEAR:    SetKey_string( "TEXTURE_FILTERING", "BILINEAR" ); break;
        case TX_TRILINEAR_1: SetKey_string( "TEXTURE_FILTERING", "TRILINEAR" ); break;
        case TX_TRILINEAR_2: SetKey_string( "TEXTURE_FILTERING", "2_TRILINEAR" ); break;
        case TX_ANISOTROPIC: SetKey_string( "TEXTURE_FILTERING", "ANISOTROPIC" ); break;

        default:
        case TX_LINEAR:      SetKey_string( "TEXTURE_FILTERING", "LINEAR" ); break;
    }

    // Max number of lights
    SetKey_int( "MAX_DYNAMIC_LIGHTS", pc->maxlights );

    // Get the FPS limit
    SetKey_int( "MAX_FPS_LIMIT", pc->frame_limit );

    // Get the particle limit
    SetKey_int( "MAX_PARTICLES", pc->maxparticles );

    // *********************************************
    // * SOUND Section
    // *********************************************

    lCurSectionName = "SOUND";

    // Enable sound
    SetKey_bool( "SOUND", pc->soundvalid );

    // Enable music
    SetKey_bool( "MUSIC", pc->musicvalid );

    // Music volume
    SetKey_int( "MUSIC_VOLUME", pc->musicvolume );

    // Sound volume
    SetKey_int( "SOUND_VOLUME", pc->soundvolume );

    // Max number of sound channels playing at the same time
    SetKey_int( "MAX_SOUND_CHANNEL", pc->maxsoundchannel );

    // The output buffer size
    SetKey_int( "OUTPUT_BUFFER_SIZE", pc->buffersize );

    // *********************************************
    // * CONTROL Section
    // *********************************************

    lCurSectionName = "CONTROL";

    // Camera control mode
    switch ( pc->camautoturn )
    {
        case bfalse: SetKey_bool( "AUTOTURN_CAMERA", bfalse ); break;
        case 255:    SetKey_string( "AUTOTURN_CAMERA", "GOOD" ); break;

        default:
        case btrue : SetKey_bool( "AUTOTURN_CAMERA", btrue );  break;
    }

    // *********************************************
    // * NETWORK Section
    // *********************************************

    lCurSectionName = "NETWORK";

    // Enable networking systems?
    SetKey_bool( "NETWORK_ON", pc->networkon );

    // Name or IP of the host or the target to join
    SetKey_string( "HOST_NAME", pc->nethostname );

    // Multiplayer name
    SetKey_string( "MULTIPLAYER_NAME", pc->netmessagename );

    // Max lag
    SetKey_int( "LAG_TOLERANCE", pc->lag );

    // *********************************************
    // * DEBUG Section
    // *********************************************

    lCurSectionName = "DEBUG";

    // Some special debug settings
    SetKey_bool( "DISPLAY_FPS", pc->fpson );
    SetKey_bool( "HIDE_MOUSE", pc->HideMouse );
    SetKey_bool( "GRAB_MOUSE", pc->GrabMouse );
    SetKey_bool( "DEV_MODE", pc->DevMode );

    return btrue;
}