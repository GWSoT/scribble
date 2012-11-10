#include <windows.h>
#include <intrin.h>
#include <psapi.h>
#include <cstdio>
#pragma comment(lib, "psapi.lib")


// �w��̃A�h���X�����݂̃��W���[���� static �̈���ł���� true
bool IsStaticMemory(void *addr)
{
    // static �̈�̓��W���[�� (exe,dll) �� map ����Ă���̈���ɂ���
    // �������̂��ߌĂяo�������W���[���̂ݒ��ׂ�
    // �����W���[�������ׂ�ꍇ ::EnumProcessModules() �Ƃ����g��
    MODULEINFO modinfo;
    {
        HMODULE mod = 0;
        void *retaddr = *(void**)_AddressOfReturnAddress();
        ::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)retaddr, &mod);
        ::GetModuleInformation(::GetCurrentProcess(), mod, &modinfo, sizeof(modinfo));
    }
    return addr>=modinfo.lpBaseOfDll && addr<reinterpret_cast<char*>(modinfo.lpBaseOfDll)+modinfo.SizeOfImage;
}

// �w��A�h���X�����݂̃X���b�h�� stack �̈���ł���� true
bool IsStackMemory(void *addr)
{
    // Thread Information Block �ɏ��������񂪓����Ă���
    // (���ꂾ�ƌ��݂̃X���b�h�� stack �̈悵�����ʂł��Ȃ��B
    //  �ʃX���b�h�� stack �������ׂ����ꍇ�̂������@���悭�킩�炸�B
    //  ::Thread32First(), ::Thread32Next() �őS�v���Z�X�̑S�X���b�h�����񂷂邵���Ȃ��c�H)
    NT_TIB *tib = reinterpret_cast<NT_TIB*>(::NtCurrentTeb());
    return addr>=tib->StackLimit && addr<tib->StackBase;
}

// �w��̃A�h���X�� heap �̈���ł���� true
bool IsHeapMemory(void *addr)
{
    // static �̈�ł͂Ȃ� && stack �̈�ł��Ȃ� && �L���ȃ����� (::VirtualQuery() ����������) �Ȃ� true
    // ::HeapWalk() �ŏƍ�����̂���V�������A�v���[�`�����A
    // �������̕����������A�ʃX���b�h��ʃ��W���[������Ăяo�����̂łȂ���Ό��ʂ��������͂�
    MEMORY_BASIC_INFORMATION meminfo;
    return !IsStackMemory(addr) && !IsStaticMemory(addr) && ::VirtualQuery(addr, &meminfo, sizeof(meminfo));
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
result:

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
