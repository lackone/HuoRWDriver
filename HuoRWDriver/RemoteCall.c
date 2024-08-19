#include "RemoteCall.h"
#include "SearchCode.h"
#include "Export.h"
#include "Tools.h"

typedef struct _FreeMemoryInfo
{
	WORK_QUEUE_ITEM workitem;
	HANDLE pid;
	ULONG64 IsExecuteAddr;
	ULONG64 FreeSize;
} FreeMemoryInfo, * PFreeMemoryInfo;

NTSTATUS NTAPI MyZwGetNextThread(
	__in HANDLE ProcessHandle,
	__in HANDLE ThreadHandle,
	__in ACCESS_MASK DesiredAccess,
	__in ULONG HandleAttributes,
	__in ULONG Flags,
	__out PHANDLE NewThreadHandle
	)
{
	typedef NTSTATUS(NTAPI* ZwGetNextThreadProc)(
		__in HANDLE ProcessHandle,
		__in HANDLE ThreadHandle,
		__in ACCESS_MASK DesiredAccess,
		__in ULONG HandleAttributes,
		__in ULONG Flags,
		__out PHANDLE NewThreadHandle
		);

	static ZwGetNextThreadProc MyZwGetNextThreadProc = NULL;

	if (!MyZwGetNextThreadProc)
	{
		//ZwGetNextThread函数不是导出的，我们需要先找到ZwGetNotificationResourceManager，然后往上减
		UNICODE_STRING name = { 0 };
		RtlInitUnicodeString(&name, L"ZwGetNextThread");
		MyZwGetNextThreadProc = (ZwGetNextThreadProc)MmGetSystemRoutineAddress(&name);

		if (!MyZwGetNextThreadProc)
		{
			UNICODE_STRING tmp = { 0 };
			RtlInitUnicodeString(&tmp, L"ZwGetNotificationResourceManager");
			PUCHAR func = (PUCHAR)MmGetSystemRoutineAddress(&tmp);

			func -= 0x50;

			for (int i = 0; i < 0x30; i++)
			{
				if (func[i] == 0x48 && func[i + 1] == 0x8B && func[i + 2] == 0xC4)
				{
					MyZwGetNextThreadProc = func + i;
					break;
				}
			}
		}

	}

	if (MyZwGetNextThreadProc)
	{
		return MyZwGetNextThreadProc(ProcessHandle, ThreadHandle, DesiredAccess, HandleAttributes, Flags, NewThreadHandle);
	}

	return STATUS_UNSUCCESSFUL;
}

PETHREAD GetProcessMainThread(PEPROCESS process)
{
	PETHREAD eThread = NULL;

	KAPC_STATE apc = { 0 };

	HANDLE hThread = NULL;

	KeStackAttachProcess(process, &apc);

	//第二个参数为NULL，获取的是主线程
	NTSTATUS status = MyZwGetNextThread(NtCurrentProcess(), NULL, THREAD_ALL_ACCESS, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, 0, &hThread);

	if (NT_SUCCESS(status))
	{
		status = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &eThread, NULL);

		if (!NT_SUCCESS(status))
		{
			eThread = NULL;
		}
	}

	KeUnstackDetachProcess(&apc);

	return eThread;
}

NTSTATUS MyPsSuspendThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL)
{
	typedef NTSTATUS (NTAPI* PsSuspendThreadProc)(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL);

	static PsSuspendThreadProc MyPsSuspendThreadProc = NULL;

	if (!MyPsSuspendThreadProc)
	{
		MyPsSuspendThreadProc = (PsSuspendThreadProc)searchCode("ntoskrnl.exe", "PAGE", "4C8BEA488BF133FF897C??65????????4C89??????6641???????48??????0F??488B01", -0x15llu);

	}

	if (MyPsSuspendThreadProc)
	{
		return MyPsSuspendThreadProc(Thread, PreviousSuspendCount);
	}

	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS MyPsResumeThread(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL)
{
	typedef NTSTATUS(NTAPI* PsResumeThreadProc)(IN PETHREAD Thread, OUT PULONG PreviousSuspendCount OPTIONAL);

	static PsResumeThreadProc MyPsResumeThreadProc = NULL;

	if (!MyPsResumeThreadProc)
	{
		MyPsResumeThreadProc = (PsResumeThreadProc)searchCode("ntoskrnl.exe", "PAGE", "405348???488BDAE8????4885DB74?890333C048???5BC3", 0);
	}

	if (MyPsResumeThreadProc)
	{
		return MyPsResumeThreadProc(Thread, PreviousSuspendCount);
	}

	return STATUS_NOT_IMPLEMENTED;
}

/**
 * 释放内存
 */
VOID ExFreeMemoryWorkItem(_In_ PVOID Parameter)
{
	PFreeMemoryInfo item = (PFreeMemoryInfo)Parameter;

	PEPROCESS process = NULL;

	NTSTATUS st = PsLookupProcessByProcessId(item->pid, &process);

	if (!NT_SUCCESS(st))
	{
		return;
	}

	if (PsGetProcessExitStatus(process) != STATUS_PENDING)
	{
		ObDereferenceObject(process);
		return;
	}

	//判断别人shellCode是否执行
	ULONG64 execVal = 0;
	SIZE_T retSize = 0;
	BOOLEAN isSuccess = FALSE;

	INT count = 0;

	while (1)
	{
		if (count > 10000)
		{
			break;
		}

		NTSTATUS status = MmCopyVirtualMemory(process, item->IsExecuteAddr, IoGetCurrentProcess(), &execVal, 8, KernelMode, &retSize);

		if (NT_SUCCESS(status) && execVal == 1)
		{
			isSuccess = TRUE;
			break;
		}

		KernelSleep(10, FALSE);
		count++;
	}

	KAPC_STATE apc = { 0 };
	KeStackAttachProcess(process, &apc);

	if (isSuccess)
	{
		PVOID BaseAddr = (PVOID)(item->IsExecuteAddr - 0x500);
		//释放内存
		ZwFreeVirtualMemory(NtCurrentProcess(), &BaseAddr, &item->FreeSize, MEM_RELEASE);
	}

	KeUnstackDetachProcess(&apc);

	ExFreePool(item);

	ObDereferenceObject(process);
}

NTSTATUS RemoteCall(HANDLE pid, PVOID shellCode, SIZE_T shellCodeSize)
{
	PEPROCESS process = NULL;

	NTSTATUS status = PsLookupProcessByProcessId(pid, &process);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (PsGetProcessExitStatus(process) != STATUS_PENDING)
	{
		ObDereferenceObject(process);
		return STATUS_UNSUCCESSFUL;
	}

	//获取主线程
	PETHREAD thread = GetProcessMainThread(process);

	//判断进程是86还是64
	PVOID wow64 = PsGetProcessWow64Process(process);
	BOOLEAN isWow64 = wow64 ? TRUE : FALSE;


	if (!thread)
	{
		ObDereferenceObject(process);
		return STATUS_UNSUCCESSFUL;
	}

	PUCHAR tmpShellCode = ExAllocatePool(PagedPool, shellCodeSize);
	if (tmpShellCode == NULL)
	{
		ObDereferenceObject(process);
		return STATUS_UNSUCCESSFUL;
	}
	memcpy(tmpShellCode, shellCode, shellCodeSize);


	KAPC_STATE apc = { 0 };
	KeStackAttachProcess(process, &apc);

	PUCHAR baseAddr = NULL;
	//多申请一个页
	SIZE_T size = shellCodeSize + PAGE_SIZE;

	do 
	{
		status = ZwAllocateVirtualMemory(NtCurrentProcess(), &baseAddr, 0, &size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		memset(baseAddr, 0, size);

		PUCHAR start = baseAddr + PAGE_SIZE;

		memcpy(start, tmpShellCode, shellCodeSize);

		//挂起 
		status = MyPsSuspendThread(thread, NULL);

		//恢复
		if (NT_SUCCESS(status))
		{
			if (isWow64)
			{
				// x86线程

				/*
				60              pushad
				B8 78563412     mov eax,12345678
				83EC 40         sub esp,40
				FFD0            call eax
				83C4 40         add esp,40
				B8 78563412     mov eax,12345678
				C700 01000000   mov dword ptr ds:[eax],1
				61              popad
				FF25 00000000   jmp dword ptr ds:[0]
				0000            add byte ptr ds:[eax],al
				0000            add byte ptr ds:[eax],al
				0000            add byte ptr ds:[eax],al
				0000            add byte ptr ds:[eax],al
				0000            add byte ptr ds:[eax],al
				*/
				char code[] = {
					0x60,
					0xB8, 0x78, 0x56, 0x34, 0x12, //要CALL的地址
					0x83, 0xEC, 0x40,
					0xFF, 0xD0,
					0x83, 0xC4, 0x40,
					0xB8, 0x78, 0x56, 0x34, 0x12, //是否执行的地址
					0xC7, 0x00,	0x01, 0x00, 0x00,0x00,  
					0x61,
					0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00,  //返回地址
					0x00, 0x00,
					0x00, 0x00,
					0x00, 0x00,
				};

				PUCHAR teb64 = (PUCHAR)PsGetThreadTeb(thread);

				//防止缺页
				SIZE_T retProc = NULL;
				MmCopyVirtualMemory(process, (PULONG64)(teb64 + 0x1488), process, (PULONG64)(teb64 + 0x1488), 8, UserMode, &retProc);

				//找到TEB64 中的 TlsSlots[1]
				PUCHAR wowContext = (PUCHAR) * (PULONG64)(teb64 + 0x1488);

				//要CALL的地址
				*(PULONG)&code[2] = start;
				//是否执行的地址
				*(PULONG)&code[15] = (ULONG)(start + 0x500);
				//返回地址
				*(PULONG)&code[32] = *(PULONG)(wowContext + 0x8B + 0x4);

				//把我们的shellcode复制到baseAddr
				memcpy(baseAddr, code, sizeof(code));

				//修改RIP
				*(PULONG)(wowContext + 0x8B + 0x4) = baseAddr;
			}
			else
			{
				// x64线程

				/*
				50											push  rax
				51                                          push  rcx
				52                                          push  rdx
				53                                          push  rbx
				55                                          push  rbp
				56                                          push  rsi
				57                                          push  rdi
				41 50                                       push  r8
				41 51                                       push  r9
				41 52                                       push  r10
				41 53                                       push  r11
				41 54                                       push  r12
				41 55                                       push  r13
				41 56                                       push  r14
				41 57                                       push  r15
				48 B8 99 89 67 45 23 01 00 00               mov  rax,0x0000012345678999
				48 81 EC A8 00 00 00                        sub  rsp,0x00000000000000A8
				FF D0                                       call  rax
				48 81 C4 A8 00 00 00                        add  rsp,0x00000000000000A8
				41 5F                                       pop  r15
				41 5E                                       pop  r14
				41 5D                                       pop  r13
				41 5C                                       pop  r12
				41 5B                                       pop  r11
				41 5A                                       pop  r10
				41 59                                       pop  r9
				41 58                                       pop  r8
				5F                                          pop  rdi
				5E                                          pop  rsi
				5D                                          pop  rbp
				5B                                          pop  rbx
				5A                                          pop  rdx
				59                                          pop  rcx
				48 B8 89 67 45 23 01 00 00 00               mov  rax,0x0000000123456789
				48 C7 00 01 00 00 00                        mov  qword ptr ds:[rax],0x0000000000000001
				58                                          pop  rax
				FF 25 00 00 00 00                           jmp  qword ptr ds:[PCHunter64.00000001403ABA27]
				00 00                                       add  byte ptr ds:[rax],al
				00 00                                       add  byte ptr ds:[rax],al
				00 00                                       add  byte ptr ds:[rax],al
				00 00                                       add  byte ptr ds:[rax],al
				*/
				char code[] = {
					0x50, //push  rax
					0x51, //push  rcx   
					0x52, //push  rdx
					0x53, //push  rbx												//
					0x55, 															//
					0x56, 															//
					0x57, 															//
					0x41, 0x50, 													//
					0x41, 0x51, 													//
					0x41, 0x52, 													//
					0x41, 0x53, 													//
					0x41, 0x54, 													//
					0x41, 0x55, 													//
					0x41, 0x56, 													//
					0x41, 0x57, 													//
					0x48, 0xB8, 0x99, 0x89, 0x67, 0x45, 0x23, 0x01, 0x00,0x00, 		//要CALL的地址
					0x48, 0x81, 0xEC, 0xA0, 0x00, 0x00, 0x00, 						//要16字节对齐
					0xFF, 0xD0, 													//
					0x48, 0x81, 0xC4, 0xA0, 0x00, 0x00, 0x00, 						//要16字节对齐
					0x41, 0x5F, 													//
					0x41, 0x5E,														//
					0x41, 0x5D, 													//
					0x41, 0x5C, 													//
					0x41, 0x5B, 													//
					0x41, 0x5A, 													//
					0x41, 0x59, 													//
					0x41, 0x58, 													//
					0x5F, 															//
					0x5E, 															//
					0x5D, 															//
					0x5B, 															//
					0x5A,															//
					0x59, 															//
					0x48, 0xB8, 0x89, 0x67, 0x45, 0x23, 0x01, 0x00, 0x00, 0x00, 	//是否执行的地址
					0x48, 0xC7, 0x00, 0x01, 0x00, 0x00, 0x00, 0x58, 				//
					0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,								//FF25
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00					//返回地址
				};

				//先找到 InitialStack
				ULONG64 InitialStack = *(PULONG64)((PUCHAR)thread + 0x28);

				PKTRAP_FRAME ptrap = (PKTRAP_FRAME)(InitialStack - sizeof(KTRAP_FRAME));

				//要CALL的地址
				*(PULONG64)&code[25] = (ULONG64)start;
				//是否执行的地址
				*(PULONG64)&code[73] = (ULONG64)baseAddr + 0x500;
				//返回地址
				*(PULONG64)&code[95] = ptrap->Rip;

				//把我们的shellcode复制到baseAddr
				memcpy(baseAddr, code, sizeof(code));

				//修改RIP
				ptrap->Rip = baseAddr;
			}

			//创建一个工作队列，释放内存
			PFreeMemoryInfo item = (PFreeMemoryInfo)ExAllocatePool(NonPagedPool, sizeof(FreeMemoryInfo));

			item->IsExecuteAddr = baseAddr + 0x500;
			item->pid = pid;
			item->FreeSize = size;
			//初始化工作项
			ExInitializeWorkItem(&item->workitem, ExFreeMemoryWorkItem, item);
			//插入队列
			ExQueueWorkItem(&item->workitem, DelayedWorkQueue);

			MyPsResumeThread(thread, NULL);
		}

	} while (0);
	
	KeUnstackDetachProcess(&apc);

	ObDereferenceObject(thread);

	ObDereferenceObject(process);

	ExFreePool(tmpShellCode);

	return status;
}