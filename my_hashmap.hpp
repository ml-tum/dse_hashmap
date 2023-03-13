#include <cstdlib>
#include <vector>
#include <mutex>
#include <shared_mutex>

using namespace std;

/**
 * The entries of a bucket
 * @tparam T type of key
 */
template<typename T>
struct Entry {
    /**
     * The value which this Entry wraps
     */
    T value;
    /**
     * The next value in bucket (singly linked list)
     */
    Entry *next;
};

template<typename T>
class MyHashtable {

private:
    /**
     * Number of buckets
     */
    int size;
    /**
     * The buckets
     */
    vector<Entry<T>*> table;
    /**
     * Each bucket has its own mutex, at the same position in the vector.
     * Read only operations uses a shared access,
     * R/W operations uses exclusive access.
     */
    vector<shared_mutex> mutexes;

    /**
     * Check if the Entry contains the sought value
     */
    bool is_equal(Entry<T> *entry, T &value) {
        return entry->value == value;
    }

    /**
     * Compute the bucket index for a given value
     */
    int index_of(T &value) {
        int hashcode = hash<T>()(value);
        return (hashcode % size + size) % size;
    }

    /**
     * Check if the given value is contained in the given bucket.
     * This method does not lock the bucket!
     */
    bool no_lock_contains(T &value, int bucket_index) {
        //iterate through the bucket
        for(Entry<T> *entry = table[bucket_index]; entry!=nullptr; entry = entry->next) {
            if(is_equal(entry, value)) {
                return true;
            }
        }
        return false;
    }

public:
    /**
     * Init a new hash table
     * @param buckets the fixed number of buckets in this hash table
     */
    MyHashtable(size_t buckets) : size(buckets), table(buckets), mutexes(buckets) {
    }

    /**
     * Insert a value, if it is not yet contained.
     */
    void insert(T value) {
        int index = index_of(value);

        mutexes[index].lock();
        if(!no_lock_contains(value, index)) {
            // Create a new entry and save it as the first entry in the linked list
            Entry<T> *entry = new Entry<T>();
            entry->value = value;
            entry->next = table[index];
            table[index] = entry;
        }
        mutexes[index].unlock();
    }

    /**
     * Remove an entry from the hash table (if it exists).
     */
    void remove(T value) {
        int index = index_of(value);

        mutexes[index].lock();
        Entry<T> *previous=nullptr; // keeps track of the predecessor
        for(Entry<T> *entry = table[index]; entry!=nullptr; entry = entry->next) {
            if(is_equal(entry, value)) {
                if(previous==nullptr) {
                    // this deletes the head of the linked list
                    table[index] = entry->next;
                } else {
                    // removes an element from the middle of the linked list
                    previous->next = entry->next;
                }
                delete(entry);
                break;
            }
            previous = entry;
        }
        mutexes[index].unlock();
    }

    /**
     * Check whether the table contains the given value.
     */
    bool contains(T value) {
        int index = index_of(value);

        bool result;
        mutexes[index].lock_shared();
        result = no_lock_contains(value, index);
        mutexes[index].unlock_shared();

        return result;
    }

};