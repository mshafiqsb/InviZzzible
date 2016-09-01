#include "ve_detection.h"
#include <iostream>
#include <boost\foreach.hpp>

using std::cout;
using std::endl;

namespace SandboxEvasion {

	// return first: html information
	// return second: debug information
	std::pair<std::string, std::string> VEDetection::GenerateReportEntry(const std::string &name, const json_tiny &j, bool detected) const {
		std::ostringstream ostream_debug;
		std::ostringstream ostream_html;

		std::string desc = j.get<std::string>(Config::cg2s[Config::ConfigGlobal::DESCRIPTION], "");
		std::string wtd = j.get<std::string>(Config::cg2s[Config::ConfigGlobal::COUNTERMEASURES], "");
		std::string dtype = j.get<std::string>(Config::cg2s[Config::ConfigGlobal::TYPE], "");

		ostream_debug << name  << "> " << desc << ": " << detected << std::endl;
		
		// add entry to report
		if (p_report) {
			p_report->add_entry({ name, desc, detected ? "1" : "0", wtd });
		}

		return std::pair<std::string, std::string>(ostream_html.str(), ostream_debug.str());
	}

	void VEDetection::CheckAll() {
		CheckAllCustom();
		CheckAllRegistry();
		CheckAllFilesExist();
		CheckAllDevicesExists();
		CheckAllProcessRunning();
		CheckAllMacVendors();
		CheckAllAdaptersName();
		CheckAllFirmwareTables();
		CheckAllDirectoryObjects();
		CheckAllCpuid();
		CheckAllWindows();

		if (p_report) {
			p_report->flush(module_name);
		}
	}

	void VEDetection::AddReportModule(Report *_report) {
		p_report = _report;
	}

	void VEDetection::CheckAllRegistry() const {
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::REGISTRY]);

		CheckAllRegistryKeyExists(jl);
		CheckAllRegistryKeyValueContains(jl);
		CheckAllRegistryEnumKeys(jl);
		CheckAllRegistryEnumValues(jl);
	}

	void VEDetection::CheckAllRegistryKeyExists(const std::list<std::pair<std::string, json_tiny>> &jl) const {
		bool detected;
		std::pair<std::string, std::string> report;
		json_tiny jt;
		std::list<std::string> keys;

		// iterate through all registry exists detections
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());
			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::carct2s[Config::ConfigArgsRegCheckType::EXISTS]) {

				keys = jt.get_entries(Config::ca2s[Config::ConfigArgs::KEY]);
				for (auto &k : keys) {
					detected = CheckRegKeyExists(jt.get<std::string>(Config::ca2s[Config::ConfigArgs::HKEY], ""), k );
					if (detected) break;
				}

				report = GenerateReportEntry(o.first, o.second, detected);
				log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
			}
		}
	}

	void VEDetection::CheckAllRegistryKeyValueContains(const std::list<std::pair<std::string, json_tiny>> &jl) const {
		bool detected;
		std::pair<std::string, std::string> report;
		json_tiny jt;
		std::list<std::string> vn;
		std::list<std::string> vd;

		// iterate through all registry keys contains specific values
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::carct2s[Config::ConfigArgsRegCheckType::CONTAINS]) {

				vn = jt.get_entries(Config::ca2s[Config::ConfigArgs::VALUE_NAME]);
				vd = jt.get_entries(Config::ca2s[Config::ConfigArgs::VALUE_DATA]);
				for (auto &_vd : vd) {
					detected = false;
					for (auto &_vn : vn) {
						detected = CheckRegKeyValueContains(
							jt.get<std::string>(Config::ca2s[Config::ConfigArgs::HKEY], ""),
							jt.get<std::string>(Config::ca2s[Config::ConfigArgs::KEY], ""),
							_vn, _vd,
							jt.get<std::string>(Config::ca2s[Config::ConfigArgs::RECURSIVE], "") == Config::cge2s[Config::ConfigGlobalEnabled::YES]);
						if (detected) break;
					}
					if (detected) break;
				}

				report = GenerateReportEntry(o.first, o.second, detected);
				log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
			}
		}
	}

	void VEDetection::CheckAllRegistryEnumKeys(const std::list<std::pair<std::string, json_tiny>>& jl) const {
		bool detected;
		std::pair<std::string, std::string> report;
		json_tiny jt;
		std::list<std::string> subkeys;

		// iterate through all registry keys contains specific values
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::carct2s[Config::ConfigArgsRegCheckType::ENUM_KEYS]) {

				subkeys = jt.get_entries(Config::ca2s[Config::ConfigArgs::SUBKEY]);
				for (auto &sk : subkeys) {
					detected = CheckRegKeyEnumKeys(
						jt.get<std::string>(Config::ca2s[Config::ConfigArgs::HKEY], ""),
						jt.get<std::string>(Config::ca2s[Config::ConfigArgs::KEY], ""),
						sk);

					if (detected) break;
				}

				report = GenerateReportEntry(o.first, o.second, detected);
				log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
			}
		}
	}

	void VEDetection::CheckAllRegistryEnumValues(const std::list<std::pair<std::string, json_tiny>>& jl) const {
		bool detected;
		std::pair<std::string, std::string> report;
		json_tiny jt;
		std::list<std::string> values;

		// iterate through all registry keys contains specific values
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::carct2s[Config::ConfigArgsRegCheckType::ENUM_VALUES]) {
				
				values = jt.get_entries(Config::ca2s[Config::ConfigArgs::VALUE_NAME]);
				for (auto &v : values) {
					detected = CheckRegKeyEnumValues(
						jt.get<std::string>(Config::ca2s[Config::ConfigArgs::HKEY], ""),
						jt.get<std::string>(Config::ca2s[Config::ConfigArgs::KEY], ""),
						v);

					if (detected) break;
				}

				report = GenerateReportEntry(o.first, o.second, detected);
				log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
			}
		}
	}

	void VEDetection::CheckAllFilesExist() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::FILE]);
		json_tiny jt;
		std::list<std::string> fnames;

		// check for the presence of all files
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			fnames = jt.get_entries(Config::ca2s[Config::ConfigArgs::NAME]);
			for (auto &fn : fnames) {
				detected = CheckFileExists(fn);
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	void VEDetection::CheckAllDevicesExists() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::DEVICE]);
		json_tiny jt;
		std::list<std::string> devicenames;

		// check for the presence of devices
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			devicenames = jt.get_entries(Config::ca2s[Config::ConfigArgs::NAME]);
			for (auto &dn : devicenames) {
				detected = CheckDeviceExists(dn);
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	void VEDetection::CheckAllProcessRunning() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::PROCESS]);
		json_tiny jt;
		std::list<std::string> procnames;

		// check for the presence of devices
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			procnames = jt.get_entries(Config::ca2s[Config::ConfigArgs::NAME]);
			for (auto &pn : procnames) {
				detected = CheckProcessIsRunning(pn);
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	void VEDetection::CheckAllMacVendors() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::MAC]);
		json_tiny jt;
		std::list<std::string> macaddrs;

		// check for the presence of devices
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			macaddrs = jt.get_entries(Config::ca2s[Config::ConfigArgs::VENDOR]);
			for (auto &mac : macaddrs) {
				detected = CheckMacVendor(mac);
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	void VEDetection::CheckAllAdaptersName() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::ADAPTER]);
		json_tiny jt;
		std::list<std::string> adapters;

		// check for the presence of devices
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			adapters = jt.get_entries(Config::ca2s[Config::ConfigArgs::NAME]);
			for (auto &adp : adapters) {
				detected = CheckAdapterName(adp);
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	void VEDetection::CheckAllFirmwareTables() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::FIRMWARE]);
		json_tiny jt;
		std::list<std::string> firmwares;

		// check for the presence of all files
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			firmwares = jt.get_entries(Config::ca2s[Config::ConfigArgs::NAME]);
			for (auto &f : firmwares) {
				if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::cafct2s[Config::ConfigArgsFirmwareCheckType::FIRMBIOS])
					detected = CheckFirmwareTableFIRM(f);
				else if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::cafct2s[Config::ConfigArgsFirmwareCheckType::RSMBBIOS])
					detected = CheckFirmwareTableRSMB(f);
				else detected = false;
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}

	}

	void VEDetection::CheckAllDirectoryObjects() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::OBJECT]);
		json_tiny jt;
		std::list<std::string> dirobjects;

		// check for the presence of specific directory objects
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			dirobjects = jt.get_entries(Config::ca2s[Config::ConfigArgs::NAME]);
			for (auto &dir : dirobjects) {
				detected = CheckDirectoryObject(jt.get<std::string>(Config::ca2s[Config::ConfigArgs::DIRECTORY], ""), dir);
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	void VEDetection::CheckAllCpuid() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::CPUID]);
		json_tiny jt;
		std::list<std::string> vendors;

		// check for the presence of specific directory objects
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			vendors = jt.get_entries(Config::ca2s[Config::ConfigArgs::VENDOR]);
			for (auto &v : vendors) {
				detected = CheckCpuid(v);
				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	void VEDetection::CheckAllWindows() const {
		bool detected;
		std::pair<std::string, std::string> report;
		std::list<std::pair<std::string, json_tiny>> jl = conf.get_objects(Config::cg2s[Config::ConfigGlobal::TYPE], Config::cgt2s[Config::ConfigGlobalType::WINDOW]);
		json_tiny jt;
		std::list<std::string> windows;

		// check for the presence of specific directory objects
		for each (auto &o in jl) {
			jt = o.second.get(Config::cg2s[Config::ConfigGlobal::ARGUMENTS], pt::ptree());

			if (!IsEnabled(o.first, conf.get<std::string>(o.first + std::string(".") + Config::cg2s[Config::ConfigGlobal::ENABLED], "")))
				continue;

			windows = jt.get_entries(Config::ca2s[Config::ConfigArgs::NAME]);
			for (auto &w : windows) {				
				if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::cawct2s[Config::ConfigArgsWindowCheckType::CLASS])
					detected = CheckWindowClassName(w);
				else if (jt.get<std::string>(Config::ca2s[Config::ConfigArgs::CHECK], "") == Config::cawct2s[Config::ConfigArgsWindowCheckType::WINDOW])
					detected = CheckWindowWindowName(w);
				else detected = false;

				if (detected) break;
			}

			report = GenerateReportEntry(o.first, o.second, detected);
			log_message(LogMessageLevel::INFO, module_name, report.second, detected ? RED : GREEN);
		}
	}

	bool VEDetection::CheckRegKeyExists(const std::string &key_root, const std::string &key) const {
		HKEY hRootKey = get_hkey(key_root);
		if (hRootKey == reinterpret_cast<HKEY>(INVALID_HKEY))
			return false;

		return check_regkey_exists(hRootKey, key);
	}

	bool VEDetection::CheckRegKeyValueContains(const std::string &key_root, const std::string &key, const std::string &subkey, const std::string &value, bool rec) const {
		HKEY hRootKey = get_hkey(key_root);
		if (hRootKey == reinterpret_cast<HKEY>(INVALID_HKEY))
			return false;

		return check_regkey_subkey_value(hRootKey, key, subkey, value, rec);
	}

	bool VEDetection::CheckRegKeyEnumKeys(const std::string & key_root, const std::string & key, const std::string & subkey) const {
		HKEY hRootKey = get_hkey(key_root);
		if (hRootKey == reinterpret_cast<HKEY>(INVALID_HKEY))
			return false;

		return check_regkey_enum_keys(hRootKey, key, subkey);
	}

	bool VEDetection::CheckRegKeyEnumValues(const std::string & key_root, const std::string & key, const std::string & value) const {
		HKEY hRootKey = get_hkey(key_root);
		if (hRootKey == reinterpret_cast<HKEY>(INVALID_HKEY))
			return false;
		
		return check_regkey_enum_values(hRootKey, key, value);
	}

	bool VEDetection::CheckFileExists(const file_name_t &file_name) const {
		return check_file_exists(file_name);
	}

	bool VEDetection::CheckDeviceExists(const file_name_t &dev_name) const {
		return check_device_exists(dev_name);
	}

	bool VEDetection::CheckProcessIsRunning(const process_name_t &proc_name) const {
		return check_process_is_running(proc_name);
	}

	bool VEDetection::CheckMacVendor(const std::string &ven_id) const {
		return check_mac_vendor(ven_id);
	}

	bool VEDetection::CheckAdapterName(const std::string &adapter_name) const {
		return check_adapter_name(adapter_name);
	}

	bool VEDetection::CheckFirmwareTableFIRM(const std::string &vendor) const {
		CHAR *sfti;
		DWORD data_size;
		bool found;

		sfti = reinterpret_cast<CHAR *>(get_firmware_table(&data_size, FIRM, 0xC0000));
		if (!sfti)
			return false;

		found = !!scan_mem(sfti, data_size, const_cast<char *>(vendor.c_str()), vendor.length());

		LocalFree(sfti);

		return found;
	}

	bool VEDetection::CheckFirmwareTableRSMB(const std::string &vendor) const {
		CHAR *sfti;
		DWORD data_size;
		bool found;

		sfti = reinterpret_cast<CHAR *>(get_firmware_table(&data_size, RSMB, 0x0));
		if (!sfti)
			return false;

		found = !!scan_mem(sfti, data_size, const_cast<char *>(vendor.c_str()), vendor.length());

		LocalFree(sfti);

		return found;
	}

	bool VEDetection::CheckDirectoryObject(const std::string &directory, const std::string &object) const {
		std::wstring directory_w;
		std::wstring object_w;

		directory_w.assign(directory.begin(), directory.end());
		object_w.assign(object.begin(), object.end());

		return !!check_system_objects(directory_w, object_w);
	}

	bool VEDetection::CheckCpuid(const std::string &cpuid_s) const {
		char cpuid_[sizeof(DWORD) * 3] = {};
		size_t s = cpuid_s.length() > _countof(cpuid_) ? _countof(cpuid_) : cpuid_s.length();

		get_cpuid_vendor(cpuid_);

		return !strncmp(cpuid_, cpuid_s.c_str(), s);
	}

	bool VEDetection::CheckWindowClassName(const std::string & cname) const {
		return !!FindWindowA(cname.c_str(), NULL);
	}

	bool VEDetection::CheckWindowWindowName(const std::string &wname) const {
		return !!FindWindowA(NULL, wname.c_str());
	}

	bool VEDetection::IsEnabled(const std::string &detection_name, const std::string &enabled) const {
		return detection_name != "" && enabled == Config::cge2s[Config::ConfigGlobalEnabled::YES];
	}

} // SandboxEvasion
