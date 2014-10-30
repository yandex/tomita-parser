#pragma once

#include <util/generic/stroka.h>

Stroka GetEnv(const Stroka& key);
void SetEnv(const Stroka& key, const Stroka& value);
