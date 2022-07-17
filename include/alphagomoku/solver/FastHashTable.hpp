/*
 * FastHashTable.hpp
 *
 *  Created on: Mar 3, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SOLVER_FASTHASHTABLE_HPP_
#define ALPHAGOMOKU_SOLVER_FASTHASHTABLE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <array>
#include <vector>
#include <cassert>

namespace ag::experimental
{
	template<typename KeyType, typename ValueType, int N = 4>
	class FastHashTable
	{
		private:
			struct HashTableEntry
			{
					std::array<KeyType, N> hash;
					std::array<ValueType, N> value;
					HashTableEntry()
					{
						std::fill(hash.begin(), hash.end(), KeyType { });
						std::fill(value.begin(), value.end(), ValueType { });
					}
			};

			std::vector<KeyType> m_hashing_keys;
			std::vector<HashTableEntry> m_hashtable;
			size_t m_mask;
			int m_rows = 0;
			int m_cols = 0;
			KeyType m_current_hash = 0;

		public:
			FastHashTable(size_t boardSize, size_t initialSize = 1024) :
					m_hashing_keys(2 * boardSize),
					m_hashtable(roundToPowerOf2(initialSize)),
					m_mask(m_hashtable.size() - 1)
			{
				for (size_t i = 0; i < m_hashing_keys.size(); i++)
					m_hashing_keys[i] = randLong();
			}

			void setBoard(const matrix<Sign> &board) noexcept
			{
				m_rows = board.rows();
				m_cols = board.cols();
				assert(static_cast<int>(m_hashing_keys.size()) == 2 * board.size());

				m_current_hash = 0;
				for (int i = 0; i < board.size(); i++)
					switch (board[i])
					{
						default:
							break;
						case Sign::CROSS:
							m_current_hash ^= m_hashing_keys[2 * i];
							break;
						case Sign::CIRCLE:
							m_current_hash ^= m_hashing_keys[2 * i + 1];
							break;
					}
			}
			void clear() noexcept
			{
				m_hashtable.assign(m_hashtable.size(), HashTableEntry());
			}
			ValueType get(KeyType hash) const noexcept
			{
				const HashTableEntry &entry = m_hashtable[hash & m_mask];
				for (int i = 0; i < N; i++)
					if (entry.hash[i] == hash)
						return entry.value[i];
				return ValueType { };
			}
			bool insert(KeyType hash, ValueType value) noexcept
			{
				assert(value != ValueType { }); // cannot insert default value
				HashTableEntry &entry = m_hashtable[hash & m_mask];
				for (int i = 0; i < N; i++)
					if (entry.value[i] == ValueType { }) // look for empty entry
					{
						entry.hash[i] = hash;
						entry.value[i] = value;
						return true; // successfully inserted
					}
				return false; // could not insert element
			}
			KeyType getHash() const noexcept
			{
				return m_current_hash;
			}
			void updateHash(Move move) noexcept
			{
				assert(move.sign == Sign::CROSS || move.sign == Sign::CIRCLE);
				m_current_hash ^= m_hashing_keys[2 * (move.row * m_cols + move.col) + static_cast<int>(move.sign) - 1];
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SOLVER_FASTHASHTABLE_HPP_ */
