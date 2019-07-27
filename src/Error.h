#pragma once

#include <assert.h>

#define ASSERT(x) assert(x)
#define ASSERT_GL() ASSERT(glGetError() == 0)
#define ASSERT_GL_VAL(x) ASSERT((x = glGetError()) == 0)

namespace rhythmus
{
}