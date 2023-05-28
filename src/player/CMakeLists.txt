add_subdirectory("controllers")
target_sources(${LibName} PRIVATE 	benchmark.cpp
									configuration.cpp
									EngineController.cpp
									EngineSettings.cpp
									ProgramManager.cpp
									SearchEngine.cpp
									SearchThread.cpp
									TimeManager.cpp)