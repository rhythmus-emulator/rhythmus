#include "Script.h"
#include "ScriptLR2.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Setting.h"
#include "KeyPool.h"
#include "tinyxml2.h"
#include <fstream>

#include "Util.h"
#include "common.h"

namespace rhythmus
{

using namespace tinyxml2;

// ----------------------------------------------------------------- XMLContext

XMLContext::XMLContext() : ctx(nullptr), curr_node(nullptr)
{
  ctx = new tinyxml2::XMLDocument();
  curr_node = nullptr;
}

XMLContext::XMLContext(const std::string &rootnodename) : ctx(nullptr), curr_node(nullptr)
{
  ctx = new tinyxml2::XMLDocument();
  auto* doc = (tinyxml2::XMLDocument*)ctx;
  curr_node = doc->InsertFirstChild(doc->NewElement(rootnodename.c_str()));
}

XMLContext::~XMLContext()
{
  delete (tinyxml2::XMLDocument*)ctx;
}

bool XMLContext::Load(const FilePath& path)
{
  auto* doc = (tinyxml2::XMLDocument*)ctx;

  if (path.valid() && doc->LoadFile(path.get_cstr()) != XML_SUCCESS)
  {
    //throw FileNotFoundException(path);
    return false;
  }

  node_level.clear();
  curr_node = doc->RootElement();

  return true;
}

bool XMLContext::Load(const std::string &path)
{
  return Load(FilePath(path));
}

bool XMLContext::Save(const FilePath& path)
{
  auto* doc = (tinyxml2::XMLDocument*)ctx;
  if (!doc) return false;
  return doc->SaveFile(path.get_cstr()) == XML_SUCCESS;
}

bool XMLContext::Save(const std::string &path)
{
  return Load(FilePath(path));
}

bool XMLContext::next()
{
  auto *n = (tinyxml2::XMLElement*)curr_node;
  if (!n) return false;
  curr_node = n->NextSiblingElement();
  return curr_node != 0;
}

bool XMLContext::step_in()
{
  auto *n = (tinyxml2::XMLElement*)curr_node;
  if (!n) return false;
  tinyxml2::XMLElement *child = n->FirstChildElement();
  if (!child) return false;
  node_level.push_back(curr_node);
  curr_node = child;
  return true;
}

bool XMLContext::step_out()
{
  if (node_level.empty()) return false;
  curr_node = node_level.back();
  node_level.pop_back();
  return true;
}

void* XMLContext::get_document()
{
  return ctx;
}

void* XMLContext::get_rootnode()
{
  return ctx ? ((tinyxml2::XMLDocument*)ctx)->RootElement() : nullptr;
}

void* XMLContext::get_node()
{
  return curr_node;
}

MetricGroup XMLContext::GetCurrentMetric() const
{
  MetricGroup g;
  auto *p = (tinyxml2::XMLElement*)curr_node;
  if (p)
  {
    const auto *attr = p->FirstAttribute();
    while (attr)
    {
      g.set(attr->Name(), attr->Value());
      attr = attr->Next();
    }
  }
  return g;
}

// ----------------------------------------------------------------- XMLExecutor

static std::map<std::string, XMLCommandHandler> &getXMLHandler()
{
  static std::map<std::string, XMLCommandHandler> gXMLHandlers;
  return gXMLHandlers;
}

XMLExecutor::XMLExecutor(XMLContext *ctx) : ctx_(ctx), current_(nullptr) {}

void XMLExecutor::AddHandler(const std::string &cmd, XMLCommandHandler handler)
{
  getXMLHandler()[cmd] = handler;
}

bool XMLExecutor::RunInternal()
{
  while (ctx_->get_node())
  {
    tinyxml2::XMLElement* p = (tinyxml2::XMLElement*)ctx_->get_node();
    auto i = getXMLHandler().find(p->Name());
    if (i != getXMLHandler().end())
      (*i->second)(this, ctx_);
    if (ctx_->step_in())
    {
      parent_.push_back(current_);
      RunInternal();
      parent_.pop_back();
      ctx_->step_out();
    }
    ctx_->next();
  }
  return true;
}

bool XMLExecutor::Run()
{
  bool r = false;
  if (ctx_->step_in())
  {
    parent_.push_back(current_);
    r = RunInternal();
    parent_.pop_back();
    ctx_->step_out();
  }
  return r;
}

void XMLExecutor::SetCurrentObject(void *p)
{
  current_ = p;
}

void* XMLExecutor::GetParent()
{
  return parent_.empty() ? nullptr : parent_.back();
}


// ----------------------------------------------------------------- CSVContext

CSVContext::CSVContext() : sep(','), row_idx(0), col_size(0)
{
  memset(cols, 0, sizeof(cols));
}

CSVContext::~CSVContext()
{
}

void CSVContext::set_separator(char c) { sep = c; }

bool CSVContext::Load(const FilePath& path)
{
  std::string r;
  std::ifstream is(path.get_cstr());
  if (is.fail()) return false;
  rows.clear();
  while (std::getline(is, r)) {
    rows.push_back(r);
  }
  row_idx = 0;
  next();
  return true;
}

bool CSVContext::Load(const std::string &path)
{
  return Load(FilePath(path));
}

bool CSVContext::next()
{
  unsigned i = 0, ci = 0;
  if (row_idx >= rows.size()) return false;
  auto &r = rows[row_idx];
  memset(cols, 0, sizeof(cols));
  cols[ci++] = r.c_str();
  for (; i < r.size() && ci < MAX_CSV_COL; ++i) {
    if (r[i] == sep) {
      r[i] = '\0';
      cols[ci++] = &r[i + 1];
      i++;
    }
  }
  col_size = ci;
  row_idx++;
  return true;
}

const char* const* CSVContext::get_row() const
{
  return cols;
}

const char *CSVContext::get_str(unsigned idx) const
{
  return cols[idx] ? cols[idx] : "";
}

int CSVContext::get_int(unsigned idx) const
{
  const char *c = get_str(idx);
  if (c[0] == '-') return -atoi(c + 1);
  else return atoi(c);
}

float CSVContext::get_float(unsigned idx) const
{
  const char *c = get_str(idx);
  if (c[0] == '-') return -atof(c + 1);
  else return atof(c);
}

unsigned CSVContext::get_col_size() const
{
  return col_size;
}

namespace Script
{

bool Load(const FilePath& path, void* baseobject, const std::string& type)
{
  std::string ext = GetExtension(path.get());
  if (ext == "xml") {
    XMLContext ctx;
    if (!ctx.Load(path))
    {
      Logger::Error("XML file load failed: %s", path.get_cstr());
      return false;
    }
    XMLExecutor loader(&ctx);
    loader.SetCurrentObject(baseobject);
    return loader.Run();
  }
  else if (ext == "lr2skin" || ext == "lr2ss") {
    LR2CSVContext ctx;
    if (!ctx.Load(path)) {
      Logger::Error("LR2CSV file load failed: %s", path.get_cstr());
      return false;
    }
    LR2CSVExecutor loader(&ctx);
    loader.Run(baseobject, type);
    return true;
  }
  else {
    Logger::Error("Unsupported script: %s", path.get_cstr());
    return false;
  }
}

bool Load(const std::string &path, void *baseobject, const std::string& type)
{
  return Load(FilePath(path), baseobject, type);
}

} /* namespace Script */

} /* namespace rhythmus */