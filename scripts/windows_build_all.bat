cd ..
mkdir build

cmake -B build/Release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_CUDA=OFF -DBUILD_WITH_OPENCL=OFF
cmake --build build/Release
cmake -B build/Release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_CUDA=ON -DBUILD_WITH_OPENCL=OFF
cmake --build build/Release
cmake -B build/Release -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_CUDA=OFF -DBUILD_WITH_OPENCL=ON
cmake --build build/Release

cmake -B build/Debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_CUDA=OFF -DBUILD_WITH_OPENCL=OFF
cmake --build build/Debug
cmake -B build/Debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_CUDA=ON -DBUILD_WITH_OPENCL=OFF
cmake --build build/Debug
cmake -B build/Debug -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_WITH_CUDA=OFF -DBUILD_WITH_OPENCL=ON
cmake --build build/Debug