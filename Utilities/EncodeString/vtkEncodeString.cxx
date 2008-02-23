/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Encode a string in a C++ file from a text file.
// For example, it can be used to encode a GLSL source file (in Rendering or
// VolumeRendering) or an event log (in Widgets/Testing).

#include "vtkSystemIncludes.h" // for cout,endl
#include <vtkstd/string> 
#include <vtksys/ios/sstream>

class Output
{
public:
  Output()
    {
    }
  Output(const Output&);
  void operator=(const Output&);
  ~Output()
    {
    }
  vtksys_ios::ostringstream Stream;

  bool ProcessFile(const char *inputFile,
                   const char *stringName)
    {
    FILE *fp=fopen(inputFile, "r");
    if(!fp)
      {
      cout << "Cannot open file: " << inputFile
           << " (check path and permissions)" << endl;
      return false;
      }
    int ch;
    this->Stream << "// Define the " << stringName << " string." << endl
      << "//" << endl
      << "// Generated from file: " << inputFile << endl
      << "//" << endl
      << "const char* " << stringName << " =" 
      << endl << "\"";
    while ( ( ch = fgetc(fp) ) != EOF )
      {
      if ( ch == '\n' )
        {
        this->Stream << "\\n\"" << endl << "\"";
        }
      else if ( ch == '\\' )
        {
        this->Stream << "\\\\";
        }
      else if ( ch == '\"' )
        {
        this->Stream << "\\\"";
        }
      else if ( ch != '\r' )
        {
        this->Stream << static_cast<unsigned char>(ch);
        }
      }
    this->Stream << "\\n\";" << endl;
    fclose(fp);
    return true;
    }
};

int main(int argc,
         char *argv[])
{
  if(argc<4)
    {
    cout << "Encode a string in a C++ file from a text file." << endl;
    cout << "Usage: " << argv[0] << " <output-file> <input-path> <stringname>"
         << endl;
    return 1;
    }
  Output ot;
  ot.Stream << "// DO NOT EDIT." << endl
            << "// Generated by " << argv[0] << endl;
    
  vtkstd::string output = argv[1];
  vtkstd::string input = argv[2];
  
  if(!ot.ProcessFile(input.c_str(), argv[3]))
    {
    cout << "Problem generating c++ file from source text file: " <<
      input.c_str() << endl;
    return 1;
    }
  
  ot.Stream << endl;
    
  FILE *fp=fopen(output.c_str(),"w");
  if(!fp)
    {
    cout << "Cannot open output file: " << output.c_str() << endl;
    return 1;
    }
  fprintf(fp, "%s", ot.Stream.str().c_str());
  fclose(fp);
  return 0;
}
