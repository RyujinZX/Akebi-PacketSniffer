#pragma once
#include "IFilterContainer.h"

#include <sniffer/script/Script.h>
namespace sniffer::filter
{
    class FilterLua :
        public IFilterContainer
    {
    public:
        FilterLua();
        ~FilterLua();
        bool Execute(const packet::Packet& packet) override;

        inline bool IsRemovable() const override { return true; };
        inline bool IsEnabled() const override { return true; };

        const script::Script* GetScript() const;
        script::Script* GetScript();

        void SetScript(script::Script* script);

		void to_json(nlohmann::json& j) const override;
		void from_json(const nlohmann::json& j) override;

    private:

        script::Script* m_Script;
        std::string m_ExecuteError;

        void OnScriptChanged(script::Script* script);
        void OnPreScriptRemove(int64_t id, script::Script* script);
        void ChangeScript(script::Script* newScript);
    };
}
