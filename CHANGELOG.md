
# Change Log
All notable changes to this project will be documented in this file.
 
## [Unreleased] - 2022-09-23
 
### Added
- Added support for Renju and Caro rule (without new networks so far).
- Added basic support for Yixin protocol.
- Added support for more opening rules.
- Search with GPU uses double buffering to improve performance (on my machine up to 20% faster for 1 thread/GPU, 10% for 2 threads/GPU).
- Neural network is using Gomoku-specific input features.
- Neural network has additional output head for action values.
 
### Changed
- Reduced memory consumption by about 8% (on average).
- Improved solver.
 
### Fixed

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
