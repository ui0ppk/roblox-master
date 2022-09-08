// This is version 2 of the golden hash patcher.  Version 1 was an external program.
// This new version has the advantage that I know the locations of all of the data.
// I can also call functions from my program.
// This makes this approach much more convenient.
//
// Todo: determine if the PE can be modified after write to remove .zero entirely.
#include "stdafx.h"
#include <windows.h>
#include <psapi.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include "Util/ProgramMemoryChecker.h"
#include "Util/CheatEngine.h"
#include "ReleasePatcher.h"
#include "Security/ApiSecurity.h"
#include "Network/NetPmc.h"

LOGVARIABLE(Zero,1)
#pragma optimize("s", on)

namespace {
struct MemRange
{
    uintptr_t base;
    size_t size;
    MemRange() : base(0), size(0) {}
    MemRange(uintptr_t base, size_t size) : base(base), size(size) {}
    static bool sizeCmpGreater(const MemRange& a, const MemRange& b)
    {
        return b.size < a.size;
    }
};

enum VmpRangeIdx
{
    kVmpPlain = 0,
    kVmp0Misc = 1,
    kVmpMutant = 2,
    kVmp1Misc = 3
};
// PE tools

typedef std::vector<IMAGE_SECTION_HEADER*> SectionPtrVector;

__declspec(code_seg(".zero")) bool getSectionInfo(const SectionPtrVector& sections, const char* name, uintptr_t& baseAddr, size_t& size)
{
    const size_t kPeSectionNameLimit = 9;
    for (size_t i = 0; i < sections.size(); ++i)
    {
        if (strncmp(reinterpret_cast<const char*>(sections[i]->Name), name, kPeSectionNameLimit) == 0) // 
        {
            baseAddr = sections[i]->VirtualAddress + reinterpret_cast<size_t>(GetModuleHandle(NULL));
            size = sections[i]->Misc.VirtualSize; // Virtual size ignores file alignment padding.
            return true;
        }
    }
    return false;
}

__declspec(code_seg(".zero")) int getSections(void* pImageBase, SectionPtrVector& sections)
{
    IMAGE_DOS_HEADER *pDosHdr = nullptr;
    IMAGE_SECTION_HEADER *pSection = nullptr;

    // Get DOS header
    pDosHdr = reinterpret_cast<IMAGE_DOS_HEADER*>(pImageBase);

    // File not a valid PE file
    if (pDosHdr->e_magic != IMAGE_DOS_SIGNATURE)
        return -1;

    // Get image header
    IMAGE_NT_HEADERS32* pImageHdr32 = reinterpret_cast<IMAGE_NT_HEADERS32*>(
        reinterpret_cast<BYTE*>(pDosHdr) + pDosHdr->e_lfanew);

    // File not a valid PE file
    if (pImageHdr32->Signature != IMAGE_NT_SIGNATURE)
        return -2;

    pSection = reinterpret_cast<IMAGE_SECTION_HEADER*>(pImageHdr32 + 1);

    // Sections
    for (int i = 0; i < pImageHdr32->FileHeader.NumberOfSections; ++i, ++pSection)
        sections.push_back( pSection );

    return 0;
}

// This is a heuristic -- checks for 32 0's at end of the last page.
__declspec(code_seg(".zero")) bool isStartOfSection(uintptr_t addr)
{
    static const unsigned char kZeros[32] = {};
    for (int i = 1; i <= 2; ++i)
    {
        if (0 == memcmp(reinterpret_cast<const char*>(addr) - i*sizeof(kZeros), &kZeros, sizeof(kZeros)))
        {
            return true;
        }
    }
    return false;
}

__declspec(code_seg(".zero")) bool getVmpSections(const SectionPtrVector& sections, std::vector<MemRange>& ranges, uintptr_t& vmpBase, size_t& vmpSize)
{
    // get the .vmp0 section
    if (!getSectionInfo(sections, ".vmp0", vmpBase, vmpSize))
    {
        FASTLOG(FLog::Zero, "couldn't find .vmp0");
        return false;
    }

    // Find the first gold section 
    // The section starts @ vmp base and ends at a section that starts with .idata.  I find this 
    // by looking for "EncodePointer" near the beginning of that section (heuristic)
    ranges.resize(4);
    std::string strBuffer;
    uintptr_t vmpAddr = vmpBase;
    uintptr_t vmpMiscBeginAddr = vmpAddr;
    bool endFound = false;
    while (vmpAddr < (vmpBase + vmpSize - 4096))
    {
        strBuffer = std::string(reinterpret_cast<const char*>(vmpAddr), 1024);
        if (strBuffer.find("EncodePointer") != strBuffer.npos)
        {
            ranges[kVmpPlain] = MemRange(vmpBase, vmpAddr-vmpBase);
            vmpMiscBeginAddr = vmpAddr;
            endFound = true;
            break;
        }
        vmpAddr += 4096;
    }
    if (!endFound)
    {
        FASTLOG(FLog::Zero, "failed first split.");
        ranges[kVmpPlain] = MemRange(vmpBase, 4);
    }
    
    // get the vmp1 section
    uintptr_t vmp1Base;
    size_t vmp1Size;
    if (!getSectionInfo(sections, ".vmp1", vmp1Base, vmp1Size))
    {
        FASTLOG(FLog::Zero, "couldn't find .vmp1");
        return false;
    }
    vmpSize = (vmp1Base+vmp1Size) - vmpBase;

    // Get the second gold section.
    // This is done by looking at the end of the vmp0 section to find the end of that
    // iat/rdata/rsrc/??? section from above.  the section will end with 32 aligned zeros 
    // near the end of the page.
    uintptr_t vmpMiscEndAddr = vmp1Base - 4096;
    vmpAddr = vmpMiscEndAddr;
    endFound = false;
    while ( vmpAddr > (vmpBase+4096))
    {
        if (isStartOfSection(vmpAddr))
        {
            vmpMiscEndAddr = vmpAddr;
            endFound = true;
            break;
        }
        vmpAddr -= 4096;
    }
    if (endFound)
    {
        // add the misc section, then the gold mutant section
        ranges[kVmp0Misc] = MemRange(vmpMiscBeginAddr, vmpMiscEndAddr - vmpMiscBeginAddr);
        ranges[kVmpMutant] = MemRange(vmpMiscEndAddr, vmp1Base - vmpMiscEndAddr);
    }
    else
    {
        // this could be an error, but for now I'll allow it.
        FASTLOG(FLog::Zero, "failed second split.");
        ranges[kVmp0Misc] = MemRange(vmpMiscBeginAddr, vmp1Base - vmpMiscBeginAddr);
        ranges[kVmpMutant] = MemRange(vmp1Base, 4);
    }

    // The second misc section.  
    ranges[kVmp1Misc] = MemRange(vmp1Base, vmp1Size);

    return true;
}

// File IO
__declspec(code_seg(".zero")) int readFile(const char* fileName, std::vector<char>& buffer)
{
    std::ifstream file(fileName, std::ifstream::binary);
    if (file.is_open())
    {
        file.seekg(0,std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0,std::ios::beg);

#pragma warning(disable : 4244)
        buffer.resize(length);
#pragma warning(default : 4244)
        file.read(&buffer[0],length);
        return buffer.size();
    }
    else
    {
        return 0;
    }
}

__declspec(code_seg(".zero")) int writeFile(const char* fileName, const std::vector<char>& buffer)
{
    std::ofstream file(fileName, std::ofstream::binary);
    if (file.is_open())
    {
        file.write(&buffer[0],buffer.size());
    }
    else
    {
        return -1;
    }
    return buffer.size();
}

__declspec(code_seg(".zero")) uintptr_t getFileOffsetOfVa(const SectionPtrVector& sections, uintptr_t addr)
{
    for (size_t i = 0; i < sections.size(); ++i)
    {
        if (addr - sections[i]->VirtualAddress < sections[i]->SizeOfRawData)
        {
            return (uintptr_t)(
                (addr - sections[i]->VirtualAddress) // relative to section
                + sections[i]->PointerToRawData);    // relative to buffer
        }
    }
    return 0;
}

__declspec(code_seg(".zero")) bool getImportThunkSection(const SectionPtrVector& sections, uintptr_t& iatBase, size_t& iatSize)
{
    iatBase = 0;
    iatSize = 0;
    uintptr_t rdataVa = 0;
    uintptr_t rdataRva = 0;
    size_t rdataSize = 0;
    for (size_t i = 0; i < sections.size(); ++i)
    {
        if (strncmp(reinterpret_cast<const char*>(sections[i]->Name), ".rdata", 7) == 0)
        {
            rdataVa = sections[i]->PointerToRawData;
            rdataRva = sections[i]->VirtualAddress + reinterpret_cast<uintptr_t>(GetModuleHandle(NULL));
            rdataSize = sections[i]->SizeOfRawData;
            break;
        }
    }

    // need to open the _original_ file (not the one modified by VMProtect)
    TCHAR name[MAX_PATH]; // assumed to be RobloxPlayerBeta.exe
    GetModuleFileName(GetModuleHandle(NULL), name, sizeof(name)/sizeof(TCHAR));
    std::string pathName(name);
    std::stringstream modifiedName;
    if (pathName.substr(pathName.size()-7) != "Raw.exe")
    {
        modifiedName << pathName.substr(0, pathName.size()-4) << "Raw.exe";
    }
    else
    {
        modifiedName << pathName;
    }
    std::vector<char> rawBinary;
    readFile(modifiedName.str().c_str(), rawBinary);
    if (rawBinary.empty())
    {
        return false;
    }

    // need to read RobloxPlayerBetaRaw to get the original table.
    // assume the .text and .rdata sections are in the same locations.
    // change ".rdata" to point to the locations in RobloxPlayerBetaRaw.exe
    DWORD base = reinterpret_cast<DWORD>(&rawBinary[0]);
    rdataVa = rdataVa + base;
    std::vector<BYTE> buffer(rdataSize);

    // get the import descriptor table -- the list of (non-delay) loaded dll's.
    // see https://msdn.microsoft.com/en-us/library/ms809762.aspx "PE File Imports"
    // Visual Studio 2012 seems to place the address table at the start of .rdata.
    DWORD* loc = 0;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)(&rawBinary[0]);
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)(dosHeader->e_lfanew + (char *)dosHeader);
    IMAGE_DATA_DIRECTORY* imageDataDir = ntHeader->OptionalHeader.DataDirectory;
    DWORD importVa = imageDataDir[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    IMAGE_IMPORT_DESCRIPTOR* idtEntry = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        getFileOffsetOfVa(sections, importVa) + base);

    // iterate over the import directory table (until null entry found)
    for(;idtEntry->Name; ++idtEntry)
    {
        // iterate over thunks until null entry found
        DWORD* thunk = reinterpret_cast<DWORD*>(getFileOffsetOfVa(sections, idtEntry->FirstThunk) + base);
        do
        {
            if (reinterpret_cast<uintptr_t>(thunk) - rdataVa < rdataSize)
            {
                size_t idx = reinterpret_cast<uintptr_t>(thunk) - rdataVa;
                memset(&buffer[idx], 0xFF, sizeof(void*));
            }
        } while(*(thunk++));
    }

    // Check that there is a single contiguous section
    uintptr_t firstThunkByte = rdataSize;
    uintptr_t lastThunkByte = rdataSize;
    size_t thunkSize = 0;
    for (size_t i = 0; i < rdataSize; ++i)
    {
        if (buffer[i])
        {
            firstThunkByte = (firstThunkByte == rdataSize) ? i : firstThunkByte;
            lastThunkByte = i;
            ++thunkSize;
        }
    }
    if (lastThunkByte+1 - firstThunkByte == thunkSize)
    {
        iatBase = rdataRva + firstThunkByte;
        iatSize = thunkSize;
        return true;
    }

    return false;
}

// Convenient ways to set values
class SectionMapping
{
    size_t loadedRva;
    size_t fileOffset;
public:
    __declspec(code_seg(".zero")) SectionMapping(size_t loadedRva, size_t fileOffset) : loadedRva(loadedRva), fileOffset(fileOffset) {};
    template<typename T> __declspec(code_seg(".zero")) void set(const volatile T* virtualAddr, T& value)
    {
        // Set in the PE file in RAM
        uintptr_t modAddr = (reinterpret_cast<uintptr_t>(virtualAddr) - loadedRva) // location relative to .rdata after loaded
            + fileOffset;                                                          // location of the filemapped .rdata
        T* ptr = reinterpret_cast<T*>(modAddr);
        *ptr = value;

        // Set in the current process
        T* evilAddr = const_cast<T*>(virtualAddr);
        *evilAddr = value;
    }
};


__declspec(code_seg(".zero")) bool updateNetPmcPartial(uintptr_t origBase, size_t origSize, RBX::Security::NetPmcChallenge* resultArray)
{
    uintptr_t base = origBase;
    size_t term = origBase+origSize;
    size_t stepAvg = (term-base+31)/32;
    for (int i = 0; i < 32; ++i)
    {
        size_t thisTerm = 32*((base+stepAvg+31)/32);
        size_t thisBase = (thisTerm < term) ? base : (base - (thisTerm - term));
        resultArray[i].base = thisBase;
        resultArray[i].size = (thisTerm < term) ? (thisTerm - thisBase) : (term - thisBase);
        resultArray[i].seed = i;
        resultArray[i].result = netPmcHashCheck(resultArray[i]);
        base += (resultArray[i].size);
    }
    return true;
}

__declspec(code_seg(".zero")) bool updateNetPmcResult(uint32_t key, RBX::Security::NetPmcChallenge* result)
{
    uint64_t msb = static_cast<uint64_t>(key) << 32;
    result->result = RBX::Security::teaEncrypt(msb | result->result);
    return true;
}

// this function modifies the code caves in the .text section to add references to a specific peice of code.
__declspec(code_seg(".zero")) bool addRefsToWcPage(uintptr_t textBase, size_t textSize, uintptr_t textFileBase)
{
    static const unsigned char kMov = 0xA3;
    static const unsigned char kInt3 = 0xCC;
    static const unsigned char kRet = 0xC3;
    static const unsigned char kRetPop = 0xC2;
    static const unsigned char kHotpatchFirst = 0x8B;
    static const unsigned char kPadding[8] = {kInt3, kInt3, kInt3, kInt3, kInt3, kInt3, kInt3, kInt3 };

    std::string textCopy(reinterpret_cast<char*>(textBase), textSize);
    size_t pos = 0;
    while ((pos = textCopy.find(reinterpret_cast<const char*>(kPadding), pos, sizeof(kPadding))) && pos != std::string::npos)
    {
        if ((pos > 3) // next checks are safe
            && ( (static_cast<unsigned char>(textCopy[pos-3]) == kRetPop)  || (static_cast<unsigned char>(textCopy[pos-1]) == kRet) ) // end of function
            && (pos + sizeof(kPadding) < textCopy.size()) // not at end of file
            && (static_cast<unsigned char>(textCopy[pos+sizeof(kPadding)]) != kHotpatchFirst) ) // not part of hotpatch
        {
            while( (pos < textCopy.size()) && (static_cast<unsigned char>(textCopy[pos]) == kInt3) && pos++ );
            pos -= 5;

            // update the file in memory
            *reinterpret_cast<unsigned char*>(textFileBase + pos) = kMov;
            *reinterpret_cast<uint32_t*>(textFileBase + pos + 1) = reinterpret_cast<uint32_t>(&RBX::writecopyTrap);

            // update the currently running program
            *reinterpret_cast<unsigned char*>(textBase + pos) = kMov;
            *reinterpret_cast<uint32_t*>(textBase + pos + 1) = reinterpret_cast<uint32_t>(&RBX::writecopyTrap);

            pos += 5;
        }
        else
        {
            pos+= sizeof(kPadding);
        }
    }
    return true;
}


__declspec(code_seg(".zero")) bool createUpdatedExe(HANDLE hChild)
{
    // get section info
    SectionPtrVector procSections;
    getSections(GetModuleHandleA(NULL), procSections);
    uintptr_t textBase;
    size_t textSize;
    if (!getSectionInfo(procSections, ".text", textBase, textSize))
    {
        FASTLOG(FLog::Zero, "couldn't find .text");
        return false;
    }

    uintptr_t rdataBase;
    size_t rdataSize;
    if (!getSectionInfo(procSections, ".rdata", rdataBase, rdataSize))
    {
        FASTLOG(FLog::Zero, "couldn't find .rdata");
        return false;
    }

    // generate some values to allow comparison to rdata section in a less obvious way
    uintptr_t textEndNeg = (~(textBase+textSize))+1;
    size_t textSizeNeg = (~textSize)+1;

    uintptr_t vmpBase;
    size_t vmpSize;
    std::vector<MemRange> vmpRanges; 
    if (!getVmpSections(procSections, vmpRanges, vmpBase, vmpSize))
    {
        FASTLOG(FLog::Zero, "couldn't find .vmp");
        return false;
    }

    // Get a copy of the .text
    DWORD amount = 0;
    std::vector<BYTE> buffer;
    buffer.resize(textSize);
    if (!ReadProcessMemory(hChild, reinterpret_cast<void*>(textBase), &buffer[0], textSize, &amount))
    {
        FASTLOG1(FLog::Zero, "RPM failed: 0x%08X", GetLastError());
        return false;
    }

    // Compare to mine
    // WARNING -- software breakpoints in .text will affect this.
    BYTE* mapOfText = reinterpret_cast<BYTE*>(textBase);
    int errCnt = 0;
    uintptr_t errLoc = 0;
    uintptr_t patchLoc;
    for (size_t i = 0; i < textSize; ++i)
    {
        if ((i-errLoc > 4) && (buffer[i] != mapOfText[i]))
        {
            ++errCnt;
            patchLoc = textBase + i;
            errLoc = i;
        }
    }
    if (errCnt == 0)
    {
        // This could actually happen and should cause a retry.
        FASTLOG(FLog::Zero, ".text is the same");
        return false;
    }
    else if (errCnt > 1)
    {
        FASTLOG1(FLog::Zero, ".text has %d differences", errCnt);
        return false;
    }

    // split the .text as needed
    // This allows some bytes past the end of where I think the DWORD difference is,
    // as well as some bytes before where I think the difference is.
    patchLoc &= ~3; // ignore 2 lsb (difference could be on any/all bytes)
    const size_t kUpperSlack = 8;
    const size_t kLowerSlack = 4;
    uintptr_t textUpperBase = patchLoc + kUpperSlack;
    size_t textUpperSize = textBase + textSize - textUpperBase;
    uintptr_t textLowerBase = textBase;
    size_t textLowerSize = textSize - (textUpperSize + kUpperSlack) - kLowerSlack;

    // read exe file into RAM
    TCHAR name[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), name, sizeof(name)/sizeof(TCHAR));
    std::vector<char> fileBuffer;
    int fileSize = readFile(name, fileBuffer);
    if (0 == fileSize)
    {
        FASTLOG(FLog::Zero, "can't read file");
        return false;
    }

    // get the file offset of the .rdata and .text sections
    SectionPtrVector fileSections;
    getSections(&fileBuffer[0], fileSections);
    uintptr_t rdataFileBase = 0;
    uintptr_t rdataFileRva = 0;
    uintptr_t textFileBase = 0;
    uintptr_t textFileRva = 0;
    for (size_t i = 0; i < fileSections.size(); ++i)
    {
        if (strncmp(reinterpret_cast<const char*>(fileSections[i]->Name), ".rdata", 7) == 0)
        {
            rdataFileBase = reinterpret_cast<uintptr_t>(fileSections[i]->PointerToRawData + &fileBuffer[0]);
            rdataFileRva = fileSections[i]->VirtualAddress + reinterpret_cast<size_t>(GetModuleHandle(NULL));
        }
        else if (strncmp(reinterpret_cast<const char*>(fileSections[i]->Name), ".text", 6) == 0)
        {
            textFileBase = reinterpret_cast<uintptr_t>(fileSections[i]->PointerToRawData + &fileBuffer[0]);
            textFileRva = fileSections[i]->VirtualAddress + reinterpret_cast<size_t>(GetModuleHandle(NULL));
        }
    }
    FASTLOG1(FLog::Zero, "rdata filebase = 0x%08X", rdataFileBase);
    FASTLOG1(FLog::Zero, "rdata filerva = 0x%08X", rdataFileRva);
    FASTLOG1(FLog::Zero, "text filebase = 0x%08X", textFileBase);
    FASTLOG1(FLog::Zero, "text filerva = 0x%08X", textFileRva);

    // Get the import address table
    uintptr_t iatBase = 0;
    uintptr_t iatSize = 0;
    if (!getImportThunkSection(fileSections, iatBase, iatSize))
    {
        // This part was speculative when I first added it.  Fail gracefully.
        iatBase = rdataBase;
        FASTLOG(FLog::Zero, "Cannot find Import Address Table");
    }

    uintptr_t rdataNoIatBase = rdataBase + iatSize;
    size_t rdataNoIatSize = rdataSize - iatSize;

    // update all of the values in .rdata
    // Section mapping updates the current process as well.
    // The IAT will not be modified, so it is safe to use the modified rdataBase/rdataSize.
    DWORD unused = 0; // not an optional argument.
    VirtualProtect(reinterpret_cast<void*>(rdataBase), rdataSize, PAGE_READWRITE, &unused);
    SectionMapping rdataMapping(rdataFileRva, rdataFileBase);
    rdataMapping.set(&RBX::Security::rbxTextBase, textBase);
    rdataMapping.set(&RBX::Security::rbxTextSize, textSize);
    rdataMapping.set(&RBX::Security::rbxLowerBase, textLowerBase);
    rdataMapping.set(&RBX::Security::rbxLowerSize, textLowerSize);
    rdataMapping.set(&RBX::Security::rbxUpperBase, textUpperBase);
    rdataMapping.set(&RBX::Security::rbxUpperSize, textUpperSize);
    rdataMapping.set(&RBX::Security::rbxRdataBase, rdataBase);
    rdataMapping.set(&RBX::Security::rbxRdataSize, rdataSize);
    rdataMapping.set(&RBX::Security::rbxIatBase, iatBase);
    rdataMapping.set(&RBX::Security::rbxIatSize, iatSize);
    rdataMapping.set(&RBX::Security::rbxRdataNoIatBase, rdataNoIatBase);
    rdataMapping.set(&RBX::Security::rbxRdataNoIatSize, rdataNoIatSize);
    rdataMapping.set(&RBX::Security::rbxVmpBase, vmpBase);
    rdataMapping.set(&RBX::Security::rbxVmpSize, vmpSize);
    rdataMapping.set(&RBX::Security::rbxVmpPlainBase, vmpRanges[kVmpPlain].base);
    rdataMapping.set(&RBX::Security::rbxVmpPlainSize, vmpRanges[kVmpPlain].size);
    rdataMapping.set(&RBX::Security::rbxVmpMutantBase, vmpRanges[kVmpMutant].base);
    rdataMapping.set(&RBX::Security::rbxVmpMutantSize, vmpRanges[kVmpMutant].size);
    rdataMapping.set(&RBX::Security::rbxVmp0MiscBase, vmpRanges[kVmp0Misc].base);
    rdataMapping.set(&RBX::Security::rbxVmp0MiscSize, vmpRanges[kVmp0Misc].size);
    rdataMapping.set(&RBX::Security::rbxVmp1MiscBase, vmpRanges[kVmp1Misc].base);
    rdataMapping.set(&RBX::Security::rbxVmp1MiscSize, vmpRanges[kVmp1Misc].size);
    rdataMapping.set(&RBX::Security::rbxTextEndNeg, textEndNeg);
    rdataMapping.set(&RBX::Security::rbxTextSizeNeg, textSizeNeg);

    // update the .text section padding here.
    VirtualProtect(reinterpret_cast<void*>(textBase), textSize, PAGE_EXECUTE_READWRITE, &unused);
    addRefsToWcPage(textBase, textSize, textFileBase);

    // Update the NetPmc values here.
    std::vector<RBX::Security::NetPmcChallenge> netPmcChallenges;
    netPmcChallenges.resize(RBX::Security::kNumChallenges);
    updateNetPmcPartial(textLowerBase, textLowerSize, &netPmcChallenges[ 0]);
    updateNetPmcPartial(textUpperBase, textUpperSize, &netPmcChallenges[32]);
    updateNetPmcPartial(vmpRanges[kVmpPlain].base, vmpRanges[kVmpPlain].size, &netPmcChallenges[64]);
    updateNetPmcPartial(vmpRanges[kVmpMutant].base, vmpRanges[kVmpMutant].size, &netPmcChallenges[96]);

    // shuffle them
    std::vector<unsigned char> randIdx;
    randIdx.resize(RBX::Security::kNumChallenges);
    for (unsigned char i = 0;i < RBX::Security::kNumChallenges; ++i)
    {
        randIdx[i] = i;
    }
    std::random_shuffle(randIdx.begin(), randIdx.end());

    // Stream Encryption
    std::vector<RBX::Security::NetPmcChallenge> salsaKey = RBX::Security::generateNetPmcKeys();
    for (unsigned char i = 0; i < RBX::Security::kNumChallenges; ++i)
    {
        updateNetPmcResult(randIdx[i], &netPmcChallenges[i]);
        netPmcChallenges[i] ^= salsaKey[randIdx[i]];
        rdataMapping.set(&RBX::Security::kChallenges[randIdx[i]], netPmcChallenges[i]);
    }

    // hash is auto computed in constructor
    RBX::ProgramMemoryChecker pmc;
    size_t newHash = pmc.getLastGoldenHash();
    rdataMapping.set(&RBX::Security::rbxGoldHash, newHash);

    // This has affected .rdata, so regenerate the hash.  This has updated the pmcHash global.
    RBX::ProgramMemoryChecker pmcForFile;

    // Write items of checkIdx,value,failMask for each check for this version of the client.
    // checkIdx is the index of the hash within the pmcHash.hashes vector.
    // value is the value that it should be.
    // failMask is the bitmask that will be or'd into the result.
    std::ofstream goldMemHashWriter("goldMemHash.txt", std::ofstream::binary);
    goldMemHashWriter << RBX::Hasher::kGoldHashStart << "," << RBX::pmcHash.hash[RBX::Hasher::kGoldHashStart] << "," << RBX::Hasher::kGoldHashStartFail << ";";
    goldMemHashWriter << RBX::Hasher::kGoldHashEnd << "," << RBX::pmcHash.hash[RBX::Hasher::kGoldHashEnd] << "," << RBX::Hasher::kGoldHashEndFail<< ";";
    goldMemHashWriter << RBX::Hasher::kRdataHash << "," << RBX::pmcHash.hash[RBX::Hasher::kRdataHash] << "," << RBX::Hasher::kRdataHashFail << ";";
    goldMemHashWriter << RBX::Hasher::kVmpPlainHash << "," << RBX::pmcHash.hash[RBX::Hasher::kVmpPlainHash] << "," << RBX::Hasher::kVmpPlainHashFail << ";";
    goldMemHashWriter << RBX::Hasher::kVmpMutantHash << "," << RBX::pmcHash.hash[RBX::Hasher::kVmpMutantHash] << "," << RBX::Hasher::kVmpMutantHashFail << ";";
    goldMemHashWriter << RBX::Hasher::kGoldHashStruct << "," << RBX::pmcHash.hash[RBX::Hasher::kGoldHashStruct] << "," << RBX::Hasher::kGoldHashStructFail;
    goldMemHashWriter.close();

    // Remove the .zero section from the final exe
    for (size_t i = 0; i < fileSections.size(); ++i)
    {
        if (strncmp(reinterpret_cast<const char*>(fileSections[i]->Name), ".zero", 5) == 0)
        {
            uintptr_t zeroFileBase = reinterpret_cast<uintptr_t>(fileSections[i]->PointerToRawData + &fileBuffer[0]);
            memset(reinterpret_cast<void*>(zeroFileBase), 0, fileSections[i]->SizeOfRawData); // SizeOfRawData is size in file.
            fileSections[i]->Characteristics &= ~(
                IMAGE_SCN_CNT_CODE 
                | IMAGE_SCN_CNT_INITIALIZED_DATA 
                | IMAGE_SCN_CNT_UNINITIALIZED_DATA
                | IMAGE_SCN_MEM_READ
                | IMAGE_SCN_MEM_WRITE
                | IMAGE_SCN_MEM_EXECUTE);
            break;
        }
    }

    // Write file back to disk.
    size_t nameSize = strnlen(name,MAX_PATH);
    strcpy_s(&name[nameSize-4], nameSize, ".tmp");
    FASTLOG1(FLog::Zero, "final file: %s", name);
    if (!writeFile(name, fileBuffer))
    {
        FASTLOG(FLog::Zero, "can't write file");
        return false;
    }

    return true;
}


} // namespace

namespace RBX { namespace Security {

// look into if the VMP section can be got from the create_suspended child.
__declspec(code_seg(".zero")) bool patchMain()
{
    TCHAR name[MAX_PATH];
    GetModuleFileName(GetModuleHandle(NULL), name, sizeof(name)/sizeof(TCHAR));
    FASTLOG1(FLog::Zero, "exe = %s", name);

    char* cli = " -w 10558381 ";
    FASTLOG1(FLog::Zero, "cli = %s", cli);
    int rerunCount = 2;

    // Call the EXE a second  time
    do 
    {
        PROCESS_INFORMATION childInfo;
        STARTUPINFO startInfo;
        ZeroMemory( &startInfo, sizeof(startInfo) );
        startInfo.cb = sizeof(startInfo);
        ZeroMemory( &childInfo, sizeof(childInfo) );
        if (!CreateProcess(name, 
            cli, 
            NULL, 
            NULL, 
            false, 
            CREATE_SUSPENDED, 
            NULL, 
            NULL, 
            &startInfo, 
            &childInfo)) 
        {
            FASTLOG1(FLog::Zero, "child failed to start = %s", cli);
            return false;
        }
        if (createUpdatedExe(childInfo.hProcess))
        {
            rerunCount = 0;
        }
        else
        {
            --rerunCount;
        }
        ResumeThread(childInfo.hThread);
        WaitForSingleObject(childInfo.hProcess, INFINITE); 
    } while (rerunCount > 0);

    return true;
}

} // ::Security
} // ::RBX

#pragma optimize("", on)
