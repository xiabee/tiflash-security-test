// Copyright 2023 PingCAP, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <Common/PODArray.h>
#include <Core/Types.h>

#include <iostream>

#define ASSERT_CHECK(cond, res)                                 \
    do                                                          \
    {                                                           \
        if (!(cond))                                            \
        {                                                       \
            std::cerr << __FILE__ << ":" << __LINE__ << ":"     \
                      << "Assertion " << #cond << " failed.\n"; \
            if ((res))                                          \
            {                                                   \
                (res) = false;                                  \
            }                                                   \
        }                                                       \
    } while (0)

void test1()
{
    using namespace DB;

    static constexpr size_t initial_size = 8;
    static constexpr size_t stack_threshold = 32;
    using Array = PODArray<UInt64, initial_size, AllocatorWithStackMemory<Allocator<false>, stack_threshold>>;

    bool res = true;

    {
        Array arr;
        Array arr2;
        arr2 = std::move(arr);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        Array arr2;

        arr2 = std::move(arr);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);

        arr = std::move(arr2);

        ASSERT_CHECK((arr.size() == 3), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);
        ASSERT_CHECK((arr[2] == 3), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);
        arr.push_back(4);
        arr.push_back(5);

        Array arr2;

        arr2 = std::move(arr);

        ASSERT_CHECK((arr2.size() == 5), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);
        ASSERT_CHECK((arr2[3] == 4), res);
        ASSERT_CHECK((arr2[4] == 5), res);

        arr = std::move(arr2);

        ASSERT_CHECK((arr.size() == 5), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);
        ASSERT_CHECK((arr[2] == 3), res);
        ASSERT_CHECK((arr[3] == 4), res);
        ASSERT_CHECK((arr[4] == 5), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        Array arr2;

        arr2.push_back(4);
        arr2.push_back(5);
        arr2.push_back(6);
        arr2.push_back(7);

        arr2 = std::move(arr);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        Array arr2;

        arr2.push_back(4);
        arr2.push_back(5);
        arr2.push_back(6);
        arr2.push_back(7);
        arr2.push_back(8);

        arr = std::move(arr2);

        ASSERT_CHECK((arr.size() == 5), res);
        ASSERT_CHECK((arr[0] == 4), res);
        ASSERT_CHECK((arr[1] == 5), res);
        ASSERT_CHECK((arr[2] == 6), res);
        ASSERT_CHECK((arr[3] == 7), res);
        ASSERT_CHECK((arr[4] == 8), res);
    }

    if (!res)
        std::cerr << "Some errors were found in test 1\n";
}

void test2()
{
    using namespace DB;

    static constexpr size_t initial_size = 8;
    static constexpr size_t stack_threshold = 32;
    using Array = PODArray<UInt64, initial_size, AllocatorWithStackMemory<Allocator<false>, stack_threshold>>;

    bool res = true;

    {
        Array arr;
        Array arr2;
        arr.swap(arr2);
        arr2.swap(arr);
    }

    {
        Array arr;

        Array arr2;

        arr2.push_back(1);
        arr2.push_back(2);
        arr2.push_back(3);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 3), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);
        ASSERT_CHECK((arr[2] == 3), res);

        ASSERT_CHECK((arr2.size() == 0), res);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 0), res);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);
    }

    {
        Array arr;

        Array arr2;

        arr2.push_back(1);
        arr2.push_back(2);
        arr2.push_back(3);
        arr2.push_back(4);
        arr2.push_back(5);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 5), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);
        ASSERT_CHECK((arr[2] == 3), res);
        ASSERT_CHECK((arr[3] == 4), res);
        ASSERT_CHECK((arr[4] == 5), res);

        ASSERT_CHECK((arr2.size() == 0), res);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 0), res);

        ASSERT_CHECK((arr2.size() == 5), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);
        ASSERT_CHECK((arr2[3] == 4), res);
        ASSERT_CHECK((arr2[4] == 5), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        Array arr2;

        arr2.push_back(4);
        arr2.push_back(5);
        arr2.push_back(6);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 3), res);
        ASSERT_CHECK((arr[0] == 4), res);
        ASSERT_CHECK((arr[1] == 5), res);
        ASSERT_CHECK((arr[2] == 6), res);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 3), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);
        ASSERT_CHECK((arr[2] == 3), res);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 4), res);
        ASSERT_CHECK((arr2[1] == 5), res);
        ASSERT_CHECK((arr2[2] == 6), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);

        Array arr2;

        arr2.push_back(3);
        arr2.push_back(4);
        arr2.push_back(5);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 3), res);
        ASSERT_CHECK((arr[0] == 3), res);
        ASSERT_CHECK((arr[1] == 4), res);
        ASSERT_CHECK((arr[2] == 5), res);

        ASSERT_CHECK((arr2.size() == 2), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 2), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 3), res);
        ASSERT_CHECK((arr2[1] == 4), res);
        ASSERT_CHECK((arr2[2] == 5), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        Array arr2;

        arr2.push_back(4);
        arr2.push_back(5);
        arr2.push_back(6);
        arr2.push_back(7);
        arr2.push_back(8);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 5), res);
        ASSERT_CHECK((arr[0] == 4), res);
        ASSERT_CHECK((arr[1] == 5), res);
        ASSERT_CHECK((arr[2] == 6), res);
        ASSERT_CHECK((arr[3] == 7), res);
        ASSERT_CHECK((arr[4] == 8), res);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 3), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);
        ASSERT_CHECK((arr[2] == 3), res);

        ASSERT_CHECK((arr2.size() == 5), res);
        ASSERT_CHECK((arr2[0] == 4), res);
        ASSERT_CHECK((arr2[1] == 5), res);
        ASSERT_CHECK((arr2[2] == 6), res);
        ASSERT_CHECK((arr2[3] == 7), res);
        ASSERT_CHECK((arr2[4] == 8), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);
        arr.push_back(4);
        arr.push_back(5);

        Array arr2;

        arr2.push_back(6);
        arr2.push_back(7);
        arr2.push_back(8);
        arr2.push_back(9);
        arr2.push_back(10);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 5), res);
        ASSERT_CHECK((arr[0] == 6), res);
        ASSERT_CHECK((arr[1] == 7), res);
        ASSERT_CHECK((arr[2] == 8), res);
        ASSERT_CHECK((arr[3] == 9), res);
        ASSERT_CHECK((arr[4] == 10), res);

        ASSERT_CHECK((arr2.size() == 5), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);
        ASSERT_CHECK((arr2[3] == 4), res);
        ASSERT_CHECK((arr2[4] == 5), res);

        arr.swap(arr2);

        ASSERT_CHECK((arr.size() == 5), res);
        ASSERT_CHECK((arr[0] == 1), res);
        ASSERT_CHECK((arr[1] == 2), res);
        ASSERT_CHECK((arr[2] == 3), res);
        ASSERT_CHECK((arr[3] == 4), res);
        ASSERT_CHECK((arr[4] == 5), res);

        ASSERT_CHECK((arr2.size() == 5), res);
        ASSERT_CHECK((arr2[0] == 6), res);
        ASSERT_CHECK((arr2[1] == 7), res);
        ASSERT_CHECK((arr2[2] == 8), res);
        ASSERT_CHECK((arr2[3] == 9), res);
        ASSERT_CHECK((arr2[4] == 10), res);
    }

    if (!res)
        std::cerr << "Some errors were found in test 2\n";
}

void test3()
{
    using namespace DB;

    static constexpr size_t initial_size = 8;
    static constexpr size_t stack_threshold = 32;
    using Array = PODArray<UInt64, initial_size, AllocatorWithStackMemory<Allocator<false>, stack_threshold>>;

    bool res = true;

    {
        Array arr;
        Array arr2{std::move(arr)};
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);

        Array arr2{std::move(arr)};

        ASSERT_CHECK((arr.size() == 0), res);

        ASSERT_CHECK((arr2.size() == 3), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);
    }

    {
        Array arr;

        arr.push_back(1);
        arr.push_back(2);
        arr.push_back(3);
        arr.push_back(4);
        arr.push_back(5);

        Array arr2{std::move(arr)};

        ASSERT_CHECK((arr.size() == 0), res);

        ASSERT_CHECK((arr2.size() == 5), res);
        ASSERT_CHECK((arr2[0] == 1), res);
        ASSERT_CHECK((arr2[1] == 2), res);
        ASSERT_CHECK((arr2[2] == 3), res);
        ASSERT_CHECK((arr2[3] == 4), res);
        ASSERT_CHECK((arr2[4] == 5), res);
    }

    if (!res)
        std::cerr << "Some errors were found in test 3\n";
}

int main()
{
    std::cout << "test 1\n";
    test1();
    std::cout << "test 2\n";
    test2();
    std::cout << "test 3\n";
    test3();

    return 0;
}
