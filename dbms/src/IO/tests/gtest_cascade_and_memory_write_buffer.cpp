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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <gtest/gtest.h>
#pragma GCC diagnostic pop

#include <Common/typeid_cast.h>
#include <IO/CascadeWriteBuffer.h>
#include <IO/ConcatReadBuffer.h>
#include <IO/MemoryReadWriteBuffer.h>
#include <IO/ReadBufferFromString.h>
#include <IO/WriteBufferFromString.h>
#include <IO/WriteBufferFromTemporaryFile.h>
#include <IO/copyData.h>
#include <Poco/File.h>

#include <stdexcept>

using namespace DB;


static std::string makeTestArray(size_t size)
{
    std::string res(size, '\0');
    for (size_t i = 0; i < res.size(); ++i)
        res[i] = i % 256;
    return res;
}

static void testCascadeBufferRedability(
    std::string data,
    CascadeWriteBuffer::WriteBufferPtrs && arg1,
    CascadeWriteBuffer::WriteBufferConstructors && arg2)
{
    CascadeWriteBuffer cascade{std::move(arg1), std::move(arg2)};

    cascade.write(&data[0], data.size());
    EXPECT_EQ(cascade.count(), data.size());

    std::vector<WriteBufferPtr> write_buffers;
    std::vector<ReadBufferPtr> read_buffers;
    std::vector<ReadBuffer *> read_buffers_raw;
    cascade.getResultBuffers(write_buffers);

    for (WriteBufferPtr & wbuf : write_buffers)
    {
        if (!wbuf)
            continue;

        auto wbuf_readable = dynamic_cast<IReadableWriteBuffer *>(wbuf.get());
        ASSERT_FALSE(!wbuf_readable);

        auto rbuf = wbuf_readable->tryGetReadBuffer();
        ASSERT_FALSE(!rbuf);

        read_buffers.emplace_back(rbuf);
        read_buffers_raw.emplace_back(rbuf.get());
    }

    ConcatReadBuffer concat(read_buffers_raw);
    std::string decoded_data;
    {
        WriteBufferFromString decoded_data_writer(decoded_data);
        copyData(concat, decoded_data_writer);
    }

    ASSERT_EQ(data, decoded_data);
}


TEST(CascadeWriteBuffer, RereadWithTwoMemoryBuffers)
try
{
    size_t max_s = 32;
    for (size_t s = 0; s < max_s; ++s)
    {
        testCascadeBufferRedability(
            makeTestArray(s),
            {std::make_shared<MemoryWriteBuffer>(s / 2, 1, 2.0),
             std::make_shared<MemoryWriteBuffer>(s - s / 2, 1, 2.0)},
            {});

        testCascadeBufferRedability(
            makeTestArray(s),
            {
                std::make_shared<MemoryWriteBuffer>(s, 2, 1.5),
            },
            {});

        testCascadeBufferRedability(
            makeTestArray(s),
            {
                std::make_shared<MemoryWriteBuffer>(0, 1, 1.0),
            },
            {});

        testCascadeBufferRedability(
            makeTestArray(s),
            {
                std::make_shared<MemoryWriteBuffer>(std::max(1ul, s / 2), std::max(2ul, s / 4), 0.5),
                std::make_shared<MemoryWriteBuffer>(0, 4, 1.0),
            },
            {});

        testCascadeBufferRedability(makeTestArray(max_s), {std::make_shared<MemoryWriteBuffer>(s, 1, 2.0)}, {[=](auto) {
                                        return std::make_shared<MemoryWriteBuffer>(max_s - s, 1, 2.0);
                                    }});

        testCascadeBufferRedability(
            makeTestArray(max_s),
            {},
            {[=](auto) { return std::make_shared<MemoryWriteBuffer>(max_s - s, 1, 2.0); },
             [=](auto) {
                 return std::make_shared<MemoryWriteBuffer>(s, 1, 2.0);
             }});
    }
}
catch (...)
{
    std::cerr << getCurrentExceptionMessage(true) << "\n";
    throw;
}


static void checkHTTPHandlerCase(size_t input_size, size_t memory_buffer_size)
{
    std::string src = makeTestArray(input_size);
    std::string res_str(DBMS_DEFAULT_BUFFER_SIZE, '\0');

    {
        auto res_buf = std::make_shared<WriteBufferFromString>(res_str);

        CascadeWriteBuffer cascade(
            {std::make_shared<MemoryWriteBuffer>(memory_buffer_size)},
            {[res_buf](const WriteBufferPtr & prev_buf) {
                auto prev_memory_buffer = typeid_cast<MemoryWriteBuffer *>(prev_buf.get());
                auto rdbuf = prev_memory_buffer->tryGetReadBuffer();
                copyData(*rdbuf, *res_buf);
                return res_buf;
            }});

        cascade.write(&src[0], src.size());
        EXPECT_EQ(cascade.count(), src.size());
    }

    ASSERT_EQ(src.size(), res_str.size());
    ASSERT_TRUE(src == res_str);
}

TEST(CascadeWriteBuffer, HTTPHandlerCase)
{
    std::vector<size_t> sizes{1, 500000, DBMS_DEFAULT_BUFFER_SIZE, 1000000, 1451424, 1500000, 2000000, 2500000};

    for (size_t input_size : sizes)
    {
        for (size_t memory_buffer_size : sizes)
        {
            if (input_size > memory_buffer_size)
                checkHTTPHandlerCase(input_size, memory_buffer_size);
        }
    }
}


static void checkMemoryWriteBuffer(std::string data, MemoryWriteBuffer && buf)
{
    buf.write(&data[0], data.size());
    ASSERT_EQ(buf.count(), data.size());

    auto rbuf = buf.tryGetReadBuffer();
    ASSERT_TRUE(rbuf != nullptr);
    ASSERT_TRUE(buf.tryGetReadBuffer() == nullptr);

    String res;
    {
        WriteBufferFromString res_buf(res);
        copyData(*rbuf, res_buf);
    }

    ASSERT_EQ(data, res);
}


TEST(MemoryWriteBuffer, WriteAndReread)
{
    for (size_t s = 0; s < 2500000; s += 500000)
    {
        std::string data = makeTestArray(s);
        size_t min_s = std::max(s, 1ul);

        checkMemoryWriteBuffer(data, MemoryWriteBuffer(min_s));
        checkMemoryWriteBuffer(data, MemoryWriteBuffer(min_s * 2, min_s));
        checkMemoryWriteBuffer(data, MemoryWriteBuffer(min_s * 4, min_s));

        if (s > 1)
        {
            MemoryWriteBuffer buf(s - 1);
            EXPECT_THROW(buf.write(&data[0], data.size()), DB::Exception);
        }
    }

    checkMemoryWriteBuffer(makeTestArray(1451424), MemoryWriteBuffer(1451424));
}


TEST(TemporaryFileWriteBuffer, WriteAndReread)
try
{
    for (size_t s = 0; s < 2500000; s += 500000)
    {
        std::string tmp_template = "tmp/TemporaryFileWriteBuffer/";
        std::string data = makeTestArray(s);

        auto buf = WriteBufferFromTemporaryFile::create(tmp_template);
        buf->write(&data[0], data.size());

        std::string tmp_filename = buf->getFileName();
        ASSERT_EQ(tmp_template, tmp_filename.substr(0, tmp_template.size()));

        auto reread_buf = buf->tryGetReadBuffer();
        std::string decoded_data;
        {
            WriteBufferFromString wbuf_decode(decoded_data);
            copyData(*reread_buf, wbuf_decode);
        }

        ASSERT_EQ(data.size(), decoded_data.size());
        ASSERT_TRUE(data == decoded_data);

        buf.reset();
        reread_buf.reset();
        ASSERT_TRUE(!Poco::File(tmp_filename).exists());
    }
}
catch (...)
{
    std::cerr << getCurrentExceptionMessage(true) << "\n";
    throw;
}


TEST(CascadeWriteBuffer, RereadWithTemporaryFileWriteBuffer)
try
{
    const std::string tmp_template = "tmp/RereadWithTemporaryFileWriteBuffer/";

    for (size_t s = 0; s < 4000000; s += 1000000)
    {
        testCascadeBufferRedability(makeTestArray(s), {}, {[=](auto) {
                                        return WriteBufferFromTemporaryFile::create(tmp_template);
                                    }});

        testCascadeBufferRedability(
            makeTestArray(s),
            {
                std::make_shared<MemoryWriteBuffer>(std::max(1ul, s / 3ul), 2, 1.5),
            },
            {[=](auto) {
                return WriteBufferFromTemporaryFile::create(tmp_template);
            }});
    }
}
catch (...)
{
    std::cerr << getCurrentExceptionMessage(true) << "\n";
    throw;
}
