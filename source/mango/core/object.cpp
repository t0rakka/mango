/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/object.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------
    // Object
    // ----------------------------------------------------------------------------

    Object::Object()
    : refcount(1)
    {
    }

    Object::~Object()
    {
    }

    int Object::retain()
    {
        return ++refcount;
    }

    int Object::release()
    {
        const int count = --refcount;
        if (!count)
        {
            delete this;
        }

        return count;
    }

    int Object::count() const
    {
        return refcount;
    }

} // namespace mango
