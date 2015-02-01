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

/// @file game/mesh_BSP.h
/// @brief BSPs for meshes

#pragma once

#include "game/egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

// Forward declarations.
struct ego_mesh_t;
struct egolib_frustum_t;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	A BSP housing a mesh.
 */
struct mesh_BSP_t
{
    size_t count;
    oct_bb_t volume;
    BSP_tree_t tree;
	/**
	 * @brief
	 *	Construct a mesh BSP tree.
	 * @param mesh
	 *	the mesh
	 * @return
	 *	a pointer to this mesh BSP tree on success, @a NULL on failure
	 */
	static mesh_BSP_t *ctor(mesh_BSP_t *self, const ego_mesh_t *mesh);

	/**
	 * @brief
	 *	Destuct a mesh BSP.
	 * @param self
	 *	the mesh BSP
	 */
	static void dtor(mesh_BSP_t *self);

	/**
	 * @brief
	 *	Fill the collision list with references to tiles that the object volume may overlap.
	 * @return
	 *	the new number of collisions in @a collisions
	 */
	size_t collide(const aabb_t *aabb, BSP_leaf_test_t *test, std::vector<BSP_leaf_t *> *collisions) const;

	/**
	 * @brief
	 * 	Fill the collision list with references to tiles that the object volume may overlap.
	 * @return
	 *	the new number of collisions in @a collisions
	 */
	size_t collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, std::vector<BSP_leaf_t *> *collisions) const;

};


/**
 * @brief
 *	Create a new mesh BSP tree for a mesh.
 * @param mesh
 *	the mesh used in initialization
 * @return
 *	the mesh BSP tree on success, @a NULL failure
 * @author
 *	BB
 * @author
 *	MH
 * @details
 *	These parameters duplicate the max resolution of the old system.
 */
mesh_BSP_t *mesh_BSP_new(const ego_mesh_t *mesh);

/**
 * @brief
 *	Delete a mesh BSP.
 * @param self
 *	the mesh BSP
 */
void mesh_BSP_delete(mesh_BSP_t *self);

bool mesh_BSP_fill(mesh_BSP_t *self, const ego_mesh_t *mesh);
bool mesh_BSP_can_collide(BSP_leaf_t *leaf);
bool mesh_BSP_is_visible(BSP_leaf_t *leaf);
