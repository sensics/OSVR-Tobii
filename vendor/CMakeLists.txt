# Interface target for Eigen 3.2.8
add_library(eigen-headers INTERFACE)
set(TOBII_VENDORED_EIGEN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/eigen" CACHE INTERNAL "" FORCE)
target_include_directories(eigen-headers INTERFACE "${AVEGANT_VENDORED_EIGEN_ROOT}")
target_compile_definitions(eigen-headers INTERFACE EIGEN_MPL2_ONLY)