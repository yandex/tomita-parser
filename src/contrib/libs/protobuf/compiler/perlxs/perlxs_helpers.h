// The following code is the modified part of the protobuf library
// available at http://code.google.com/p/protobuf/
// under the terms of the BSD 3-Clause License
// http://opensource.org/licenses/BSD-3-Clause

#ifndef GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__

#include <map>
#include <string>

using namespace std;

namespace google {
namespace protobuf {
namespace compiler {
namespace perlxs {

void SetupDepthVars(std::map<string, string>& vars, int depth);

}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__
