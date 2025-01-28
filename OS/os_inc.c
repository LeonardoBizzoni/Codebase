#if OS_LINUX
#  include "Linux/dynlib.c"
#  include "Linux/file.c"
#  include "Linux/thread.c"
#  include "Linux/linux_core.c"

#  include "Linux/X11/window.c"
#elif OS_BSD
#  include "BSD/dynlib.c"
#  include "BSD/file.c"
#  include "BSD/thread.c"
#  include "BSD/X11/window.c"
#elif OS_WINDOWS
#  include "Win32/os_core_win32.c"
#endif
