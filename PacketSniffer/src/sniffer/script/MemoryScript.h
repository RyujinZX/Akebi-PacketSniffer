#pragma once
#include <sniffer/script/Script.h>

namespace sniffer::script
{
    class MemoryScript :
        public Script
    {
    public:
        MemoryScript();
        MemoryScript(const std::string& name);
        MemoryScript(const std::string& name, const std::string& content);
        MemoryScript(const std::string& name, std::string&& content);

        void to_json(nlohmann::json& j) const override;
        void from_json(const nlohmann::json& j) override;
    };
}
