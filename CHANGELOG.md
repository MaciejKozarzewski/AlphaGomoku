
# Change Log
All notable changes to this project will be documented in this file.
 
## [Unreleased] - 2022-04-19
 
### Added
 
### Changed
 
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
