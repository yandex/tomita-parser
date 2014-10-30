#pragma once

#define STATIC_ASSERT(cond) static_assert(cond, "");
#define STATIC_CHECK(expression, message) static_assert(expression, #message)
