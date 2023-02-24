/*
 * SharedHashTable.hpp
 *
 *  Created on: Nov 1, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ALPHA_BETA_SHAREDHASHTABLE_HPP_
#define ALPHAGOMOKU_SEARCH_ALPHA_BETA_SHAREDHASHTABLE_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/search/ZobristHashing.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/utils/os_utils.hpp>
#include <alphagomoku/utils/AlignedAllocator.hpp>

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

	class SharedHashTable;

	class SharedTableData
	{
			friend class SharedHashTable;

			union
			{
					struct
					{
							uint8_t flags_bound_gen;
							int8_t depth;
							uint16_t score;
							uint16_t move;
							uint16_t key;
					} data;
					uint64_t raw_bytes;
			};

			static constexpr uint64_t key_mask = 0xFFFF000000000000ull;
			void set_generation_and_key(int gen, uint64_t key) noexcept
			{
				assert(0 <= gen && gen < max_generation);
				raw_bytes &= ~(key_mask | 0x00000000000000F0); // clear 16 highest bits and 4 bits where generation is stored
				raw_bytes |= static_cast<uint64_t>(gen) << 4ull; // 4 bits
				raw_bytes |= (key & key_mask); // 16 highest bits
			}
		public:
			static constexpr int max_generation = 16;
			SharedTableData() noexcept :
		SharedTableData(false, false, Bound::NONE, 0, Score(), Move())
			{
			}
			SharedTableData(bool mustDefend, bool hasInitiative, Bound bound, int depth, Score score, Move bestMove) noexcept
			{
				data.flags_bound_gen = static_cast<uint8_t>(mustDefend); // 1 bit
				data.flags_bound_gen |= static_cast<uint8_t>(hasInitiative) << 1ull; // 1 bit
				data.flags_bound_gen |= static_cast<uint8_t>(bound) << 2ull; // 2 bits
				// omitting 4 bits for generation (it is set by the hashtable, not by the user)
				assert(-128 <= depth && depth <= 127);
				data.depth = static_cast<int8_t>(depth); // 8 bits
				data.score = Score::to_short(score); // 16 bits
				data.move = bestMove.toShort(); // 16 bits
				data.key = 0u; // key is set by the hashtable, not by the user
			}

			SharedTableData(const SharedTableData &other) noexcept :
					raw_bytes(other.raw_bytes)
			{
			}
			SharedTableData(SharedTableData &&other) noexcept :
					raw_bytes(other.raw_bytes)
			{
			}
			SharedTableData& operator=(const SharedTableData &other) noexcept
			{
				this->raw_bytes = other.raw_bytes;
				return *this;
			}
			SharedTableData& operator=(SharedTableData &&other) noexcept
			{
				this->raw_bytes = other.raw_bytes;
				return *this;
			}
			bool mustDefend() const noexcept
			{
				return data.flags_bound_gen & 1ull;
			}
			bool hasInitiative() const noexcept
			{
				return data.flags_bound_gen & 2ull;
			}
			Bound bound() const noexcept
			{
				return static_cast<Bound>((data.flags_bound_gen >> 2ull) & 3ull);
			}
			int generation() const noexcept
			{
				return (data.flags_bound_gen >> 4ull) & 15ull;
			}
			int depth() const noexcept
			{
				return data.depth;
			}
			Score score() const noexcept
			{
				return Score::from_short(data.score);
			}
			Move move() const noexcept
			{
				return Move::move_from_short(data.move);
			}
			bool key_matches(uint64_t key) const noexcept
			{
				return (raw_bytes & key_mask) == (key & key_mask);
			}

	};
	class SharedHashTable
	{
			using bucket_type = std::array<SharedTableData, 4>;

			std::vector<bucket_type, AlignedAllocator<bucket_type, 64>> m_hashtable;
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
			constexpr int getBucketSize() const noexcept
			{
				return 4;
			}
			int64_t getMemory() const noexcept
			{
				return sizeof(bucket_type) * m_hashtable.size() + m_hash_function.getMemory();
			}
			const FastZobristHashing& getHashFunction() const noexcept
			{
				return m_hash_function;
			}
			void clear() noexcept
			{
				std::fill(m_hashtable.begin(), m_hashtable.end(), bucket_type());
			}
			void increaseGeneration() noexcept
			{
				m_base_generation = (m_base_generation + 1) % SharedTableData::max_generation;
			}
			SharedTableData seek(const HashKey64 &hash) const noexcept
			{
				const bucket_type local_bucket = m_hashtable[hash & m_mask];
				for (size_t i = 0; i < local_bucket.size(); i++)
					if (local_bucket[i].key_matches(hash))
						return local_bucket[i];
				return SharedTableData();
			}
			void insert(const HashKey64 &hash, SharedTableData value) noexcept
			{
				value.set_generation_and_key(m_base_generation, hash);

				bucket_type &shared_bucket = m_hashtable[hash & m_mask];
				const bucket_type local_bucket = shared_bucket;
				// first check if this position is already stored in the bucket
				if (value.score().isProven() or value.bound() == Bound::EXACT) // but only if the new score is proven or exact
					for (size_t i = 0; i < local_bucket.size(); i++)
						if (local_bucket[i].key_matches(hash))
						{
							shared_bucket[i] = value;
							return;
						}

				// now find the least valuable entry to replace
				int idx = 0;
				for (size_t i = 1; i < local_bucket.size(); i++)  // loop over entries that are replaced based on depth
					if (get_value_of(local_bucket[i]) < get_value_of(local_bucket[idx]))
						idx = i;
				shared_bucket[idx] = value;
			}
			void prefetch(const HashKey64 &hash) const noexcept
			{
				prefetchMemory<PrefetchMode::READ, 1>(m_hashtable.data() + (hash & m_mask));
			}
			double loadFactor(bool approximate = false) const noexcept
			{
				const size_t size = approximate ? std::max(m_hashtable.size() >> 10, (size_t) 1024) : m_hashtable.size();
				uint64_t result = 0;
				for (size_t i = 0; i < size; i++)
				{
					const bucket_type local_bucket = m_hashtable[i];
					for (size_t j = 0; j < local_bucket.size(); j++)
						result += (local_bucket[j].bound() != Bound::NONE);
				}
				return static_cast<double>(result) / (size * getBucketSize());
			}
		private:
			int get_value_of(const SharedTableData &entry) const noexcept
			{
				return entry.depth() - (m_base_generation - entry.generation());
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ALPHA_BETA_SHAREDHASHTABLE_HPP_ */
