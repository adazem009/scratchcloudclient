// SPDX-License-Identifier: MIT

#include "cloudlogrecord.h"

using namespace scratchcloud;

const std::unordered_map<std::string, CloudLogRecord::Type>
    CloudLogRecord::RECORD_TYPES = { { "create_var", Type::CreateVar }, { "del_var", Type::DelVar }, { "rename_var", Type::RenameVar }, { "set_var", Type::SetVar } };

CloudLogRecord::CloudLogRecord(nlohmann::json json)
{
    try {
        m_user = json["user"];
        auto it = RECORD_TYPES.find(json["verb"]);

        if (it == RECORD_TYPES.cend())
            std::cerr << "invalid cloud log record type: " << json["verb"] << std::endl;
        else
            m_type = it->second;

        m_name = json["name"];
        m_name.erase(m_name.find(u8"☁ "), 4);

        if (json["value"].is_number())
            m_value = json["value"].dump();
        else
            m_value = json["value"];

        m_timestamp = json["timestamp"];
    } catch (std::exception &e) {
        std::cerr << "invalid cloud log record: " << json.dump();
        std::cerr << e.what() << std::endl;
    }
}

const std::string &CloudLogRecord::user() const
{
    return m_user;
}

CloudLogRecord::Type CloudLogRecord::type() const
{
    return m_type;
}

const std::string &CloudLogRecord::name() const
{
    return m_name;
}

const std::string &CloudLogRecord::value() const
{
    return m_value;
}

long CloudLogRecord::timestamp() const
{
    return m_timestamp;
}
