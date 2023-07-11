#include "test_utils/test.h"
#include <seastar/core/sleep.hh>

//TEST(Counter, Foo) {
//    run_with_seastar([] {
//        return seastar::make_ready_future<>();
//    }
//  );
//}

RP_TEST(Counter, Foo2) {
    return seastar::sleep(std::chrono::seconds(1)).then([] {
        int* p = reinterpret_cast<int*>(0x309239);
        std::cout << *p << std::endl;
    });
}
