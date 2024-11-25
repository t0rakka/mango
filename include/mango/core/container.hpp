/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstddef>
#include <unordered_map>
#include <optional>
#include <list>
#include <utility>

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
            auto it = index.find(key);
            if (it != index.end())
            {
                it->second->second = value; // the key already exists, update the value
                values.splice(values.begin(), values, it->second); // make the value most recently used
                return;
            }

            if (values.size() == Capacity)
            {
                // evict the least recently used value
                const auto& evicted = values.back();
                index.erase(evicted.first);
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

            // make the value most recently used
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

        void clear()
        {
            index.clear();
            values.clear();
        }

        auto begin() { return values.begin(); }
        auto end() { return values.end(); }
    };

} // namespace mango
