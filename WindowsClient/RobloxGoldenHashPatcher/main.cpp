#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <string.h>

int readFile(const char* fileName, std::vector<char>& buffer)
{
    std::ifstream file(fileName, std::ifstream::binary);
    if (file)
    {
        /*
         * Get the size of the file
         */
        file.seekg(0,std::ios::end);
        std::streampos length = file.tellg();
        file.seekg(0,std::ios::beg);

        /*
         * Use a vector as the buffer.
         * It is exception safe and will be tidied up correctly.
         * This constructor creates a buffer of the correct length.
         *
         * Then read the whole file into the buffer.
         */
        buffer.resize(length);
        file.read(&buffer[0],length);
    }
    else
    {
        return -1;
    }
    return buffer.size();
}

int writeFile(const char* fileName, const std::vector<char>& buffer)
{
    std::ofstream file(fileName, std::ifstream::binary);
    if (file)
    {
        file.write(&buffer[0],buffer.size());
    }
    else
    {
        return -1;
    }
    return buffer.size();
}

std::string getFileName(const std::string& sysPath, const std::string& fileName)
{
    return sysPath + '\\' + fileName;
}

std::string setCommand(const std::string& sysPath, const std::string& sysExe, const std::string& fileName)
{
    return getFileName(sysPath, sysExe) + " -w 195936478 --globalBasicSettingsPath " + fileName;
}

size_t findAndSetOffset(const std::vector<char>& exeBuffer, size_t idx, const char* search, size_t& location)
{
    if (strcmp(&exeBuffer[idx], search) == 0)
    {
        if (location != 0)
        {
            return -1;
        }
        location = idx;
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Patcher <Dir of exe> <Name of exe>\n";
        return 1;
    }

    std::vector<char> cmpBufferOne;
    std::vector<char> cmpBufferTwo;
    std::vector<char> cmpBufferGold;
    std::vector<char> exeBuffer;
    
    // Step 1: Run the program with the secret options.
    // "195936478" is the magic number = 0x0BADC0DE
    std::string sysPath(argv[1]);
    std::string sysExe(argv[2]);
    std::string sysCommand;
    std::string relFile;  // filename with path
    std::string baseFile; // filename without path
    std::string exeFile = getFileName(sysPath, sysExe);

    // Run player with special options to dump the .text section to "buffer1.bin"
    // The dump will write:
    // .text
    // 32b int = Golden Hash, calculated
    // 32b int = Golden Hash to compare.
    baseFile = "buffer1.bin";
    relFile = getFileName(sysPath, baseFile);
    sysCommand = setCommand(sysPath, sysExe, baseFile);
    std::cout << sysCommand << std::endl;
    system(sysCommand.c_str());
    int fileErrorCode = 0;
    fileErrorCode = readFile(relFile.c_str(), cmpBufferOne);
    if (fileErrorCode < 0)
    {
        std::cerr << " couldn't read buffer1.bin.\n";
        return 1;
    }
    const int kPadLen = sizeof(int) + sizeof(int);
    int codeLen = cmpBufferOne.size() - kPadLen;
    int bufferGhOffset = codeLen;               // calculated goldHash offset in buffer (what app calculates)
    int bufferRhOffset = codeLen + sizeof(int); // referenced goldHash offset in buffer (what app checks against)
    std::cout << "codeLen = " << codeLen << std::endl;

    // Step 2: find the location that is different.
    // VM protect has a pointer that is different in .text.  This will find the 
    // location.  Only run 3 times to find it.
    int maxIters = 3;
    int diffLocation = -1;
    baseFile = "buffer2.bin";
    relFile = getFileName(sysPath, baseFile);
    sysCommand = setCommand(sysPath, sysExe, baseFile);
    while (maxIters)
    {
        system(sysCommand.c_str());
        fileErrorCode = readFile(relFile.c_str(), cmpBufferTwo);
        if (fileErrorCode < 0)
        {
            std::cerr << "couldn't read buffer2.bin.\n";
            return 1;
        }
        // make sure only one place in the file is different.
        // if multiple are different, make sure ASLR is disabled
        // (note that .rdata also has vtbls from some dll's...)
        for (int i = 0; i < codeLen; ++i)
        {
            if(cmpBufferOne[i] != cmpBufferTwo[i])
            {
                if ((diffLocation >= 0) && ((i-diffLocation) > 4))
                {
                    std::cerr << "Multiple places have differences.\n";
                    return -1;
                }
                else
                {
                    diffLocation = i & 0xFFFFFFFC; // ignore 2 lsb
                }
            }
        }
        if (diffLocation >= 0)
        {
            break;
        }
        else
        {
            --maxIters;
        }
    }
    if (maxIters == 0)
    {
        std::cerr << "Could not find differences between runs.\n";
        return -1;
    }
    
    // Step 3: Find the locations to modify in the exe
    fileErrorCode = readFile(exeFile.c_str(), exeBuffer);
    if (fileErrorCode < 0)
    {
        std::cerr << "couldn't read executable.\n";
        return 1;
    }
    int exeLen = exeBuffer.size();

    // There is a common, unique prefix.  Check to ensure there is only 
    // one for maskAddr and one for goldHash
    size_t maskAddrOffset = 0;
    size_t goldHashOffset = 0;
    const char* magicMaskAddrCstr = "1(m$n9.[?y`z+f : a&Rn5<d*nGD9.@93Dr7&";
    const char* magicGoldHashCstr = "1N9S6,*%D-&m8sH%/~m _qvZ=&be*db elI^s";
    // Added the base/size to the patcher.
    const size_t imageBase = 0x00401000;
    const char magicImageBaseCstr[24] = {
        0x26, 0x53, 0x25, 0x58, 0x64, 0x40, 0x6E, 0x42, 0x34, 0x58, 0x30, 0x20,
        0x63, 0x18, 0x0A, 0x34, 0x75, 0x44, 0x26, 0x46, 0x35, 0x29, 0x68, 0x00};
    const char magicImageSizeCstr[24] = {
        0x35, 0x10, 0x0F, 0x72, 0x38, 0x52, 0x57, 0x43, 0x47, 0x20, 0x39, 0x33,
        0x23, 0x42, 0x66, 0x2A, 0x2E, 0x3C, 0x3B, 0x22, 0x5C, 0x73, 0x45, 0x00};
    size_t imageBaseOffset = 0;
    size_t imageSizeOffset = 0;
    bool useImageInfo = true;

    // I realize there are some better string matching algorithms out there.
    // the exe isn't that large.
    for (int i = 0; i < exeLen; ++i)
    {
        if (findAndSetOffset(exeBuffer, i, magicMaskAddrCstr, maskAddrOffset))
        {
            std::cerr << "maskAddrOffset is not unique!\n";
            return 1;
        }
        if (findAndSetOffset(exeBuffer, i, magicGoldHashCstr, goldHashOffset))
        {
            std::cerr << "goldHashOffset is not unique!\n";
            return 1;
        }
        if (findAndSetOffset(exeBuffer, i, magicImageBaseCstr, imageBaseOffset))
        {
            std::cerr << "imageBaseOffset is not unique!\n";
            useImageInfo = false;
        }
        if (findAndSetOffset(exeBuffer, i, magicImageSizeCstr, imageSizeOffset))
        {
            std::cerr << "imageSizeOffset is not unique!\n";
            useImageInfo = false;
        }
    }
    if (maskAddrOffset == 0)
    {
        std::cerr << "Unable to find maskAddr.\n";
        return 1;
    }
    if (goldHashOffset == 0)
    {
        std::cerr << "Unable to find goldHash.\n";
        return 1;
    }
    if (imageBaseOffset == 0)
    {
        std::cerr << "Unable to find imageBase.\n";
        useImageInfo = false;
    }
    if (imageSizeOffset == 0)
    {
        std::cerr << "Unable to find imageSize.\n";
        useImageInfo = false;
    }
    size_t alignedMaskAddrOffset = (maskAddrOffset | 7) + 1;
    size_t alignedGoldHashOffset = (goldHashOffset | 7) + 1;
    imageBaseOffset -= sizeof(size_t); // found byte 4 (on 32b)
    imageSizeOffset -= sizeof(size_t);
    std::cout << "diffLocation = " << std::hex << diffLocation << std::endl;
    std::cout << "maskAddr = " << std::hex << maskAddrOffset << std::endl;
    std::cout << "goldHash = " << std::hex << goldHashOffset << std::endl;
    std::cout << "imageBase = " << std::hex << imageBaseOffset << std::endl;
    std::cout << "imageSize = " << std::hex << imageSizeOffset << std::endl;

    // Step 4: Modify the maskAddr
    // The aligned versions are used in the application and place the values
    // on 8B boundaries.  I've noticed that it VC++ doesn't align strings in
    // all cases.
    //
    // Also update the imageBase and imageSize here.
    int* maskAddrPtr = (int*) &exeBuffer[alignedMaskAddrOffset];
    *maskAddrPtr = diffLocation;
    if (useImageInfo)
    {
        *((size_t*) &exeBuffer[imageBaseOffset]) = imageBase;
        *((size_t*) &exeBuffer[imageSizeOffset]) = codeLen;
    }
    fileErrorCode = writeFile(exeFile.c_str(), exeBuffer);
    if (fileErrorCode < 0)
    {
        std::cerr << "Failed to write first.\n";
        return -1;
    }

    // Step 5: Run the program to get the golden hash
    baseFile = "buffer2.bin";
    relFile = getFileName(sysPath, baseFile);
    sysCommand = setCommand(sysPath, sysExe, baseFile);
    system(sysCommand.c_str());
    fileErrorCode = readFile(relFile.c_str(), cmpBufferTwo);
    if (fileErrorCode < 0)
    {
        std::cerr << "couldn't read buffer2.bin.\n";
        return 1;
    }
    int goldHashValue = *((int*) (&cmpBufferTwo[bufferGhOffset]));

    // Step 6: Modify the goldHash and confirm the hash doesn't change
    int* goldHashPtr = (int*) &exeBuffer[alignedGoldHashOffset];
    *goldHashPtr = goldHashValue;
    fileErrorCode = writeFile(exeFile.c_str(), exeBuffer);
    if (fileErrorCode < 0)
    {
        std::cerr << "Failed to write first.\n";
        return -1;
    }
    baseFile = "buffer3.bin";
    relFile = getFileName(sysPath, baseFile);
    sysCommand = setCommand(sysPath, sysExe, baseFile);
    system(sysCommand.c_str());
    fileErrorCode = readFile(relFile.c_str(), cmpBufferGold);
    if (fileErrorCode < 0)
    {
        std::cerr << "couldn't read buffer3.bin.\n";
        return 1;
    }
    int goldHashCheck = *((int*) (&cmpBufferGold[bufferGhOffset]));  // the value reported by the 2nd run of the hash check.
    int goldHashRCheck = *((int*) (&cmpBufferGold[bufferRhOffset])); // the value reported as the golden hash in the code.
    std::cout << std::hex << goldHashCheck << " == " << std::hex << goldHashValue << " : " << std::hex << goldHashRCheck << std::endl;
    // This is the check to see if the previously recorded goldHash matches
    // both the calculated hash from the new run AND the goldHash that our app will check against.
    if ((goldHashValue != goldHashCheck) || (goldHashValue != goldHashRCheck))
    {
        std::cerr << "Error, golden hash didn't work!\n";
        return 1;
    }

    return 0;
}
