#include "Util.h"
#include "rparser.h"
#include "common.h"

# include <sys/types.h>
# include <sys/stat.h>
#ifdef WIN32
# include <Windows.h>
# define stat _wstat
#else
# include <dirent.h>
# include <unistd.h>
#endif

namespace rhythmus
{

/* copied from rparser/rutil */
  
#ifdef WIN32
bool GetDirectoryItems(const std::string& dirpath, std::vector<DirItem>& out)
{
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATAW ffd;
  std::wstring spec, wpath, mask=L"*";
  std::stack<std::wstring> directories; // currently going-to-search directories
  std::stack<std::wstring> dir_name;    // name for current directories
  std::wstring curr_dir_name;

  rutil::DecodeToWStr(dirpath, wpath, rutil::E_UTF8);
  directories.push(wpath);
  dir_name.push(L"");

  while (!directories.empty()) {
    wpath = directories.top();
    spec = wpath + L"\\" + mask;
    curr_dir_name = dir_name.top();
    directories.pop();
    dir_name.pop();

    hFind = FindFirstFileW(spec.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)  {
      return false;
    } 

    do {
      if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
      {
        int is_file = 0;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          /// will not recursive currently...
          //directories.push(wpath + L"\\" + ffd.cFileName);
          //dir_name.push(curr_dir_name + ffd.cFileName + L"\\");
        }
        else {
          is_file = 1;
        }
        std::string fn;
        std::wstring wfn = curr_dir_name + ffd.cFileName;
        struct _stat result;
        stat(wfn.c_str(), &result);
        rutil::EncodeFromWStr(wfn, fn, rutil::E_UTF8);
        out.push_back({
          fn, is_file, result.st_mtime
          });
      }
    } while (FindNextFileW(hFind, &ffd) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
      FindClose(hFind);
      return false;
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;
  }

  return true;
}
#else
bool GetDirectoryItems(const std::string& path, std::vector<DirItem>& out)
{
  DIR *dp;
  struct dirent *dirp;
  std::stack<std::string> directories;
  std::stack<std::string> dir_name;

  directories.push(path);
  dir_name.push("");

  std::string curr_dir_name;
  std::string curr_dir;

  while (directories.size() > 0)
  {
    curr_dir = directories.top();
    curr_dir_name = dir_name.top();
    directories.pop();
    dir_name.pop();
    if (curr_dir_name == "./" || curr_dir_name == "../")
      continue;
    if((dp  = opendir(curr_dir.c_str())) == NULL) {
      printf("Error opening %s dir.\n", curr_dir.c_str());
      return false;
    }
    while ((dirp = readdir(dp)) != NULL) {
      int is_file = 0;
      if (dirp->d_type == DT_DIR)
      {
        //directories.push(curr_dir + "/" + dirp->d_name);
        //dir_name.push(curr_dir_name + dirp->d_name + "/");
      }
      else if (dirp->d_type == DT_REG)
      {
        is_file = 1;
      }
      std::string fn = curr_dir_name + std::string(dirp->d_name);
      struct stat result;
      stat(fn.c_str(), &result);
      out.push_back({
        fn, is_file, result.st_mtime
        });
    }
    closedir(dp);
  }

  return true;
}
#endif

std::string GetExtension(const std::string& path)
{
  return rutil::GetExtension(path);
}

std::string Upper(const std::string &s)
{
  return rutil::upper(s);
}

std::string Lower(const std::string &s)
{
  return rutil::lower(s);
}

std::string Substitute(const std::string& path_, const std::string& startswith, const std::string& relplacewith)
{
  std::string path = path_;
  for (unsigned i = 0; i < path.size(); ++i)
    if (path[i] == '\\') path[i] = '/';
  if (strncmp(path.c_str(), "./", 2) == 0)
    path = path.substr(2);
  if (strnicmp(path.c_str(),
    startswith.c_str(),
    startswith.size()) == 0)
  {
    path = relplacewith + path.substr(startswith.size());
  }
  return path;
}

std::string Replace(const std::string& org, const std::string& target, const std::string& replaceto)
{
  if (target.empty())
    return org;

  std::string s;
  for (unsigned i = 0; i < org.size(); ++i)
  {
    if (strcmp(org.c_str(), target.c_str()) == 0)
    {
      s += replaceto;
      i += target.size() - 1;
      continue;
    }
    s.push_back(org[i]);
  }
  return s;
}

bool startsWith(const std::string &s, const std::string &pattern)
{
  return s.size() >= pattern.size()
    && std::equal(pattern.begin(), pattern.end(), s.begin());
}

bool endsWith(const std::string &s, const std::string &pattern)
{
  return s.size() >= pattern.size()
    && std::equal(pattern.rbegin(), pattern.rend(), s.rbegin());
}

void Split(const std::string& str, char sep, std::vector<std::string>& vsOut)
{
  rutil::split(str, sep, vsOut);
}

void Split(const std::string& str, char sep, std::string &s1, std::string &s2)
{
  rutil::split(str, sep, s1, s2);
}

bool IsFile(const std::string& path)
{
  return rutil::IsFile(path);
}

void GetDirectoriesFromPath(const std::string& dir_path, std::vector<std::string> &out)
{
  std::vector<DirItem> diritem;
  GetDirectoryItems(dir_path, diritem);
  for (auto item : diritem)
  {
    if (!item.is_file)
      out.push_back(item.filename);
  }
}

void GetFilesFromPath(const std::string& file_or_filter_path, std::vector<std::string> &out)
{
  // check is target path is masking
  // if not masking, then consider it as directory
  auto i = file_or_filter_path.find('*');
  if (i == std::string::npos)
  {
    std::vector<DirItem> diritem;
    GetDirectoryItems(file_or_filter_path, diritem);
    for (auto item : diritem)
    {
      if (item.is_file)
        out.push_back(item.filename);
    }
  }
  else
  {
    rutil::GetDirectoryEntriesMasking(file_or_filter_path, out, false);
  }
}

void GetFilesFromDirectory(const std::string& dir, std::vector<std::string> &out, int depth)
{
  std::string dir_ = dir;
  if (dir_.back() != '/')
    dir_.push_back('/');

  rutil::DirFileList files;
  if (rutil::GetDirectoryFiles(dir_, files, depth, true))
  {
    for (auto &file : files)
    {
      /* path must be regularized (not only for filtering) */
      for (size_t i = 0; i < file.first.size(); ++i)
        if (file.first[i] == '\\') file.first[i] = '/';

      out.push_back(dir_ + file.first);
    }
  }
}

bool GetFilepathSmart(const std::string& file_or_filter_path, std::string& out, int index)
{
  std::vector<std::string> name_entries;
  GetFilesFromPath(file_or_filter_path, name_entries);
  if (name_entries.empty())
    return false;
  if (index < 0)
  {
    // random select
    index = rand();
  }
  index %= name_entries.size();
  out = name_entries[index];
  return true;
}

bool GetFilepathSmartFallback(const std::string& file_path, const std::string& fallback, std::string& out, int index)
{
  if (IsFile(file_path))
  {
    out = file_path;
    return true;
  }
  else return GetFilepathSmart(fallback, out, index);
}

void MakeParamCountSafe(std::vector<std::string> &v, size_t expected_count)
{
  while (v.size() < expected_count)
    v.emplace_back(std::string());
}

void MakeParamCountSafe(const std::string& in,
  std::vector<std::string> &vsOut, size_t required_size)
{
  Split(in, ',', vsOut);
  if (required_size < 0) return;
  for (auto i = vsOut.size(); i < required_size; ++i)
    vsOut.emplace_back(std::string());
}

std::string GetFirstParam(const std::string& in, char sep)
{
  return in[0] != sep ? in.substr(0, in.find(',')) : std::string();
}

bool CheckMasking(const std::string& path, const std::string& mask)
{
  return rutil::wild_match(path, mask);
}

bool CompareFilename(const std::string &path1, const std::string &path2)
{
  return rutil::GetAlternativeFilename(path1) == rutil::GetAlternativeFilename(path2);
}

size_t FindUtf8FirstByteReversed(const std::string &str)
{
  // find character which is 11xxxxxx or 0xxxxxxx (startpoint)
  if (str.empty()) return 0;
  const char *s = str.c_str();
  const char *p = s + str.size() - 1;
  while (p > s)
  {
    if (*p & 0b11000000 == 0b11000000 || *p & 0b10000000 == 0)
      break;
    --p;
  }
  return p - s;
}

// borrowed from XMLUtil library.
void ConvertUTF32ToUTF8(uint32_t input, char* output, unsigned* length)
{
  const unsigned long BYTE_MASK = 0xBF;
  const unsigned long BYTE_MARK = 0x80;
  const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

  if (input < 0x80) {
    *length = 1;
  }
  else if (input < 0x800) {
    *length = 2;
  }
  else if (input < 0x10000) {
    *length = 3;
  }
  else if (input < 0x200000) {
    *length = 4;
  }
  else {
    *length = 0;    // This code won't convert this correctly anyway.
    return;
  }

  output += *length;

  // Scary scary fall throughs are annotated with carefully designed comments
  // to suppress compiler warnings such as -Wimplicit-fallthrough in gcc
  switch (*length) {
  case 4:
    --output;
    *output = (char)((input | BYTE_MARK) & BYTE_MASK);
    input >>= 6;
    //fall through
  case 3:
    --output;
    *output = (char)((input | BYTE_MARK) & BYTE_MASK);
    input >>= 6;
    //fall through
  case 2:
    --output;
    *output = (char)((input | BYTE_MARK) & BYTE_MASK);
    input >>= 6;
    //fall through
  case 1:
    --output;
    *output = (char)(input | FIRST_BYTE_MARK[*length]);
    break;
  default:
    // Invalid but NOTHROW, just ignore the character.
    *length = 0;
    break;
  }
}

#if WIN32
std::string GetUtf8FromWString(const std::wstring& wstring)
{
  std::string s;
  rutil::EncodeFromWStr(wstring, s, rutil::E_UTF8);
  return s;
}
#endif

// -------------------------------- CommandArgs

CommandArgs::CommandArgs(const std::string &argv)
  : sep_(','), len_(0), trim_(true) { Parse(argv); }

CommandArgs::CommandArgs(const std::string &argv, size_t arg_count, bool fit_size)
  : sep_(','), len_(0), trim_(true) { Parse(argv, arg_count, true); }

void CommandArgs::Parse(const std::string &argv)
{
  Parse(argv, kMaxCommandArgs, false);
}

void CommandArgs::Parse(const std::string &argv, size_t arg_count, bool fit_size)
{
  size_t a = 0, b = 0;
  memset(args_, 0, sizeof(args_));
  len_ = 0;
  s_ = argv;
  if (arg_count > kMaxCommandArgs) arg_count = kMaxCommandArgs;

  if (trim_)
  {
    while (s_[b] && (s_[b] == ' ' || s_[b] == '\t' || s_[b] == '\r' || s_[b] == '\n'))
      b++;
    a = b;
  }

  while (len_ < arg_count)
  {
    if (s_[b] == sep_ || s_[b] == 0)
    {
      args_[len_++] = s_.c_str() + a;
      if (s_[b] == 0)
        break;
      s_[b] = 0;
      a = b = b + 1;
      if (trim_)
      {
        while (s_[b] && (s_[b] == ' ' || s_[b] == '\t' || s_[b] == '\r' || s_[b] == '\n'))
          b++;
        a = b;
      }
    }
    else b++;
  }

  if (fit_size)
  {
    while (len_ < arg_count)
    {
      args_[len_++] = "";
    }
  }
}

void CommandArgs::set_separator(char sep)
{
  sep_ = sep;
}

template <>
int CommandArgs::Get(size_t arg_index) const
{
  if (kMaxCommandArgs <= arg_index) return 0;
  return atoi(args_[arg_index]);
}

template <>
size_t CommandArgs::Get(size_t arg_index) const
{
  if (kMaxCommandArgs <= arg_index) return 0;
  return (size_t)atoi(args_[arg_index]);
}

template <>
double CommandArgs::Get(size_t arg_index) const
{
  if (kMaxCommandArgs <= arg_index) return 0;
  return atof(args_[arg_index]);
}

template <>
float CommandArgs::Get(size_t arg_index) const
{
  if (kMaxCommandArgs <= arg_index) return 0;
  return (float)atof(args_[arg_index]);
}

template <>
std::string CommandArgs::Get(size_t arg_index) const
{
  if (kMaxCommandArgs <= arg_index) return "";
  return args_[arg_index];
}

const char* CommandArgs::Get_str(size_t arg_index) const
{
  if (kMaxCommandArgs <= arg_index) return "";
  return args_[arg_index];
}

size_t CommandArgs::size() const { return len_; }

void CommandArgs::set_trim(bool v) { trim_ = v; }

void Sleep(size_t milisecond)
{
#ifdef _MSC_VER
  _sleep(milisecond);
#else
  usleep(milisecond * 1000);
#endif
}

#ifndef WIN32
int stricmp(const char *a, const char *b)
{
  int ca, cb;
  do {
    ca = (unsigned char) *a++;
    cb = (unsigned char) *b++;
    ca = tolower(toupper(ca));
    cb = tolower(toupper(cb));
  } while (ca == cb && ca != '\0');
  return ca - cb;
}

int strnicmp(const char *a, const char *b, size_t len)
{
  if (len == 0) return 0;
  int ca, cb;
  do {
    len--;
    ca = (unsigned char) *a++;
    cb = (unsigned char) *b++;
    ca = tolower(toupper(ca));
    cb = tolower(toupper(cb));
  } while (ca == cb && ca != '\0' && len > 0);
  return ca - cb;
}
#endif

}
