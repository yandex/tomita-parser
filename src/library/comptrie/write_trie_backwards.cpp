#include "write_trie_backwards.h"

#include "comptrie_impl.h"

namespace NCompactTrie {

size_t WriteTrieBackwards(TOutputStream& os, TReverseNodeEnumerator& enumerator, bool verbose)
{
    if (verbose) {
        Cerr << "Writing down the trie..." << Endl;
    }

    // Rewrite everything from the back, removing unused pieces.
    const size_t chunksize = 0x10000;
    yvector<char*> resultData;

    resultData.push_back(new char[chunksize]);
    char* chunkend = resultData.back() + chunksize;

    size_t resultLength = 0;
    size_t chunkLength = 0;

    size_t counter = 0;
    TBuffer bufferHolder;
    while (enumerator.Move()) {
        if (verbose)
            ShowProgress(++counter);

        size_t bufferLength = 64 + enumerator.GetLeafLength(); // never know how big leaf data can be
        bufferHolder.Clear();
        bufferHolder.Proceed(bufferLength);
        char* buffer = bufferHolder.Data();

        size_t nodelength = enumerator.RecreateNode(buffer, resultLength);
        YASSERT(nodelength <= bufferLength);

        resultLength += nodelength;

        if (chunkLength + nodelength <= chunksize) {
            chunkLength += nodelength;
            memcpy(chunkend - chunkLength, buffer, nodelength);
        }
        else { // allocate a new chunk
            memcpy(chunkend - chunksize, buffer + nodelength - (chunksize - chunkLength), chunksize - chunkLength);
            chunkLength = chunkLength + nodelength - chunksize;

            resultData.push_back(new char[chunksize]);
            chunkend = resultData.back() + chunksize;

            while (chunkLength > chunksize) { // allocate a new chunks
                chunkLength -= chunksize;
                memcpy(chunkend - chunksize, buffer + chunkLength, chunksize);

                resultData.push_back(new char[chunksize]);
                chunkend = resultData.back() + chunksize;
            }

            memcpy(chunkend - chunkLength, buffer, chunkLength);
        }
    }

    if (verbose)
        Cerr << counter << Endl;

    // Write the whole thing down
    while (!resultData.empty()) {
        char* chunk = resultData.back();
        os.Write(chunk + chunksize - chunkLength, chunkLength);
        chunkLength = chunksize;
        delete[] chunk;
        resultData.pop_back();
    }

    return resultLength;
}

} // namespace NCompactTrie
