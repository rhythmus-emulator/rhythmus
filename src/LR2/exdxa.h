#pragma once

#include <vector>
#include <string>

class DXAExtractor
{
public:
  DXAExtractor();
  ~DXAExtractor();
  bool Open(const char* path);
  void Close();

  struct DXAFile
  {
    char* p;
    int len;
    std::string filename;
  };

  typedef std::vector<DXAFile>::const_iterator DXAFileIter;

  DXAFileIter begin() const { return files_.cbegin(); }
  DXAFileIter end() const { return files_.cend(); }
private:
  std::vector<DXAFile> files_;
};