﻿#include <windows.h>
#include <intrin.h>
#include <psapi.h>
#include <cstdio>
#pragma comment(lib, "psapi.lib")


// 指定アドレスが有効なメモリ領域 (VirtualQery() が成功する && free 状態ではない) であれば true
// 読み込み or 書き込み不可 領域でも true を返す点に若干注意が必要
bool IsValidMemory(void *p)
{
    MEMORY_BASIC_INFORMATION meminfo;
    return p!=NULL && ::VirtualQuery(p, &meminfo, sizeof(meminfo))!=0 && meminfo.State!=MEM_FREE;
}


// 指定のアドレスが現在のモジュールの static 領域内であれば true
bool IsStaticMemory(void *addr)
{
    // static 領域はモジュール (exe,dll) が map されている領域内にある
    // 高速化のため呼び出し元モジュールのみ調べる
    // 他モジュールも調べる場合 ::EnumProcessModules() とかを使う
    MODULEINFO modinfo;
    {
        HMODULE mod = 0;
        void *retaddr = *(void**)_AddressOfReturnAddress();
        ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)retaddr, &mod);
        ::GetModuleInformation(::GetCurrentProcess(), mod, &modinfo, sizeof(modinfo));
    }
    return addr>=modinfo.lpBaseOfDll && addr<reinterpret_cast<char*>(modinfo.lpBaseOfDll)+modinfo.SizeOfImage;
}

// 指定アドレスが現在のスレッドの stack 領域内であれば true
bool IsStackMemory(void *addr)
{
    // Thread Information Block に上限下限情報が入っている
    // (これだと現在のスレッドの stack 領域しか判別できない。
    //  別スレッドの stack かも調べたい場合のいい方法がよくわからず。
    //  ::Thread32First(), ::Thread32Next() で全プロセスの全スレッドを巡回するしかない？)
    NT_TIB *tib = reinterpret_cast<NT_TIB*>(::NtCurrentTeb());
    return addr>=tib->StackLimit && addr<tib->StackBase;
}

// 指定のアドレスが heap 領域内であれば true
bool IsHeapMemory(void *addr)
{
    // static 領域ではない && stack 領域でもない && 有効なメモリなら true
    // ::HeapWalk() で照合するのが礼儀正しいアプローチだが、こっちの方が圧倒的に速い。
    // 代償として、別スレッドの stack 領域や別モジュールの static 領域への問い合わせは正しくない結果を返す。
    return !IsStackMemory(addr) && !IsStaticMemory(addr) && IsValidMemory(addr);
}


int main()
{
    static char static_memory[1024];
    char stack_memory[1024];
    char *heap_memory = (char*)malloc(1024);

    printf("IsStaticMemory(static_memory): %d\n", IsStaticMemory(static_memory));
    printf("IsStaticMemory(stack_memory): %d\n", IsStaticMemory(stack_memory));
    printf("IsStaticMemory(heap_memory): %d\n", IsStaticMemory(heap_memory));
    printf("\n");
    printf("IsStackMemory(static_memory): %d\n", IsStackMemory(static_memory));
    printf("IsStackMemory(stack_memory): %d\n", IsStackMemory(stack_memory));
    printf("IsStackMemory(heap_memory): %d\n", IsStackMemory(heap_memory));
    printf("\n");
    printf("IsHeapMemory(static_memory): %d\n", IsHeapMemory(static_memory));
    printf("IsHeapMemory(stack_memory): %d\n", IsHeapMemory(stack_memory));
    printf("IsHeapMemory(heap_memory): %d\n", IsHeapMemory(heap_memory));

    free(heap_memory);
}
/*
$ cl MemoryLocations.cpp && ./MemoryLocations

IsStaticMemory(static_memory): 1
IsStaticMemory(stack_memory): 0
IsStaticMemory(heap_memory): 0

IsStackMemory(static_memory): 0
IsStackMemory(stack_memory): 1
IsStackMemory(heap_memory): 0

IsHeapMemory(static_memory): 0
IsHeapMemory(stack_memory): 0
IsHeapMemory(heap_memory): 1
*/
