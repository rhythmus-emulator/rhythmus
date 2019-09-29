#include "Util.h"
#include "rutil.h"
#include <stack>

#ifdef WIN32
# include <Windows.h>
# include <sys/types.h>
# include <sys/stat.h>
# define stat _wstat
#else
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
      struct _stat result;
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

std::string Substitute(const std::string& path_, const std::string& startswith, const std::string& relplacewith)
{
  std::string path = path_;
  for (int i = 0; i < path.size(); ++i)
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
  for (int i = 0; i < org.size(); ++i)
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

void Split(const std::string& str, char sep, std::vector<std::string>& vsOut)
{
  rutil::split(str, sep, vsOut);
}

bool IsFile(const std::string& path)
{
  return rutil::IsFile(path);
}

void GetFolderNameEntries(const std::string& file_or_filter_path, std::vector<std::string> &out)
{
  // TODO: use rparser directory api
}

bool GetFilepathSmart(const std::string& file_or_filter_path, std::string& out, int index)
{
  std::vector<std::string> name_entries;
  GetFolderNameEntries(file_or_filter_path, name_entries);
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
  std::vector<std::string> &vsOut, int required_size, char sep)
{
  Split(in, sep, vsOut);
  if (required_size < 0) return;
  for (auto i = vsOut.size(); i < required_size; ++i)
    vsOut.emplace_back(std::string());
}

std::string GetFirstParam(const std::string& in, char sep)
{
  return in[0] != sep ? in.substr(0, in.find(',')) : std::string();
}

}