// The following code is the modified part of the protobuf library
// available at http://code.google.com/p/protobuf/
// under the terms of the BSD 3-Clause License
// http://opensource.org/licenses/BSD-3-Clause

#include <vector>
#include <sstream>
#include <compiler/perlxs/perlxs_helpers.h>
#include <io/printer.h>
#include <descriptor.pb.h>

namespace google {
namespace protobuf {

extern string StringReplace(const string& s, const string& oldsub,
			    const string& newsub, bool replace_all);

namespace compiler {
namespace perlxs {

void
SetupDepthVars(map<string, string>& vars, int depth)
{
  ostringstream ost_pdepth;
  ostringstream ost_depth;
  ostringstream ost_ndepth;

  ost_pdepth << depth;
  ost_depth  << depth + 1;
  ost_ndepth << depth + 2;

  vars["pdepth"] = ost_pdepth.str();
  vars["depth"]  = ost_depth.str();
  vars["ndepth"] = ost_ndepth.str();
}

}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
