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

#pragma once

#include <Storages/IStorage.h>

#include <ext/singleton.h>
#include <unordered_map>


namespace DB
{
class Context;
class ASTCreateQuery;
class ASTStorage;


/** Allows to create a table by the name and parameters of the engine.
  * In 'columns' Nested data structures must be flattened.
  * You should subsequently call IStorage::startup method to work with table.
  */
class StorageFactory : public ext::Singleton<StorageFactory>
{
public:
    struct Arguments
    {
        const String & engine_name;
        ASTs & engine_args;
        ASTStorage * storage_def;
        const ASTCreateQuery & query;
        const String & data_path;
        const String & table_name;
        const String & database_name;
        const String & database_engine;
        Context & local_context;
        Context & context;
        const ColumnsDescription & columns;
        bool attach;
        bool has_force_restore_data_flag;
    };

    using Creator = std::function<StoragePtr(const Arguments & arguments)>;

    StoragePtr get(
        ASTCreateQuery & query,
        const String & data_path,
        const String & table_name,
        const String & database_name,
        const String & database_engine,
        Context & local_context,
        Context & context,
        const ColumnsDescription & columns,
        bool attach,
        bool has_force_restore_data_flag) const;

    /// Register a table engine by its name.
    /// No locking, you must register all engines before usage of get.
    void registerStorage(const std::string & name, Creator creator);

private:
    using Storages = std::unordered_map<std::string, Creator>;
    Storages storages;
};

} // namespace DB
