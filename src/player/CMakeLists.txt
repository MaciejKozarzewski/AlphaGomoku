add_subdirectory("controllers")
target_sources(AlphaGomoku PRIVATE 	benchmark.cpp
									configuration.cpp
									EngineController.cpp
									EngineSettings.cpp
									ProgramManager.cpp
									SearchEngine.cpp
									SearchThread.cpp
									TimeManager.cpp)