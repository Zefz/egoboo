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

/// @file   egolib/Math/EuclideanSpace.hpp
/// @brief  The \f$n\f$-dimensional Euclidean space (aka \f$n\f$-dimensional Cartesian space).
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/Point.hpp"
#include "egolib/Math/Vector.hpp"
#include "egolib/Math/OrderedField.hpp"
#include "egolib/Math/VectorSpace.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  An \f$n\f$-dimensional Euclidean space (sometimes called \f$n\f$-dimensional Cartesian space).
 * @tparam _VectorSpaceType 
 *  the underlying \f$n\f$-dimensional vector space type.
 *  Must fulfil the <em>VectorSpace</em> concept.
 */
template <typename _VectorSpaceType,
          typename _Enabled = void>
struct EuclideanSpace;

template <typename _VectorSpaceType>
struct EuclideanSpace<_VectorSpaceType, std::enable_if_t<true>> {
public:
    /**
     * @brief
     *  The type of the vector space.
     */
    typedef _VectorSpaceType VectorSpaceType;

    /**
     * @brief
     *  The type of this template/template specialization.
     */
    typedef EuclideanSpace<VectorSpaceType> MyType;

    /**
     * @brief
     *  The type of the scalar field.
     */
    typedef typename VectorSpaceType::ScalarFieldType ScalarFieldType;

    /**
     * @brief
     *  Get the dimensionality.
     * @return
     *  the dimensionality
     */
    static constexpr size_t dimensionality() {
        return VectorSpaceType::dimensionality();
    }

    /**
     * @brief
     *  The type of a scalar.
     */
    typedef typename VectorSpaceType::ScalarType ScalarType;

    /**
     * @brief
     *  The type of a vector.
     */
    typedef typename VectorSpaceType::VectorType VectorType;

    /**
     * @brief
     *  The type of a point.
     */
    typedef Point<VectorSpaceType> PointType;

}; // struct EuclideanSpace

} // namespace Math
} // namespace Ego

/// @brief A macro to simplify the reoccuring type definitions for types defined over Euclidean space types.
/// @param _Type the name of the type
/// @remark The following code snipped demonstrates how the macro is used.
/// @code
/// template <typename _EuclideanSpaceType>
/// struct Line : public Translatable<typename _EuclideanSpaceType::VectorSpaceType> {
/// public:
///   Ego_Math_EuclideanSpace_CommonDefinitions(Line);
///   .
///   .
///   .
/// };
/// @endcode
#define Ego_Math_EuclideanSpace_CommonDefinitions(_Type) \
    /** @brief The Euclidean space type over which this type is defined. */ \
    typedef _EuclideanSpaceType EuclideanSpaceType; \
    /** @brief The vector space type (of the Euclidean space type). */ \
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType; \
    /** @brief The scalar field type (of the vector space type). */ \
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType; \
    /** @brief The vector type (of the vector space type). */ \
    typedef typename EuclideanSpaceType::VectorType VectorType; \
    /** @brief The scalar type (of the scalar field type). */ \
    typedef typename EuclideanSpaceType::ScalarType ScalarType; \
    /** @brief The point type (of the Euclidean space type). */ \
    typedef typename EuclideanSpaceType::PointType PointType; \
    /** @brief @a MyType is the type of this template/template specialization. */ \
    typedef _Type<EuclideanSpaceType> MyType;