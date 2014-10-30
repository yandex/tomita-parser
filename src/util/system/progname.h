#pragma once

#include <util/generic/stroka.h>

void SetProgramName(const char *argv0);

#define SAVE_PROGRAM_NAME do { SetProgramName(argv[0]); } while(0)

/// guaranted return the same immutable instance of Stroka
const Stroka& GetProgramName();
const Stroka& GetProgramDir();
