#pragma once
#include "const.h"

/*��ȡconfig.ini�ļ�����*/

//�洢�����ļ���ĳ�����ֵļ�ֵ��
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
			//û�ҵ�
			return "";
		}

		return m_section_datas[key];

	}
};
// �������������ļ��Ķ������
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
	//����
	static ConfigMgr& Inst()
	{
		//c++11 �ֲ���̬�����ٶ��߳����ǰ�ȫ��
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

