/*
 * EdgePool.hpp
 *
 *  Created on: Sep 12, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_MCTS_EDGEPOOL_HPP_
#define ALPHAGOMOKU_MCTS_EDGEPOOL_HPP_

#include <vector>
#include <memory>

namespace ag
{
	class Edge;
}

namespace ag
{
	class EdgePool
	{
		private:
			/**
			 * @brief Class describing block of continuous memory but not owning it.
			 */
			class BlockDescriptor
			{
				private:
					Edge *block_start = nullptr; // non-owning
					size_t block_size = 0;
				public:
					BlockDescriptor() = default;
					BlockDescriptor(Edge *start, size_t size) noexcept;
					Edge* getEdges(size_t size) noexcept;
					size_t size() const noexcept;
			};
			/**
			 * @brief Class that collects multiple blocks of memory.
			 */
			class BlockPool
			{
				private:
					std::vector<BlockDescriptor> list_of_blocks;
				public:
					BlockDescriptor pop() noexcept;
					const BlockDescriptor& peek() const noexcept;
					void push(const BlockDescriptor &bd);
					size_t size() const noexcept;
			};
			/**
			 * @brief Class that owns a block of memory for edges.
			 */
			class BlockOfEdges
			{
				private:
					std::unique_ptr<Edge[]> edges;
					std::unique_ptr<bool[]> is_edge_free;
					size_t size;
				public:
					BlockOfEdges(size_t size);
			};
		public:
			[[nodiscard]] Edge* allocate(size_t size);
			void free(Edge *ptr, size_t size);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_MCTS_EDGEPOOL_HPP_ */
