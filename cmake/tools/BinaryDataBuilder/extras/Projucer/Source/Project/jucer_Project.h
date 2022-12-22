// Copyright (C) 2016-2019  Alain Martin
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

#pragma once


class Project
{

public:
  Project(const String &rootDir, const String& outputDir, const String& uid)
    : projectRoot(rootDir)
    , binaryDataFilesOutputDir{File::getCurrentWorkingDirectory().getChildFile(outputDir)}
    , projectUID{uid}
  {
  }

  const File& getProjectRoot() const
  {
    return projectRoot;
  }

  const String& getProjectUID() const
  {
    return projectUID;
  }

  File getBinaryDataCppFile(int index) const
  {
    if (index > 0)
    {
      return binaryDataFilesOutputDir.getChildFile("BinaryData" + String{index + 1}
                                                  + ".cpp");
    }

    return binaryDataFilesOutputDir.getChildFile("BinaryData.cpp");
  }

  File getBinaryDataHeaderFile() const
  {
    return binaryDataFilesOutputDir.getChildFile("BinaryData.h");
  }

private:

  const File projectRoot;

  const File binaryDataFilesOutputDir;

  const String projectUID;

};
