/*
 * LocalHashTable.hpp
 *
 *  Created on: Oct 10, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_AB_SEARCH_LOCALHASHTABLE_HPP_
#define ALPHAGOMOKU_AB_SEARCH_LOCALHASHTABLE_HPP_

#include <alphagomoku/ab_search/FastHashFunction.hpp>
#include <alphagomoku/ab_search/Score.hpp>

#include <memory>
#include <cassert>

namespace ag
{
	namespace obsolete
	{
		class LocalHashTable
		{
			private:
				static constexpr uint64_t hash_key_mask = 0x0000FFFFFFFFFFFF;
				class Entry
				{
						/*
						 * bits  0:48 - hash
						 * bits 48:64 - value (score)
						 */
						uint64_t m_data;
					public:
						Entry() noexcept :
								Entry(0, Score())
						{
						}
						Entry(HashKey<64> key, Score score) noexcept :
								m_data((static_cast<uint64_t>(Score::to_short(score)) << 48ull) | (static_cast<uint64_t>(key) & hash_key_mask))
						{
						}
						HashKey<64> key() const noexcept
						{
							return HashKey<64>(m_data & hash_key_mask);
						}
						Score value() const noexcept
						{
							return Score::from_short(m_data >> 48ull);
						}
				};

				std::vector<Entry> m_hashtable;
				HashKey<64> m_mask;
			public:
				LocalHashTable(size_t initialSize = 1024) :
						m_hashtable(roundToPowerOf2(initialSize)),
						m_mask(m_hashtable.size() - 1)
				{
				}
				int64_t getMemory() const noexcept
				{
					return sizeof(this) + m_hashtable.size() * sizeof(Entry) + sizeof(m_mask);
				}
				void clear() noexcept
				{
					std::fill(m_hashtable.begin(), m_hashtable.end(), Entry());
//				m_hashtable.assign(m_hashtable.size(), Entry());
				}
				Score seek(HashKey<64> hash) const noexcept
				{
					const Entry &entry = m_hashtable[hash & m_mask];
					if (entry.key() == (hash & HashKey<64>(hash_key_mask)))
						return entry.value();
					return Score();
				}
				void insert(HashKey<64> hash, Score value) noexcept
				{
					m_hashtable[hash & m_mask] = Entry(hash, value);
				}
		};
	} /* namespace obsolete */
} /* namespace ag */

#endif /* ALPHAGOMOKU_AB_SEARCH_LOCALHASHTABLE_HPP_ */
