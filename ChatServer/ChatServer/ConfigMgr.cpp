#include "ConfigMgr.h"

using namespace boost;

ConfigMgr::ConfigMgr()
{
	filesystem::path current_path = filesystem::current_path();	//当前文件路径
	filesystem::path config_path = current_path / "config.ini";	//拼接配置文件路径
	std::cout << "Config path : " << config_path << std::endl;

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(config_path.string(), pt);	//读取ini文件

	for (const auto& section_pair:pt)
	{
		const std::string& section_name = section_pair.first;			//	[GateServer]
		const property_tree::ptree& section_tree = section_pair.second;	//	Host = XXX,Port = XXX,...
		std::map<std::string, std::string> section_config;
		for (const auto& key_value_pair : section_tree)
		{
			const std::string& key = key_value_pair.first;
			const std::string& value = key_value_pair.second.get_value<std::string>();
			section_config[key] = value;
		}

		SectionInfo sectionInfo;
		sectionInfo.m_section_datas = section_config;
		m_config_map[section_name] = sectionInfo;//[GateServer] - sectioninfo

		// 输出所有的section和key-value对  
		for (const auto& section_entry : m_config_map) 
		{
			const std::string& section_name = section_entry.first;
			SectionInfo section_config = section_entry.second;
			std::cout << "[" << section_name << "]" << std::endl;
			for (const auto& key_value_pair : section_config.m_section_datas) 
			{
				std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
			}
		}
	}
	
}
