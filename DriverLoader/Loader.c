#include "Loader.h"
#include <ntimage.h>
#include "Tools.h"

typedef NTSTATUS(NTAPI* DriverEntryProc)(PDRIVER_OBJECT pDriver, PUNICODE_STRING pReg);

typedef struct _IMAGE_RELOC
{
	UINT16 Offset : 12;
	UINT16 Type : 4;
} IMAGE_RELOC, *PIMAGE_RELOC;

PUCHAR FileToImage(CHAR* fileBuffer)
{
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)fileBuffer;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(fileBuffer + dos->e_lfanew);

	//获取镜像大小
	ULONG SizeOfImage = nt->OptionalHeader.SizeOfImage;
	//申请内存
	PUCHAR imageBuffer = ExAllocatePool(NonPagedPool, SizeOfImage);
	if (imageBuffer == NULL)
	{
		return NULL;
	}
	memset(imageBuffer, 0, SizeOfImage);

	//复制PE头
	memcpy(imageBuffer, fileBuffer, nt->OptionalHeader.SizeOfHeaders);

	//获取节表数量
	USHORT NumberOfSections = nt->FileHeader.NumberOfSections;

	//获取第一个节区
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);

	//复制节区
	for (USHORT i = 0; i < NumberOfSections; i++)
	{
		memcpy(imageBuffer + section->VirtualAddress, fileBuffer + section->PointerToRawData, section->SizeOfRawData);
		section++;
	}

	return imageBuffer;
}

BOOLEAN UpdateReloc(CHAR* imageBuffer)
{
	if (!imageBuffer)
	{
		return FALSE;
	}

	PIMAGE_NT_HEADERS nt = RtlImageNtHeader(imageBuffer);

	//获取重定位表
	PIMAGE_DATA_DIRECTORY dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	PIMAGE_BASE_RELOCATION base = (PIMAGE_BASE_RELOCATION)(imageBuffer + dir->VirtualAddress);

	while (base->SizeOfBlock && base->VirtualAddress)
	{
		PIMAGE_RELOC reloc = (PIMAGE_RELOC)((PUCHAR)base + sizeof(PIMAGE_BASE_RELOCATION));

		UINT32 nums = (base->SizeOfBlock - sizeof(PIMAGE_BASE_RELOCATION)) / sizeof(IMAGE_RELOC);

		for (UINT32 i = 0; i < nums; i++)
		{
			if (reloc[i].Type == IMAGE_REL_BASED_DIR64) //64位
			{
				PUINT64 addr = (PUINT64)((PUINT8)imageBuffer + base->VirtualAddress + reloc[i].Offset);
				UINT64 delta = *addr - nt->OptionalHeader.ImageBase + (PUINT8)imageBuffer;
				*addr = delta;
			}
			else if (reloc[i].Type == IMAGE_REL_BASED_HIGHLOW) //32位
			{
				PUINT32 addr = (PUINT32)((PUINT8)imageBuffer + base->VirtualAddress + reloc[i].Offset);
				UINT32 delta = *addr - nt->OptionalHeader.ImageBase + (PUINT8)imageBuffer;
				*addr = delta;
			}
		}

		base = (PIMAGE_BASE_RELOCATION)((PUINT8)base + base->SizeOfBlock);
	}

	return TRUE;
}

//获取到 LoadLibraryExW
ULONG64 ExportTableFuncByName(CHAR* pData, CHAR* funcName)
{
	PIMAGE_DOS_HEADER pHead = (PIMAGE_DOS_HEADER)pData;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pData + pHead->e_lfanew);
	PIMAGE_DATA_DIRECTORY pDir = (PIMAGE_DATA_DIRECTORY)&pNt->OptionalHeader.DataDirectory[0];

	PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)(pData + pDir->VirtualAddress);

	ULONG64 funcAddr = 0;

	for (INT i = 0; i < pExport->NumberOfNames; i++)
	{
		INT* funcAddress = pData + pExport->AddressOfFunctions;
		INT* names = pData + pExport->AddressOfNames;
		SHORT* fh = pData + pExport->AddressOfNameOrdinals;
		INT index = -1;
		CHAR* name = pData + names[i];

		if (strcmp(name, funcName) == 0)
		{
			index = fh[i];
		}

		if (index != -1)
		{
			funcAddr = pData + funcAddress[index];
			break;
		}
	}

	if (!funcAddr)
	{
		KdPrint(("没有找到函数%s\r\n", funcName));

	}
	else
	{
		KdPrint(("找到函数%s addr %p\r\n", funcName, funcAddr));
	}

	return funcAddr;
}

BOOLEAN UpdateIAT(CHAR* imageBuffer)
{
	if (!imageBuffer)
	{
		return FALSE;
	}

	PIMAGE_NT_HEADERS nt = RtlImageNtHeader(imageBuffer);

	PIMAGE_DATA_DIRECTORY dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	PIMAGE_IMPORT_DESCRIPTOR base = (PIMAGE_IMPORT_DESCRIPTOR)(imageBuffer + dir->VirtualAddress);

	BOOLEAN ret = TRUE;

	for (; base->Name; base++)
	{
		PUCHAR name  = (PUCHAR)(imageBuffer + base->Name);

		ULONG_PTR baseAddr = QueryModule(name, NULL);

		if (!baseAddr)
		{
			ret = FALSE;
			break;
		}

		PIMAGE_THUNK_DATA thunkName = (PIMAGE_THUNK_DATA)(imageBuffer + base->OriginalFirstThunk);
		PIMAGE_THUNK_DATA thunkFunc = (PIMAGE_THUNK_DATA)(imageBuffer + base->FirstThunk);

		for (; thunkName->u1.ForwarderString; ++thunkName, ++thunkFunc)
		{
			PIMAGE_IMPORT_BY_NAME funcName = (PIMAGE_IMPORT_BY_NAME)(imageBuffer + thunkName->u1.AddressOfData);

			ULONG_PTR func = ExportTableFuncByName((char*)baseAddr, funcName->Name);

			if (func)
			{
				thunkFunc->u1.Function = (ULONG_PTR)func;
			}
			else
			{
				ret = FALSE;
				break;
			}
		}

		if (!ret)
		{
			break;
		}
	}

	return ret;
}

BOOLEAN UpdateCookie(CHAR* imageBuffer)
{
	if (!imageBuffer)
	{
		return FALSE;
	}

	PIMAGE_NT_HEADERS nt = RtlImageNtHeader(imageBuffer);

	PIMAGE_DATA_DIRECTORY dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG];

	PIMAGE_LOAD_CONFIG_DIRECTORY config = (PIMAGE_LOAD_CONFIG_DIRECTORY)(dir->VirtualAddress + imageBuffer);

	*(PULONG_PTR)(config->SecurityCookie) += 10;

	return TRUE;
}

BOOLEAN LoadDriver(PUCHAR fileBuffer)
{
	PUCHAR imageBuffer = FileToImage(fileBuffer);
	if (!imageBuffer)
	{
		return FALSE;
	}

	BOOLEAN ret = FALSE;

	do 
	{
		ret = UpdateReloc(imageBuffer);
		if (!ret)
		{
			break;
		}
		ret = UpdateIAT(imageBuffer);
		if (!ret)
		{
			break;
		}

		//修复cookie
		ret = UpdateCookie(imageBuffer);
		if (!ret)
		{
			break;
		}

		//call 入口点
		PIMAGE_NT_HEADERS nt = RtlImageNtHeader(imageBuffer);
		ULONG_PTR entry = nt->OptionalHeader.AddressOfEntryPoint;

		DriverEntryProc entryProc = (DriverEntryProc)(imageBuffer + entry);

		NTSTATUS status = entryProc(NULL, NULL);

		if (!NT_SUCCESS(status))
		{
			ret = FALSE;
			break;
		}

		//把PE头给去掉
		memset(imageBuffer, 0, PAGE_SIZE);

	} while (0);

	if (imageBuffer)
	{
		ExFreePool(imageBuffer);
	}

	return ret;
}