
# Change Log
All notable changes to this project will be documented in this file.
 
## [Unreleased] - 
### Added
### Changed
### Fixed

## [5.8.1] - 2024-05-22
This is the version that played in Gomocup 2024 with bugfixes.

### Added
- added OpenCL backend.
- new network for freestyle rule on 15x15 board.

### Changed
- MCTS now tracks the speed of computations on GPU so it can better plan the time for alpha-beta search.
- reduced memory usage by about 20% (on average).
- style_factor parameter was removed.

### Fixed
- Fixed bugs in move generator in caro and renju rules.
- Fixed bug in "SWAP2BOARD" command.
- Fixed "INFO evaluate" command in Gomocup protocol.


## [5.7.1] - 2023-11-08
Better YixinBoard support.

### Added
- In YixinBoard the amount of memory is controlled by 'max_hash_size' parameter (memory = pow(2, max_hash_size) [in MB]). Minimal accepted value is 8 (256MB), max value is 20 (1TB).
- Added realtime info processing in YixinBoard. Messages 'POS' are used to indicate moves that are being considered by the search (followed by 'DONE' to change color to yellow). Messages 'LOSE' indicate provably losing moves. Messages 'BEST' indicate current best move. Messages are sent every 100ms, but only if anything changed since last time.

### Changed
- When using YixinBoard, move coordinates are no longer internally transposed to match with the logfile printing.
- Parameter 'caution_factor' is ignored (it will be removed from the algorithm in the future).
- Parameter 'thread_split_depth' is ignored (it does not make sense in monte-carlo search).
- Final info message (right before returning a move) is now formatted in the YixinBoard style.

### Fixed
- Fixed printing forbidden moves in YixinBoard.
- Fixed swap2 openings.
- Fixed missing paths in default configuration file.
- Fixed setting number of threads in YixinBoard.


## [5.7.0] - 2023-10-30
Bugfixes and speed improvements.

### Added
- New command line option for listing available devices.
- Added fp16 support for Ivy Bridge CPUs.
- New fused gemm+bias+relu kernels for CPU.
- Support for avx512.

### Changed
- Removed support for bf16 data type.
- Reimplemented Winograd transforms in assembly.

### Fixed
- Fixed Yixinboard protocol implementation.
- On Windows, zlib is linked statically to mitigate problems when running in Yixinboard.

## [5.6.1] - 2023-06-19

### Added
- Added self-check utility.

### Changed
- Improved automatic fonfiguration.

### Fixed
- Fixed several SIGILL errors in the ML backend.
- Fixed typo in Yixinboard protocol implementation.
- Removed unnecessary logging from search threads.

## [5.6.0] - 2023-06-12
 
### Added
- Added support for Renju and Caro rule.
- Added some support for Yixin protocol (WIP).
- Added possibility to support more opening rules (WIP).
- Search with GPU uses double buffering to improve performance (on my machine up to 20% faster for 1 thread/GPU, 10% for 2 threads/GPU).
- Added more architectures of the main neural network.
- Implemented NNUE-like evaluation function (not used for now).
 
### Changed
- Reduced memory consumption of MCTS by about 8% (on average).
- Solver turned into alpha-beta search.
- Final move selection is based on Lower Confidence Bound (LCB) rather than some heuristic formula.
- Tuned search parameters.
 
### Fixed
- Fixed crash when the time for turn is very small and no nodes have been searched (now there is always at least 1).
- Swap2 opening book is no longer required to run the engine (but must be present to use swap2/swap opening commands).


## [5.3.4] - 2022-04-20
 
### Fixed
- Fixed error with parsing launch path on Windows.

 
## [5.3.3] - 2022-04-19
  
This is a version that played in the unlimited tournament of Gomocup 2022. It contains some fixes and improvements over a version that played in regular leagues.
 
### Added
- Added several command line options, like automatic configuration and benchmarking.
- Implemented new protocol that extends the one used in Gomocup.
- Implemented graph search.
- Checking for correctness of protocol commands.
- Solver now automatically tunes itself for maximum performance.
- Added time management.
 
### Changed
- Improved allocation of the tree nodes.
- Solver can also recognize open threes.
- Optimized feature calculation for solver. It can now detect more different threats while being faster.
- Swap2 now uses two different implementations depending on time controls.
- Decreased initialization time.
 
### Fixed
- Fixed implementation of PLAY command.
- Fixed error with correcting information leaks in the search.
- Fixed multiple errors in the solver.

 
## [5.0.1] - 2021-05-17
First public release of full version.
 
### Added
   
### Changed
 
### Fixed
- Fixed error in the VCF solver.
