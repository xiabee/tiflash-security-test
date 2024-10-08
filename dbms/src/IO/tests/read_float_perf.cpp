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

#include <Common/Stopwatch.h>
#include <Common/formatReadable.h>
#include <Core/Types.h>
#include <IO/CompressedReadBuffer.h>
#include <IO/Operators.h>
#include <IO/ReadBufferFromFileDescriptor.h>
#include <IO/WriteBufferFromFileDescriptor.h>
#include <IO/WriteHelpers.h>
#include <IO/readFloatText.h>

#include <fstream>
#include <iostream>
#include <string>


/** How to test:

# Prepare data

$ clickhouse-local --query="SELECT number FROM system.numbers LIMIT 10000000" > numbers1.tsv
$ clickhouse-local --query="SELECT number % 10 FROM system.numbers LIMIT 10000000" > numbers2.tsv
$ clickhouse-local --query="SELECT number / 1000 FROM system.numbers LIMIT 10000000" > numbers3.tsv
$ clickhouse-local --query="SELECT rand64() / 1000 FROM system.numbers LIMIT 10000000" > numbers4.tsv
$ clickhouse-local --query="SELECT rand64() / 0xFFFFFFFF FROM system.numbers LIMIT 10000000" > numbers5.tsv
$ clickhouse-local --query="SELECT log(rand64()) FROM system.numbers LIMIT 10000000" > numbers6.tsv
$ clickhouse-local --query="SELECT 1 / rand64() FROM system.numbers LIMIT 10000000" > numbers7.tsv
$ clickhouse-local --query="SELECT exp(rand() / 0xFFFFFF) FROM system.numbers LIMIT 10000000" > numbers8.tsv
$ clickhouse-local --query="SELECT number FROM system.numbers LIMIT 10000000" > numbers1.tsv
$ clickhouse-local --query="SELECT toString(rand64(1)) || toString(rand64(2)) || toString(rand64(3)) || toString(rand64(4)) FROM system.numbers LIMIT 10000000" > numbers9.tsv
$ clickhouse-local --query="SELECT toString(rand64(1)) || toString(rand64(2)) || '.' || toString(rand64(3)) || toString(rand64(4)) FROM system.numbers LIMIT 10000000" > numbers10.tsv

# Run test

$ for i in {1..10}; do echo $i; time ./read_float_perf < numbers$i.tsv; done

  */


using namespace DB;


template <typename T, void F(T &, ReadBuffer &)>
void NO_INLINE loop(ReadBuffer & in, WriteBuffer & out)
{
    T sum = 0;
    T x = 0;

    Stopwatch watch;

    while (!in.eof())
    {
        F(x, in);
        in.ignore();
        sum += x;
    }

    watch.stop();
    out << "Read in " << watch.elapsedSeconds() << " sec, "
        << formatReadableSizeWithBinarySuffix(in.count() / watch.elapsedSeconds()) << "/sec, result = " << sum << "\n";
}


int main(int argc, char ** argv)
try
{
    int method = 0;
    if (argc >= 2)
        method = parse<int>(argv[1]);

    using T = Float64;

    ReadBufferFromFileDescriptor in(STDIN_FILENO);
    WriteBufferFromFileDescriptor out(STDOUT_FILENO);

    if (method == 1)
        loop<T, readFloatTextPrecise>(in, out);
    if (method == 2)
        loop<T, readFloatTextFast>(in, out);
    if (method == 3)
        loop<T, readFloatTextSimple>(in, out);

    return 0;
}
catch (const Exception & e)
{
    std::cerr << e.what() << ", " << e.displayText() << std::endl;
    return 1;
}
