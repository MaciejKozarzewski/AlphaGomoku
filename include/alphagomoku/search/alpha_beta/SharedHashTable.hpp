/*
 * SharedHashTable.hpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TSS_SHAREDHASHTABLE_HPP_
#define ALPHAGOMOKU_TSS_SHAREDHASHTABLE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/utils/AlignedStorage.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/os_utils.hpp>

#include <array>
#include <vector>
#include <cassert>

namespace ag
{
	enum class Bound
	{
		NONE,
		LOWER,
		UPPER,
		EXACT
	};

	template<int N>
	class SharedHashTable;

	class SharedTableData
	{
			template<int N>
			friend class SharedHashTable;

			static constexpr uint64_t mask = 0xFFFF000000000000ull;

			uint64_t m_data = 0;
			void set_generation_and_key(int gen, uint64_t key) noexcept
			{
				m_data &= ~(mask | 0x00000000000000F0); // clear 16 highest bits and 4 bits where generation is stored
				m_data |= static_cast<uint64_t>(std::max(0, std::min(15, gen))) << 4ull; // 4 bits
				m_data |= (key & mask); // 16 highest bits
			}
		public:
			SharedTableData() noexcept :
					SharedTableData(false, false, Bound::NONE, 0, Score(), Move())
			{
			}
			SharedTableData(uint64_t data) :
					m_data(data)
			{
			}
			SharedTableData(bool mustDefend, bool hasInitiative, Bound bound, int depth, Score score, Move bestMove) :
					m_data(0)
			{
				m_data |= static_cast<uint64_t>(mustDefend); // 1 bit
				m_data |= static_cast<uint64_t>(hasInitiative) << 1ull; // 1 bit
				m_data |= static_cast<uint64_t>(bound) << 2ull; // 2 bits
				// omitting 4 bits for generation (it is set by the hashtable, not by the user)
				m_data |= static_cast<uint64_t>(128 + std::max(-128, std::min(127, depth))) << 8ull; // 8 bits
				m_data |= static_cast<uint64_t>(Score::to_short(score)) << 16ull; // 16 bits
				m_data |= static_cast<uint64_t>(bestMove.toShort()) << 32ull; // 16 bits
				// omitting 16 bits for key (it is set by the hashtable, not by the user)
			}
			operator uint64_t() const noexcept
			{
				return m_data;
			}
			bool mustDefend() const noexcept
			{
				return m_data & 1ull;
			}
			bool hasInitiative() const noexcept
			{
				return (m_data >> 1ull) & 1ull;
			}
			Bound bound() const noexcept
			{
				return static_cast<Bound>((m_data >> 2ull) & 3ull);
			}
			int generation() const noexcept
			{
				return (m_data >> 4ull) & 15ull;
			}
			int depth() const noexcept
			{
				return static_cast<int>((m_data >> 8ull) & 255ull) - 128;
			}
			Score score() const noexcept
			{
				return Score::from_short((m_data >> 16ull) & 65535ull);
			}
			Move move() const noexcept
			{
				return Move::move_from_short((m_data >> 32ull) & 65535ull);
			}
			HashKey64 key() const noexcept
			{
				return m_data & mask; // extract 16 highest bits
			}
	};
	template<int N>
	class SharedHashTable
	{
			class Entry
			{
					static constexpr HashKey64 mask = 0xFFFF000000000000ull;

					HashKey64 m_key = 0u;
					SharedTableData m_value;
				public:
					Entry() noexcept = default;
					Entry(const HashKey128 &key, SharedTableData value) noexcept :
							m_key(key.getHigh() ^ value),
							m_value(value)
					{
					}
					HashKey64 getKey() const noexcept
					{
						return m_key ^ m_value;
					}
					SharedTableData getValue() const noexcept
					{
						return m_value;
					}
					bool key_matches(const HashKey128 &key) const noexcept
					{
						return getKey() == key.getHigh() and getValue().key() == (key.getLow() & mask);
					}
			};

			AlignedStorage<std::array<Entry, N>, 64> m_hashtable;
			FastZobristHashing m_hash_function;
			HashKey64 m_mask;
			int m_base_generation = 0;
		public:
			SharedHashTable(int rows, int columns, size_t initialSize = 1024) :
					m_hashtable(roundToPowerOf2(initialSize)),
					m_hash_function(rows, columns),
					m_mask(m_hashtable.size() - 1)
			{
				clear();
			}
			int64_t getMemory() const noexcept
			{
				return m_hashtable.sizeInBytes() + m_hash_function.getMemory();
			}
			const FastZobristHashing& getHashFunction() const noexcept
			{
				return m_hash_function;
			}
			void clear() noexcept
			{
				std::fill(m_hashtable.begin(), m_hashtable.end(), std::array<Entry, N>());
			}
			void increaseGeneration() noexcept
			{
				m_base_generation = (m_base_generation + 1) % 16;
			}
			SharedTableData seek(const HashKey128 &hash) const noexcept
			{
				const std::array<Entry, N> &bucket = m_hashtable[hash.getLow() & m_mask];
				for (size_t i = 0; i < bucket.size(); i++)
				{
					const Entry entry = bucket[i]; // copy entry to stack to prevent surprises from read/write race
					if (entry.key_matches(hash))
						return entry.getValue();
				}
				return SharedTableData();
			}
			void insert(const HashKey128 &hash, SharedTableData value) noexcept
			{
				value.set_generation_and_key(m_base_generation, hash.getLow());
				const Entry new_entry(hash, value);

				std::array<Entry, N> &bucket = m_hashtable[hash.getLow() & m_mask];
				// first check if this position is already stored in the bucket
				if (value.score().isProven() or value.bound() == Bound::EXACT) // but only if the new score is proven or exact
					for (size_t i = 0; i < bucket.size(); i++)
					{
						const Entry entry = bucket[i]; // copy entry to stack to prevent surprises from read/write race
						if (entry.key_matches(hash))
						{
							bucket[i] = new_entry;
							return;
						}
					}

				// now find the least valuable entry to replace
				Entry *found = &bucket[0]; // first entry is the 'always replace' one
				for (size_t i = 1; i < bucket.size(); i++)
				{ // loop over entries that are replaced based on depth
					const Entry entry = bucket[i]; // copy entry to stack to prevent surprises from read/write race
					if (get_value_of(entry) < get_value_of(*found))
						found = &bucket[i];
				}
				*found = new_entry;
			}
			void prefetch(const HashKey128 &hash) const noexcept
			{
				prefetch_memory<PrefetchMode::READ, 1>(m_hashtable.data() + (hash.getLow() & m_mask));
			}
			double loadFactor(bool approximate = false) const noexcept
			{
				const size_t size = approximate ? std::max(m_hashtable.size() >> 10, (size_t) 1024) : m_hashtable.size();
				double result = 0.0;
				for (size_t i = 0; i < size; i++)
					for (int j = 0; j < N; j++)
						result += (m_hashtable[i][j].getValue().bound() != Bound::NONE);
				return result / (size * N);
			}
		private:
			int get_value_of(const Entry &entry) const noexcept
			{
				return entry.getValue().depth() - (m_base_generation - entry.getValue().generation());
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_TSS_SHAREDHASHTABLE_HPP_ */