/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2023 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <unordered_map>
#include <optional>
#include <list>

namespace mango
{

    template <typename Key, typename Value, size_t Capacity>
    class LRUCache
    {
    private:
        std::list<std::pair<Key, Value>> values;
        std::unordered_map<Key, typename decltype(values)::iterator> index;

    public:
        void insert(const Key& key, const Value& value)
        {
            if (values.size() == Capacity)
            {
                // delete the least-recently-used value
                index.erase(values.back().first);
                values.pop_back();
            }

            values.emplace_front(key, value);
            index.emplace(key, values.begin());
        }

        std::optional<Value> get(const Key& key)
        {
            auto it = index.find(key);
            if (it == index.end())
            {
                return {};
            }

            // make the value most-recently-used
            values.splice(values.begin(), values, it->second);

            return it->second->second;
        }

        void erase(const Key& key)
        {
            auto it = index.find(key);
            if (it != index.end())
            {
                values.erase(it->second);
                index.erase(it);
            }
        }
    };

} // namespace mango
