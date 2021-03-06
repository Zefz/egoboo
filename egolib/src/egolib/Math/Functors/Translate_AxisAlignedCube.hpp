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

/// @file egolib/Math/Functors/Translate_AxisAlignedCube.hpp
/// @brief Translation of axis aligned cubes.
/// @author Michael Heilmann

#pragma once


#include "egolib/Math/AxisAlignedCube.hpp"
#include "egolib/Math/Functors/Translate.hpp"


namespace Ego {
namespace Math {

template <typename _EuclideanSpaceType>
struct Translate<AxisAlignedCube<_EuclideanSpaceType>> {
    using X = AxisAlignedCube<_EuclideanSpaceType>;
    using T = typename _EuclideanSpaceType::VectorType;
    X operator()(const X& x, const T& t) {
        return X(x.getCenter() + t, x.getSize());
    }
};

template <typename _EuclideanSpaceType>
AxisAlignedCube<_EuclideanSpaceType> translate(const AxisAlignedCube<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    Translate<AxisAlignedCube<_EuclideanSpaceType>> f;
    return f(x, t);
}

} // namespace Math
} // namespace Ego
