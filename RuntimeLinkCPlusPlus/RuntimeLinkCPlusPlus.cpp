﻿#include "RuntimeLinkCPlusPlus.h"
#ifdef RLCPP_Enable_Dynamic_Link

#include <windows.h>
#include <imagehlp.h>
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#pragma comment(lib, "imagehlp.lib")
#pragma warning(disable: 4996) // _s じゃない CRT 関数使うとでるやつ


namespace rlcpp {

namespace stl = std;


#define istPrint(...) DebugPrint(__VA_ARGS__)

template<size_t N>
inline int istvsprintf(char (&buf)[N], const char *format, va_list vl)
{
    return _vsnprintf(buf, N, format, vl);
}

static const int DPRINTF_MES_LENGTH  = 4096;
void DebugPrintV(const char* fmt, va_list vl)
{
    char buf[DPRINTF_MES_LENGTH];
    istvsprintf(buf, fmt, vl);
    ::OutputDebugStringA(buf);
}

void DebugPrint(const char* fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    DebugPrintV(fmt, vl);
    va_end(vl);
}

bool InitializeDebugSymbol(HANDLE proc=::GetCurrentProcess())
{
    if(!::SymInitialize(proc, NULL, TRUE)) {
        return false;
    }
    ::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

    return true;
}

bool MapFile(const stl::string &path, stl::vector<char> &data)
{
    if(FILE *f=fopen(path.c_str(), "rb")) {
        fseek(f, 0, SEEK_END);
        size_t size = ftell(f);
        if(size > 0) {
            data.resize(size);
            fseek(f, 0, SEEK_SET);
            fread(&data[0], 1, size, f);
        }
        fclose(f);
        return true;
    }
    return false;
}

void MakeExecutable(void *p, size_t size)
{
    DWORD old_flag;
    VirtualProtect(p, size, PAGE_EXECUTE_READWRITE, &old_flag);
}

void* FindSymbolInExe(const char *name)
{
    char buf[sizeof(SYMBOL_INFO)+MAX_PATH];
    PSYMBOL_INFO sinfo = (PSYMBOL_INFO)buf;
    sinfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    sinfo->MaxNameLen = MAX_PATH;
    if(SymFromName(::GetCurrentProcess(), name, sinfo)==FALSE) {
        return NULL;
    }
    return (void*)sinfo->Address;
}



class ObjLoader;

class ObjFile
{
public:
    ObjFile(ObjLoader *loader) : m_loader(loader) {}

    bool load(const stl::string &path);
    void unload();

    // 外部シンボルのリンケージ解決
    void link();

    void* findSymbol(const char *name);

    // f: functor [](const stl::string &symbol_name, const void *data)
    template<class F>
    void eachSymbol(const F &f)
    {
        for(SymbolTable::iterator i=m_symbols.begin(); i!=m_symbols.end(); ++i) {
            f(i->first, i->second);
        }
    }

private:
    typedef stl::map<stl::string, void*> SymbolTable;
    stl::vector<char> m_data;
    stl::string m_filepath;
    SymbolTable m_symbols;
    ObjLoader *m_loader;
};


class ObjLoader
{
public:
    ObjLoader();
    ~ObjLoader();

    // .obj のロードを行う。
    // 既に読まれているファイルを指定した場合リロード処理を行う。
    void load(const stl::string &path);

    // 依存関係の解決処理。ロード後実行前に必ず呼ぶ必要がある。
    // load の中で link までやってもいいが、.obj の数が増えるほど無駄が多くなる上、
    // 未解決シンボルを判別しづらくなるので手順を分割した。
    void link();

    // ロード済み obj 検索
    ObjFile* findObj(const stl::string &path);

    // 全ロード済み obj からシンボルを検索
    void* findSymbol(const stl::string &name);

    // exe 側 obj 側問わずシンボルを探す。link 処理用
    void* resolveExternalSymbol(const stl::string &name);

private:
    typedef stl::map<stl::string, ObjFile*> ObjTable;
    typedef stl::map<stl::string, void*> SymbolTable;

    ObjTable m_objs;
    SymbolTable m_symbols;
};



void ObjFile::unload()
{
    m_data.clear();
    m_filepath.clear();
    m_symbols.clear();
}

bool ObjFile::load(const stl::string &path)
{
    m_filepath = path;
    if(!MapFile(path, m_data)) {
        return false;
    }

    size_t ImageBase = (size_t)(&m_data[0]);
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)ImageBase;
    if( pDosHeader->e_magic!=IMAGE_FILE_MACHINE_I386 || pDosHeader->e_sp!=0 ) {
        return false;
    }

    // 以下 symbol 収集処理
    PIMAGE_FILE_HEADER pImageHeader = (PIMAGE_FILE_HEADER)ImageBase;
    PIMAGE_OPTIONAL_HEADER *pOptionalHeader = (PIMAGE_OPTIONAL_HEADER*)(pImageHeader+1);

    PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)(ImageBase + sizeof(IMAGE_FILE_HEADER) + pImageHeader->SizeOfOptionalHeader);
    PIMAGE_SYMBOL pSymbolTable = (PIMAGE_SYMBOL)((size_t)pImageHeader + pImageHeader->PointerToSymbolTable);
    DWORD SymbolCount = pImageHeader->NumberOfSymbols;

    PSTR StringTable = (PSTR)&pSymbolTable[SymbolCount];

    for( size_t i=0; i < SymbolCount; ++i ) {
        IMAGE_SYMBOL &sym = pSymbolTable[i];
        if(sym.N.Name.Short == 0 && sym.SectionNumber>0) {
            IMAGE_SECTION_HEADER &sect = pSectionHeader[sym.SectionNumber-1];
            const char *name = (const char*)(StringTable + sym.N.Name.Long);
            void *data = (void*)(ImageBase + sect.PointerToRawData + sym.Value);
            if(sym.SectionNumber!=IMAGE_SYM_UNDEFINED) {
                MakeExecutable(data, sect.SizeOfRawData);
                m_symbols[name] = data;
            }
        }
        i += pSymbolTable[i].NumberOfAuxSymbols;
    }

    return true;
}

// 外部シンボルのリンケージ解決
void ObjFile::link()
{
    size_t ImageBase = (size_t)(&m_data[0]);
    PIMAGE_FILE_HEADER pImageHeader = (PIMAGE_FILE_HEADER)ImageBase;
    PIMAGE_OPTIONAL_HEADER *pOptionalHeader = (PIMAGE_OPTIONAL_HEADER*)(pImageHeader+1);

    PIMAGE_SECTION_HEADER pSectionHeader = (PIMAGE_SECTION_HEADER)(ImageBase + sizeof(IMAGE_FILE_HEADER) + pImageHeader->SizeOfOptionalHeader);
    PIMAGE_SYMBOL pSymbolTable = (PIMAGE_SYMBOL)((size_t)pImageHeader + pImageHeader->PointerToSymbolTable);
    DWORD SymbolCount = pImageHeader->NumberOfSymbols;

    PSTR StringTable = (PSTR)&pSymbolTable[SymbolCount];

    for( size_t i=0; i < SymbolCount; ++i ) {
        IMAGE_SYMBOL &sym = pSymbolTable[i];
        if(sym.N.Name.Short == 0 && sym.SectionNumber>0) {
            IMAGE_SECTION_HEADER &sect = pSectionHeader[sym.SectionNumber-1];
            const char *name = (const char*)(StringTable + sym.N.Name.Long);
            size_t SectionBase = (size_t)(ImageBase + sect.PointerToRawData + sym.Value);

            DWORD NumRelocations = sect.NumberOfRelocations;
            PIMAGE_RELOCATION pRelocation = (PIMAGE_RELOCATION)(ImageBase + sect.PointerToRelocations);
            for(size_t ri=0; ri<NumRelocations; ++ri) {
                PIMAGE_RELOCATION pReloc = pRelocation + ri;
                PIMAGE_SYMBOL rsym = pSymbolTable + pReloc->SymbolTableIndex;
                const char *rname = (const char*)(StringTable + rsym->N.Name.Long);
                void *rdata = m_loader->resolveExternalSymbol(rname);
                if(rdata==NULL) {
                    istPrint("!danger! %s: シンボル %s を解決できませんでした。\n", m_filepath.c_str(), rname);
                    continue;
                }
                // IMAGE_REL_I386_REL32 の場合相対アドレスに直す必要がある
                if(pReloc->Type==IMAGE_REL_I386_REL32) {
                    size_t rel = (size_t)rdata - SectionBase - pReloc->VirtualAddress - 4;
                    rdata = (void*)rel;
                }
                *(void**)(SectionBase + pReloc->VirtualAddress) = rdata;
            }
        }
        i += pSymbolTable[i].NumberOfAuxSymbols;
    }
}

void* ObjFile::findSymbol(const char *name)
{
    SymbolTable::iterator i = m_symbols.find(name);
    if(i == m_symbols.end()) { return NULL; }
    return i->second;
}



ObjLoader::ObjLoader()
{
}

ObjLoader::~ObjLoader()
{
    for(ObjTable::iterator i=m_objs.begin(); i!=m_objs.end(); ++i) {
        delete i->second;
    }
    m_objs.clear();
}

void ObjLoader::load( const stl::string &path )
{
    ObjFile *&loader = m_objs[path];
    if(loader==NULL) {
        loader = new ObjFile(this);
    }
    loader->eachSymbol([&](const stl::string &name, void *data){ m_symbols.erase(name); });
    loader->unload();
    loader->load(path);
    loader->eachSymbol([&](const stl::string &name, void *data){ m_symbols.insert(stl::make_pair(name, data)); });
}

void ObjLoader::link()
{
    for(ObjTable::iterator i=m_objs.begin(); i!=m_objs.end(); ++i) {
        i->second->link();
    }
}

ObjFile* ObjLoader::findObj( const stl::string &path )
{
    ObjTable::iterator i = m_objs.find(path);
    if(i==m_objs.end()) { return NULL; }
    return i->second;
}

void* ObjLoader::findSymbol( const stl::string &name )
{
    SymbolTable::iterator i = m_symbols.find(name);
    if(i==m_symbols.end()) { return NULL; }
    return i->second;
}

void* ObjLoader::resolveExternalSymbol( const stl::string &name )
{
    void *sym = findSymbol(name);
    if(sym==NULL) { sym=FindSymbolInExe(name.c_str()); }
    return sym;
}



ObjLoader *m_objs = NULL;

} // namespace rlcpp

using namespace rlcpp;

void  _RLCPP_InitializeLoader()
{
    if(m_objs==NULL) {
        InitializeDebugSymbol();
        m_objs = new ObjLoader();
    }
}

void  _RLCPP_FinalizeLoader()
{
    if(m_objs!=NULL) {
        delete m_objs;
        m_objs = NULL;
    }
}

void  _RLCPP_Load(const char *path)
{
    m_objs->load(path);
}

void  _RLCPP_Link()
{
    m_objs->link();
}

void* _RLCPP_FindSymbol(const char *name)
{
    return m_objs->findSymbol(name);
}

#endif // RLCPP_Enable_Dynamic_Link
