/*
 * LocalHashTable.hpp
 *
 *  Created on: Mar 15, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_LOCALHASHTABLE_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_LOCALHASHTABLE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include <array>
#include <vector>
#include <cassert>

namespace ag
{
	class LocalHashTable
	{
			static const uint16_t null_move = Move(0).toShort();

			std::vector<uint16_t> m_hashtable;
			FastZobristHashing m_hash_function;
			HashKey64 m_mask;
		public:
			LocalHashTable(int rows, int columns, size_t initialSize = 1024) :
					m_hashtable(roundToPowerOf2(initialSize)),
					m_hash_function(rows, columns),
					m_mask(m_hashtable.size() - 1)
			{
			}
			int64_t getMemory() const noexcept
			{
				return sizeof(uint16_t) * m_hashtable.size() + m_hash_function.getMemory();
			}
			const FastZobristHashing& getHashFunction() const noexcept
			{
				return m_hash_function;
			}
			void clear() noexcept
			{
				std::fill(m_hashtable.begin(), m_hashtable.end(), null_move);
			}
			Move seek(HashKey128 hash) const noexcept
			{
				return Move(m_hashtable[get_index_of(hash)]);
			}
			void insert(HashKey128 hash, Move move) noexcept
			{
				m_hashtable[get_index_of(hash)] = move.toShort();
			}
			void prefetch(HashKey128 hash) const noexcept
			{
				prefetchMemory<PrefetchMode::READ, 1>(m_hashtable.data() + get_index_of(hash));
			}
			double loadFactor(bool approximate = false) const noexcept
			{
				const size_t size = approximate ? std::min(m_hashtable.size(), (size_t) 1024) : m_hashtable.size();
				uint64_t result = 0;
				for (size_t i = 0; i < size; i++)
					result += (m_hashtable[i] != null_move);
				return static_cast<double>(result) / size;
			}
		private:
			size_t get_index_of(HashKey128 hash) const noexcept
			{
				return hash.getLow() & m_mask;
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_LOCALHASHTABLE_HPP_ */
