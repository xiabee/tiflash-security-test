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

#include <AggregateFunctions/AggregateFunctionFactory.h>
#include <AggregateFunctions/AggregateFunctionGroupArray.h>
#include <AggregateFunctions/FactoryHelpers.h>
#include <AggregateFunctions/Helpers.h>
#include <DataTypes/DataTypeDate.h>
#include <DataTypes/DataTypeDateTime.h>


namespace DB
{
namespace ErrorCodes
{
extern const int NUMBER_OF_ARGUMENTS_DOESNT_MATCH;
}

namespace
{
template <template <typename, typename> class AggregateFunctionTemplate, typename Data, typename... TArgs>
static IAggregateFunction * createWithNumericOrTimeType(const IDataType & argument_type, TArgs &&... args)
{
    if (typeid_cast<const DataTypeDate *>(&argument_type))
        return new AggregateFunctionTemplate<UInt16, Data>(std::forward<TArgs>(args)...);
    if (typeid_cast<const DataTypeDateTime *>(&argument_type))
        return new AggregateFunctionTemplate<UInt32, Data>(std::forward<TArgs>(args)...);
    return createWithNumericType<AggregateFunctionTemplate, Data, TArgs...>(
        argument_type,
        std::forward<TArgs>(args)...);
}


template <typename has_limit, typename... TArgs>
inline AggregateFunctionPtr createAggregateFunctionGroupArrayImpl(const DataTypePtr & argument_type, TArgs... args)
{
    if (auto res = createWithNumericOrTimeType<GroupArrayNumericImpl, has_limit>(
            *argument_type,
            argument_type,
            std::forward<TArgs>(args)...))
        return AggregateFunctionPtr(res);

    if (typeid_cast<const DataTypeString *>(argument_type.get()))
        return std::make_shared<GroupArrayGeneralListImpl<GroupArrayListNodeString, has_limit::value>>(
            argument_type,
            std::forward<TArgs>(args)...);

    return std::make_shared<GroupArrayGeneralListImpl<GroupArrayListNodeGeneral, has_limit::value>>(
        argument_type,
        std::forward<TArgs>(args)...);
};


static AggregateFunctionPtr createAggregateFunctionGroupArray(
    const std::string & name,
    const DataTypes & argument_types,
    const Array & parameters)
{
    assertUnary(name, argument_types);

    bool limit_size = false;
    UInt64 max_elems = std::numeric_limits<UInt64>::max();

    if (parameters.empty())
    {
        // no limit
    }
    else if (parameters.size() == 1)
    {
        auto type = parameters[0].getType();
        if (type != Field::Types::Int64 && type != Field::Types::UInt64)
            throw Exception(
                "Parameter for aggregate function " + name + " should be positive number",
                ErrorCodes::BAD_ARGUMENTS);

        if ((type == Field::Types::Int64 && parameters[0].get<Int64>() < 0)
            || (type == Field::Types::UInt64 && parameters[0].get<UInt64>() == 0))
            throw Exception(
                "Parameter for aggregate function " + name + " should be positive number",
                ErrorCodes::BAD_ARGUMENTS);

        limit_size = true;
        max_elems = parameters[0].get<UInt64>();
    }
    else
        throw Exception(
            "Incorrect number of parameters for aggregate function " + name + ", should be 0 or 1",
            ErrorCodes::NUMBER_OF_ARGUMENTS_DOESNT_MATCH);

    if (!limit_size)
        return createAggregateFunctionGroupArrayImpl<std::false_type>(argument_types[0]);
    else
        return createAggregateFunctionGroupArrayImpl<std::true_type>(argument_types[0], max_elems);
}

} // namespace


void registerAggregateFunctionGroupArray(AggregateFunctionFactory & factory)
{
    factory.registerFunction("groupArray", createAggregateFunctionGroupArray);
}

} // namespace DB
