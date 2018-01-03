# OSVR-Tobii
OSVR plugin for Tobii eye trackers

## Build notes:

When configuring CMake for this project, you will need to let CMake know (via `CMAKE_PREFIX_PATH`, etc...) where to find OSVR-Core and libfunctionality. These can be the cmake install directories of those two projects, if you built them from source, or the OSVR SDK directory.
