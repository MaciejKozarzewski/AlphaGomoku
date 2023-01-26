/*
 * FastHashFunction.hpp
 *
 *  Created on: Sep 24, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_TSS_FASTHASHFUNCTION_HPP_
#define ALPHAGOMOKU_TSS_FASTHASHFUNCTION_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/misc.hpp>

#include <vector>
#include <cassert>

namespace ag
{
	template<int Bits>
	class HashKey
	{
	};

	template<>
	class HashKey<32>
	{
			uint32_t m_key = 0;
		public:
			HashKey() noexcept = default;
			constexpr HashKey(uint32_t key) noexcept :
					m_key(key)
			{
			}
			operator uint64_t() const noexcept
			{
				return m_key;
			}
			HashKey<32>& operator^=(const HashKey<32> &other) noexcept
			{
				m_key ^= other.m_key;
				return *this;
			}
			void init()
			{
				m_key = randInt();
			}
			friend HashKey<32> operator^(const HashKey<32> &lhs, const HashKey<32> &rhs) noexcept
			{
				return HashKey<32>(lhs.m_key ^ rhs.m_key);
			}
			friend HashKey<32> operator&(const HashKey<32> &lhs, const HashKey<32> &rhs) noexcept
			{
				return HashKey<32>(lhs.m_key & rhs.m_key);
			}
			friend bool operator==(const HashKey<32> &lhs, const HashKey<32> &rhs) noexcept
			{
				return lhs.m_key == rhs.m_key;
			}
	};

	template<>
	class HashKey<64>
	{
			uint64_t m_key = 0;
		public:
			HashKey() noexcept = default;
			constexpr HashKey(uint64_t key) noexcept :
					m_key(key)
			{
			}
			operator uint64_t() const noexcept
			{
				return m_key;
			}
			HashKey<64>& operator^=(const HashKey<64> &other) noexcept
			{
				m_key ^= other.m_key;
				return *this;
			}
			void init()
			{
				m_key = randLong();
			}
			HashKey<32> getLow() const noexcept
			{
				return m_key & 0x00000000FFFFFFFF;
			}
			HashKey<32> getHigh() const noexcept
			{
				return m_key >> 32ull;
			}
			friend HashKey<64> operator^(const HashKey<64> &lhs, const HashKey<64> &rhs) noexcept
			{
				return HashKey<64>(lhs.m_key ^ rhs.m_key);
			}
			friend HashKey<64> operator&(const HashKey<64> &lhs, const HashKey<64> &rhs) noexcept
			{
				return HashKey<64>(lhs.m_key & rhs.m_key);
			}
			friend bool operator==(const HashKey<64> &lhs, const HashKey<64> &rhs) noexcept
			{
				return lhs.m_key == rhs.m_key;
			}
	};

	template<>
	class HashKey<128>
	{
			uint64_t m_lower = 0;
			uint64_t m_upper = 0;
		public:
			HashKey() noexcept = default;
			constexpr HashKey(uint64_t key_lo, uint64_t key_hi) noexcept :
					m_lower(key_lo),
					m_upper(key_hi)
			{
			}
			HashKey<128>& operator^=(const HashKey<128> &other) noexcept
			{
				m_lower ^= other.m_lower;
				m_upper ^= other.m_upper;
				return *this;
			}
			void init()
			{
				m_lower = randLong();
				m_upper = randLong();
			}
			HashKey<64> getLow() const noexcept
			{
				return m_lower;
			}
			HashKey<64> getHigh() const noexcept
			{
				return m_upper;
			}
			friend HashKey<128> operator^(const HashKey<128> &lhs, const HashKey<128> &rhs) noexcept
			{
				return HashKey<128>(lhs.m_lower ^ rhs.m_lower, lhs.m_upper ^ rhs.m_upper);
			}
			friend HashKey<128> operator&(const HashKey<128> &lhs, const HashKey<128> &rhs) noexcept
			{
				return HashKey<128>(lhs.m_lower & rhs.m_lower, lhs.m_upper & rhs.m_upper);
			}
			friend bool operator==(const HashKey<128> &lhs, const HashKey<128> &rhs) noexcept
			{
				return lhs.m_lower == rhs.m_lower and lhs.m_upper == rhs.m_upper;
			}
	};

	template<int Bits>
	class FastHashFunction
	{
		private:
			std::vector<HashKey<Bits>> m_keys;
			int m_rows = 0;
			int m_columns = 0;
		public:
			FastHashFunction(int rows, int columns) :
					m_keys(2 * rows * columns),
					m_rows(rows),
					m_columns(columns)
			{
				for (size_t i = 0; i < m_keys.size(); i++)
					m_keys[i].init();
			}
			int64_t getMemory() const noexcept
			{
				return m_keys.size() * sizeof(HashKey<Bits> );
			}
			HashKey<Bits> getHash(const matrix<Sign> &board) const noexcept
			{
				assert(board.rows() == m_rows);
				assert(board.cols() == m_columns);
				assert(static_cast<int>(m_keys.size()) == 2 * board.size());

				HashKey<Bits> result;
				for (int i = 0; i < board.size(); i++)
					switch (board[i])
					{
						default:
							break;
						case Sign::CROSS:
							result ^= m_keys[2 * i];
							break;
						case Sign::CIRCLE:
							result ^= m_keys[2 * i + 1];
							break;
					}
				return result;
			}
			void updateHash(HashKey<Bits> &hash, Move move) const noexcept
			{
				assert(move.sign == Sign::CROSS || move.sign == Sign::CIRCLE);
				hash ^= m_keys[2 * (move.row * m_columns + move.col) + static_cast<int>(move.sign) - 1];
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_TSS_FASTHASHFUNCTION_HPP_ */
