#pragma once

#include <cheat-base/events/event.hpp>
#include <sniffer/script/Script.h>

namespace sniffer::script
{
	class FileScript :
		public Script
	{
	public:
		FileScript();
		FileScript(const std::string& name, const std::string& filepath);

		const std::string& GetFilePath() const;
		
		void Save();
		void Load();

		void Update();

		void to_json(nlohmann::json& j) const override;
		void from_json(const nlohmann::json& j) override;

		TCancelableEvent<FileScript*> ExternalChangedEvent;

		void SetContent(const std::string& content) override;
		void SetContent(std::string&& content) override;

	private:
		std::string m_Filepath;
		std::int64_t m_LastFileTimestamp;

		std::string ReadFile();
		std::string ReadFile(const std::string& filepath);

		void UpdateTimestamp();
	};
}


