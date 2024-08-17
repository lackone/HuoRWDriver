#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

int main(int argc, char* argv[], char** env)
{
	if (argc < 2)
	{
		printf("�������� \r\n");
		printf("�ٸ����ӣ�build.exe dll·�� \r\n");
		return 0;
	}

	char* dllpath = argv[1];
	FILE* file = NULL;

	fopen_s(&file, dllpath, "rb");
	if (!file)
	{
		printf("�ļ�������\r\n");
		return 0;
	}

	fseek(file, 0, SEEK_END);
	long len = ftell(file);

	//�ص�ͷ��
	rewind(file);

	unsigned char* fileBuffer = (unsigned char*)malloc(len);
	if (!fileBuffer)
	{
		printf("�����ڴ�ʧ��\r\n");
		return 0;
	}
	memset(fileBuffer, 0, len);

	fread_s(fileBuffer, len, len, 1, file);

	fclose(file);

	//����һ���ļ���д�����ǵ�Ӳ����
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