
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

const size_t INIT_SIZE = 1;
const size_t LOAD_FACTOR = 8;
const size_t RESIZE_COEFFICIENT = 2;


template<typename KeyType, typename ValueType>
struct map_entry {
    std::pair<KeyType, ValueType> pair;
    size_t cost = 0;

    const KeyType &key() const {
        return pair.first;
    }

    ValueType &value() {
        return pair.second;
    }

    map_entry() = default;

    map_entry(const KeyType key, ValueType value, int cost = 0) : pair(key, value), cost(cost) {}


    map_entry(std::pair<const KeyType, ValueType> pair, int cost = 0) : pair(pair), cost(cost) {}

};


template<typename KeyType, typename ValueType, class Hash = std::hash<KeyType>>
class HashMap {
private:
    using o_map_entry = std::optional<map_entry<KeyType, ValueType>>;

    Hash hash;
    std::unique_ptr<o_map_entry[]> data_;

    size_t size_ = 0;
    size_t capacity_ = 1;

    size_t find_place(const KeyType &key) const {
        for (size_t h = hash(key) % capacity_;; ++h) {
            if (h == capacity_ || !data_[h] || (data_[h] && data_[h]->key() == key)) {
                return h;
            }
        }
    }

    void resize() {
        std::unique_ptr<o_map_entry[]> old_data = nullptr;
        size_t old_capacity = capacity_;
        old_data.swap(data_);
        capacity_ *= RESIZE_COEFFICIENT;
        data_ = std::unique_ptr<o_map_entry[]>(new o_map_entry[capacity_]());

        size_ = 0;
        for (size_t i = 0; i < old_capacity; ++i) {
            if (old_data[i]) {
                insert_entry(old_data[i]);
            }
        }
    }

    void shift_back(size_t ind) {
        for (size_t i = ind; i < capacity_ && data_[i] && data_[i]->cost > 0; ++i) {
            --data_[i]->cost;
            data_[i - 1] = data_[i];
            data_[i].reset();

        }
    }

    void insert_entry(std::optional<map_entry<KeyType, ValueType>> &entry) {
        entry->cost = 0;
        for (size_t h = hash(entry->key()) % capacity_;; ++h) {
            if (h == capacity_) {
                resize();
                entry->cost = 0;
                h = hash(entry->key()) % capacity_;
            }
            if (!data_[h]) {
                ++size_;
                data_[h].swap(entry);
                return;
            }
            if (data_[h]->cost < entry->cost) {
                data_[h].swap(entry);
            }
            ++entry->cost;
        }
    }


public:
    class iterator {
    private:
        HashMap<KeyType, ValueType, Hash> *container = nullptr;
        size_t ind = 0;


    public:
        iterator() = default;

        explicit iterator(HashMap *hm) {
            container = hm;
            ind = 0;
            while (ind < container->capacity_ && !container->data_[ind]) {
                ++ind;
            }
        }

        iterator(HashMap *hm, size_t ind) {
            container = hm;
            this->ind = ind;
            while (this->ind < container->capacity_ && !container->data_[this->ind]) {
                ++this->ind;
            }
        }

        iterator &operator++() {
            ++ind;
            while (ind < container->capacity_ && !container->data_[ind]) {
                ++ind;
            }
            return *this;
        }

        iterator operator++(int) {
            auto old = *this;
            ++ind;
            while (ind < container->capacity_ && !container->data_[ind]) {
                ++ind;
            }
            return old;
        }


        std::pair<const KeyType, ValueType> &operator*() {
            return *operator->();
        }

        std::pair<const KeyType, ValueType> *operator->() {
            return (std::pair<const KeyType, ValueType> *) &container->data_[ind]->pair;
        }


        bool operator==(const iterator &rhs) const {
            return container == rhs.container &&
                   ind == rhs.ind;
        }

        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    class const_iterator {
    private:
        const HashMap<KeyType, ValueType, Hash> *container = nullptr;
        size_t ind{};


    public:
        const_iterator() = default;

        explicit const_iterator(const HashMap *hm) {
            container = hm;
            ind = 0;
            while (ind < container->capacity_ && !container->data_[ind]) {
                ++ind;
            }
        }

        const_iterator(const HashMap *hm, size_t ind) {
            container = hm;
            this->ind = ind;
            while (this->ind < container->capacity_ && !container->data_[this->ind]) {
                ++this->ind;
            }
        }


        const_iterator &operator++() {
            ++ind;
            while (ind < container->capacity_ && !container->data_[ind]) {
                ++ind;
            }
            return *this;
        }

        const_iterator operator++(int) {
            auto old = *this;
            ++ind;
            while (ind < container->capacity_ && !container->data_[ind]) {
                ++ind;
            }
            return old;
        }


        const std::pair<const KeyType, ValueType> &operator*() const {
            return *operator->();
        }

        const std::pair<const KeyType, ValueType> *operator->() const {
            return (std::pair<const KeyType, ValueType> *) &container->data_[ind]->pair;
        }

        bool operator==(const const_iterator &rhs) const {
            return container == rhs.container &&
                   ind == rhs.ind;
        }

        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };


    explicit HashMap(Hash hash = Hash()) : hash(hash) {
        data_ = std::unique_ptr<o_map_entry[]>(new o_map_entry[INIT_SIZE]());
        capacity_ = INIT_SIZE;
        size_ = 0;

    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, Hash hash = Hash()) : HashMap(hash) {
        for (const auto [k, v]: list) {
            insert({k, v});
        }
    }

    template<class Iterator>
    HashMap(Iterator begin, Iterator end, Hash hash = Hash()) : HashMap(hash) {

        for (auto i = begin; i != end; ++i) {
            insert(*i);
        }
    }


    bool empty() const {
        return size_ == 0;
    }

    size_t size() const {
        return size_;
    }

    Hash hash_function() const {
        return hash;
    }

    void insert(const std::pair<KeyType, ValueType> &pair) {
        if (find(pair.first) != end()) {
            return;
        }
        auto entry = std::make_optional<map_entry<KeyType, ValueType>>(pair);
        insert_entry(entry);
    }


    void erase(const KeyType &key) {
        size_t ind = find_place(key);
        if (ind != capacity_ && data_[ind] && data_[ind]->key() == key) {
            --size_;
            data_[ind].reset();
            shift_back(ind + 1);
        }
    }


    iterator find(const KeyType &key) {
        size_t ind = find_place(key);
        return iterator(this,
                        (ind == capacity_ || !data_[ind] || !(data_[ind]->key() == key)) ? capacity_ : ind);
    }

    const_iterator find(const KeyType &key) const {
        size_t ind = find_place(key);
        return const_iterator(this,
                              (ind == capacity_ || !data_[ind] || !(data_[ind]->key() == key)) ? capacity_
                                                                                               : ind);
    }

    ValueType &operator[](const KeyType &key) {
        size_t ind = find_place(key);
        if (capacity_ == ind || !data_[ind] || !(data_[ind]->key() == key)) {
            this->insert({key, ValueType()});
        }
        ind = find_place(key);
        return data_[ind]->value();
    }

    const ValueType &at(const KeyType &key) const {
        size_t ind = find_place(key);
        if (capacity_ == ind || !data_[ind] || !(data_[ind]->key() == key)) {
            throw std::out_of_range("MAKAKA");
        }
        ind = find_place(key);
        return data_[ind]->value();
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    const_iterator end() const {
        return const_iterator(this, capacity_);
    }

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, capacity_);
    }

    HashMap(const HashMap<KeyType, ValueType> &other) {
        hash = other.hash;
        size_ = other.size();
        capacity_ = other.capacity_;
        data_ = std::unique_ptr<o_map_entry[]>(new o_map_entry[other.capacity_]());
        for (size_t i = 0; i < other.capacity_; ++i) {
            if (other.data_[i]) {
                data_[i] = other.data_[i];
            }
        }
    }

    HashMap &operator=(const HashMap<KeyType, ValueType> &other) {
        if (data_ == other.data_) {
            return *this;
        }
        hash = other.hash;
        size_ = other.size();
        capacity_ = other.capacity_;
        data_ = std::unique_ptr<o_map_entry[]>(new o_map_entry[other.capacity_]());
        for (size_t i = 0; i < other.capacity_; ++i) {
            if (other.data_[i]) {
                data_[i] = other.data_[i];
            }
        }
        return *this;
    }

    ~HashMap() {
        data_.reset();
    }

    void clear() {
        size_ = 0;
        for (size_t i = 0; i < capacity_; ++i) {
            data_[i].reset();
        }
    }
};


