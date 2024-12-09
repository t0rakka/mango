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

    template <typename Key, typename Value>
    class LRUCache
    {
    private:
        std::list<std::pair<Key, Value>> m_values;
        std::unordered_map<Key, typename decltype(m_values)::iterator> m_index;
        size_t m_capacity;

    public:
        LRUCache(size_t capacity)
            : m_capacity(capacity)
        {
        }

        void insert(const Key& key, const Value& value)
        {
            auto it = m_index.find(key);
            if (it != m_index.end())
            {
                // the key already exists, update the value
                it->second->second = value;

                // make the value most recently used
                m_values.splice(m_values.begin(), m_values, it->second);

                return;
            }

            if (m_values.size() == m_capacity)
            {
                // evict the least recently used value
                const auto& evicted = m_values.back();
                m_index.erase(evicted.first);
                m_values.pop_back();
            }

            m_values.emplace_front(key, value);
            m_index.emplace(key, m_values.begin());
        }

        std::optional<Value> get(const Key& key)
        {
            auto it = m_index.find(key);
            if (it == m_index.end())
            {
                return {};
            }

            // make the value most recently used
            m_values.splice(m_values.begin(), m_values, it->second);

            return it->second->second;
        }

        void erase(const Key& key)
        {
            auto it = m_index.find(key);
            if (it != m_index.end())
            {
                m_values.erase(it->second);
                m_index.erase(it);
            }
        }

        void clear()
        {
            m_index.clear();
            m_values.clear();
        }

        auto begin()
        {
            return m_values.begin();
        }

        auto end()
        {
            return m_values.end();
        }
    };

} // namespace mango
