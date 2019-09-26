/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/object.hpp>

namespace mango
{

    // ----------------------------------------------------------------------------
    // RefCounted
    // ----------------------------------------------------------------------------

    int RefCounted::retain()
    {
        return ++m_count;
    }

    int RefCounted::release()
    {
        const int count = --m_count;
        if (!count)
        {
            delete this;
        }

        return count;
    }

    int RefCounted::count() const
    {
        return m_count;
    }

} // namespace mango
