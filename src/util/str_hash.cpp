#include "str_hash.h"

#include <util/stream/ios.h>

HashSet::HashSet(const char** array, size_type size) {
    Resize(size);
    while (*array && **array)
        AddPermanent(*array++);
}

void HashSet::Read(TInputStream* input) {
    Stroka s;

    while (input->ReadLine(s)) {
        AddUniq(stroka(s).c_str());
    }
}

void HashSet::Write(TOutputStream* output) const {
    for (const_iterator it = begin(); it != end(); ++it) {
        *output << it->first << "\n";
    }
}

#ifdef TEST_STRHASH
#include <ctime>
#include <fstream>
#include <cstdio>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: stoplist <stop-words file ...\n");
        exit(EXIT_FAILURE); // FreeBSD: EX_USAGE
    }
    Hash hash;
    hash.Read(cin);
    for (--argc,++argv; argc > 0; --argc,++argv) {
        ifstream input(argv[0]);
        if (!input.good()) {
            perror(argv[0]);
            continue;
        }
        stroka s;
        while (input >> s) {
            if (!hash.Has(s))
                cout << s << "\n";
            else
                cout << "[[" << s << "]]" << "\n";
        }
    }
    return EXIT_SUCCESS; // EX_OK
}

#endif
