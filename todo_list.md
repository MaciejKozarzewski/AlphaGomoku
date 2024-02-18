
# TODO List
All notable ideas will be documented in this file.

## dataset
### Compressing the dataset
- currently an entry has following structure:
	bits	field			optimization possibilites
	16	location		elsewhere in the code it is already assumed that board is at most 20x20 (9 bits)
	16	visit_count		maybe we can assume at most 1023 visits (10 bits)
	16	policy_prior		it depend on how many bits the CompressedFloat must have (9 or 10)
	16	score			score is minimum 12 bits
	16	win_rate		same as policy_prior (9 or 10)
	16	draw_rate		same as policy_prior (9 or 10)
	which sums up to 58, maybe we can remove draw_rate and keep only expectation Q value?
	Or switch to some delta-base format?
IMPORTANT:	Must be done at the same time as reducing bit-width of Score from 16 to 12 bits

- investigate the impact of different samplers (especially the Q-values based sampler).


## game
- nothing to be done here

## networks
- maybe implement nnue ops in assembly (after settling with final architecture)?
- check if input features provide any improvement or not. If not, remove the support for them (potential speedup).
- Add SE-block (potential elo gain).

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
- Pointer to the Node can be removed at the cost of 2x slowdown of select phase (maybe irrelevant, needs testing).
- Datatype used for inference can be made parametrizable in the config (easy, but nice to have).
- check Thompson sampling at root or at all levels (virtual loss would then not be necessary - 2 free bytes). <- a lot of research required

## selfplay
- refactor Evaluation so that different Players can be used. <- TOP PRIORITY (required for a-b search testing)

## utils
- nothing to be done here
