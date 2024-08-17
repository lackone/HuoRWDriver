#pragma once
#include <string>
#include <Windows.h>

using namespace std;

class LoadDriver
{
public:
	LoadDriver();
	~LoadDriver();
	bool load(std::string path,std::string serviceName);
	bool unload(std::string serviceName);
	bool installDriver(std::string path, std::string serviceName);
	static HMODULE getDllBase();
};

