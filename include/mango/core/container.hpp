/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include "../external/parallel_hashmap/phmap.h"

namespace mango
{

    template <class T, 
        class Hash  = phmap::priv::hash_default_hash<T>,
        class Eq    = phmap::priv::hash_default_eq<T>,
        class Alloc = phmap::priv::Allocator<T>>
    using unordered_set = phmap::flat_hash_set<T, Hash, Eq, Alloc>;

    template <class K, class V,
        class Hash  = phmap::priv::hash_default_hash<K>,
        class Eq    = phmap::priv::hash_default_eq<K>,
        class Alloc = phmap::priv::Allocator<phmap::priv::Pair<const K, V>>>
    using unordered_map = phmap::flat_hash_map<K, V, Hash, Eq, Alloc>;

    template <class T, 
        class Hash  = phmap::priv::hash_default_hash<T>,
        class Eq    = phmap::priv::hash_default_eq<T>,
        class Alloc = phmap::priv::Allocator<T>>
    using set = phmap::node_hash_set<T, Hash, Eq, Alloc>;

    template <class Key, class Value,
        class Hash  = phmap::priv::hash_default_hash<Key>,
        class Eq    = phmap::priv::hash_default_eq<Key>,
        class Alloc = phmap::priv::Allocator<phmap::priv::Pair<const Key, Value>>>
    using map = phmap::node_hash_map<Key, Value, Hash, Eq, Alloc>;

} // namespace mango
