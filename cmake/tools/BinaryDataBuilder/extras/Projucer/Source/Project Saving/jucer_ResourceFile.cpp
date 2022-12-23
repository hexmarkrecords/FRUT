// Copyright (C) 2017-2019  Alain Martin
//
// This file is part of FRUT.
//
// FRUT is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FRUT is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FRUT.  If not, see <http://www.gnu.org/licenses/>.

// clang-format off

// Lines 30-97, 100-110, 114-128, 132-151, 154-161, 165-173, 177-244, 247-252, 255-271, and 275-289 of this file were copied from
// https://github.com/juce-framework/JUCE/blob/4.2.0/extras/Projucer/Source/Project%20Saving/jucer_ResourceFile.cpp

// Lines 292-316, 320-326, 330-344, and 348-365 of this file were copied from
// https://github.com/juce-framework/JUCE/blob/5.0.0/extras/Projucer/Source/Project%20Saving/jucer_ResourceFile.cpp

// Lines 368-392, 396-402, 406-420, 424-448, 452-458, 462-470, and 474-559 of this file were copied from
// https://github.com/juce-framework/JUCE/blob/5.3.2/extras/Projucer/Source/ProjectSaving/jucer_ResourceFile.cpp


/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include "../jucer_Headers.h"
#include "jucer_ResourceFile.h"


// Constants ===================================================================

constexpr const char *FILENAME_ENCRYPT_ME = ".encrypt-me";

static const char* resourceFileIdentifierString = "JUCER_BINARY_RESOURCE";

//==============================================================================

ResourceFile::ResourceFile (Project& p)
    : project (p),
      className ("BinaryData")
{
}

//==============================================================================

File ResourceFile::findEncryptionKeyFile (const File &file, const File &root)
{
    auto thisDirectory = file.isDirectory() ? file : file.getParentDirectory();
    auto encryptMeFile = thisDirectory.getChildFile(FILENAME_ENCRYPT_ME);

    if (encryptMeFile.exists())
    {
      return encryptMeFile;
    }
    else if (thisDirectory == root)
    {
      return {};
    }
    return findEncryptionKeyFile(thisDirectory.getParentDirectory(), root);
}

String ResourceFile::getEncryptionKey (const File &file, const File &root)
{
    auto encryptMeFile = findEncryptionKeyFile(file, root);

    if (encryptMeFile.exists())
    {
      MemoryBlock mb;
      encryptMeFile.loadFileAsData(mb);
      return mb.toString();
    }
    else
    {
      return {};
    }
}

//==============================================================================

void ResourceFile::setClassName (const String& name)
{
    className = name;
}

void ResourceFile::addFile (const File& file)
{
    files.add (file);

    const String variableNameRoot (CodeHelpers::makeBinaryDataIdentifierName (file));
    String variableName (variableNameRoot);

    int suffix = 2;
    while (variableNames.contains (variableName))
        variableName = variableNameRoot + String (suffix++);

    variableNames.add (variableName);
}

static String getComment()
{
    String comment;
    comment << newLine << newLine
            << "   This is an auto-generated file: Any edits you make may be overwritten!" << newLine
            << newLine
            << "*/" << newLine
            << newLine;

    return comment;
}

template <ProjucerVersion>
Result ResourceFile::writeHeader (MemoryOutputStream& header)
{
    const String headerGuard ("BINARYDATA_H_" + String (project.getProjectUID().hashCode() & 0x7ffffff) + "_INCLUDED");

    header << "/* ========================================================================================="
           << getComment()
           << "#ifndef " << headerGuard << newLine
           << "#define " << headerGuard << newLine
           << newLine
           << "namespace " << className << newLine
           << "{" << newLine;

    // bool containsAnyImages = false;

    for (int i = 0; i < files.size(); ++i)
    {
        const File& file = files.getReference(i);

        if (! file.existsAsFile())
            return Result::fail ("Can't open resource file: " + file.getFullPathName());

        const int64 dataSize = file.getSize();

        const String variableName (variableNames[i]);

        FileInputStream fileStream (file);

        if (fileStream.openedOk())
        {
            // containsAnyImages = containsAnyImages
            //                      || (ImageFileFormat::findImageFormatForStream (fileStream) != nullptr);

            header << "    extern const char*   " << variableName << ";" << newLine;
            header << "    const int            " << variableName << "Size = " << (int) dataSize << ";" << newLine << newLine;
        }
    }

    header << "    // Points to the start of a list of resource names." << newLine
           << "    extern const char* namedResourceList[];" << newLine
           << newLine
           << "    // Number of elements in the namedResourceList array." << newLine
           << "    const int namedResourceListSize = " << files.size() <<  ";" << newLine
           << newLine
           << "    // If you provide the name of one of the binary resource variables above, this function will" << newLine
           << "    // return the corresponding data and its size (or a null pointer if the name isn't found)." << newLine
           << "    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();" << newLine
           << "}" << newLine
           << newLine
           << "#endif" << newLine;

    return Result::ok();
}

template <ProjucerVersion>
Result ResourceFile::writeCpp (MemoryOutputStream& cpp, const File& headerFile, int& i, const int maxFileSize)
{
    const bool isFirstFile = (i == 0);

    cpp << "/* ==================================== " << resourceFileIdentifierString << " ===================================="
        << getComment()
        << "namespace " << className << newLine
        << "{" << newLine;

    // bool containsAnyImages = false;

    while (i < files.size())
    {
        const File& file = files.getReference(i);
        const String variableName (variableNames[i]);

        FileInputStream fileStream (file);

        if (fileStream.openedOk())
        {
            // containsAnyImages = containsAnyImages
            //                      || (ImageFileFormat::findImageFormatForStream (fileStream) != nullptr);

            const String tempVariable ("temp_binary_data_" + String (i));

            cpp  << newLine << "//================== " << file.getFileName() << " ==================" << newLine
                << "static const unsigned char " << tempVariable << "[] =" << newLine;

            {
                MemoryBlock data;
                fileStream.readIntoMemoryBlock (data);
                CodeHelpers::writeDataAsCppLiteral (data, cpp, true, true);
            }

            cpp << newLine << newLine
                << "const char* " << variableName << " = (const char*) " << tempVariable << ";" << newLine;
        }

        ++i;

        if (cpp.getPosition() > maxFileSize)
            break;
    }

    if (isFirstFile)
    {
        if (i < files.size())
        {
            cpp << newLine
                << "}" << newLine
                << newLine
                << "#include \"" << headerFile.getFileName() << "\"" << newLine
                << newLine
                << "namespace " << className << newLine
                << "{";
        }

        cpp << newLine
            << newLine
            << "const char* getNamedResource (const char*, int&) throw();" << newLine
            << "const char* getNamedResource (const char* resourceNameUTF8, int& numBytes) throw()" << newLine
            << "{" << newLine;

        StringArray returnCodes;
        for (int j = 0; j < files.size(); ++j)
        {
            const File& file = files.getReference(j);
            const int64 dataSize = file.getSize();
            returnCodes.add ("numBytes = " + String (dataSize) + "; return " + variableNames[j] + ";");
        }

        CodeHelpers::createStringMatcher (cpp, "resourceNameUTF8", variableNames, returnCodes, 4);

        cpp << "    numBytes = 0;" << newLine
            << "    return 0;" << newLine
            << "}" << newLine
            << newLine
            << "const char* namedResourceList[] =" << newLine
            << "{" << newLine;

        for (int j = 0; j < files.size(); ++j)
            cpp << "    " << variableNames[j].quoted() << (j < files.size() - 1 ? "," : "") << newLine;

        cpp << "};" << newLine;
    }

    cpp << newLine
        << "}" << newLine;

    return Result::ok();
}

template <ProjucerVersion jucerVersion>
Result ResourceFile::write (Array<File>& filesCreated, const int maxFileSize)
{
    const File headerFile (project.getBinaryDataHeaderFile());

    {
        MemoryOutputStream mo;
        Result r (writeHeader<jucerVersion> (mo));

        if (r.failed())
            return r;

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (headerFile, mo))
            return Result::fail ("Can't write to file: " + headerFile.getFullPathName());

        filesCreated.add (headerFile);
    }

    int i = 0;
    int fileIndex = 0;

    for (;;)
    {
        File cpp (project.getBinaryDataCppFile (fileIndex));

        MemoryOutputStream mo;

        Result r (writeCpp<jucerVersion> (mo, headerFile, i, maxFileSize));

        if (r.failed())
            return r;

        if (! FileHelpers::overwriteFileWithNewDataIfDifferent (cpp, mo))
            return Result::fail ("Can't write to file: " + cpp.getFullPathName());

        filesCreated.add (cpp);
        ++fileIndex;

        if (i >= files.size())
            break;
    }

    return Result::ok();
}


/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

template <>
Result ResourceFile::writeHeader<ProjucerVersion::v5_0_0> (MemoryOutputStream& header)
{
    header << "/* ========================================================================================="
           << getComment()
           << "#pragma once" << newLine
           << newLine
           << "namespace " << className << newLine
           << "{" << newLine;

    // bool containsAnyImages = false;

    for (int i = 0; i < files.size(); ++i)
    {
        const File& file = files.getReference(i);

        if (! file.existsAsFile())
            return Result::fail ("Can't open resource file: " + file.getFullPathName());

        const int64 dataSize = file.getSize();

        const String variableName (variableNames[i]);

        FileInputStream fileStream (file);

        if (fileStream.openedOk())
        {
            // containsAnyImages = containsAnyImages
            //                      || (ImageFileFormat::findImageFormatForStream (fileStream) != nullptr);

            header << "    extern const char*   " << variableName << ";" << newLine;
            header << "    const int            " << variableName << "Size = " << (int) dataSize << ";" << newLine << newLine;
        }
    }

    header << "    // Points to the start of a list of resource names." << newLine
           << "    extern const char* namedResourceList[];" << newLine
           << newLine
           << "    // Number of elements in the namedResourceList array." << newLine
           << "    const int namedResourceListSize = " << files.size() <<  ";" << newLine
           << newLine
           << "    // If you provide the name of one of the binary resource variables above, this function will" << newLine
           << "    // return the corresponding data and its size (or a null pointer if the name isn't found)." << newLine
           << "    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();" << newLine
           << "}" << newLine;

    return Result::ok();
}


/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

template <>
Result ResourceFile::writeHeader<ProjucerVersion::v5_3_1> (MemoryOutputStream& header)
{
    header << "/* ========================================================================================="
           << getComment()
           << "#pragma once" << newLine
           << newLine
           << "namespace " << className << newLine
           << "{" << newLine;

    // bool containsAnyImages = false;

    for (int i = 0; i < files.size(); ++i)
    {
        auto& file = files.getReference(i);

        if (! file.existsAsFile())
            return Result::fail ("Can't open resource file: " + file.getFullPathName());

        auto dataSize = file.getSize();

        auto variableName = variableNames[i];

        FileInputStream fileStream (file);

        if (fileStream.openedOk())
        {
            // containsAnyImages = containsAnyImages
            //                      || (ImageFileFormat::findImageFormatForStream (fileStream) != nullptr);

            auto encryptionKey = ResourceFile::getEncryptionKey(file, project.getProjectRoot()).toStdString();

            if (!encryptionKey.empty())
            {
                MemoryBlock data;
                fileStream.readIntoMemoryBlock (data);

                juce::BlowFish bf(encryptionKey.data(), static_cast<int>(encryptionKey.size()));

                // This section encrypts the payload.
                bf.encrypt(data);

                dataSize = static_cast<int64>(data.getSize());

                header << "    // (Encrypted)" << newLine;
            }

            header << "    extern const char*   " << variableName << ";" << newLine;
            header << "    const int            " << variableName << "Size = " << (int) dataSize << ";" << newLine << newLine;
        }
    }

    header << "    // Number of elements in the namedResourceList and originalFileNames arrays."                             << newLine
           << "    const int namedResourceListSize = " << files.size() <<  ";"                                               << newLine
           << newLine
           << "    // Points to the start of a list of resource names."                                                      << newLine
           << "    extern const char* namedResourceList[];"                                                                  << newLine
           << newLine
           << "    // Points to the start of a list of resource filenames."                                                  << newLine
           << "    extern const char* originalFilenames[];"                                                                  << newLine
           << newLine
           << "    // If you provide the name of one of the binary resource variables above, this function will"             << newLine
           << "    // return the corresponding data and its size (or a null pointer if the name isn't found)."               << newLine
           << "    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);"                       << newLine
           << newLine
           << "    // If you provide the name of one of the binary resource variables above, this function will"             << newLine
           << "    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found)."  << newLine
           << "    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);"                             << newLine
           << "}" << newLine;

    return Result::ok();
}

template <>
Result ResourceFile::writeCpp<ProjucerVersion::v5_3_1> (MemoryOutputStream& cpp, const File& headerFile, int& i, const int maxFileSize)
{
    bool isFirstFile = (i == 0);

    cpp << "/* ==================================== " << resourceFileIdentifierString << " ===================================="
        << getComment()
        << "namespace " << className << newLine
        << "{" << newLine;

    // bool containsAnyImages = false;

    while (i < files.size())
    {
        auto& file = files.getReference(i);
        auto variableName = variableNames[i];

        FileInputStream fileStream (file);

        if (fileStream.openedOk())
        {
            auto encryptionKey = ResourceFile::getEncryptionKey(file, project.getProjectRoot()).toStdString();

            // containsAnyImages = containsAnyImages
            //                      || (ImageFileFormat::findImageFormatForStream (fileStream) != nullptr);

            auto tempVariable = "temp_binary_data_" + String (i);

            cpp  << newLine
                 << "//================== "  << file.getFileName() << (encryptionKey.empty() ? "" : " (Encrypted)") << " ==================" << newLine
                 << "static const unsigned char " << tempVariable << "[] =" << newLine;

            {
                MemoryBlock data;
                fileStream.readIntoMemoryBlock (data);

                if (!encryptionKey.empty())
                {
                    juce::BlowFish bf(encryptionKey.data(), static_cast<int>(encryptionKey.size()));

                    // This section encrypts the payload.
                    bf.encrypt(data);
                }

                CodeHelpers::writeDataAsCppLiteral (data, cpp, true, true);
            }

            cpp << newLine << newLine
                << "const char* " << variableName << " = (const char*) " << tempVariable << ";" << newLine;
        }

        ++i;

        if (cpp.getPosition() > maxFileSize)
            break;
    }

    if (isFirstFile)
    {
        if (i < files.size())
        {
            cpp << newLine
                << "}" << newLine
                << newLine
                << "#include \"" << headerFile.getFileName() << "\"" << newLine
                << newLine
                << "namespace " << className << newLine
                << "{";
        }

        cpp << newLine
            << newLine
            << "const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)" << newLine
            << "{" << newLine;

        StringArray returnCodes;
        for (auto& file : files)
        {
            auto dataSize = file.getSize();
            returnCodes.add ("numBytes = " + String (dataSize) + "; return " + variableNames[files.indexOf (file)] + ";");
        }

        CodeHelpers::createStringMatcher (cpp, "resourceNameUTF8", variableNames, returnCodes, 4);

        cpp << "    numBytes = 0;" << newLine
            << "    return nullptr;" << newLine
            << "}" << newLine
            << newLine;

        cpp << "const char* namedResourceList[] =" << newLine
            << "{" << newLine;

        for (int j = 0; j < files.size(); ++j)
            cpp << "    " << variableNames[j].quoted() << (j < files.size() - 1 ? "," : "") << newLine;

        cpp << "};" << newLine << newLine;

        cpp << "const char* originalFilenames[] =" << newLine
            << "{" << newLine;

        for (auto& f : files)
            cpp << "    " << f.getFileName().quoted() << (files.indexOf (f) < files.size() - 1 ? "," : "") << newLine;

        cpp << "};" << newLine << newLine;

        cpp << "const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)"                         << newLine
            << "{"                                                                                                   << newLine
            << "    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)" << newLine
            << "    {"                                                                                               << newLine
            << "        if (namedResourceList[i] == resourceNameUTF8)"                                               << newLine
            << "            return originalFilenames[i];"                                                            << newLine
            << "    }"                                                                                               << newLine
            <<                                                                                                          newLine
            << "    return nullptr;"                                                                                 << newLine
            << "}"                                                                                                   << newLine
            <<                                                                                                          newLine;
    }

    cpp << "}" << newLine;

    return Result::ok();
}


template Result ResourceFile::write<ProjucerVersion::v4_2_0>(Array<File>&, const int);
template Result ResourceFile::write<ProjucerVersion::v5_0_0>(Array<File>&, const int);
template Result ResourceFile::write<ProjucerVersion::v5_3_1>(Array<File>&, const int);
