#pragma once
#include "const.h"

/*读取config.ini文件配置*/

//存储配置文件中某个部分的键值对
struct SectionInfo
{
	SectionInfo(){}
	~SectionInfo() { m_section_datas.clear(); }
	SectionInfo(const SectionInfo& src)
	{
		m_section_datas = src.m_section_datas;
	}

	SectionInfo& operator=(const SectionInfo& src)
	{
		if (&src == this)
		{
			return *this;
		}

		this->m_section_datas = src.m_section_datas;
		return *this;
	}

	std::map<std::string, std::string> m_section_datas;
	std::string operator[](const std::string& key)
	{
		if (m_section_datas.find(key)==m_section_datas.end())
		{
			//没找到
			return "";
		}

		return m_section_datas[key];

	}
};
// 管理整个配置文件的多个部分
class ConfigMgr
{
public:
	~ConfigMgr()
	{
		m_config_map.clear();
	}
	SectionInfo operator[](const std::string& section)
	{
		if (m_config_map.find(section) == m_config_map.end())
		{
			return SectionInfo();
		}
		return m_config_map[section];
	}
	//单例
	static ConfigMgr& Inst()
	{
		//c++11 局部静态变量再多线程中是安全的
		static ConfigMgr cfg_mgr;
		return cfg_mgr;
	}

	ConfigMgr(const ConfigMgr& src)
	{
		m_config_map = src.m_config_map;
	}

	ConfigMgr& operator=(const ConfigMgr& src)
	{
		if (&src == this)
		{
			return *this;
		}
		m_config_map = src.m_config_map;
	}

private:
	ConfigMgr();
	std::map<std::string, SectionInfo> m_config_map;

};

