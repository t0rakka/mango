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

    // -------------------------------------------------------------------------
    // LRUCache
    //
    // Eviction: least recently used (back of the MRU list).
    //
    // Pros:
    //   - Simple, fast, predictable; O(1) get/insert.
    //   - Excellent when access has strong temporal locality (flip back/forth).
    //
    // Cons:
    //   - One-shot scans pollute the cache (folder walk 0..N).
    //   - Prefetch touches look like real use; can evict genuinely hot entries.
    //   - No frequency signal; no cost/size awareness.
    //
    // Eviction drops the cache entry only; retain values externally (shared_ptr,
    // renderer handle refcount, etc.) if they must outlive the index slot.
    // -------------------------------------------------------------------------

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

        size_t capacity() const
        {
            return m_capacity;
        }

        size_t size() const
        {
            return m_values.size();
        }

        auto begin()
        {
            return m_values.begin();
        }

        auto end()
        {
            return m_values.end();
        }

        auto begin() const
        {
            return m_values.begin();
        }

        auto end() const
        {
            return m_values.end();
        }

        template <typename Fn>
        void for_each(Fn&& fn)
        {
            for (auto& entry : m_values)
            {
                fn(entry.first, entry.second);
            }
        }

        template <typename Fn>
        void for_each(Fn&& fn) const
        {
            for (const auto& entry : m_values)
            {
                fn(entry.first, entry.second);
            }
        }
    };

    // -------------------------------------------------------------------------
    // ARCCache
    //
    // Adaptive Replacement Cache (Megiddo & Modha). Maintains:
    //   T1 - recent (seen once)     B1 - ghost of evicted T1 keys
    //   T2 - frequent (seen 2+)     B2 - ghost of evicted T2 keys
    //
    // Adapts split parameter p (target T1 size) using hits in B1/B2.
    //
    // Pros:
    //   - Handles both recency and frequency without manual tuning.
    //   - Resists one-pass scans better than LRU (ghost lists learn patterns).
    //   - Good general-purpose policy for mixed browse + revisit workloads.
    //
    // Cons:
    //   - More bookkeeping than LRU; larger constant factors.
    //   - Still slot-based (not byte/cost aware).
    //   - Ghost lists add memory (~capacity extra keys, not values).
    // -------------------------------------------------------------------------

    template <typename Key, typename Value>
    class ARCCache
    {
    private:
        enum class ListKind
        {
            T1,
            T2,
            B1,
            B2,
        };

        using Entry = std::pair<Key, Value>;
        using EntryList = std::list<Entry>;
        using EntryIterator = typename EntryList::iterator;
        using GhostList = std::list<Key>;
        using GhostIterator = typename GhostList::iterator;

        struct Location
        {
            ListKind kind;
            std::variant<EntryIterator, GhostIterator> position;
        };

        size_t m_capacity;
        size_t m_p = 0;

        EntryList m_t1;
        EntryList m_t2;
        GhostList m_b1;
        GhostList m_b2;

        std::unordered_map<Key, Value> m_values;
        std::unordered_map<Key, Location> m_locations;

        static size_t adapt_step(size_t numerator, size_t denominator)
        {
            if (denominator == 0)
            {
                return 1;
            }

            return std::max(numerator / denominator, size_t(1));
        }

        void remove_ghost(const Key& key)
        {
            auto it = m_locations.find(key);
            if (it == m_locations.end())
            {
                return;
            }

            switch (it->second.kind)
            {
                case ListKind::B1:
                    m_b1.erase(std::get<GhostIterator>(it->second.position));
                    break;
                case ListKind::B2:
                    m_b2.erase(std::get<GhostIterator>(it->second.position));
                    break;
                default:
                    return;
            }

            m_locations.erase(it);
        }

        void move_to_mru(ListKind kind, const Key& key)
        {
            auto it = m_locations.find(key);
            if (it == m_locations.end())
            {
                return;
            }

            switch (kind)
            {
                case ListKind::T1:
                {
                    EntryIterator entry = std::get<EntryIterator>(it->second.position);
                    m_t1.splice(m_t1.begin(), m_t1, entry);
                    it->second.position = m_t1.begin();
                    break;
                }
                case ListKind::T2:
                {
                    EntryIterator entry = std::get<EntryIterator>(it->second.position);
                    m_t2.splice(m_t2.begin(), m_t2, entry);
                    it->second.position = m_t2.begin();
                    break;
                }
                default:
                    break;
            }
        }

        void promote_to_t2(const Key& key, const Value& value)
        {
            auto it = m_locations.find(key);
            if (it != m_locations.end() && it->second.kind == ListKind::T1)
            {
                m_t1.erase(std::get<EntryIterator>(it->second.position));
            }

            m_values[key] = value;
            m_t2.emplace_front(key, value);
            m_locations[key] = { ListKind::T2, EntryIterator(m_t2.begin()) };
        }

        bool evict_entry(EntryList& list, ListKind from, ListKind ghost)
        {
            if (list.empty())
            {
                return false;
            }

            auto victim = std::prev(list.end());

            const Key key = victim->first;
            m_values.erase(key);

            std::list<Key>& ghost_list = (ghost == ListKind::B1) ? m_b1 : m_b2;
            ghost_list.emplace_front(key);
            m_locations[key] = { ghost, GhostIterator(ghost_list.begin()) };

            list.erase(victim);
            return true;
        }

        void replace(bool favor_t1)
        {
            if (!m_t2.empty() && (m_t1.size() > m_p || (favor_t1 && m_t1.size() == m_p)))
            {
                if (evict_entry(m_t1, ListKind::T1, ListKind::B1))
                {
                    return;
                }
            }

            evict_entry(m_t2, ListKind::T2, ListKind::B2);
        }

        void trim_ghosts()
        {
            while (m_b1.size() + m_b2.size() > m_capacity)
            {
                if (!m_b2.empty() && (m_b1.size() < m_b2.size() || m_b1.empty()))
                {
                    const Key key = m_b2.back();
                    m_b2.pop_back();
                    m_locations.erase(key);
                }
                else if (!m_b1.empty())
                {
                    const Key key = m_b1.back();
                    m_b1.pop_back();
                    m_locations.erase(key);
                }
                else
                {
                    break;
                }
            }
        }

        void insert_t1(const Key& key, const Value& value)
        {
            m_values[key] = value;
            m_t1.emplace_front(key, value);
            m_locations[key] = { ListKind::T1, EntryIterator(m_t1.begin()) };
        }

        void request(const Key& key, const Value& value)
        {
            auto loc = m_locations.find(key);
            if (loc != m_locations.end())
            {
                switch (loc->second.kind)
                {
                    case ListKind::T1:
                        promote_to_t2(key, m_values[key]);
                        return;
                    case ListKind::T2:
                        move_to_mru(ListKind::T2, key);
                        return;
                    case ListKind::B1:
                    {
                        const size_t delta = adapt_step(m_b2.size(), std::max(m_b1.size(), size_t(1)));
                        m_p = std::min(m_p + delta, m_capacity);
                        remove_ghost(key);
                        replace(true);
                        promote_to_t2(key, value);
                        return;
                    }
                    case ListKind::B2:
                    {
                        const size_t delta = adapt_step(m_b1.size(), std::max(m_b2.size(), size_t(1)));
                        m_p = (m_p >= delta) ? m_p - delta : 0;
                        remove_ghost(key);
                        replace(false);
                        promote_to_t2(key, value);
                        return;
                    }
                }
            }

            if (m_values.size() >= m_capacity)
            {
                if (!m_t1.empty() && m_t1.size() == m_p && !m_b2.empty())
                {
                    const Key ghost = m_b2.back();
                    m_b2.pop_back();
                    m_locations.erase(ghost);
                }
                else if (!m_b1.empty())
                {
                    const Key ghost = m_b1.back();
                    m_b1.pop_back();
                    m_locations.erase(ghost);
                }

                replace(false);
            }

            insert_t1(key, value);
            trim_ghosts();
        }

    public:
        explicit ARCCache(size_t capacity)
            : m_capacity(std::max(capacity, size_t(1)))
        {
        }

        void insert(const Key& key, const Value& value)
        {
            auto loc = m_locations.find(key);
            if (loc != m_locations.end())
            {
                switch (loc->second.kind)
                {
                    case ListKind::T1:
                        promote_to_t2(key, value);
                        return;
                    case ListKind::T2:
                        m_values[key] = value;
                        std::get<EntryIterator>(loc->second.position)->second = value;
                        move_to_mru(ListKind::T2, key);
                        return;
                    case ListKind::B1:
                    case ListKind::B2:
                        break;
                }
            }

            request(key, value);
        }

        std::optional<Value> get(const Key& key)
        {
            auto loc = m_locations.find(key);
            if (loc == m_locations.end())
            {
                return {};
            }

            switch (loc->second.kind)
            {
                case ListKind::T1:
                {
                    Value value = m_values[key];
                    promote_to_t2(key, value);
                    return value;
                }
                case ListKind::T2:
                {
                    move_to_mru(ListKind::T2, key);
                    return m_values.at(key);
                }
                default:
                    return {};
            }
        }

        void erase(const Key& key)
        {
            auto loc = m_locations.find(key);
            if (loc == m_locations.end())
            {
                return;
            }

            switch (loc->second.kind)
            {
                case ListKind::T1:
                    m_t1.erase(std::get<EntryIterator>(loc->second.position));
                    m_values.erase(key);
                    break;
                case ListKind::T2:
                    m_t2.erase(std::get<EntryIterator>(loc->second.position));
                    m_values.erase(key);
                    break;
                case ListKind::B1:
                    m_b1.erase(std::get<GhostIterator>(loc->second.position));
                    break;
                case ListKind::B2:
                    m_b2.erase(std::get<GhostIterator>(loc->second.position));
                    break;
            }

            m_locations.erase(loc);
        }

        void clear()
        {
            m_values.clear();
            m_locations.clear();
            m_t1.clear();
            m_t2.clear();
            m_b1.clear();
            m_b2.clear();
            m_p = 0;
        }

        size_t capacity() const
        {
            return m_capacity;
        }

        size_t size() const
        {
            return m_values.size();
        }

        template <typename Fn>
        void for_each(Fn&& fn)
        {
            for (auto& entry : m_t1)
            {
                fn(entry.first, entry.second);
            }

            for (auto& entry : m_t2)
            {
                fn(entry.first, entry.second);
            }
        }

        template <typename Fn>
        void for_each(Fn&& fn) const
        {
            for (const auto& entry : m_t1)
            {
                fn(entry.first, entry.second);
            }

            for (const auto& entry : m_t2)
            {
                fn(entry.first, entry.second);
            }
        }
    };

    // -------------------------------------------------------------------------
    // TwoQCache
    //
    // 2Q (Johnson & Shasha): separates first-time (probation) from hot entries.
    //   A1in - FIFO probation queue (first access lands here)
    //   A1out - ghost keys evicted from A1in (metadata only)
    //   Am   - LRU main queue (second access and beyond)
    //
    // Default sizing for capacity c: Kin = c/4 (A1in), Kout = c/2 (A1out ghost).
    // Cached values: |A1in| + |Am| <= c.
    //
    // Pros:
    //   - Cheap protection against one-pass scans and prefetch pollution.
    //   - Simpler than ARC; no adaptive p parameter.
    //   - Good middle ground for image folders (sequential + revisit).
    //
    // Cons:
    //   - Kin/Kout split is fixed unless tuned; less adaptive than ARC.
    //   - Ghost list uses extra key storage (~Kout entries).
    //   - Still slot-based, not cost-aware.
    // -------------------------------------------------------------------------

    template <typename Key, typename Value>
    class TwoQCache
    {
    private:
        enum class ListKind
        {
            A1in,
            Am,
            A1out,
        };

        using Entry = std::pair<Key, Value>;
        using EntryList = std::list<Entry>;
        using EntryIterator = typename EntryList::iterator;
        using GhostList = std::list<Key>;
        using GhostIterator = typename GhostList::iterator;

        struct Location
        {
            ListKind kind;
            std::variant<EntryIterator, GhostIterator> position;
        };

        size_t m_capacity;
        size_t m_kin;
        size_t m_kout;

        EntryList m_a1in;
        EntryList m_am;
        GhostList m_a1out;

        std::unordered_map<Key, Location> m_locations;

        static size_t default_kin(size_t capacity)
        {
            return std::max(size_t(1), capacity / 4);
        }

        static size_t default_kout(size_t capacity)
        {
            return capacity / 2;
        }

        void trim_a1out()
        {
            while (m_a1out.size() > m_kout)
            {
                const Key key = m_a1out.back();
                m_a1out.pop_back();
                m_locations.erase(key);
            }
        }

        bool evict_am_lru()
        {
            if (m_am.empty())
            {
                return false;
            }

            auto victim = std::prev(m_am.end());

            m_locations.erase(victim->first);
            m_am.erase(victim);
            return true;
        }

        void evict_a1in_to_ghost()
        {
            if (m_a1in.empty())
            {
                return;
            }

            const Entry entry = m_a1in.back();
            m_a1in.pop_back();
            m_locations.erase(entry.first);

            m_a1out.emplace_front(entry.first);
            m_locations[entry.first] = { ListKind::A1out, GhostIterator(m_a1out.begin()) };
            trim_a1out();
        }

        void ensure_room_for_main()
        {
            while (m_a1in.size() + m_am.size() >= m_capacity)
            {
                if (!m_am.empty() && evict_am_lru())
                {
                    continue;
                }

                if (!m_a1in.empty())
                {
                    evict_a1in_to_ghost();
                    continue;
                }

                break;
            }
        }

        void promote_to_am(const Key& key, const Value& value)
        {
            auto loc = m_locations.find(key);
            if (loc != m_locations.end() && loc->second.kind == ListKind::A1in)
            {
                m_a1in.erase(std::get<EntryIterator>(loc->second.position));
                m_locations.erase(loc);
            }
            else
            {
                remove_ghost(key);
            }

            ensure_room_for_main();

            m_am.emplace_front(key, value);
            m_locations[key] = { ListKind::Am, EntryIterator(m_am.begin()) };
        }

        void remove_ghost(const Key& key)
        {
            auto loc = m_locations.find(key);
            if (loc == m_locations.end() || loc->second.kind != ListKind::A1out)
            {
                return;
            }

            m_a1out.erase(std::get<GhostIterator>(loc->second.position));
            m_locations.erase(loc);
        }

        void insert_a1in(const Key& key, const Value& value)
        {
            while (m_a1in.size() >= m_kin || m_a1in.size() + m_am.size() >= m_capacity)
            {
                if (m_a1in.size() >= m_kin || m_a1in.size() + m_am.size() >= m_capacity)
                {
                    if (!m_a1in.empty())
                    {
                        evict_a1in_to_ghost();
                        continue;
                    }
                }

                if (m_a1in.size() + m_am.size() >= m_capacity)
                {
                    if (!evict_am_lru())
                    {
                        return;
                    }
                    continue;
                }

                break;
            }

            m_a1in.emplace_front(key, value);
            m_locations[key] = { ListKind::A1in, EntryIterator(m_a1in.begin()) };
        }

    public:
        explicit TwoQCache(size_t capacity)
            : m_capacity(std::max(capacity, size_t(1)))
            , m_kin(default_kin(m_capacity))
            , m_kout(default_kout(m_capacity))
        {
        }

        TwoQCache(size_t capacity, size_t kin, size_t kout)
            : m_capacity(std::max(capacity, size_t(1)))
            , m_kin(std::max(kin, size_t(1)))
            , m_kout(kout)
        {
        }

        size_t kin() const
        {
            return m_kin;
        }

        size_t kout() const
        {
            return m_kout;
        }

        void insert(const Key& key, const Value& value)
        {
            auto loc = m_locations.find(key);
            if (loc != m_locations.end())
            {
                switch (loc->second.kind)
                {
                    case ListKind::Am:
                        std::get<EntryIterator>(loc->second.position)->second = value;
                        m_am.splice(m_am.begin(), m_am, std::get<EntryIterator>(loc->second.position));
                        loc->second.position = m_am.begin();
                        return;
                    case ListKind::A1in:
                        promote_to_am(key, value);
                        return;
                    case ListKind::A1out:
                        promote_to_am(key, value);
                        return;
                }
            }

            insert_a1in(key, value);
        }

        std::optional<Value> get(const Key& key)
        {
            auto loc = m_locations.find(key);
            if (loc == m_locations.end())
            {
                return {};
            }

            switch (loc->second.kind)
            {
                case ListKind::Am:
                {
                    EntryIterator entry = std::get<EntryIterator>(loc->second.position);
                    m_am.splice(m_am.begin(), m_am, entry);
                    loc->second.position = m_am.begin();
                    return entry->second;
                }
                case ListKind::A1in:
                {
                    Value value = std::get<EntryIterator>(loc->second.position)->second;
                    promote_to_am(key, value);
                    return value;
                }
                case ListKind::A1out:
                    return {};
            }

            return {};
        }

        void erase(const Key& key)
        {
            auto loc = m_locations.find(key);
            if (loc == m_locations.end())
            {
                return;
            }

            switch (loc->second.kind)
            {
                case ListKind::A1in:
                    m_a1in.erase(std::get<EntryIterator>(loc->second.position));
                    break;
                case ListKind::Am:
                    m_am.erase(std::get<EntryIterator>(loc->second.position));
                    break;
                case ListKind::A1out:
                    m_a1out.erase(std::get<GhostIterator>(loc->second.position));
                    break;
            }

            m_locations.erase(loc);
        }

        void clear()
        {
            m_locations.clear();
            m_a1in.clear();
            m_am.clear();
            m_a1out.clear();
        }

        size_t capacity() const
        {
            return m_capacity;
        }

        size_t size() const
        {
            return m_a1in.size() + m_am.size();
        }

        template <typename Fn>
        void for_each(Fn&& fn)
        {
            for (auto& entry : m_a1in)
            {
                fn(entry.first, entry.second);
            }

            for (auto& entry : m_am)
            {
                fn(entry.first, entry.second);
            }
        }

        template <typename Fn>
        void for_each(Fn&& fn) const
        {
            for (const auto& entry : m_a1in)
            {
                fn(entry.first, entry.second);
            }

            for (const auto& entry : m_am)
            {
                fn(entry.first, entry.second);
            }
        }
    };

    // -------------------------------------------------------------------------
    // CostLRUCache
    //
    // Eviction: LRU among entries, but capacity is a budget sum(cost), not a
    // slot count. Each insert supplies a cost (bytes, or other similar metric).
    //
    // Pros:
    //   - One large entry cannot evict many small ones.
    //   - Natural fit for provisioning memory.
    //
    // Cons:
    //   - Requires meaningful cost estimates.
    //   - insert(key, value) without cost defaults to 1 (slot semantics).
    //   - LRU on cost-budget still ignores re-fetch difficulty beyond cost.
    //
    // insert fails only when cost exceeds budget or capacity is zero.
    // -------------------------------------------------------------------------

    template <typename Key, typename Value, typename Cost = size_t>
    class CostLRUCache
    {
        static_assert(std::is_arithmetic_v<Cost>, "Cost must be numeric");

    private:
        struct Entry
        {
            Key key;
            Value value;
            Cost cost;
        };

        using List = std::list<Entry>;
        using Iterator = typename List::iterator;

        List m_values;
        std::unordered_map<Key, Iterator> m_index;
        std::unordered_map<Key, Cost> m_costs;
        Cost m_budget;
        Cost m_used {};

    public:
        explicit CostLRUCache(Cost budget)
            : m_budget(budget)
        {
        }

        void insert(const Key& key, const Value& value, Cost cost = Cost(1))
        {
            if (cost > m_budget)
            {
                return;
            }

            erase(key);

            while (m_used + cost > m_budget)
            {
                if (m_values.empty())
                {
                    return;
                }

                auto victim = std::prev(m_values.end());

                m_used -= victim->cost;
                m_costs.erase(victim->key);
                m_index.erase(victim->key);
                m_values.erase(victim);
            }

            m_values.emplace_front(Entry { key, value, cost });
            m_index.emplace(key, m_values.begin());
            m_costs[key] = cost;
            m_used += cost;
        }

        std::optional<Value> get(const Key& key)
        {
            auto it = m_index.find(key);
            if (it == m_index.end())
            {
                return {};
            }

            m_values.splice(m_values.begin(), m_values, it->second);
            return it->second->value;
        }

        void erase(const Key& key)
        {
            auto it = m_index.find(key);
            if (it == m_index.end())
            {
                return;
            }

            m_used -= it->second->cost;
            m_costs.erase(key);
            m_values.erase(it->second);
            m_index.erase(it);
        }

        void clear()
        {
            m_costs.clear();
            m_index.clear();
            m_values.clear();
            m_used = Cost(0);
        }

        Cost budget() const
        {
            return m_budget;
        }

        Cost capacity() const
        {
            return m_budget;
        }

        Cost used() const
        {
            return m_used;
        }

        size_t size() const
        {
            return m_values.size();
        }

        auto begin()
        {
            return m_values.begin();
        }

        auto end()
        {
            return m_values.end();
        }

        auto begin() const
        {
            return m_values.begin();
        }

        auto end() const
        {
            return m_values.end();
        }

        template <typename Fn>
        void for_each(Fn&& fn)
        {
            for (auto& entry : m_values)
            {
                fn(entry.key, entry.value);
            }
        }

        template <typename Fn>
        void for_each(Fn&& fn) const
        {
            for (const auto& entry : m_values)
            {
                fn(entry.key, entry.value);
            }
        }
    };

} // namespace mango
