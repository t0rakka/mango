/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2019 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <memory>
#include <atomic>
#include "configure.hpp"

namespace mango
{

    class NonCopyable
    {
    protected:
        NonCopyable() = default;

    private:
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator = (const NonCopyable&) = delete;
    };

    class RefCounted : private NonCopyable
    {
    protected:
        std::atomic<int> m_count { 1 };

    public:
        RefCounted() = default;
        virtual ~RefCounted() = default;

        int retain();
        int release();
        int count() const;
    };

    template <typename T>
    class SharedObject : public std::shared_ptr<T>
    {
    public:
        template <typename... Args>
        SharedObject(Args... args)
            : std::shared_ptr<T>(std::make_shared<T>(args...))
        {
        }

        SharedObject(T* object)
            : std::shared_ptr<T>(object)
        {
        }

        ~SharedObject() = default;
    };

    template <typename T>
    class UniqueObject : public std::unique_ptr<T>
    {
    public:
#if __cplusplus >= 201402L
        template <typename... Args>
        UniqueObject(Args... args)
            : std::unique_ptr<T>(std::make_unique<T>(args...))
        {
        }
#else
        template <typename... Args>
        UniqueObject(Args... args)
            : std::unique_ptr<T>(new T(args...))
        {
        }
#endif

        UniqueObject(T* object)
            : std::unique_ptr<T>(object)
        {
        }

        ~UniqueObject() = default;
    };

} // namespace mango
