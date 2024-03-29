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

#include <string>

#include <iostream>
#include <fstream>

#include <IO/ReadBufferFromFile.h>
#include <IO/WriteBufferFromFile.h>
#include <DataTypes/DataTypesNumber.h>
#include <DataTypes/DataTypeString.h>
#include <DataStreams/TabSeparatedRowInputStream.h>
#include <DataStreams/TabSeparatedRowOutputStream.h>
#include <DataStreams/copyData.h>
#include <DataStreams/BlockInputStreamFromRowInputStream.h>
#include <DataStreams/BlockOutputStreamFromRowOutputStream.h>


using namespace DB;

int main(int, char **)
try
{
    Block sample;
    {
        ColumnWithTypeAndName col;
        col.type = std::make_shared<DataTypeUInt64>();
        sample.insert(std::move(col));
    }
    {
        ColumnWithTypeAndName col;
        col.type = std::make_shared<DataTypeString>();
        sample.insert(std::move(col));
    }

    ReadBufferFromFile in_buf("test_in");
    WriteBufferFromFile out_buf("test_out");

    RowInputStreamPtr row_input = std::make_shared<TabSeparatedRowInputStream>(in_buf, sample);
    RowOutputStreamPtr row_output = std::make_shared<TabSeparatedRowOutputStream>(out_buf, sample);

    BlockInputStreamFromRowInputStream block_input(row_input, sample, DEFAULT_INSERT_BLOCK_SIZE, 0, 0);
    BlockOutputStreamFromRowOutputStream block_output(row_output, sample);

    copyData(block_input, block_output);
    return 0;
}
catch (...)
{
    std::cerr << getCurrentExceptionMessage(true) << '\n';
    return 1;
}
