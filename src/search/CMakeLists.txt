add_subdirectory("alpha_beta")
add_subdirectory("monte_carlo")
target_sources(AlphaGomoku PRIVATE 	Score.cpp
									Value.cpp
									ZobristHashing.cpp)