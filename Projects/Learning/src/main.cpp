#include <util/util.hpp>
#include <util/string_generator.hpp>
#include <util/logger.hpp>


//djb2
uint32_t hash1(const std::string& str) {
    uint32_t hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}
//fnv1a
uint32_t hash2(const std::string& str) {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}
//murmur3
uint32_t hash3(const std::string& str) {
    uint32_t hash = 0;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;
    
    for (size_t i = 0; i + 3 < str.length(); i += 4) {
        uint32_t k = *(uint32_t*)&str[i];
        k *= c1;
        k = (k << 15) | (k >> 17);
        k *= c2;
        
        hash ^= k;
        hash = (hash << 13) | (hash >> 19);
        hash = hash * 5 + 0xe6546b64;
    }
        
    hash ^= str.length();
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    
    return hash;
}

struct hashes {
    hashes(uint32_t a,uint32_t b,uint32_t c) {
        h[0] = a; h[1] = b; h[2] = c;
    }
    uint32_t h[3];
};

class bloom_filter {
private:
    uint64_t* bits;
    size_t num_bits;

    inline void set_bit(size_t pos) {
        bits[pos / 64] |= (1ULL << (pos % 64));
    }
    
    inline bool get_bit(size_t pos) {
        return bits[pos / 64] & (1ULL << (pos % 64));
    }

public:
    bloom_filter(size_t num_bits_) : num_bits(num_bits_) {
        size_t num_words = (num_bits + 63) / 64;
        bits = new uint64_t[num_words](); 
    }
    
    ~bloom_filter() { delete[] bits; }

    void rebuild(size_t num_bits_) {
        if(bits) delete[] bits;
        size_t num_words = (num_bits + 63) / 64;
        bits = new uint64_t[num_words](); 
    }
    
    hashes hashRun(const std::string& key) {
        return hashes(
            hash1(key) % num_bits,
            hash2(key) % num_bits,
            hash3(key) % num_bits
        );
    }
    
    void add(const std::string& key) {
        hashes h = hashRun(key);
        for(int i=0;i<3;i++) {
            set_bit(h.h[i]);
        }
    }
    
    bool query(const std::string& key) {
        hashes h = hashRun(key);
        bool result = true;
        for(int i=0;i<3;i++) {
            if(!get_bit(h.h[i])) {
                result=false;
                break;
            }
        }
        return result;
    }
};

class mphs {
public:
    int num_buckets;
    int num_keys;
    list<uint32_t> seeds;
    list<list<uint64_t>> fingerprints;

    mphs() {}

    mphs(list<std::string> keys) {        
        build(keys);
    }

    void build(list<std::string> keys) {
        //Try to ensure there aren't duplicates, I won't add duplicate culling here because preformance, but I could
        num_keys = keys.length();
        num_buckets = std::max(1,num_keys/10);
        list<list<std::string>> buckets;
        seeds = list<uint32_t>(num_buckets,0);
        fingerprints.clear();
        for(int i=0;i<num_keys;i++) {
            fingerprints << list<uint64_t>{};
        }
        for(int i=0;i<num_buckets;i++) {
            list<std::string> lst;
            buckets << lst;
        }
        for(auto key : keys) {
            int idx = hash1(key)%num_buckets;
            buckets[idx] << key;
        }
        for(int b=0;b<num_buckets;b++) {
            for(int i=0;i<1000;i++) {
                bool valid = true;
                list<uint32_t> used_indexes;
                for(auto key : buckets[b]) {
                    uint32_t index = seedHash(key,i)%num_keys;
                    if(used_indexes.has(index)) {
                        valid = false;
                        break;
                    } else {
                        used_indexes << index;
                    }
                }
                if(valid) {
                    seeds[b] = i;
                    break;
                }
            }
        }

        for(auto key : keys) {
            uint32_t seed = seeds[hash1(key) % num_buckets];
            uint32_t index = seedHash(key, seed) % num_keys;
            uint64_t fp = fingerprint(key);

            fingerprints[index] << fp;
        }
    }

    uint32_t hash_secondary(const std::string& key) {
        return hash1(key) ^ hash3(key);
    }

    uint32_t seedHash(const std::string& key, int seed) {
        return hash2(key) + seed * hash3(key);
    }

    uint64_t fingerprint(const std::string& key) {
        return ((uint64_t)hash1(key) << 32) | hash2(key);
    }

    bool contains(const std::string& key) {
        uint32_t first = hash1(key);
        uint32_t second = hash2(key);
        uint32_t seed = seeds[first % num_buckets];
        uint32_t index = (second + seed * hash3(key)) % num_keys;
        uint64_t fp = ((uint64_t)first << 32) | second;
        
        return fingerprints[index].has(fp);
    }
};


//More hash-table-y idea
class mpht {
public:
    int num_buckets;
    int num_keys;
    list<uint32_t> seeds;
    list<uint64_t> fingerprints;
    list<uint32_t> values;

    mpht() {}

    mpht(list<entry<std::string,uint32_t>> entries) {        
        build(entries);
    }

    void build(list<entry<std::string,uint32_t>> entries) {
        //Try to ensure there aren't duplicates, I won't add duplicate culling here because preformance, but I could
        num_keys = entries.length();
        num_buckets = std::max(1,num_keys/10);
        list<list<std::string>> buckets;
        seeds = list<uint32_t>(num_buckets);
        values = list<uint32_t>(num_keys);
        fingerprints = list<uint64_t>(num_keys);
        for(int i=0;i<num_buckets;i++) {
            list<std::string> lst;
            buckets << lst;
        }
        for(auto e : entries) {
            int idx = hash1(e.key)%num_buckets;
            buckets[idx] << e.key;
        }
        for(int b=0;b<num_buckets;b++) {
            for(int i=0;i<1000;i++) {
                bool valid = true;
                list<uint32_t> used_indexes;
                for(auto key : buckets[b]) {
                    uint32_t index = seedHash(key,i)%num_keys;
                    if(used_indexes.has(index)) {
                        valid = false;
                        break;
                    } else {
                        used_indexes << index;
                    }
                }
                if(valid) {
                    seeds[b] = i;
                    break;
                }
            }
        } 
        for(auto e : entries) {
            uint32_t seed = seeds[hash1(e.key) % num_buckets];
            uint32_t index = seedHash(e.key, seed) % num_keys;
            fingerprints[index] = fingerprint(e.key);
            values[index] = e.value;
        }
    }

    uint32_t seedHash(const std::string& key, int seed) {
        return hash2(key) + seed * hash3(key);
    }

    uint64_t fingerprint(const std::string& key) {
        return ((uint64_t)hash1(key) << 32) | hash2(key);
    }

    uint32_t getIndex(const std::string& key) {
        uint32_t first = hash1(key);
        uint32_t second = hash2(key);
        uint32_t seed = seeds[first % num_buckets];
        uint32_t index = (second + seed * hash3(key)) % num_keys;
        uint64_t fp = ((uint64_t)first << 32) | second;
        
        if(fingerprints[index] == fp) {
            //print("INDEX: ",index," KEY: ",key," VALUE: ",values[index]);
            return values[index]; 
        }
        //print("EXRETURN, INDEX: ",index," KEY: ",key);
        return -1;
    }
};

int main() {
    int bloom_size = 200000;
    bloom_filter bloom(bloom_size); 
    mphs hs;

    int a_len = 20000;

    //int duplicates = 0;
    list<std::string> added;
    for(int i=0;i<a_len;i++) {
        std::string name = sgen::randsgen(sgen::RANDOM);
        //if(added.has(name)) duplicates++;
        added << name;
    }
    //print("DUPLICATE NAMES: ",duplicates);

    list<std::string> n_added;
    for(int i = 0; i < a_len; i++) {
        n_added << "notinset"+std::to_string(i);
    }

    log::rig r;
    r.add_process("bloom_clean",[&](int i){
        if(i==0)
            bloom.rebuild(bloom_size);
    });
    r.add_process("bloom_add",[&](int i){
        bloom.add(added[i]);
    });
    int hits = 0;
    r.add_process("bloom_query",[&](int i){
        if(bloom.query(added[i])) hits++;
    });

    r.add_process("mphs_build",[&](int i){
        if(i==0)
            hs.build(added);
    },1);
    r.add_process("mphs_query",[&](int i){
        hs.contains(added[i]);
    },1);

    map<std::string,uint32_t> nm;
    r.add_process("map_add",[&](int i){
            nm.put(added[i],i);
    },2);
    r.add_process("map_get",[&](int i){
        volatile uint32_t a = nm.get(added[i]);
    },2);

    mpht ht;
    list<entry<std::string,uint32_t>> entries;
    for(int i = 0;i<added.length();i++) {
        entries << entry<std::string,uint32_t>{added[i],i};
    }

    // ht.build(entries);
    // print(ht.getIndex(added[50]));

    r.add_process("mpht_build",[&](int i){
        if(i==0)
            ht.build(entries);
    },3);
    int fails = 0;
    r.add_process("mpht_retrive",[&](int i){
       volatile uint32_t a = ht.getIndex(added[i]);
       if(a == -1) fails++;
    },3);

    r.run(100,false,a_len);

    int bloom_fn = 0;
    int mpht_fn = 0;
    for(auto key : added) {
        if(!bloom.query(key)) bloom_fn++;
        if(!hs.contains(key)) mpht_fn++;
    }
    
    int bloom_fp = 0;
    int mpht_fp = 0;
    for(auto key : n_added) {
        if(bloom.query(key)) bloom_fp++;
        if(hs.contains(key)) mpht_fp++;
    }

    print("MPHT Failed queries: ",fails," / ",a_len,
        " (", (fails / (double)a_len) * 100, "%)");
    
    print("=== FALSE NEGATIVES ===");
    print("Bloom: ", bloom_fn, " / ", a_len);
    print("MPHS:  ", mpht_fn, " / ", a_len);
    
    print("\n=== FALSE POSITIVES ===");
    print("Bloom: ", bloom_fp, " / ", a_len, 
          " (", (bloom_fp / (double)a_len) * 100, "%)");
    print("MPHS:  ", mpht_fp, " / ", a_len,
          " (", (mpht_fp / (double)a_len) * 100, "%)");
    return 0;
}



