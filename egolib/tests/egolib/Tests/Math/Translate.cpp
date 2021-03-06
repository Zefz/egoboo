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

#include "egolib/Tests/Math/MathTestUtilities.hpp"

namespace Ego {
namespace Math {

namespace Test {

EgoTest_TestCase(Translate) {
    EgoTest_Test(Point3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Point3f(-1.0f, -1.0f, -1.0f);
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(AxisAlignedBox3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::AxisAlignedBox3f(::Point3f(-1.0f, -1.0f, -1.0f), ::Point3f(+1.0f, +1.0f, +1.0f));
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(AxisAlignedCube3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::AxisAlignedCube3f(::Point3f(0.0f, 0.0f, 0.0f), 1.0f);
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Sphere3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Sphere3f(::Point3f(0.0f, 0.0f, 0.0f), +1.0f);
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Line3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Line3f(::Point3f(-1.0f, -1.0f, -1.0f), ::Point3f(-1.0f, -1.0f, -1.0f));
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Ray3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Ray3f(::Point3f(0.0f, 0.0f, 0.0f), Vector3f(+1.0f,+1.0f,+1.0f));
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Cone3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Cone3f();
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
    EgoTest_Test(Plane3f) {
        auto t = Vector3f(+1.0f, +1.0f, +1.0f);
        auto x = ::Plane3f();
        auto y = translate(x, t);
        y = translate(y, -t);
        EgoTest_Assert(x == y);
    }
};

} // namespace Test
} // namespace Math
} // namespace Ego
