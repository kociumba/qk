/// this example shows how to use the qk::traits module
///
/// this package provides a "simple" trait/impl imitation using concepts and default implementation
/// structs
///
/// all of these are designed to have minimal runtime impact, for that reason any virtual functions
/// are avoided at all costs since they introduce a huge runtime overhead
///
/// the traits provided in the base qk::traits module are all simple and only require the standard
/// lib. traits provided if the extras are enabled, require reflection and use a variety of tricks
/// to deliver things like value hashing

#include <qk/qk_traits.h>
#include <print>

// for a full list of traits just check out the headers implementing them
using namespace qk::traits;

struct Data : RefCounted_base {
    int data;
};

// this trait is from the trait extras extension, and it uses reflection to hash type deeply based
// on their values, if a simple fast hash based on the memory address is needed, use the simple
// Hashable trait
struct ExtraData : ValueHashable_base {
    int data;
};

// a ref counted type starts at 1 reference, and returns false on a decrement attempt that will
// result in 0 references
template <RefCounted Type>
void ref_counting(Type& var) {
    var.increment();
    std::print("references: {}\n", var._ref);

    var.decrement();
    if (!var.decrement()) std::printf("tried to decrement below 0 references\n");
}

int main() {
    Data d;

    // any data type fulfilling the trait(concept) can be used when a function is templated using
    // that trait
    ref_counting(d);

    ExtraData e = {.data = 69};

    std::print("value hash of e is: {}", e.hash());

    return 0;
}