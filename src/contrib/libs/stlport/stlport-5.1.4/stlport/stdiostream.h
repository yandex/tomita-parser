#pragma once

# if (__SUNPRO_CC >= 0x500 )
#  include <../CCios/stdiostream.h>
# else if defined (__SUNPRO_CC)
#  include <../CC/stdiostream.h>
# else
#  error "This file is for SUN CC only. Please remove it if it causes any harm for other compilers."
# endif
