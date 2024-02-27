/*
 * SharedHashTable.hpp
 *
 *  Created on: Mar 22, 2023
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_SHAREDHASHTABLE_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_SHAREDHASHTABLE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/utils/math_utils.hpp>
#include <alphagomoku/utils/os_utils.hpp>
#include <alphagomoku/utils/AlignedAllocator.hpp>
//#include <alphagomoku/utils/SpinLock.hpp>

#include <array>
#include <vector>
#include <cassert>

namespace ag
{
	class SharedHashTable;

	class SharedTableData
	{
			friend class SharedHashTable;

			static constexpr uint64_t mask = 0xFFFF000000000000ull;

			uint64_t m_data = 0;
			void set_generation_and_key(int gen, uint64_t key) noexcept
			{
				assert(0 <= gen && gen < 64);
				m_data &= ~(mask | 0x00000000000000FC); // clear 16 highest bits and 6 bits where generation is stored
				m_data |= static_cast<uint64_t>(gen) << 2ull; // 6 bits
				m_data |= (key & mask); // 16 highest bits
			}
		public:
			SharedTableData() noexcept :
					SharedTableData(Bound::NONE, 0, Score(), Move())
			{
			}
			SharedTableData(uint64_t data) noexcept :
					m_data(data)
			{
			}
			SharedTableData(Bound bound, int depth, Score score, Move bestMove) noexcept :
					m_data(0)
			{
				assert(0 <= depth && depth < 256);
				m_data |= static_cast<uint64_t>(bound); // 2 bits
				// omitting 6 bits for generation (it is set by the hashtable, not by the user)
				m_data |= static_cast<uint64_t>(depth) << 8ull; // 8 bits
				m_data |= static_cast<uint64_t>(Score::to_short(score)) << 16ull; // 16 bits
				m_data |= static_cast<uint64_t>(bestMove.toShort()) << 32ull; // 16 bits
				// omitting 16 bits for key (it is set by the hashtable, not by the user)
			}
			operator uint64_t() const noexcept
			{
				return m_data;
			}
			Bound bound() const noexcept
			{
				return static_cast<Bound>(m_data & 3ull);
			}
			int generation() const noexcept
			{
				return (m_data >> 2ull) & 63ull;
			}
			int depth() const noexcept
			{
				return static_cast<int>((m_data >> 8ull) & 255ull);
			}
			Score score() const noexcept
			{
				return Score::from_short((m_data >> 16ull) & 65535ull);
			}
			Move move() const noexcept
			{
				return Move(static_cast<uint16_t>(m_data >> 32ull) & 65535ull);
			}
			HashKey64 key() const noexcept
			{
				return m_data & mask; // extract 16 highest bits
			}
	};
	class SharedHashTable
	{
			class Entry
			{
					static constexpr HashKey64 mask = 0xFFFF000000000000ull;

					HashKey64 m_key;
					SharedTableData m_value;
				public:
					Entry() noexcept = default;
					Entry(const HashKey128 &key, SharedTableData value) noexcept :
							m_key(key.getHigh()),
							m_value(value)
					{
					}
					HashKey64 getKey() const noexcept
					{
						return m_key;
					}
					SharedTableData getValue() const noexcept
					{
						return m_value;
					}
					bool key_matches(const HashKey128 &key) const noexcept
					{
						return (getKey() == key.getHigh()) & (getValue().key() == (key.getLow() & mask));
					}
			};

			using Bucket = std::array<Entry, 4>;

			std::vector<Bucket, AlignedAllocator<Bucket, sizeof(Bucket)>> m_hashtable;
//			mutable std::vector<SpinLock, AlignedAllocator<SpinLock, sizeof(SpinLock)>> m_locks;
			FastZobristHashing m_hash_function;
			HashKey64 m_bucket_mask;
//			HashKey64 m_lock_mask;
			int m_base_generation = 0;
		public:
			SharedHashTable(int rows, int columns, size_t initialSize = 1024) :
					m_hashtable(roundToPowerOf2(initialSize) / 4),
//					m_locks(64),
					m_hash_function(rows, columns),
					m_bucket_mask(m_hashtable.size() - 1)
//					m_lock_mask(m_locks.size() - 1)
			{
				clear();
			}
			constexpr int getBucketSize() const noexcept
			{
				return 4;
			}
			int64_t getMemory() const noexcept
			{
				return sizeof(Bucket) * m_hashtable.size() + m_hash_function.getMemory();
			}
			const FastZobristHashing& getHashFunction() const noexcept
			{
				return m_hash_function;
			}
			void clear() noexcept
			{
				std::fill(m_hashtable.begin(), m_hashtable.end(), Bucket());
			}
			void increaseGeneration() noexcept
			{
				m_base_generation = (m_base_generation + 1) % 64;
			}
			SharedTableData seek(const HashKey128 &hash) const noexcept
			{
//				SpinLockGuard guard(get_lock(hash));

				const Bucket &bucket = m_hashtable[get_index_of(hash)];
				for (size_t i = 0; i < bucket.size(); i++)
					if (bucket[i].key_matches(hash))
						return bucket[i].getValue();
				return SharedTableData();
			}
			void insert(const HashKey128 &hash, SharedTableData value) noexcept
			{
//				SpinLockGuard guard(get_lock(hash));

				value.set_generation_and_key(m_base_generation, hash.getLow());
				const Entry new_entry(hash, value);

				Bucket &bucket = m_hashtable[get_index_of(hash)];
				// first check if this position is already stored in the bucket
				if (value.score().isProven() or value.bound() == Bound::EXACT) // but only if the new score is proven or exact
					for (size_t i = 0; i < bucket.size(); i++)
						if (bucket[i].key_matches(hash))
						{
							bucket[i] = new_entry;
							return;
						}

				// now find the least valuable entry to replace
				int idx = 0;
				for (size_t i = 1; i < bucket.size(); i++)
					if (get_value_of(bucket[i]) < get_value_of(bucket[idx]))
						idx = i;
				bucket[idx] = new_entry;
			}
			void prefetch(const HashKey128 &hash) const noexcept
			{
				prefetchMemory<PrefetchMode::READ, 1>(m_hashtable.data() + get_index_of(hash));
			}
			double loadFactor(bool approximate = false) const noexcept
			{
				const size_t size = approximate ? std::max(m_hashtable.size() >> 10, (size_t) 1024) : m_hashtable.size();
				uint64_t result = 0;
				for (size_t i = 0; i < size; i++)
				{
					const Bucket &bucket = m_hashtable[i];
					for (size_t j = 0; j < bucket.size(); j++)
						result += (bucket[j].getValue().bound() != Bound::NONE);
				}
				return static_cast<double>(result) / (size * getBucketSize());
			}
		private:
//			SpinLock& get_lock(const HashKey128 &hash) const noexcept
//			{
//				return m_locks[hash.getLow() & m_lock_mask];
//			}
			size_t get_index_of(const HashKey128 &hash) const noexcept
			{
				return hash.getLow() & m_bucket_mask;
			}
			int get_value_of(const Entry &entry) const noexcept
			{
				return entry.getValue().depth() - (m_base_generation - entry.getValue().generation());
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_SHAREDHASHTABLE_HPP_ */
