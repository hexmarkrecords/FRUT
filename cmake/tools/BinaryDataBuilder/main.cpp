// Copyright (C) 2016-2020  Alain Martin
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

#include "extras/Projucer/Source/jucer_Headers.h"

#include "extras/Projucer/Source/Project Saving/jucer_ResourceFile.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

/*
  ==============================================================================

   Utility to turn a bunch of binary files into a .cpp file and .h file full of
   data, so they can be built directly into an executable.

   It also allows for content to be encrypted by placing an .encrypt-me file
   that includes the secret cipher text so that this binary builder can
   perform encryption:

    root
    ├── anotherDir
    │   ├── .encrypt-me
    │   ├── anotherMessage.txt
    │   └── deepDir
    │       └── deepText.txt
    └── message.txt


   In the example above anotherMessage.txt and deepText.txt would both be
   encrypted using the cipher text found within the .encrypt-me file.
   Furthermore, message.txt would remain unencrypted.

   Use this code at your own risk! It carries no warranty!

  ==============================================================================
*/


//==============================================================================

int main(int argc, char* argv[])
{
  if (argc < 6)
  {
    std::cerr << "usage: BinaryDataBuilder"
              << " <Projucer-version>"
              << " <BinaryData-files-output-dir>"
              << " <Project-UID>"
              << " <BinaryData.cpp-size-limit>"
              << " <BinaryData-namespace>"
              << " <Project-list-dir>"
              << " <resource-files>..." << std::endl;
    return 1;
  }

  const std::vector<std::string> args{argv, argv + argc};

  using Version = std::tuple<int, int, int>;

  const auto jucerVersion = [&args]() {
    if (args.at(1) == "latest")
    {
      return Version{5, 3, 1};
    }

    const auto versionTokens = StringArray::fromTokens(String{args.at(1)}, ".", {});
    if (versionTokens.size() != 3)
    {
      std::cerr << "Invalid Projucer version" << std::endl;
      std::exit(1);
    }

    try
    {
      return Version{std::stoi(versionTokens[0].toStdString()),
                     std::stoi(versionTokens[1].toStdString()),
                     std::stoi(versionTokens[2].toStdString())};
    }
    catch (const std::invalid_argument&)
    {
      std::cerr << "Invalid Projucer version" << std::endl;
      std::exit(1);
    }
  }();


  // Here we record the project root
  // useful for a sensible stop for the recursive
  // hunt for any .encrypt-me files in the directory
  // ancestry.

  Project project{args.at(6), args.at(2), args.at(3)};

  const auto maxSize = [&args]() {
    try
    {
      return std::stoi(args.at(4));
    }
    catch (const std::invalid_argument&)
    {
      std::cerr << "Invalid size limit" << std::endl;
      std::exit(1);
    }
  }();

  ResourceFile resourceFile{project};
  resourceFile.setClassName(args.at(5));

  for (auto i = 7u; i < args.size(); ++i)
  {
    resourceFile.addFile(File{args.at(i)});
  }

  Array<File> binaryDataFiles;

  const auto result =
    jucerVersion < Version{5, 0, 0}
      ? resourceFile.write<ProjucerVersion::v4_2_0>(binaryDataFiles, maxSize)
      : jucerVersion < Version{5, 3, 1}
          ? resourceFile.write<ProjucerVersion::v5_0_0>(binaryDataFiles, maxSize)
          : resourceFile.write<ProjucerVersion::v5_3_1>(binaryDataFiles, maxSize);

  if (!result.wasOk())
  {
    std::cerr << result.getErrorMessage() << std::endl;
    return 1;
  }

  for (auto i = 0; i < binaryDataFiles.size(); ++i)
  {
    if (i != 0)
    {
      std::cout << ";";
    }

    std::cout << binaryDataFiles.getUnchecked(i).getFileName();
  }
  std::cout << std::flush;

  return 0;
}
