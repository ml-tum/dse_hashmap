#include "my_hashmap.hpp"
#include <iostream>
#include <random>
#include <thread>
#include <set>
#include <cassert>

const int ITERATIONS = 1e6;
const int MAXVALUE = 16;

void demo() {
    // initialize random number generator
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

    MyHashtable<int> table(4);
    std::set<int> stdSet;

    // randomly insert and remove elements
    for(int i=0; i<ITERATIONS; i++) {

        // random operation and the value
        int add_or_remove = uniform_int_distribution<int>(0, 1)(rng);
        int value = uniform_int_distribution<int>(0, MAXVALUE)(rng);

        if(add_or_remove==1) {
            table.remove(value);
            stdSet.erase(value);
        } else {
            table.insert(value);
            stdSet.insert(value);
        }

        for(int j=0; j<=MAXVALUE; j++) {
            assert(stdSet.count(j) == table.contains(j));
        }
    }
}


int main() {

    std::thread t1(demo);
//    std::thread t2(demo);

    cout<<"Start"<<endl;
    t1.join();
//    t2.join();
    cout<<"Finish"<<endl;

    return 0;
}
