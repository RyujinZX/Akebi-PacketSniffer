#include "pch.h"
#include "FileScript.h"

namespace sniffer::script
{

	FileScript::FileScript() : Script(ScriptType::FileScript), m_Filepath({}), m_LastFileTimestamp(0)
	{
		UpdateTimestamp();
	}

	FileScript::FileScript(const std::string& name, const std::string& filepath) : Script(ScriptType::FileScript, name, ReadFile(filepath)), m_Filepath(filepath), m_LastFileTimestamp(0)
	{
		auto& lua = GetLua();
		lua.SetPackagePath(std::filesystem::path(filepath).parent_path().string());
		CompileScript();
		UpdateTimestamp();
	}

	const std::string& FileScript::GetFilePath() const
	{
		return m_Filepath;
	}

	void FileScript::Save()
	{
		std::ofstream ofs(m_Filepath);
		ofs << GetContent();
		ofs.close();

		UpdateTimestamp();
	}

	void FileScript::Load()
	{
		SetContent(ReadFile());
	}

	void FileScript::Update()
	{
		UPDATE_DELAY(2000);

		if (!std::filesystem::exists(m_Filepath))
		{
			SetError(ErrorType::Environment, "File doesn't exists!");
			return;
		}

		auto write_time = std::filesystem::last_write_time(m_Filepath);
		auto current_timestamp = write_time.time_since_epoch().count();
		if (m_LastFileTimestamp != current_timestamp)
		{
			m_LastFileTimestamp = current_timestamp;
			bool replacing = ExternalChangedEvent(this);
			if (replacing)
				Load();
		}
	}

	std::string FileScript::ReadFile()
	{
		return ReadFile(m_Filepath);
	}

	std::string FileScript::ReadFile(const std::string& filepath)
	{
		std::ifstream ifs(filepath);
		std::string content((std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));
		return content;
	}

	void FileScript::to_json(nlohmann::json& j) const
	{
		Script::to_json(j);
		j["filepath"] = m_Filepath;
	}

	void FileScript::from_json(const nlohmann::json& j)
	{
		Script::from_json(j);
		j.at("filepath").get_to(m_Filepath);
		GetLua().SetPackagePath(std::filesystem::path(m_Filepath).parent_path().string());
		SetContent(ReadFile());
		UpdateTimestamp();
	}

	void FileScript::UpdateTimestamp()
	{
		if (!std::filesystem::exists(m_Filepath))
		{
			SetError(ErrorType::Environment, "File doesn't exists!");
			return;
		}

		auto write_time = std::filesystem::last_write_time(m_Filepath);
		m_LastFileTimestamp = write_time.time_since_epoch().count();
	}

	void FileScript::SetContent(const std::string& content)
	{
		Script::SetContent(content);
		Save();
	}

	void FileScript::SetContent(std::string&& content)
	{
		Script::SetContent(content);
		Save();
	}

}