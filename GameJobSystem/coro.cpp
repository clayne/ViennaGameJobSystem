

#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <functional>
#include <string>
#include <algorithm>
#include <chrono>
#include <glm.hpp>


#include "VEGameJobSystem.h"
#include "VECoro.h"

using namespace std::chrono;


namespace coro {

    using namespace vgjs;

    auto g_global_mem4 = std::pmr::synchronized_pool_resource({ .max_blocks_per_chunk = 20, .largest_required_pool_block = 1 << 20 }, std::pmr::new_delete_resource());

    class CoroClass {
        int number = 1;
    public:
        CoroClass( int nr ) : number(nr) {};

        Coro<int> Number10() {
            std::cout << "Number x 10 = " << number * 10 << "\n";
            co_return number * 10;
        }

    };

    Coro<int> recursive2(std::allocator_arg_t, std::pmr::memory_resource* mr, int i, int N) {
        if (i < N ) {
            co_await recursive2(std::allocator_arg, mr, i + 1, N);
            co_await recursive2(std::allocator_arg, mr, i + 1, N);
        }
        co_return 0;
    }

    Coro<int> recursive(std::allocator_arg_t, std::pmr::memory_resource* mr, int i, int N) {
        std::cout << "Recursive " << i << " of " << N << std::endl;

        if (i < N) {
            co_await recursive(std::allocator_arg, mr, i + 1, N);
            co_await recursive(std::allocator_arg, mr, i + 1, N);
        }
        co_return 0;
    }

    Coro<float> computeF(std::allocator_arg_t, std::pmr::memory_resource* mr, int i) {

        //co_await 0;
        float f = i + 0.5f;
        std::cout << "ComputeF " << f << std::endl;

        co_return 10.0f * i;
    }


    Coro<int> compute(std::allocator_arg_t, std::pmr::memory_resource* mr, int i) {

        co_await 1;

        std::cout << "Compute " << i << std::endl;

        co_return 2 * i;
    }

    Coro<int> do_compute(std::allocator_arg_t, std::pmr::memory_resource* mr) {
        std::cout << "DO Compute " << std::endl;

        auto tk1 = compute(std::allocator_arg, mr, 1);

        co_await tk1;

        co_return tk1.get();
    }

    void FCompute( int i ) {
        std::cout << "FCompute " << i << std::endl;
    }

    void FuncCompute(int i) {
        std::cout << "FuncCompute " << i << std::endl;
    }

    Coro<int> loop(std::allocator_arg_t, std::pmr::memory_resource* mr, int count) {
        int sum = 0;
        std::cout << "Starting loop\n";

        CoroClass cc(99);

        auto tv = std::pmr::vector<Coro<int>>{mr};

        auto tk = std::make_tuple(std::pmr::vector<Coro<int>>{mr}, std::pmr::vector<Coro<float>>{mr});
        
        auto fv = std::pmr::vector<std::function<void(void)>>{ mr };

        std::pmr::vector<Function> jv{ mr };

        for (int i = 0; i < count; ++i) {
            tv.emplace_back( do_compute(std::allocator_arg, &g_global_mem4 ) );

            get<0>(tk).emplace_back(compute(std::allocator_arg, &g_global_mem4, i));
            get<1>(tk).emplace_back(computeF(std::allocator_arg, &g_global_mem4, i));

            fv.emplace_back( F( FCompute(i) ) );

            Function f( F(FuncCompute(i)), -1, 0, 0 );
            jv.push_back( f );

            jv.push_back( Function( F(FuncCompute(i)), -1, 0, 0) );
        }
        
        std::cout << "Before loop " << std::endl;

        auto mf = cc.Number10();
        co_await mf;
        std::cout << "Class member function " << mf.get() << std::endl;


        co_await tv;

        co_await tk;

        co_await recursive(std::allocator_arg, &g_global_mem4, 1, 10)( 1, 0, 0);

        co_await F( FCompute(999) );

        co_await Function( F(FCompute(999)) );

        co_await fv;

        co_await jv;

        std::cout << "Ending loop " << std::endl;

        co_return sum;
    }


    void driver() {
        schedule( loop(std::allocator_arg, &g_global_mem4, 90) );

        //schedule( recursive2(std::allocator_arg, &g_global_mem4, 1, 18) );

        //schedule( compute(std::allocator_arg, &g_global_mem4, 90) );

    }

	void test() {
        std::cout << "Starting coro test()\n";

        schedule( FUNCTION( driver()) );

        std::cout << "Ending coro test()\n";

	}

}
