add_executable(${TestName}	test_launcher.cpp
				dataset/test_CompressedFloat.cpp
				game/test_Board.cpp
				game/test_caro.cpp
				game/test_freestyle.cpp
				game/test_Move.cpp
				game/test_renju.cpp
				game/test_standard.cpp
				networks/test_NNInputFeatures.cpp
				protocols/test_ExtendedGomocupProtocol.cpp
				protocols/test_GomocupProtocol.cpp
				protocols/test_protocol.cpp
				search/alpha_beta/test_move_generator.cpp
				search/monte_carlo/test_Edge.cpp
				search/monte_carlo/test_Node.cpp
				search/monte_carlo/test_NodeCache.cpp
				search/monte_carlo/test_SearchTask.cpp
				search/monte_carlo/test_Tree.cpp
				utils/test_ArgumentParser.cpp
				utils/test_augmentations.cpp
				utils/test_configs.cpp
				utils/test_file_util.cpp
				utils/test_misc.cpp)
				
set_target_properties(${TestName} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_include_directories(${TestName} PUBLIC "${PROJECT_SOURCE_DIR}/include")

target_link_libraries(${TestName} PUBLIC gtest ${LibName})
