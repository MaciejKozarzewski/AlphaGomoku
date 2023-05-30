target_sources(${LibName} PRIVATE 	nnue_ops/def_ops.cpp
									nnue_ops/layers.cpp
									AGNetwork.cpp
									blocks.cpp
									networks.cpp
									NNInputFeatures.cpp
									NNUE.cpp)
									
add_library(sse41_nnue OBJECT nnue_ops/sse41_ops.cpp)
target_compile_options(sse41_nnue PRIVATE -msse4.1)
set_target_properties(sse41_nnue PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_include_directories(sse41_nnue PUBLIC PUBLIC "${PROJECT_SOURCE_DIR}/include")

add_library(avx2_nnue OBJECT nnue_ops/avx2_ops.cpp)
target_compile_options(avx2_nnue PRIVATE -mavx2 -mfma)
set_target_properties(avx2_nnue PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_include_directories(avx2_nnue PUBLIC PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(${LibName} PRIVATE sse41_nnue avx2_nnue)
