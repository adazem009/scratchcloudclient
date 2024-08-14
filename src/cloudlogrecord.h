// SPDX-License-Identifier: MIT

#pragma once

#include <iostream>
#include <nlohmann/json.hpp>

namespace scratchcloud
{

class CloudLogRecord
{
    public:
        enum class Type
        {
            Invalid,
            CreateVar,
            DelVar,
            RenameVar,
            SetVar
        };

        CloudLogRecord(nlohmann::json json);

        const std::string &user() const;
        Type type() const;
        const std::string &name() const;
        const std::string &value() const;
        long timestamp() const;

    private:
        static const std::unordered_map<std::string, CloudLogRecord::Type> RECORD_TYPES;
        std::string m_user;
        Type m_type = Type::Invalid;
        std::string m_name;
        std::string m_value;
        long m_timestamp = 0;
};

} // namespace scratchcloud
