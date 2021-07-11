#include "Script.h"

namespace rhythmus
{

class LR2SSHandlers
{
public:
  static void INFORMATION(const char *path, LR2CSVContext *ctx)
  {
    METRIC->set("SelectSceneBgm", FilePath(path).get());
  }
  LR2SSHandlers()
  {
    LR2SSExecutor::AddHandler("#SELECT", (LR2SSHandlerFunc)INFORMATION);
  }
private:
};

LR2SSHandlers _LR2SSHandlers;

}