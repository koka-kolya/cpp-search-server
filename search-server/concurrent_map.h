#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {

    struct InnerMutexMap {
        std::mutex bucket_mutex_;
        std::map<Key, Value> bucket_map_;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access (const Key& key, InnerMutexMap& inner_bucket)
            : guard(inner_bucket.bucket_mutex_),
            ref_to_value(inner_bucket.bucket_map_[key])
        {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : inner_maps_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        auto key_plus = static_cast<uint64_t>(key);
        auto& to_access = inner_maps_[key_plus % inner_maps_.size()];
        return { key, to_access };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> output;
        for (auto& [bucket_mutex, bucket_map] : inner_maps_) {
            std::lock_guard guard(bucket_mutex);
            output.insert(bucket_map.begin(), bucket_map.end());
        }
        return output;
    }

    void Erase (const Key& key) {
        auto key_plus = static_cast<uint64_t>(key);
        for (auto& [bucket_mutex, bucket_map] : inner_maps_) {
            std::lock_guard guard(bucket_mutex);
            bucket_map.erase(key_plus);
        }
    }

private:
    std::vector<InnerMutexMap> inner_maps_;
};
