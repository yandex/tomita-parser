#pragma once

/** MapFindPtr usage:

if (T* value = MapFindPtr(myMap, someKey) {
    Cout << *value;
}

*/

template <class Map, class K>
typename Map::mapped_type* MapFindPtr(Map& map, const K& key) {
    typename Map::iterator i = map.find(key);

    return (i == map.end() ? 0 : &i->second);
}

template <class Map, class K>
const typename Map::mapped_type* MapFindPtr(const Map& map, const K& key) {
    typename Map::const_iterator i = map.find(key);

    return (i == map.end() ? 0 : &i->second);
}

/** helper for yhash/ymap */
template <class V, class Derived>
struct TMapOps {
    template <class K>
    inline V* FindPtr(const K& key) {
        return MapFindPtr(static_cast<Derived&>(*this), key);
    }

    template <class K>
    inline const V* FindPtr(const K& key) const {
        return MapFindPtr(static_cast<const Derived&>(*this), key);
    }
};
