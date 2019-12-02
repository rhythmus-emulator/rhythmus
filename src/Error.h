#pragma once

#include <assert.h>
#include <stdexcept>

#define ASSERT(x) assert(x)
#define ASSERT_M(x, s) while (!(x)) { throw std::runtime_error(s); break; }
#define ASSERT_GL() ASSERT(glGetError() == 0)
#define ASSERT_GL_VAL(x) ASSERT((x = glGetError()) == 0)

namespace rhythmus
{
}