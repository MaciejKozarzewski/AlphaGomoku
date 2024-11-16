# TODO List
All notable ideas will be documented in this file.

## dataset
- investigate the impact of different samplers (especially the Q-values based sampler).

## networks
- maybe implement nnue ops in assembly (after settling with final architecture)?
- check if input features provide any improvement or not. If not, remove support for them (potential speedup).

## patterns
- maybe it is possible to optimize defensive move table (unlikely, low priority)?

## player
- more controllers for different opening rules (nice to have).
- support for opening book/position database.
- tune time manager (maybe add something to predict moves left, instead of using statistical approach).
- add early stopping if the best move has been determined with enough confidence.
- add memory management.

## protocols
- more support for opening rules

## search-ab
- investigate why the alpha-beta search does not benefit from cutoffs ??? <- TOP PRIORITY
- maybe optimize move generator (some already done, rest is unlikely).
- reduce Score range to -1000/+1000 to save 4 bits (easy, need to ensure compatibility with old dataset format).

## search-mcts
- if the 'style' parameter is removed, Edge can only store expectation values, this leaves 4 bytes for something else.
- Move could be compressed to save another 2 bytes.
- Datatype used for inference can be made parametrizable in the config (easy, but nice to have).
- check Thompson sampling at root or at all levels (virtual loss would then not be necessary - 2 free bytes). <- a lot of research required

## selfplay
- refactor Evaluation so that different Players can be used.
