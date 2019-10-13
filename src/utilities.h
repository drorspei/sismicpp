#ifndef INCLUDE_SISMICPP_UTILITIES
#define INCLUDE_SISMICPP_UTILITIES

#include <algorithm>
#include <map>
#include <vector>

namespace sismicpp {

template <typename It, typename KeyFunc>
auto sorted_groupby(const It& it, KeyFunc&& key_func, bool reverse) {
    using Key = decltype(key_func(*std::begin(it)));
    using T = std::vector<typename It::value_type>;
    std::map<Key, T> groups;
    std::vector<std::pair<Key, T>> ret;

    for (auto& value : it) {
        groups[key_func(value)].push_back(value);
    }

    for (auto&& group_pair : groups) {
        ret.push_back(std::move(group_pair));
    }

    if (reverse) {
        std::reverse(ret.begin(), ret.end());
    }

    return ret;
}

}  // namespace sismicpp

#endif  // INCLUDE