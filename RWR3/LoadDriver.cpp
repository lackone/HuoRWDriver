#include "pch.h"
#include "LoadDriver.h"
#include <windows.h>
#include <Winsvc.h>
#include "dll.h"

LoadDriver::LoadDriver()
{
}


LoadDriver::~LoadDriver()
{
}


bool LoadDriver::load(std::string path, std::string serviceName)
{
	bool bRet = false;
	DWORD dwLastError;
	SC_HANDLE hSCManager;
	SC_HANDLE hService = NULL;

	if (hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS))
	{
		hService = CreateServiceA(
			hSCManager, serviceName.c_str(),
			serviceName.c_str(), SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
			SERVICE_ERROR_IGNORE, path.c_str(),
			NULL, NULL, NULL, NULL, NULL
		);

		if (hService == NULL)
		{
			hService = OpenServiceA(hSCManager, serviceName.c_str(), SERVICE_ALL_ACCESS);

			if (!hService)
			{
				CloseServiceHandle(hSCManager);
				return false;
			}

		}

		bRet = StartServiceA(hService, 0, NULL);
		if (!bRet)
		{
			dwLastError = GetLastError();
			printf("%d\r\n", dwLastError);
		}

	}

	if (hService)
	{
		CloseServiceHandle(hService);
	}

	if (hSCManager)
	{
		CloseServiceHandle(hSCManager);
	}

	return bRet;
}

bool LoadDriver::unload(std::string serviceName)
{
	BOOL bRet = FALSE;
	SC_HANDLE hServiceMgr = NULL;
	SC_HANDLE hServiceDDK = NULL;
	SERVICE_STATUS SvrSta;

	do
	{
		// 打开SCM管理器
		hServiceMgr = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if (hServiceMgr == NULL)
		{
			break;
		}

		// 打开驱动所对应的服务
		hServiceDDK = OpenServiceA(hServiceMgr, serviceName.c_str(), SERVICE_ALL_ACCESS);

		if (hServiceDDK == NULL)
		{
			break;
		}

		ControlService(hServiceDDK, SERVICE_CONTROL_STOP, &SvrSta);

		if (DeleteService(hServiceDDK))
		{
			bRet = TRUE;
		}

	} while (FALSE);

	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}

	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}

	return bRet;
}

HMODULE GetSelfModuleHandle()
{
	MEMORY_BASIC_INFORMATION mbi;

	return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0)
		? (HMODULE)mbi.AllocationBase : NULL);
}

HMODULE LoadDriver::getDllBase()
{
	return GetSelfModuleHandle();
}

bool LoadDriver::installDriver(std::string path, std::string serviceName)
{
	DWORD dwImageSize;
	HANDLE hFile;
	DWORD dwByteWrite;
	CHAR str[512] = { 0 };

	// 或许是上次由于未知错误, 导致驱动卸载
	// 不干净, 这里卸载一次.
	this->unload(serviceName.c_str());

	// 在自定义资源中释放出sys

	dwImageSize = sizeof(sysData);
	unsigned char* pMemory = (unsigned char*)malloc(dwImageSize);
	if (pMemory == NULL)
	{
		return false;
	}
	memcpy(pMemory, sysData, dwImageSize);
	for (ULONG i = 0; i < dwImageSize; i++)
	{
		pMemory[i] ^= 0xE9;
		pMemory[i] ^= 0xE8;
	}

	hFile = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		OutputDebugStringA(path.c_str());
		return false;
	}

	if (!WriteFile(hFile, pMemory, dwImageSize, &dwByteWrite, NULL))
	{
		OutputDebugStringA(path.c_str());
		CloseHandle(hFile);
		return false;
	}

	if (dwByteWrite != dwImageSize)
	{
		OutputDebugStringA(path.c_str());
		CloseHandle(hFile);
		return false;
	}

	CloseHandle(hFile);

	// 安装驱动
	if (!this->load(path, serviceName))
	{
		DeleteFileA(path.c_str());
		return false;
	}

	DeleteFileA(path.c_str());

	return true;
}
