/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2016 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <string>
#include <memory>
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

    class Object : private NonCopyable
    {
    protected:
        int refcount;

    public:
        std::string name;

        Object();
        virtual ~Object();

        int retain();
        int release();
        int count() const;
    };

    template <typename T>
    class shared_object : public std::shared_ptr<T>
    {
    public:
        template <typename... Args>
        shared_object(Args... args)
        : std::shared_ptr<T>(std::make_shared<T>(args...))
        {
        }

        ~shared_object()
        {
        }
    };

    template <typename T>
    class unique_object : public std::unique_ptr<T>
    {
    public:
#if __cplusplus >= 201402L
        template <typename... Args>
        unique_object(Args... args)
        : std::unique_ptr<T>(std::make_unique<T>(args...))
        {
        }
#else
        template <typename... Args>
        unique_object(Args... args)
        : std::unique_ptr<T>(new T(args...))
        {
        }
#endif

        ~unique_object()
        {
        }
    };

} // namespace mango
