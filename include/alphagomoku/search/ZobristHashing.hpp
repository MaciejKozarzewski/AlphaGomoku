/*
 * ZobristHashing.hpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SEARCH_ZOBRISTHASHING_HPP_
#define ALPHAGOMOKU_SEARCH_ZOBRISTHASHING_HPP_

#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>
#include <alphagomoku/game/Move.hpp>

#include <vector>
#include <cinttypes>

namespace ag
{
	class HashKey64
	{
			uint64_t m_key = 0;
		public:
			HashKey64() noexcept = default;
			constexpr HashKey64(uint64_t key) noexcept :
					m_key(key)
			{
			}
			HashKey64& operator^=(const HashKey64 &other) noexcept
			{
				m_key ^= other.m_key;
				return *this;
			}
			operator uint64_t() const noexcept
			{
				return m_key;
			}
			void init()
			{
				m_key = randLong();
			}
			friend HashKey64 operator^(const HashKey64 &lhs, const HashKey64 &rhs) noexcept
			{
				return HashKey64(lhs.m_key ^ rhs.m_key);
			}
			friend HashKey64 operator&(const HashKey64 &lhs, const HashKey64 &rhs) noexcept
			{
				return HashKey64(lhs.m_key & rhs.m_key);
			}
			friend bool operator==(const HashKey64 &lhs, const HashKey64 &rhs) noexcept
			{
				return lhs.m_key == rhs.m_key;
			}
	};

	class HashKey128
	{
			HashKey64 m_lower;
			HashKey64 m_upper;
		public:
			HashKey128() noexcept = default;
			constexpr HashKey128(HashKey64 key_lo, HashKey64 key_hi) noexcept :
					m_lower(key_lo),
					m_upper(key_hi)
			{
			}
			HashKey128& operator^=(const HashKey128 &other) noexcept
			{
				m_lower ^= other.m_lower;
				m_upper ^= other.m_upper;
				return *this;
			}
			void init()
			{
				m_lower.init();
				m_upper.init();
			}
			HashKey64 getLow() const noexcept
			{
				return m_lower;
			}
			HashKey64 getHigh() const noexcept
			{
				return m_upper;
			}
			friend HashKey128 operator^(const HashKey128 &lhs, const HashKey128 &rhs) noexcept
			{
				return HashKey128(lhs.m_lower ^ rhs.m_lower, lhs.m_upper ^ rhs.m_upper);
			}
			friend HashKey128 operator&(const HashKey128 &lhs, const HashKey128 &rhs) noexcept
			{
				return HashKey128(lhs.m_lower & rhs.m_lower, lhs.m_upper & rhs.m_upper);
			}
			friend bool operator==(const HashKey128 &lhs, const HashKey128 &rhs) noexcept
			{
				return lhs.m_lower == rhs.m_lower and lhs.m_upper == rhs.m_upper;
			}
	};

	class FullZobristHashing
	{
		private:
			std::vector<HashKey64> m_keys;
		public:
			FullZobristHashing() noexcept = default;
			FullZobristHashing(int boardHeight, int boardWidth);
			HashKey64 getHash(const matrix<Sign> &board, Sign signToMove) const noexcept;
	};

	class FastZobristHashing
	{
		private:
			std::vector<HashKey128> m_keys;
			int m_rows = 0;
			int m_columns = 0;
		public:
			FastZobristHashing() noexcept = default;
			FastZobristHashing(int boardHeight, int boardWidth);
			int64_t getMemory() const noexcept;
			HashKey128 getHash(const matrix<Sign> &board) const noexcept;
			void updateHash(HashKey128 &hash, Move move) const noexcept
			{
				assert(move.sign == Sign::CROSS || move.sign == Sign::CIRCLE);
				hash ^= m_keys[2 * (move.row * m_columns + move.col) + static_cast<int>(move.sign) - 1];
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SEARCH_ZOBRISTHASHING_HPP_ */
