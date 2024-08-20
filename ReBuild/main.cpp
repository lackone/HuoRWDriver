#include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	while (1)
	{

		WinExec("I:\\Program Files (x86)\\vs\\2019\\Community\\Common7\\IDE\\devenv.com I:\\cpp_projects\\HuoRWDriver\\HuoRWDriver\\HuoRWDriver.sln /ReBuild", SW_HIDE);

		Sleep(1000 * 60 * 2);
	}

	return 0;
}