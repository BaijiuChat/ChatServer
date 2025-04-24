#include "ConfigMgr.h"
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

ConfigMgr::ConfigMgr() {
	boost::filesystem::path config_path = boost::filesystem::current_path() / "config.ini";
	std::cout << "config_path is " << config_path.string() << std::endl;

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(config_path.string(), pt);

	for (const auto& [section_name, section_tree] : pt) {
		// 直接在 map 中构造 SectionInfo
		auto& section_info = _config_map[section_name];
		// 直接遍历并填充 section_datas
		for (const auto& [key, value_node] : section_tree) {
			section_info._section_datas.emplace(key, value_node.data());
		}
	}

	// 控制台输出配置文件内容
	for (const auto& [section_name, section_info] : _config_map) {
		std::cout << "Section: " << section_name << std::endl;
		for (const auto& [key, value] : section_info._section_datas) {
			std::cout << "  " << key << ": " << value << std::endl;
		}
	}
}

/////////////// 旧版代码 ///////////////
//boost::property_tree::ptree pt; // 创建一个ptree对象
//boost::property_tree::read_ini(config_path.string(), pt); // 读取配置文件
//
//for (const auto& section_pair : pt) {
//	const std::string& section_name = section_pair.first; // 获取section的名称
//
//	const boost::property_tree::ptree& section_tree = section_pair.second; // 获取section的内容
//	std::map<std::string, std::string> section_config; // 创建一个map来存储内部section的数据
//	for (const auto& item : section_tree) {
//		const std::string& key = item.first; // 获取key
//		const std::string& value = item.second.data(); // 获取value
//		section_config[key] = value; // 将key和value存入map中
//	}
//	SectionInfo section_info; // 创建一个SectionInfo对象
//	section_info._section_datas = section_config; // 将map赋值给SectionInfo对象
//
//	_config_map[section_name] = section_info; // 将SectionInfo对象存入_config_map中
//}
/////////////// 旧版代码 ///////////////