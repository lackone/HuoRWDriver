#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

int main(int argc, char* argv[], char** env)
{
	if (argc < 2)
	{
		printf("参数不对 \r\n");
		printf("举个例子：build.exe dll路径 \r\n");
		return 0;
	}

	char* dllpath = argv[1];
	FILE* file = NULL;

	fopen_s(&file, dllpath, "rb");
	if (!file)
	{
		printf("文件不存在\r\n");
		return 0;
	}

	fseek(file, 0, SEEK_END);
	long len = ftell(file);

	//回到头部
	rewind(file);

	unsigned char* fileBuffer = (unsigned char*)malloc(len);
	if (!fileBuffer)
	{
		printf("申请内存失败\r\n");
		return 0;
	}
	memset(fileBuffer, 0, len);

	fread_s(fileBuffer, len, len, 1, file);

	fclose(file);

	//创建一个文件，写入我们的硬编码
	if (argc == 2)
	{
		fopen_s(&file, "./dll.h", "wb");
	}
	else {
		fopen_s(&file, argv[2], "wb");
	}

	if (file == NULL)
	{
		free(fileBuffer);
		return 0;
	}

	fputs("#pragma once\r\n", file);
	fprintf_s(file, "unsigned char sysData[%d] = {\r\n", len);
	fprintf_s(file, "\t");

	for (int i = 0; i < len; i++)
	{
		fileBuffer[i] ^= 0xE8;
		fileBuffer[i] ^= 0xE9;

		fprintf_s(file, "0x%02X, ", fileBuffer[i]);

		if ((i + 1) % 30 == 0)
		{
			fprintf_s(file, "\r\n\t");
		}
	}
	fprintf_s(file, "\r\n};");

	free(fileBuffer);

	fclose(file);

	return 0;
}