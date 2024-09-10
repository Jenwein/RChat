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
		std::cout << "BeginOK2" << std::endl;
		if (m_section_datas.find(key)==m_section_datas.end())
		{
			//没找到
			return "";
		}
		std::cout << "ENDOK2" << std::endl;
		try
		{
			return m_section_datas.at(key);
		}
		catch (const std::out_of_range&)
		{
			return "";
		}

	}
};
// 管理整个配置文件的多个部分
class ConfigMgr
{
public:
	~ConfigMgr()
	{
	}
	SectionInfo  operator[](const std::string& section)
	{
		std::cout << "BeginOK3" << std::endl;
		if (m_config_map.find(section) == m_config_map.end())
		{
			return SectionInfo();
		}
		std::cout << "ENDOK3" << std::endl;
		try
		{
			return m_config_map.at(section);
		}
		catch (const std::out_of_range&)
		{
			return SectionInfo();
		}
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

