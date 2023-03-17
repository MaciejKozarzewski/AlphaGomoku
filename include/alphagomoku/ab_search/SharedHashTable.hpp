/*
 * SharedHashTable.hpp
 *
 *  Created on: Sep 24, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_SHAREDHASHTABLE_HPP_
#define ALPHAGOMOKU_AB_SEARCH_SHAREDHASHTABLE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/ab_search/FastHashFunction.hpp>
#include <alphagomoku/ab_search/Score.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include <array>
#include <vector>
#include <cassert>

namespace ag
{
	namespace obsolete
	{
		enum class ScoreType
		{
			NONE,
			LOWER_BOUND,
			UPPER_BOUND,
			EXACT
		};

		class SharedTableData
		{
				uint64_t m_data = 0;
			public:
				SharedTableData() noexcept :
						SharedTableData(0, Move(), Score(), ScoreType::NONE, 0, false, false)
				{
				}
				SharedTableData(uint64_t data) :
						m_data(data)
				{
				}
				SharedTableData(int nodeCount, Move bestMove, Score score, ScoreType type, int depth, bool isInDanger, bool isThreatening) :
						m_data(0)
				{
					m_data |= static_cast<uint64_t>(isInDanger); // 1 bit
					m_data |= static_cast<uint64_t>(isThreatening) << 1ull; // 1 bit
					m_data |= static_cast<uint64_t>(type) << 2ull; // 2 bits
					m_data |= static_cast<uint64_t>(std::max(0, std::min(4096, depth))) << 4ull; // 12 bits
					m_data |= static_cast<uint64_t>(Score::to_short(score)) << 16ull; // 16 bits
					m_data |= static_cast<uint64_t>(bestMove.toShort()) << 32ull; // 16 bits
					m_data |= static_cast<uint64_t>(std::min(65535, nodeCount)) << 48ull; // 16 bits
				}
				operator uint64_t() const noexcept
				{
					return m_data;
				}
				bool isInDanger() const noexcept
				{
					return m_data & 1;
				}
				bool isThreatening() const noexcept
				{
					return (m_data >> 1ull) & 1;
				}
				ScoreType getScoreType() const noexcept
				{
					return static_cast<ScoreType>((m_data >> 2ull) & 3ull);
				}
				int getDepth() const noexcept
				{
					return (m_data >> 4ull) & 4095ull;
				}
				Score getScore() const noexcept
				{
					return Score::from_short((m_data >> 16ull) & 65535ull);
				}
				Move getMove() const noexcept
				{
					return Move((m_data >> 32ull) & 65535ull);
				}
				int getNodeCount() const noexcept
				{
					return static_cast<int>((m_data >> 48ull) & 65535ull);
				}

		};

		template<int N>
		class SharedHashTable
		{
			private:
				class Entry
				{
					private:
						HashKey<64> m_key;
						SharedTableData m_value;
					public:
						HashKey<64> getKey() const noexcept
						{
							return m_key ^ m_value;
						}
						SharedTableData getValue() const noexcept
						{
							return m_value;
						}
						void set(HashKey<64> key, SharedTableData value) noexcept
						{
							m_key = key ^ value;
							m_value = value;
						}
				};

				std::vector<std::array<Entry, N>> m_hashtable;
				FastHashFunction<64> m_hash_function;
				HashKey<64> m_mask;
			public:
				SharedHashTable(int rows, int columns, size_t initialSize = 1024) :
						m_hashtable(roundToPowerOf2(initialSize)),
						m_hash_function(rows, columns),
						m_mask(m_hashtable.size() - 1)
				{
				}
				int64_t getMemory() const noexcept
				{
					return sizeof(this) + m_hashtable.size() * sizeof(Entry) * N + m_hash_function.getMemory() + sizeof(m_mask);
				}
				const FastHashFunction<64>& getHashFunction() const noexcept
				{
					return m_hash_function;
				}
				void clear() noexcept
				{
					std::fill(m_hashtable.begin(), m_hashtable.end(), Entry());
				}
				SharedTableData seek(HashKey<64> hash) const noexcept
				{
					const std::array<Entry, N> &entry = m_hashtable[hash & m_mask];
					for (size_t i = 0; i < entry.size(); i++)
						if (entry[i].getKey() == hash)
							return entry[i].getValue();
					return SharedTableData();
				}
				void insert(HashKey<64> hash, SharedTableData value) noexcept
				{
					std::array<Entry, N> &entry = m_hashtable[hash & m_mask];
					for (size_t i = 0; i < entry.size() - 1; i++)
						if (value.getDepth() > entry[i].getValue().getDepth())
						{
							entry[i].set(hash, value);
							return;
						}
					entry.back().set(hash, value);
				}
				void prefetch(HashKey<64> hash) const noexcept
				{
					prefetchMemory<PrefetchMode::READ, 1>(m_hashtable.data() + (hash & m_mask));
				}
				void prepareForNextMove() noexcept
				{
//				for (size_t i = 0; i < m_hashtable.size(); i++)
//					for (int j = 0; j < N; j++)
//					{
//						const SharedTableData original = m_hashtable[i][j].getValue();
//						if (original.getScoreType() != ScoreType::NONE)
//						{
//							if (original.getDepth() - 1 < 0)
//								m_hashtable[i][j].set(m_hashtable[i][j].getKey(), SharedTableData());
//							else
//							{
//								SharedTableData new_value(original.getNodeCount() / 2, original.getMove(), original.getScore(),
//										original.getScoreType(), original.getDepth() - 1);
//								m_hashtable[i][j].set(m_hashtable[i][j].getKey(), new_value);
//							}
//						}
//					}
				}
		};

		template<>
		class SharedHashTable<1>
		{
			private:
				class Entry
				{
					private:
						HashKey<64> m_key;
						SharedTableData m_value;
					public:
						HashKey<64> getKey() const noexcept
						{
							return m_key ^ m_value;
						}
						SharedTableData getValue() const noexcept
						{
							return m_value;
						}
						void set(HashKey<64> key, SharedTableData value) noexcept
						{
							m_key = key ^ value;
							m_value = value;
						}
				};

				std::vector<Entry> m_hashtable;
				FastHashFunction<64> m_hash_function;
				HashKey<64> m_mask;
			public:
				SharedHashTable(int rows, int columns, size_t initialSize = 1024) :
						m_hashtable(roundToPowerOf2(initialSize)),
						m_hash_function(rows, columns),
						m_mask(m_hashtable.size() - 1)
				{
				}
				int64_t getMemory() const noexcept
				{
					return sizeof(this) + m_hashtable.size() * sizeof(Entry) + m_hash_function.getMemory() + sizeof(m_mask);
				}
				const FastHashFunction<64>& getHashFunction() const noexcept
				{
					return m_hash_function;
				}
				void clear() noexcept
				{
					std::fill(m_hashtable.begin(), m_hashtable.end(), Entry());
				}
				SharedTableData seek(HashKey<64> hash) const noexcept
				{
					const Entry &entry = m_hashtable[hash & m_mask];
					if (entry.getKey() == hash)
						return entry.getValue();
					return SharedTableData();
				}
				void insert(HashKey<64> hash, SharedTableData value) noexcept
				{
					Entry &entry = m_hashtable[hash & m_mask];
					entry.set(hash, value);
				}
				void prefetch(HashKey<64> hash) const noexcept
				{
					prefetchMemory<PrefetchMode::READ, 1>(m_hashtable.data() + (hash & m_mask));
				}
		};
	} /* namespace obsolete */
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_SHAREDHASHTABLE_HPP_ */
