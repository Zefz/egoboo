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

/// @file egolib/bsp.c
/// @brief
/// @details

#include "egolib/bsp.h"

#include "egolib/log.h"

#include "egolib/frustum.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define INVALID_BSP_BRANCH_LIST(BL) ( (NULL == (BL)) || (NULL == (BL)->lst) || (0 == (BL)->lst_size) )

#define BRANCH_NODE_THRESHOLD 5

//--------------------------------------------------------------------------------------------
// special functions

static bool _generate_BSP_aabb_child(BSP_aabb_t * psrc, int index, BSP_aabb_t * pdst);
static int    _find_child_index(const BSP_aabb_t * pbranch_aabb, const aabb_t * pleaf_aabb);

//--------------------------------------------------------------------------------------------
// BSP_tree_t private functions

static BSP_branch_t *BSP_tree_alloc_branch(BSP_tree_t *self);
#if 0
static bool BSP_tree_insert_infinite(BSP_tree_t *self, BSP_leaf_t *leaf);
#endif

//--------------------------------------------------------------------------------------------
// Generic BSP functions
//--------------------------------------------------------------------------------------------
bool _generate_BSP_aabb_child(BSP_aabb_t * psrc, int index, BSP_aabb_t * pdst)
{
	size_t cnt;
	signed tnc, child_count;

	// valid source?
	if (NULL == psrc || psrc->dim <= 0) return false;

	// valid destination?
	if (NULL == pdst) return false;

	// valid index?
	child_count = 2 << (psrc->dim - 1);
	if (index < 0 || index >= child_count) return false;

	// make sure that the destination type matches the source type
	if (pdst->dim != psrc->dim)
	{
		pdst->dtor();
		pdst->ctor(psrc->dim);
	}

	// determine the bounds
	for (cnt = 0; cnt < psrc->dim; cnt++)
	{
		float maxval, minval;

		tnc = psrc->dim - 1 - cnt;

		if (0 == (index & (1 << tnc)))
		{
			minval = psrc->mins.ary[cnt];
			maxval = psrc->mids.ary[cnt];
		}
		else
		{
			minval = psrc->mids.ary[cnt];
			maxval = psrc->maxs.ary[cnt];
		}

		pdst->mins.ary[cnt] = minval;
		pdst->maxs.ary[cnt] = maxval;
		pdst->mids.ary[cnt] = 0.5f * (minval + maxval);
	}

	return true;
}

//--------------------------------------------------------------------------------------------
int _find_child_index(const BSP_aabb_t * pbranch_aabb, const aabb_t * pleaf_aabb)
{
	//---- determine which child the leaf needs to go under

	/// @note This function is not optimal, since we encode the comparisons
	/// in the 32-bit integer indices, and then may have to decimate index to construct
	/// the child  branch's bounding by calling _generate_BSP_aabb_child().
	/// The reason that it is done this way is that we would have to be dynamically
	/// allocating and deallocating memory every time this function is called, otherwise. Big waste of time.
	int    index;

	const float *branch_min_ary, *branch_mid_ary, *branch_max_ary;

	// get aliases to the leaf aabb data
	if (NULL == pleaf_aabb) return -2;
	const float *leaf_min_ary = pleaf_aabb->mins.v;
	const float *leaf_max_ary = pleaf_aabb->maxs.v;

	// get aliases to the branch aabb data
	if (NULL == pbranch_aabb || 0 == pbranch_aabb->dim || !pbranch_aabb->valid)
		return -2;

	if (NULL == pbranch_aabb->mins.ary || 0 == pbranch_aabb->mins.cp) return -2;
	branch_min_ary = pbranch_aabb->mins.ary;

	if (NULL == pbranch_aabb->mids.ary || 0 == pbranch_aabb->mids.cp) return -2;
	branch_mid_ary = pbranch_aabb->mids.ary;

	if (NULL == pbranch_aabb->maxs.ary || 0 == pbranch_aabb->maxs.cp) return -2;
	branch_max_ary = pbranch_aabb->maxs.ary;

	index = 0;
	for (size_t cnt = 0; cnt < pbranch_aabb->dim; cnt++)
	{
		index <<= 1;

		if (leaf_min_ary[cnt] >= branch_min_ary[cnt] && leaf_max_ary[cnt] <= branch_mid_ary[cnt])
		{
			// the following code does nothing
			index |= 0;
		}
		else if (leaf_min_ary[cnt] >= branch_mid_ary[cnt] && leaf_max_ary[cnt] <= branch_max_ary[cnt])
		{
			index |= 1;
		}
		else if (leaf_min_ary[cnt] >= branch_min_ary[cnt] && leaf_max_ary[cnt] <= branch_max_ary[cnt])
		{
			// this leaf belongs at this node
			index = -1;
			break;
		}
		else
		{
			// this leaf is actually bigger than this branch
			index = -2;
			break;
		}
	}

	return index;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t *BSP_leaf_t::ctor(void *data, bsp_type_t type, size_t index)
{
	this->next = nullptr;
	this->inserted = false;
	this->data_type = type;
	this->index = index;
	this->data = data;
	this->bbox.ctor();
	return this;
}

//--------------------------------------------------------------------------------------------
void BSP_leaf_t::dtor()
{
	this->bbox.dtor();
	this->data = nullptr;
	this->index = 0;
	this->data_type = BSP_LEAF_NONE;
	this->inserted = false;
	this->next = nullptr;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_t::remove_link(BSP_leaf_t *L)
{
	if (NULL == L) return false;

	bool retval = false;

	if (L->inserted)
	{
		L->inserted = false;
		retval = true;
	}

	if (NULL != L->next)
	{
		L->next = NULL;
		retval = true;
	}

	bv_self_clear(&(L->bbox));

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_t::clear(BSP_leaf_t * L)
{
	if (NULL == L) return false;

	L->data_type = BSP_LEAF_NONE;
	L->data = NULL;

	BSP_leaf_t::remove_link(L);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_t::copy(BSP_leaf_t * dst, const BSP_leaf_t * src)
{
	if (NULL == dst) return false;

	if (NULL == src)
	{
		BLANK_STRUCT_PTR(dst);
	}
	else
	{
		memmove(dst, src, sizeof(*dst));
	}

	return true;
}

//--------------------------------------------------------------------------------------------
// BSP_branch_t
//--------------------------------------------------------------------------------------------
BSP_branch_t *BSP_branch_t::create(size_t dimensionality)
{
	BSP_branch_t *branch = EGOBOO_NEW(BSP_branch_t);
	if (!branch)
	{
		return nullptr;
	}
	if (!branch->ctor(dimensionality))
	{
		EGOBOO_DELETE(branch);
		return nullptr;
	}
	return branch;
}
void BSP_branch_t::destroy(BSP_branch_t *branch)
{
	branch->dtor();
	EGOBOO_DELETE(branch);
}
BSP_branch_t *BSP_branch_t::ctor(size_t dimensionality)
{
	// Set actual depth to 0 and parent to nullptr.
	depth = 0;
	parent = nullptr;

	// Construct the list of children.
	if (!children.ctor(dimensionality))
	{
		return nullptr;
	}

	// Construct the list of leaves.
	if (!leaves.ctor())
	{
		children.dtor();
		return nullptr;
	}

	// Construct the list of unsorted leaves.
	if (!unsorted.ctor())
	{
		leaves.dtor();
		children.dtor();
		return nullptr;
	}

	// Construct the bounding box.
	if (!bsp_bbox.ctor(dimensionality))
	{
		unsorted.dtor();
		leaves.dtor();
		children.dtor();
		return nullptr;
	}
	return this;
}

//--------------------------------------------------------------------------------------------
void BSP_branch_t::dtor()
{
	// Destruct the list of children.
	children.dtor();

	// Destruct the list of unsorted leaves.
	unsorted.dtor();

	// Destruct the list of nodes.
	leaves.dtor();

	// Destruct the bounding box.
	bsp_bbox.dtor();


	// Set actual depth to 0 and parent to nullptr.
	depth = 0;
	parent = nullptr;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_unlink_all(BSP_branch_t * B)
{
	bool retval = true;

	if (NULL == B) return false;

	if (!B->unlink_parent()) retval = false;

	if (!BSP_branch_unlink_children(B)) retval = false;

	if (!BSP_branch_unlink_nodes(B)) retval = false;

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::unlink_parent()
{
	if (!parent) return true;
	BSP_branch_list_t *children = &(parent->children);

	// Unlink this branch from parent.
	parent = nullptr;

	bool found_self = false;
	// Unlink parent from this branch.
	if (!INVALID_BSP_BRANCH_LIST(children))
	{
		for (size_t i = 0; i < children->lst_size; i++)
		{
			if (this == children->lst[i])
			{
				children->lst[i] = nullptr;
				found_self = true;
			}
		}
	}

	// Re-count the parents children.
	// @todo Inefficient and stupid.
	children->inserted = 0;
	for (size_t i = 0; i < children->lst_size; i++)
	{
		if (!children->lst[i]) continue;
		children->inserted++;
	}

	return found_self;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_unlink_children(BSP_branch_t * B)
{
	// unlink all children from this branch only, not recursively
	if (NULL == B) return false;
	BSP_branch_list_t *children_ptr = &(B->children);

	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		for (size_t i = 0; i < children_ptr->lst_size; i++)
		{
			if (NULL != children_ptr->lst[i])
			{
				children_ptr->lst[i]->parent = NULL;
			}

			children_ptr->lst[i] = NULL;
		}

	}

	bv_self_clear(&(children_ptr->bbox));

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_unlink_nodes(BSP_branch_t * B)
{
	// Remove any nodes (from this branch only, NOT recursively).
	// This resets the B->nodes list.

	if (NULL == B) return false;

	return BSP_branch_free_nodes(B, false);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_insert_leaf_list(BSP_branch_t * B, BSP_leaf_t * n)
{
	if (nullptr == B || nullptr == n) return false;

	return B->leaves.push_front(n);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_update_depth_rec(BSP_branch_t * B, int depth)
{
	if (NULL == B || depth < 0) return false;
	BSP_branch_list_t *children_ptr = &(B->children);

	B->depth = depth;

	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		for (size_t cnt = 0; cnt < children_ptr->lst_size; cnt++)
		{
			if (NULL == children_ptr->lst[cnt]) continue;

			BSP_branch_update_depth_rec(children_ptr->lst[cnt], depth + 1);
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_insert_branch(BSP_branch_t *B, size_t index, BSP_branch_t * B2)
{
	// B and B2 exist?
	if (NULL == B || NULL == B2) return false;
	BSP_branch_list_t *children_ptr = &(B->children);

	// valid child list for B?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// valid index range?
	if (index >= children_ptr->lst_size) return false;

	// we can't merge two branches at this time
	if (NULL != children_ptr->lst[index])
	{
		return false;
	}

	EGOBOO_ASSERT(B->depth + 1 == B2->depth);

	children_ptr->lst[index] = B2;
	B2->parent = B;

	// update the all bboxes above B2
	for (BSP_branch_t *ptmp = B2->parent; NULL != ptmp; ptmp = ptmp->parent)
	{
		BSP_branch_list_t * tmp_children_ptr = &(ptmp->children);

		// add in the size of B2->children
		if (bv_is_clear(&(tmp_children_ptr->bbox)))
		{
			tmp_children_ptr->bbox = B2->children.bbox;
		}
		else
		{
			bv_self_union(&(tmp_children_ptr->bbox), &(B2->children.bbox));
		}

		// add in the size of B2->nodes
		if (bv_is_clear(&(tmp_children_ptr->bbox)))
		{
			tmp_children_ptr->bbox = B2->leaves.bbox;
		}
		else
		{
			bv_self_union(&(tmp_children_ptr->bbox), &(B2->leaves.bbox));
		}

		// add in the size of B2->unsorted
		if (bv_is_clear(&(tmp_children_ptr->bbox)))
		{
			tmp_children_ptr->bbox = B2->unsorted.bbox;
		}
		else
		{
			bv_self_union(&(tmp_children_ptr->bbox), &(B2->unsorted.bbox));
		}
	}

	// update the depth B2 and all children
	BSP_branch_update_depth_rec(B2, B->depth + 1);

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_clear(BSP_branch_t *self, bool recursive)
{
	if (!self) return false;

	if (recursive)
	{
		BSP_branch_list_t::clear_rec(&(self->children));
	}
	else
	{
		// Unlink any child nodes.
		BSP_branch_unlink_children(self);
	}

	// Clear the leaves.
	self->leaves.clear();

	// Clear the unsorted leaves.
	self->unsorted.clear();

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_free_nodes(BSP_branch_t *self, bool recursive)
{
	if (!self) return false;
	BSP_branch_list_t *children = &(self->children);

	if (recursive)
	{
		if (!INVALID_BSP_BRANCH_LIST(children))
		{
			// Recursively clear out any nodes in the child list
			for (size_t i = 0; i < children->lst_size; i++)
			{
				if (NULL == children->lst[i]) continue;

				BSP_branch_free_nodes(children->lst[i], true);
			}
		}
	}

	// Remove all leaves of this branch.
	self->leaves.clear();

	// Remove all unsorted leaves of this branch.
	self->unsorted.clear();

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_tree_t::prune_branch(BSP_tree_t *t, BSP_branch_t *branch, bool recursive)
{
	if (NULL == branch) return false;
	BSP_branch_list_t *children = &(branch->children);

	// prune all of the children 1st
	if (recursive)
	{
		if (!INVALID_BSP_BRANCH_LIST(children))
		{
			// Prune all the children
			for (size_t i = 0; i < children->lst_size; i++)
			{
				BSP_tree_t::prune_branch(t, children->lst[i], true);
			}
		}
	}

	// Do not remove the root node.
	if (branch == t->finite) return true;

	bool retval = true;
	if (branch->empty())
	{
		if (NULL != branch->parent)
		{
			bool found = false;

			// Unlink the parent and return the node to the free list.
			for (size_t i = 0; i < branch->parent->children.lst_size; i++)
			{
				if (branch->parent->children.lst[i] == branch)
				{
					branch->parent->children.lst[i] = NULL;
					found = true;
					break;
				}
			}
			EGOBOO_ASSERT(found);
		}

		retval = (rv_success == t->branch_free.push_back(branch));
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::add_all_leaves(std::vector<BSP_leaf_t*> *collisions) const
{
	size_t lost_nodes = 0;
	// Add any leaves in the leaves list.
	BSP_leaf_t *leave = leaves.lst;
	for (size_t cnt = 0; NULL != leave && cnt < leaves.count; leave = leave->next, cnt++)
	{
		collisions->push_back(leave);
//		if (rv_success != collisions->push_back(leave))
//		{
//			lost_nodes++;
//		}
	}
	// Warn if any leaves were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	return (collisions->size() < collisions->capacity());
}

bool BSP_branch_t::add_all_leaves(BSP_leaf_test_t& test, std::vector<BSP_leaf_t*> *collisions) const
{
	size_t lost_nodes = 0;
	// Add any leaves in the leave list (which pass the filter).
	BSP_leaf_t *leave = leaves.lst;
	for (size_t cnt = 0; nullptr != leave && cnt < leaves.count; leave = leave->next, cnt++)
	{
		if ((*test)(leave))
		{
			collisions->push_back(leave);
//			if (rv_success != collisions->push_back(leave))
//			{
//				lost_nodes++;
//			}
		}
	}
	// Warn if any leaves were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}
	return (collisions->size() < collisions->capacity());
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_add_all_unsorted(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, std::vector<BSP_leaf_t*> *colst)
{
	size_t       cnt, colst_cp, lost_nodes;
	BSP_leaf_t * ptmp;
	const BSP_leaf_list_t * unsorted_ptr;

	if (NULL == pbranch) return false;
	unsorted_ptr = &(pbranch->unsorted);

	colst_cp = colst->capacity();

	lost_nodes = 0;

	if (NULL != ptest)
	{
		// add any valid unsorted in the unsorted.lst
		for (cnt = 0, ptmp = unsorted_ptr->lst; NULL != ptmp && cnt < unsorted_ptr->count; ptmp = ptmp->next, cnt++)
		{
			if ((*ptest)(ptmp))
			{
				colst->push_back(ptmp);
				//if (rv_success != colst->push_back(ptmp))
				//{
				//	lost_nodes++;
				//}
			}
		}
	}
	else
	{
		// add any unsorted in the unsorted.lst
		for (cnt = 0, ptmp = unsorted_ptr->lst; NULL != ptmp && cnt < unsorted_ptr->count; ptmp = ptmp->next, cnt++)
		{
			colst->push_back(ptmp);
			//if (rv_success != colst->push_back(ptmp))
			//{
			//	lost_nodes++;
			//}
		}
	}

	// warn the user if any nodes were rejected
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	return (colst->size() < colst_cp);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_add_all_children(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, std::vector<BSP_leaf_t*> *colst)
{
	size_t cnt, colst_cp;

	const BSP_branch_list_t * children_ptr;

	if (NULL == pbranch) return false;
	children_ptr = &(pbranch->children);

	colst_cp = colst->capacity();

	// add all nodes from all children
	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		for (cnt = 0; cnt < children_ptr->lst_size; cnt++)
		{
			BSP_branch_t * pchild = children_ptr->lst[cnt];
			if (NULL == pchild) continue;

			BSP_branch_add_all_rec(pchild, ptest, colst);

			if (colst->size() >= colst_cp) break;
		}

	}

	return (colst->size() < colst_cp);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_add_all_rec(const BSP_branch_t * pbranch, BSP_leaf_test_t * ptest, std::vector<BSP_leaf_t*> *colst)
{
	size_t cnt, colst_cp;

	const BSP_branch_list_t * children_ptr;

	if (NULL == pbranch) return false;
	children_ptr = &(pbranch->children);

	colst_cp = colst->capacity();

	if (ptest)
	{
		pbranch->add_all_leaves(*ptest, colst);
	}
	else
	{
		pbranch->add_all_leaves(colst);
	}
	BSP_branch_add_all_unsorted(pbranch, ptest, colst);

	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		// add all nodes from all children
		for (cnt = 0; cnt < children_ptr->lst_size; cnt++)
		{
			BSP_branch_t * pchild = children_ptr->lst[cnt];
			if (NULL == pchild) continue;

			BSP_branch_add_all_rec(pchild, ptest, colst);

			if (colst->size() >= colst_cp) break;
		}

	}

	return (colst->size() < colst_cp);
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::empty() const
{
	// Assume the branch is empty.
	bool empty = true;

	// If the branch has leaves, it is not empty.
	if (!leaves.empty())
	{
		empty = false;
		goto BSP_branch_empty_exit;
	}

	// If the branch has unsorted leaves, it is not empty.
	if (!unsorted.empty())
	{
		empty = false;
		goto BSP_branch_empty_exit;
	}
	// If the branch has child branches, then it is not empty.
	if (!INVALID_BSP_BRANCH_LIST(&children))
	{
		for (size_t cnt = 0; cnt < children.lst_size; cnt++)
		{
			if (NULL != children.lst[cnt])
			{
				empty = false;
				goto BSP_branch_empty_exit;
			}
		}
	}

BSP_branch_empty_exit:

	return empty;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::collide(const BSP_branch_t *self, const aabb_t *aabb, BSP_leaf_test_t *test, std::vector<BSP_leaf_t *> *collisions)
{
	geometry_rv geom_children, geom_nodes, geom_unsorted;
	bool BSP_retval;

	// if the branch doesn't exist, stop
	if (!self) return false;
	const BSP_branch_list_t *children_ptr = &(self->children);
	const BSP_leaf_list_t *leaves_ptr = &(self->leaves);
	const BSP_leaf_list_t *unsorted_ptr = &(self->unsorted);

	// assume the worst
	bool retval = false;

	// check the unsorted nodes
	geom_unsorted = geometry_error;
	if (NULL != unsorted_ptr && NULL != unsorted_ptr->lst)
	{
		if (0 == unsorted_ptr->count)
		{
			geom_unsorted = geometry_outside;
		}
		else if (1 == unsorted_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the aabb-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_unsorted = geometry_intersect;
		}
		else
		{
			geom_unsorted = aabb_intersects_aabb(*aabb, unsorted_ptr->bbox.aabb);
		}
	}

	// check the nodes at this level of the tree
	geom_nodes = geometry_error;
	if (NULL != leaves_ptr && NULL != leaves_ptr->lst)
	{
		if (0 == leaves_ptr->count)
		{
			geom_nodes = geometry_outside;
		}
		else if (1 == leaves_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the aabb-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_nodes = geometry_intersect;
		}
		else
		{
			geom_nodes = aabb_intersects_aabb(*aabb, leaves_ptr->bbox.aabb);
		}
	}

	// check the nodes at all lower levels
	geom_children = geometry_error;
	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		if (0 == children_ptr->inserted)
		{
			geom_children = geometry_outside;
		}
		else if (1 == children_ptr->inserted)
		{
			// A speedup.
			// If there is only one entry, then the aabb-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_children = geometry_intersect;
		}
		else
		{
			geom_children = aabb_intersects_aabb(*aabb, children_ptr->bbox.aabb);
		}
	}

	if (geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside)
	{
		// the branch and the object do not overlap at all.
		// do nothing.
		return false;
	}

	switch (geom_unsorted)
	{
	case geometry_intersect:
		// The aabb and branch partially overlap. Test each item.
		BSP_retval = unsorted_ptr->collide(aabb, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		BSP_retval = BSP_branch_add_all_unsorted(self, test, collisions);
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_nodes)
	{
	case geometry_intersect:
		// The aabb and branch partially overlap. Test each item.
		BSP_retval = leaves_ptr->collide(aabb, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single leave.
		if (test)
		{
			BSP_retval = self->add_all_leaves(*test, collisions);
		}
		else
		{
			BSP_retval = self->add_all_leaves(collisions);
		}
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_children)
	{
	case geometry_intersect:
		// The aabb and branch partially overlap. Test each item.
		BSP_retval = self->children.collide(aabb, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		BSP_retval = BSP_branch_add_all_children(self, test, collisions);
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_t::collide(const BSP_branch_t *self, const egolib_frustum_t *frustum, BSP_leaf_test_t *test, std::vector<BSP_leaf_t *> *collisions)
{
	geometry_rv geom_children, geom_nodes, geom_unsorted;
	bool BSP_retval;

	// if the branch doesn't exist, stop
	if (!self) return false;
	const BSP_branch_list_t *children_ptr = &(self->children);
	const BSP_leaf_list_t *leaves_ptr = &(self->leaves);
	const BSP_leaf_list_t *unsorted_ptr = &(self->unsorted);

	// assume the worst
	bool retval = false;

	// check the unsorted nodes
	geom_unsorted = geometry_error;
	if (NULL != unsorted_ptr && NULL != unsorted_ptr->lst)
	{
		if (0 == unsorted_ptr->count)
		{
			geom_unsorted = geometry_outside;
		}
		else if (1 == unsorted_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the frustum-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_unsorted = geometry_intersect;
		}
		else
		{
			geom_unsorted = frustum->intersects_bv(&(unsorted_ptr->bbox), true);
		}
	}

	// check the nodes at this level of the tree
	geom_nodes = geometry_error;
	if (NULL != leaves_ptr && NULL != leaves_ptr->lst)
	{
		if (0 == leaves_ptr->count)
		{
			geom_nodes = geometry_outside;
		}
		else if (1 == leaves_ptr->count)
		{
			// A speedup.
			// If there is only one entry, then the frustum-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_nodes = geometry_intersect;
		}
		else
		{
			geom_nodes = frustum->intersects_bv(&(leaves_ptr->bbox), true);
		}
	}

	// check the nodes at all lower levels
	geom_children = geometry_error;
	if (!INVALID_BSP_BRANCH_LIST(children_ptr))
	{
		if (0 == children_ptr->inserted)
		{
			geom_children = geometry_outside;
		}
		else if (1 == children_ptr->inserted)
		{
			// A speedup.
			// If there is only one entry, then the frustum-aabb collision test is
			// called twice for 2 bounding boxes of the same size
			geom_children = geometry_intersect;
		}
		else
		{
			geom_children = frustum->intersects_bv(&(children_ptr->bbox), true);
		}
	}

	if (geom_unsorted <= geometry_outside && geom_nodes <= geometry_outside && geom_children <= geometry_outside)
	{
		// the branch and the object do not overlap at all.
		// do nothing.
		return false;
	}

	switch (geom_unsorted)
	{
	case geometry_intersect:
		// The frustum and branch partially overlap. Test each item.
		BSP_retval = unsorted_ptr->collide(frustum, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		BSP_retval = BSP_branch_add_all_unsorted(self, test, collisions);
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_nodes)
	{
	case geometry_intersect:
		// The frustum and branch partially overlap. Test each item.
		BSP_retval = leaves_ptr->collide(frustum, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single leave.
		if (test)
		{
			BSP_retval = self->add_all_leaves(*test, collisions);
		}
		else
		{
			BSP_retval = self->add_all_leaves(collisions);
		}
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	switch (geom_children)
	{
	case geometry_intersect:
		// The frustum and branch partially overlap. Test each item.
		BSP_retval = self->children.collide(frustum, test, collisions);
		if (BSP_retval) retval = true;
		break;

	case geometry_inside:
		// The branch is completely contained by the test aabb. Add every single node.
		BSP_retval = BSP_branch_add_all_children(self, test, collisions);
		if (BSP_retval) retval = true;
		break;

	default:
		/* do nothing */
		break;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_insert_branch_list_rec(BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int index, int depth)
{
	// the leaf is a child of this branch

	BSP_branch_t * pchild;
	bool inserted_branch;
	BSP_branch_list_t * children_ptr;

	// is there a leaf to insert?
	if (NULL == pleaf) return false;

	// does the branch exist?
	if (NULL == pbranch) return false;
	children_ptr = &(pbranch->children);

	// a valid child list?
	if (INVALID_BSP_BRANCH_LIST(children_ptr)) return false;

	// is the index within the correct range?
	if (index < 0 || (size_t)index >= children_ptr->lst_size) return false;

	// get the child. if it doesn't exist, create one
	pchild = BSP_tree_ensure_branch(ptree, pbranch, index);

	// try to insert the leaf
	inserted_branch = false;
	if (NULL != pchild)
	{
		// insert the leaf
		inserted_branch = BSP_branch_insert_leaf_rec(ptree, pchild, pleaf, depth);
	}

	// if it was inserted, update the bounding box
	if (inserted_branch)
	{
		if (0 == children_ptr->inserted)
		{
			children_ptr->bbox = pleaf->bbox;
		}
		else
		{
			bv_self_union(&(children_ptr->bbox), &(pleaf->bbox));
		}

		children_ptr->inserted++;
	}

	return inserted_branch;
}

//--------------------------------------------------------------------------------------------
int BSP_branch_insert_leaf_rec_1(BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth)
{
	int index, retval;

	bool inserted_leaf, inserted_branch;

	if (NULL == ptree || NULL == pbranch || NULL == pleaf || depth < 0) return -1;

	// not inserted anywhere, yet
	inserted_leaf = false;
	inserted_branch = false;

	// don't go too deep
	if (depth > ptree->max_depth)
	{
		// insert the node under this branch
		inserted_branch = BSP_branch_insert_leaf_list(pbranch, pleaf);
	}
	else
	{
		//---- determine which child the leaf needs to go under
		index = _find_child_index(&(pbranch->bsp_bbox), &(pleaf->bbox.aabb));

		//---- insert the leaf in the right place
		if (index < -1)
		{
			/* do nothing */
		}
		else if (-1 == index)
		{
			// the leaf belongs on this branch
			inserted_leaf = BSP_branch_insert_leaf_list(pbranch, pleaf);
		}
		else
		{
			inserted_branch = BSP_branch_insert_branch_list_rec(ptree, pbranch, pleaf, index, depth);
		}
	}

	retval = -1;
	if (inserted_leaf && !inserted_branch)
	{
		retval = 0;
	}
	else if (!inserted_leaf && inserted_branch)
	{
		retval = 1;
	}

	return retval;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_insert_leaf_rec(BSP_tree_t * ptree, BSP_branch_t * pbranch, BSP_leaf_t * pleaf, int depth)
{
	/// @author BB
	/// @details recursively insert a leaf in a tree of BSP_branch_t*. Get new branches using the
	///              BSP_tree_get_free() function to allocate any new branches that are needed.

	bool handled = false;
	size_t nodes_at_this_level, unsorted_nodes, sibling_nodes;

	BSP_leaf_list_t * unsorted_ptr;
	BSP_leaf_list_t * nodes_ptr;

	if (NULL == ptree || NULL == pleaf) return false;

	if (NULL == pbranch) return false;
	unsorted_ptr = &(pbranch->unsorted);
	nodes_ptr = &(pbranch->leaves);

	// nothing has been done with the pleaf pointer yet
	handled = false;

	// count the nodes in different categories
	sibling_nodes = nodes_ptr->count;
	unsorted_nodes = unsorted_ptr->count;
	nodes_at_this_level = sibling_nodes + unsorted_nodes;

	// is list of unsorted nodes overflowing?
	if (unsorted_nodes > 0 && nodes_at_this_level >= BRANCH_NODE_THRESHOLD)
	{
		// scan through the list and insert everyone in the next lower branch

		BSP_leaf_t * tmp_leaf;
		int inserted;

		// if you just sort the whole list, you could end up with
		// every single element in the same node at some large depth
		// (if the nodes are strange that way).
		// Since, REDUCING the depth of the BSP is the entire point, we have to limit this in some way...
		size_t target_unsorted = (sibling_nodes > BRANCH_NODE_THRESHOLD) ? 0 : BRANCH_NODE_THRESHOLD - sibling_nodes;
		size_t min_unsorted = target_unsorted >> 1;

		if (unsorted_ptr->count > min_unsorted)
		{
			tmp_leaf = pbranch->unsorted.pop_front();
			inserted = 0;
			do
			{
				if (BSP_branch_insert_leaf_rec_1(ptree, pbranch, tmp_leaf, depth + 1))
				{
					inserted++;
				}

				// Break out BEFORE popping off another pointer.
				// Otherwise, we lose drop a node by mistake.
				if (unsorted_ptr->count <= min_unsorted) break;

				tmp_leaf = pbranch->unsorted.pop_front();
			} while (NULL != tmp_leaf);

			// now clear out the old bbox.
			bv_self_clear(&(unsorted_ptr->bbox));

			if (unsorted_ptr->count > 0)
			{
				// generate the correct bbox for any remaining nodes

				size_t cnt;

				tmp_leaf = unsorted_ptr->lst;
				cnt = 0;
				if (NULL != tmp_leaf)
				{
					unsorted_ptr->bbox.aabb = tmp_leaf->bbox.aabb;
					tmp_leaf = tmp_leaf->next;
					cnt++;
				}

				for ( /* nothing */;
					NULL != tmp_leaf && cnt < unsorted_ptr->count;
					tmp_leaf = tmp_leaf->next, cnt++)
				{
					aabb_self_union(unsorted_ptr->bbox.aabb, tmp_leaf->bbox.aabb);
				}

				bv_validate(&(unsorted_ptr->bbox));
			}
		}
	}

	// re-calculate the values for this branch
	sibling_nodes = nodes_ptr->count;
	unsorted_nodes = unsorted_ptr->count;
	nodes_at_this_level = sibling_nodes + unsorted_nodes;

	// defer sorting any new leaves, if possible
	if (!handled && nodes_at_this_level < BRANCH_NODE_THRESHOLD)
	{
		handled = pbranch->unsorted.push_front(pleaf);
	}

	// recursively insert
	if (!handled)
	{
		int where;

		// keep track of the tree depth
		where = BSP_branch_insert_leaf_rec_1(ptree, pbranch, pleaf, depth + 1);

		handled = (where >= 0);
	}

	// keep a record of the highest depth
	if (handled && depth > ptree->depth)
	{
		ptree->depth = depth;
	}

	return handled;
}

//--------------------------------------------------------------------------------------------
// BSP_tree_t
//--------------------------------------------------------------------------------------------
BSP_tree_t *BSP_tree_t::ctor(size_t dimensionality, size_t maximumDepth)
{
	if (dimensionality < BSP_tree_t::DIMENSIONALITY_MIN)
	{
		log_error("%s: %d: cannot construct a BSP tree with less than 2 dimensions (dimensionality %zu given)\n", \
			      __FILE__, __LINE__, dimensionality);
		return nullptr;
	}

	BLANK_STRUCT_PTR(this);

	bbox.ctor();
	bsp_bbox.ctor(dimensionality);
	infinite.ctor();

	size_t node_count;
	if (!BSP_tree_count_nodes(dimensionality, maximumDepth, node_count))
	{
		bbox.dtor();
		bsp_bbox.dtor();
		infinite.dtor();
		return nullptr;
	}
	/********************************************************************************/
	// Construct the infinite node list
	infinite.ctor();

	// Construct the branches
	if (!branch_all.ctor(node_count))
	{
		bbox.dtor();
		bsp_bbox.dtor();
		infinite.dtor();
		/** @todo Destruct. */
		return nullptr;
	}
	// initialize the array branches
	for (size_t index = 0; index < node_count; ++index)
	{
		BSP_branch_t *branch = BSP_branch_t::create(dimensionality);
		if (!branch)
		{
			bbox.dtor();
			bsp_bbox.dtor();
			infinite.dtor();
			/** @todo Destruct. */
			return nullptr;
		}
		if (!branch_all.push_back(branch))
		{
			BSP_branch_t::destroy(branch);
			bbox.dtor();
			bsp_bbox.dtor();
			infinite.dtor();
			/** @todo Destruct. */
			return nullptr;
		}
	}

	// Construct the auxiliary array.
	branch_used.ctor(node_count);
	branch_free.ctor(node_count);

	// Set the variables.
	this->dimensions = dimensionality;
	this->max_depth = maximumDepth;
	this->depth = 0;

	// Initialize the free list.
	for (size_t i = 0; i < branch_all.size(); i++)
	{
		branch_free.push_back(branch_all.ary[i]);
	}
	return this;
}

//--------------------------------------------------------------------------------------------
void BSP_tree_t::dtor()
{
	// Destruct the infinite node list.
	infinite.dtor();

	// Destruct the auxiliary arrays.
	branch_used.dtor();
	branch_free.dtor();

	// Destruct the branches.
	while (!branch_all.empty())
	{
		BSP_branch_t *branch = *branch_all.pop_back();
		BSP_branch_t::destroy(branch);
	}

	// Destruct the all branches list.
	branch_all.dtor();

	// Destruct the root bounding box.
	bsp_bbox.dtor();
}

//--------------------------------------------------------------------------------------------
namespace Ego
{
	namespace Math
	{
		/**
		 * @brief
		 *	Perform a safe multiplication.
		 * @param x
		 *	the multiplicand
		 * @param y
		 *	the multiplier
		 * @param [out] z
		 *	the product
		 * @return
		 *	@a true if the multiplcation neither overflows nor underflows, @a false otherwise
		 * @post
		 *	If the multiplication neither overflows nor underflows, @a z was assigned the product <tt>x * y</tt>.
		 *	Otherwise @a z is assigned std::numeric_limits<Type>::max().
		 * @todo
		 *	Move into <tt>egolib/_math.h</tt>.
		 */
		template <typename Type>
		bool safe_mul(const Type& x, const Type& y, Type& z);
		/**
		* @brief
		*	Perform a safe addition.
		* @param x
		*	the augend
		* @param y
		*	the added
		* @param [out] z
		*	the sum
		* @return
		*	@a true if the addition neither overflows nor underflows, @a false otherwise
		* @post
		*	If the addition neither overflows nor underflows, @a z was assigned the sum <tt>x + y</tt>.
		*	Otherwise @a z is assigned std::numeric_limits<Type>::max().
		* @todo
		*	Move into <tt>egolib/_math.h</tt>.
		*/
		template <typename Type>
		bool safe_add(const Type& x, const Type& y, Type& z);
		/**
		 * @brief
		 *	Perform a safe subtraction.
		 * @param x
		 *	the minuend
		 * @param y
		 *	the subtrahend
		 * @param [out] z
		 *	the sum
		 * @return
		 *	@a true if the subtraction neither overflows nor underflows, @a false otherwise
		 * @post
		 *	If the addition neither overflows nor underflows, @a z was assigned the sum <tt>x + y</tt>.
		 *	Otherwise @a z is assigned std::numeric_limits<Type>::max().
		 * @todo
		 *	Move into <tt>egolib/_math.h</tt>.
		 */
		template <typename Type>
		bool safe_sub(const Type& x, const Type& y, Type& z);

		template <>
		bool safe_mul<size_t>(const size_t& x, const size_t& y, size_t &z)
		{
			size_t max = std::numeric_limits<size_t>::max();
			if (x == 0 || y == 0)
			{
				z = 0;
				return true;
			}
			// x as well as y are not 0
			if (max / x < y)
			{
				z = max;
				return false;
			}
			z = x * y;
			return true;
		}
	};
};

bool BSP_tree_count_nodes(size_t dimensionality, size_t maxdepth,size_t& numberOfNodes)
{
	// If dimensionality is too small, reject.
	if (dimensionality < BSP_tree_t::DIMENSIONALITY_MIN)
	{
		log_error("%s: %d: specified dimensionality %zu is smaller than allowed minimum dimensionality %zu\n", \
			      __FILE__, __LINE__, dimensionality, BSP_tree_t::DIMENSIONALITY_MIN);
		return false;
	}
	// If maximum depth is too big, reject.
	if (maxdepth > BSP_tree_t::DEPTH_MAX)
	{
		log_error("%s: %d: specified maximum depth %zu is greater than allowed maximum depth %zu\n", \
			      __FILE__, __LINE__, maxdepth, BSP_tree_t::DEPTH_MAX);
		return false;
	}
	// If dim * (maxdepth + 1) would overflow, reject.
	size_t tmp;
	Ego::Math::safe_mul(dimensionality, maxdepth + 1, tmp);
	// If tmp * 2 would overflow, reject.
	if (std::numeric_limits<size_t>::max()/2 < tmp) return false;

	size_t numer = (1 << tmp) - 1;
	size_t denom = (1 << dimensionality) - 1;
	numberOfNodes = numer / denom;
	return true;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_tree_alloc_branch(BSP_tree_t * t)
{
	BSP_branch_t ** pB = NULL, *B = NULL;

	if (NULL == t) return NULL;

	// grab the top branch
	pB = t->branch_free.pop_back();
	if (NULL == pB) return NULL;

	B = *pB;
	if (NULL == B) return NULL;

	// add it to the used list
	t->branch_used.push_back(B);

	return B;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t * BSP_tree_get_free(BSP_tree_t * t)
{
	BSP_branch_t *  B;

	// try to get a branch from our pre-allocated list. do all necessary book-keeping
	B = BSP_tree_alloc_branch(t);
	if (NULL == B) return NULL;

	if (NULL != B)
	{
		// make sure that this branch does not have data left over
		// from its last use
		EGOBOO_ASSERT(NULL == B->leaves.lst);

		// make sure that the data is cleared out
		BSP_branch_unlink_all(B);
	}

	return B;
}

//--------------------------------------------------------------------------------------------
bool BSP_tree_clear_rec(BSP_tree_t * t)
{
	if (NULL == t) return false;

	// clear all the branches, recursively
	BSP_branch_clear(t->finite, true);

	// Clear the infinite leaves of the tree.
	t->infinite.clear();

	// set the bbox to empty
	bv_self_clear(&(t->bbox));

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_tree_t::prune(BSP_tree_t *self)
{
	if (!self) return false;
	if (!self->finite) return true;

	// Search through all branches. This will not catch all of the
	// empty branches every time, but it should catch quite a few.
	for (size_t cnt = 0; cnt < self->branch_used.sz; cnt++)
	{
		BSP_branch_t *branch = self->branch_used.ary[cnt];
		if (!branch) continue;

		// Do not remove the root node.
		if (branch == self->finite) continue;

		bool remove = false;
		if (branch->empty())
		{
			bool found = BSP_branch_unlink_all(branch);

			// not finding yourself is an error
			if (!found)
			{
				EGOBOO_ASSERT(false);
			}

			remove = true;
		}

		if (remove)
		{
			// reduce the size of the list
			self->branch_used.sz--;

			// set branches's data to "safe" values
			// this has already been "unlinked", so some of this is redundant
			branch->parent = nullptr;
			for (size_t i = 0; i < branch->children.lst_size; i++)
			{
				branch->children.lst[i] = nullptr;
			}
			branch->leaves.clear();
			branch->unsorted.clear();
			branch->depth = -1;
			BSP_aabb_t::set_empty(&(branch->bsp_bbox));

			// move the branch that we found to the top of the list
			SWAP(BSP_branch_t *, self->branch_used.ary[cnt], self->branch_used.ary[self->branch_used.sz]);

			// add the branch to the free list
			self->branch_free.push_back(branch);
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t *BSP_tree_t::ensure_root(BSP_tree_t *self)
{
	if (!self) return nullptr;
	if (self->finite) return self->finite;

	BSP_branch_t *root = BSP_tree_get_free(self);
	if (!root) return NULL;

	// make sure that it is unlinked
	BSP_branch_unlink_all(root);

	// copy the tree bounding box to the root node
	for (size_t cnt = 0; cnt < self->dimensions; cnt++)
	{
		root->bsp_bbox.mins.ary[cnt] = self->bsp_bbox.mins.ary[cnt];
		root->bsp_bbox.mids.ary[cnt] = self->bsp_bbox.mids.ary[cnt];
		root->bsp_bbox.maxs.ary[cnt] = self->bsp_bbox.maxs.ary[cnt];
	}

	// fix the depth
	root->depth = 0;

	// assign the root to the tree
	self->finite = root;

	return root;
}

//--------------------------------------------------------------------------------------------
BSP_branch_t *BSP_tree_ensure_branch(BSP_tree_t * t, BSP_branch_t * B, size_t index)
{
	BSP_branch_t *pbranch;

	if ((NULL == t) || (NULL == B)) return NULL;
	if (index < 0 || (size_t)index > B->children.lst_size) return NULL;

	// grab any existing value
	pbranch = B->children.lst[index];

	// if this branch doesn't exist, create it and insert it properly.
	if (NULL == pbranch)
	{
		// grab a free branch
		pbranch = BSP_tree_get_free(t);

		if (NULL != pbranch)
		{
			// make sure that it is unlinked
			BSP_branch_unlink_all(pbranch);

			// generate its bounding box
			pbranch->depth = B->depth + 1;
			_generate_BSP_aabb_child(&(B->bsp_bbox), index, &(pbranch->bsp_bbox));

			// insert it in the correct position
			BSP_branch_insert_branch(B, index, pbranch);
		}
	}

	return pbranch;
}
//--------------------------------------------------------------------------------------------
bool BSP_tree_insert_leaf(BSP_tree_t *self, BSP_leaf_t *leaf)
{
	bool retval;

	if (!self || !leaf) return false;

	// If the leaf is fully contained in the tree's bounding box ...
	if (!self->bsp_bbox.contains_aabb(leaf->bbox.aabb))
	{
		// ... put the leaf at the head of the infinite list.
		retval = self->infinite.push_front(leaf);
	}
	// Otherwise:
	else
	{
		BSP_branch_t *root = BSP_tree_t::ensure_root(self);

		retval = BSP_branch_insert_leaf_rec(self, root, leaf, 0);
	}

	// Make sure the bounding box matches the maximum extend of all leaves in the tree.
	if (retval)
	{
		if (bv_is_clear(&(self->bbox)))
		{
			self->bbox = leaf->bbox;
		}
		else
		{
			bv_self_union(&(self->bbox), &(leaf->bbox));
		}
	}

	return retval;
}

//--------------------------------------------------------------------------------------------

size_t BSP_tree_t::collide(const aabb_t *aabb, BSP_leaf_test_t *test, std::vector<BSP_leaf_t*> *collisions) const
{
	if (nullptr == aabb || nullptr == collisions)
	{
		return 0;
	}
	// Collide with any "infinite" nodes.
	infinite.collide(aabb, test, collisions);

	// Collide with the rest of the BSP tree.
	BSP_branch_t::collide(finite, aabb, test, collisions);

	return collisions->size();
}

//--------------------------------------------------------------------------------------------
size_t BSP_tree_t::collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, std::vector<BSP_leaf_t*> *collisions) const
{
	if (nullptr == frustum || nullptr == collisions)
	{
		return 0;
	}
	// Collide with any "infinite" nodes.
	infinite.collide(frustum, test, collisions);

	// Collide with the rest of the BSP tree.
	BSP_branch_t::collide(finite, frustum, test, collisions);

	return collisions->size();
}

//--------------------------------------------------------------------------------------------
BSP_leaf_list_t *BSP_leaf_list_t::ctor()
{
	count = 0;
	lst = nullptr;
	bbox.ctor();
	return this;
}

//--------------------------------------------------------------------------------------------
void BSP_leaf_list_t::dtor()
{
	bbox.dtor();
	lst = nullptr;
	count = 0;
}

//--------------------------------------------------------------------------------------------
void BSP_leaf_list_t::clear()
{
	while (nullptr != lst)
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->next;
		leaf->next = nullptr;
		leaf->inserted = false;
		count--;
	}
	bv_self_clear(&bbox);
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_t::push_front(BSP_leaf_t *leaf)
{
	if (nullptr == leaf)
	{
		return false;
	}
	if (leaf->inserted)
	{
		log_warning("%s: %d: trying to insert a BSP_leaf that is claiming to be part of a list already\n",__FILE__,__LINE__);
		return false;
	}

	if (nullptr == lst)
	{
		// Insert the leaf.
		leaf->next = lst; lst = leaf;
		count++;
		leaf->inserted = true;
		// Use the leaf's bounding box as the bounding box of the leaf list.
		bbox = leaf->bbox;
	}
	else
	{
		// Insert the leaf at the beginning of the leaf list.
		leaf->next = lst; lst = leaf;
		count++;
		leaf->inserted = true;
		// Use the union of the leaf's bounding box and the old bounding box the leaf list
		// as the new bounding box of the leaf list.
		bv_self_union(&(bbox), &(leaf->bbox));
		return true;
	}
	return true;
}

//--------------------------------------------------------------------------------------------
BSP_leaf_t *BSP_leaf_list_t::pop_front()
{
	if (nullptr == lst)
	{
		return nullptr;
	}
	else
	{
		BSP_leaf_t *leaf = lst;
		lst = leaf->next;
		leaf->next = nullptr;
		leaf->inserted = false;
		count--;
		return leaf;
	}
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_t::collide(const aabb_t *aabb, BSP_leaf_test_t *test, std::vector<BSP_leaf_t *>  *collisions) const
{
	// Validate arguments.
	if (nullptr == aabb)
	{
		return false;
	}
	// If this leaf list is empty, there is nothing to do.
	if (empty())
	{
		return true;
	}
	// If the AABB does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (!aabb_intersects_aabb(*aabb, bbox.aabb))
	{
		return false;
	}

	size_t lost_nodes = 0;  // The number of lost nodes.
	BSP_leaf_t *leaf = lst; // The current leaf.
	if (nullptr != test)
	{
		// Iterate over the leaves in the leaf list.
		for (; nullptr != leaf; leaf = leaf->next)
		{
			// The leaf must have a valid type.
			EGOBOO_ASSERT(leaf->data_type > -1);
			// The leaf must be in a leaf list.
			EGOBOO_ASSERT(leaf->inserted);

			// Test geometry.
			geometry_rv geometry_test = aabb_intersects_aabb(*aabb, leaf->bbox.aabb);

			// Determine what action to take.
			bool do_insert = false;
			if (geometry_test > geometry_outside)
			{
				do_insert = (*test)(leaf);
			}

			if (do_insert)
			{
				collisions->push_back(leaf);
				//if (rv_success != collisions->push_back(leaf))
				//{
				//	lost_nodes++;
				//}
			}
		}
	}
	else
	{

		// Iterate over the leaves in the leaf list.
		for (; nullptr != leaf; leaf = leaf->next)
		{
			// The leaf must have a valid type.
			EGOBOO_ASSERT(leaf->data_type > -1);
			// The leaf must be inserted.
			EGOBOO_ASSERT(leaf->inserted);

			// Test the geometry.
			geometry_rv geometry_test = aabb_intersects_aabb(*aabb, leaf->bbox.aabb);

			// determine what action to take
			bool do_insert = false;
			if (geometry_test > geometry_outside)
			{
				do_insert = true;
			}

			if (do_insert)
			{
				collisions->push_back(leaf);
				//if (rv_success != collisions->push_back(leaf))
				//{
				//	lost_nodes++;
				//}
			}
		}
	}

	// Warn if any nodes were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	// Return false if the collision list is full.
	return collisions->size() != collisions->capacity();
}

//--------------------------------------------------------------------------------------------
bool BSP_leaf_list_t::collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, std::vector<BSP_leaf_t*> *collisions) const
{
	// Validate parameters.
	if (nullptr == frustum)
	{
		return false;
	}

	// If this leaf list is empty, there is nothing to do.
	if (empty())
	{
		return true;
	}

	// If the frustum does not intersect the bounding box enclosing the leaves in this leaf list,
	// there is nothing to do.
	if (!frustum->intersects_bv(&(bbox),true))
	{
		return false;
	}

	// If the collision list is full, return false.
	if (collisions->size() == collisions->capacity())
	{
		return false;
	}

	size_t lost_nodes = 0; // The number of lost nodes.
	BSP_leaf_t *leaf = lst;  // The current leaf.
	// Scan through every leaf
	for (; nullptr != leaf; leaf = leaf->next)
	{
		// The leaf must have a valid type.
		EGOBOO_ASSERT(leaf->data_type > -1);
		// The leaf must be inserted.
		EGOBOO_ASSERT(leaf->inserted);

		// Test the geometry
		geometry_rv geometry_test = frustum->intersects_bv(&leaf->bbox, true);

		// Determine what action to take.
		bool do_insert = false;
		if (geometry_test > geometry_outside)
		{
			if (nullptr == test)
			{
				do_insert = true;
			}
			else
			{
				// we have a possible intersection
				do_insert = (*test)(leaf);
			}
		}

		if (do_insert)
		{
			collisions->push_back(leaf);
			//if (rv_success != collisions->push_back(leaf))
			//{
			//	lost_nodes++;
			//}
		}
	}

	// Warn if any nodes were rejected.
	if (lost_nodes > 0)
	{
		log_warning("%s - %d nodes not added.\n", __FUNCTION__, lost_nodes);
	}

	// Return false if the collision list is full.
	return collisions->size() != collisions->capacity();
}

//--------------------------------------------------------------------------------------------
// BSP_branch_list_t
//--------------------------------------------------------------------------------------------
BSP_branch_list_t *BSP_branch_list_t::ctor(size_t dim)
{
	BLANK_STRUCT_PTR(this)
	// Determine the number of children from the dimensionality.
	size_t child_count = (0 == dim) ? 0 : (2 << (dim - 1));
	if (0 == child_count)
	{
		return nullptr;
	}
	// Allocate the child list.
	this->lst = EGOBOO_NEW_ARY(BSP_branch_t*, child_count);
	if (!this->lst)
	{
		return nullptr;
	}
	this->lst_size = child_count;
	for (size_t index = 0; index < child_count; ++index)
	{
		this->lst[index] = nullptr;
	}
	this->bbox.ctor();
	return this;
}

//--------------------------------------------------------------------------------------------
BSP_branch_list_t *BSP_branch_list_t::dtor()
{
	EGOBOO_DELETE_ARY(this->lst);
	this->lst_size = 0;
	BLANK_STRUCT_PTR(this);
	return this;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_list_t::clear_rec(BSP_branch_list_t * BL)
{
	size_t cnt;

	if (NULL == BL) return false;

	// recursively clear out any nodes in the children.lst
	for (cnt = 0; cnt < BL->lst_size; cnt++)
	{
		if (NULL == BL->lst[cnt]) continue;

		BSP_branch_clear(BL->lst[cnt], true);
	};

	// reset the number of inserted children
	BL->inserted = 0;

	// clear out the size of the nodes
	bv_self_clear(&(BL->bbox));

	return true;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_list_t::collide(const aabb_t *aabb, BSP_leaf_test_t *test, std::vector<BSP_leaf_t*> *collisions) const
{
	// Scan the child branches and collide with them recursively.
	size_t numberOfCollisions = 0;
	for (size_t i = 0; i < lst_size; ++i)
	{
		BSP_branch_t *child = lst[i];
		if (!child) continue;
		if (BSP_branch_t::collide(child, aabb, test, collisions))
		{
			numberOfCollisions++;
		}
	}
	return numberOfCollisions > 0;
}

//--------------------------------------------------------------------------------------------
bool BSP_branch_list_t::collide(const egolib_frustum_t *frustum, BSP_leaf_test_t *test, std::vector<BSP_leaf_t*> *collisions) const
{
	// Scan the child branches and collide with them recursively.
	size_t numberOfCollisions = 0;
	for (size_t i = 0; i < lst_size; ++i)
	{
		BSP_branch_t *child = lst[i];
		if (!child) continue;
		if (BSP_branch_t::collide(child, frustum, test, collisions))
		{
			numberOfCollisions++;
		}
	}

	return numberOfCollisions > 0;
}

bool BSP_leaf_valid(const BSP_leaf_t *self)
{
    if (!self) return false;
	if (!self->data || self->data_type < 0) return false;
    return true;
}
