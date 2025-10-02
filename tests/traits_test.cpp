#include <qk/qk_traits.h>
#include <catch2/catch_test_macros.hpp>

using namespace qk::traits;

TEST_CASE("Concepts satisfaction", "[traits]") {
    SECTION("Standard concepts") {
        REQUIRE(Copyable<int>);
        REQUIRE(Movable<int>);
    }

    SECTION("Custom concepts on derived types") {
        struct MyRefCounted : public RefCounted_base {
            int data = 69;
        };
        REQUIRE(RefCounted<MyRefCounted>);

        struct MyLockable : public Lockable_base {
            int data = 420;
        };
        REQUIRE(Lockable<MyLockable>);

        struct MyHashable : public Hashable_base {
            int data = 2137;
        };
        REQUIRE(Hashable<MyHashable>);
    }
}

TEST_CASE("RefCounted functionality on derived type", "[traits]") {
    struct MyRefCounted : public RefCounted_base {
        int data = 69;
    };

    MyRefCounted mrc;

    SECTION("initial reference count") { REQUIRE(mrc._ref == 1); }

    SECTION("increment") {
        mrc.increment();
        REQUIRE(mrc._ref == 2);
    }

    SECTION("decrement from 1") {
        bool alive = mrc.decrement();
        REQUIRE(alive == false);
        REQUIRE(mrc._ref == 0);
    }

    SECTION("decrement when already 0") {
        mrc.decrement();
        bool alive = mrc.decrement();
        REQUIRE(alive == false);
        REQUIRE(mrc._ref == 0);
    }

    SECTION("increment then decrement") {
        mrc.increment();
        REQUIRE(mrc._ref == 2);
        bool alive = mrc.decrement();
        REQUIRE(alive == true);
        REQUIRE(mrc._ref == 1);
    }
}

TEST_CASE("Lockable functionality on derived type", "[traits]") {
    struct MyLockable : public Lockable_base {
        int data = 420;
    };

    MyLockable ml;

    SECTION("lock and unlock") {
        ml.lock();
        ml.unlock();
    }

    SECTION("access mutex") { std::lock_guard l(ml.mu()); }
}

TEST_CASE("Hashable functionality on derived type", "[traits]") {
    struct MyHashable : public Hashable_base {
        int data = 2137;
    };

    MyHashable mh1;
    MyHashable mh2;

    size_t hash1 = mh1.hash();
    size_t hash2 = mh2.hash();

    SECTION("consistent hash for same object") { REQUIRE(mh1.hash() == hash1); }

    SECTION("different hashes for different objects") { REQUIRE(hash1 != hash2); }
}

#ifdef QK_TRAITS_EXTRA

TEST_CASE("ValueHashable on derived types", "[traits extra]") {
    enum E { A, B };

    struct foo : ValueHashable_base {
        int a;
        E b;
        std::vector<int> c;
        std::unordered_map<int, int> d = {{1, 1}};
    };

    foo f1 = {.a = 69, .b = A, .c = {1, 2, 3, 4}};
    foo f2 = {.a = 420, .b = B, .c = {1, 9, 3, 4}};
    foo f3 = {.a = 69, .b = A, .c = {1, 2, 3, 4}};

    SECTION("different value hashes") { REQUIRE(f1.hash() != f2.hash()); }

    SECTION("same hashes for the same value") { REQUIRE(f1.hash() == f3.hash()); }
}

#endif