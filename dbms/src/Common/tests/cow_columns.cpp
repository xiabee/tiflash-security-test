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

#include <Common/COWPtr.h>

#include <iostream>


class IColumn : public COWPtr<IColumn>
{
private:
    friend class COWPtr<IColumn>;
    virtual MutablePtr clone() const = 0;

public:
    virtual ~IColumn() {}

    virtual int get() const = 0;
    virtual void set(int value) = 0;

    virtual MutablePtr test() const = 0;
};

using ColumnPtr = IColumn::Ptr;
using MutableColumnPtr = IColumn::MutablePtr;

class ConcreteColumn : public COWPtrHelper<IColumn, ConcreteColumn>
{
private:
    friend class COWPtrHelper<IColumn, ConcreteColumn>;

    int data;
    ConcreteColumn(int data)
        : data(data)
    {}
    ConcreteColumn(const ConcreteColumn &) = default;

    MutableColumnPtr test() const override
    {
        MutableColumnPtr res = create(123);
        return res;
    }

public:
    int get() const override { return data; }
    void set(int value) override { data = value; }
};


int main(int, char **)
{
    ColumnPtr x = ConcreteColumn::create(1);
    ColumnPtr y = x; //x->test();

    std::cerr << "values:    " << x->get() << ", " << y->get() << "\n";
    std::cerr << "refcounts: " << x->use_count() << ", " << y->use_count() << "\n";
    std::cerr << "addresses: " << x.get() << ", " << y.get() << "\n";

    {
        MutableColumnPtr mut = y->mutate();
        mut->set(2);

        std::cerr << "refcounts: " << x->use_count() << ", " << y->use_count() << ", " << mut->use_count() << "\n";
        std::cerr << "addresses: " << x.get() << ", " << y.get() << ", " << mut.get() << "\n";
        y = std::move(mut);
    }

    std::cerr << "values:    " << x->get() << ", " << y->get() << "\n";
    std::cerr << "refcounts: " << x->use_count() << ", " << y->use_count() << "\n";
    std::cerr << "addresses: " << x.get() << ", " << y.get() << "\n";

    x = ConcreteColumn::create(0);

    std::cerr << "values:    " << x->get() << ", " << y->get() << "\n";
    std::cerr << "refcounts: " << x->use_count() << ", " << y->use_count() << "\n";
    std::cerr << "addresses: " << x.get() << ", " << y.get() << "\n";

    {
        MutableColumnPtr mut = y->mutate();
        mut->set(3);

        std::cerr << "refcounts: " << x->use_count() << ", " << y->use_count() << ", " << mut->use_count() << "\n";
        std::cerr << "addresses: " << x.get() << ", " << y.get() << ", " << mut.get() << "\n";
        y = std::move(mut);
    }

    std::cerr << "values:    " << x->get() << ", " << y->get() << "\n";
    std::cerr << "refcounts: " << x->use_count() << ", " << y->use_count() << "\n";

    return 0;
}
