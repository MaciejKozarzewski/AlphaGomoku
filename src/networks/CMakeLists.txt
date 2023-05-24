target_sources(AlphaGomoku PRIVATE 	nnue_ops/avx2_ops.cpp
									nnue_ops/def_ops.cpp
									nnue_ops/layers.cpp
									nnue_ops/sse41_ops.cpp
									AGNetwork.cpp
									blocks.cpp
									networks.cpp
									NNInputFeatures.cpp
									NNUE.cpp)
									
set_source_files_properties(nnue_ops/sse41_ops.cpp PROPERTIES COMPILE_OPTIONS "-msse4.1")
set_source_files_properties(nnue_ops/avx2_ops.cpp PROPERTIES COMPILE_OPTIONS "-mavx2;-mfma")
