/*
 * RawPatterns.hpp
 *
 *  Created on: Nov 7, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_RAWPATTERNS_HPP_
#define ALPHAGOMOKU_PATTERNS_RAWPATTERNS_HPP_

#include <alphagomoku/patterns/common.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <cinttypes>
#include <vector>

namespace ag
{
	template<int Pad>
	class RawPatterns
	{
			static constexpr uint64_t base_shift = 2 * Pad;
			static constexpr uint64_t out_of_board = ones<uint64_t, base_shift>();
			static constexpr uint64_t extended_mask = ones<uint64_t, 2 * base_shift + 2>();
			static constexpr uint64_t raw_mask = ones<uint64_t, 2 * base_shift - 2>();

			std::vector<uint64_t> m_data;
			int size = 0;
			uint64_t *horizontal = nullptr;
			uint64_t *vertical = nullptr;
			uint64_t *diagonal = nullptr;
			uint64_t *antidiagonal = nullptr;
		public:
			RawPatterns() noexcept = default;
			RawPatterns(int rows, int cols) :
					m_data(6 * rows),
					size(rows),
					horizontal(m_data.data()),
					vertical(m_data.data() + size),
					diagonal(m_data.data() + 3 * size),
					antidiagonal(m_data.data() + 5 * size)
			{
				assert(rows == cols);
				assert(0 <= size && size <= 20);
			}

			uint32_t getRawPatternAt(int row, int col, Direction dir) const noexcept
			{
				switch (dir)
				{
					case HORIZONTAL:
						return extract<HORIZONTAL, false>(row, col);
					case VERTICAL:
						return extract<VERTICAL, false>(row, col);
					case DIAGONAL:
						return extract<DIAGONAL, false>(row, col);
					case ANTIDIAGONAL:
						return extract<ANTIDIAGONAL, false>(row, col);
					default:
						return 0;
				}
			}
			uint32_t getExtendedPatternAt(int row, int col, Direction dir) const noexcept
			{
				switch (dir)
				{
					case HORIZONTAL:
						return extract<HORIZONTAL, true>(row, col);
					case VERTICAL:
						return extract<VERTICAL, true>(row, col);
					case DIAGONAL:
						return extract<DIAGONAL, true>(row, col);
					case ANTIDIAGONAL:
						return extract<ANTIDIAGONAL, true>(row, col);
					default:
						return 0;
				}
			}
			void set(const matrix<int16_t> &board) noexcept
			{
				assert(board.isSquare());
				assert(board.rows() == (size + 2 * Pad));
				for (int i = 0; i < size; i++)
				{
					horizontal[i] = out_of_board | (out_of_board << (base_shift + get_line_length<HORIZONTAL>(i)));
					vertical[i] = out_of_board | (out_of_board << (base_shift + get_line_length<VERTICAL>(i)));
				}
				for (int i = -size + 1; i < size; i++)
				{
					diagonal[i] = out_of_board | (out_of_board << (base_shift + get_line_length<DIAGONAL>(i)));
					antidiagonal[i] = out_of_board | (out_of_board << (base_shift + get_line_length<ANTIDIAGONAL>(i)));
				}

				for (int row = 0; row < size; row++)
					for (int col = 0; col < size; col++)
						{
							const uint64_t tmp = static_cast<uint64_t>(board.at(Pad + row, Pad + col));
							add<HORIZONTAL>(row, col, tmp);
							add<VERTICAL>(row, col, tmp);
							add<DIAGONAL>(row, col, tmp);
							add<ANTIDIAGONAL>(row, col, tmp);
						}
			}
			void addMove(Move m) noexcept
			{
				assert(m.sign != ag::Sign::NONE);
				assert(0 <= m.row && m.row < size && 0 <= m.col && m.col < size);
				const uint64_t tmp = static_cast<uint64_t>(m.sign);
				add<HORIZONTAL>(m.row, m.col, tmp);
				add<VERTICAL>(m.row, m.col, tmp);
				add<DIAGONAL>(m.row, m.col, tmp);
				add<ANTIDIAGONAL>(m.row, m.col, tmp);
			}
			void undoMove(Move m) noexcept
			{
				assert(m.sign != ag::Sign::NONE);
				assert(0 <= m.row && m.row < size && 0 <= m.col && m.col < size);
				undo<HORIZONTAL>(m.row, m.col);
				undo<VERTICAL>(m.row, m.col);
				undo<DIAGONAL>(m.row, m.col);
				undo<ANTIDIAGONAL>(m.row, m.col);
			}
		private:
			template<Direction Dir, bool Extended>
			uint32_t extract(int row, int col) const noexcept
			{
				assert(0 <= row && row < size && 0 <= col && col < size);
				const uint64_t tmp = data<Dir>()[get_line_index<Dir>(row, col)] >> get_shift_in_line<Dir>(row, col);
				return Extended ? static_cast<uint32_t>(tmp & extended_mask) : static_cast<uint32_t>((tmp >> 2) & raw_mask);
			}
			template<Direction Dir>
			uint64_t* data() noexcept
			{
				switch (Dir)
				{
					case HORIZONTAL:
						return horizontal;
					case VERTICAL:
						return vertical;
					case DIAGONAL:
						return diagonal;
					case ANTIDIAGONAL:
						return antidiagonal;
					default:
						return nullptr;
				}
			}
			template<Direction Dir>
			const uint64_t* data() const noexcept
			{
				switch (Dir)
				{
					case HORIZONTAL:
						return horizontal;
					case VERTICAL:
						return vertical;
					case DIAGONAL:
						return diagonal;
					case ANTIDIAGONAL:
						return antidiagonal;
					default:
						return nullptr;
				}
			}
			template<Direction Dir>
			void add(int row, int col, uint64_t val) noexcept
			{
				data<Dir>()[get_line_index<Dir>(row, col)] |= (val << (base_shift + get_shift_in_line<Dir>(row, col)));
			}
			template<Direction Dir>
			void undo(int row, int col) noexcept
			{
				data<Dir>()[get_line_index<Dir>(row, col)] &= ~(3ull << (base_shift + get_shift_in_line<Dir>(row, col)));
			}
			template<Direction Dir>
			uint64_t get_line_index(int row, int col) const noexcept
			{
				assert(0 <= row && row < size && 0 <= col && col < size);
				switch (Dir)
				{
					case HORIZONTAL:
						return row;
					case VERTICAL:
						return col;
					case DIAGONAL:
						return col - row;
					case ANTIDIAGONAL:
						return row - (size - 1 - col);
					default:
						return 0;
				}
			}
			template<Direction Dir>
			uint64_t get_shift_in_line(int row, int col) const noexcept
			{
				assert(0 <= row && row < size && 0 <= col && col < size);
				switch (Dir)
				{
					case HORIZONTAL:
						return 2 * col;
					case VERTICAL:
						return 2 * row;
					case DIAGONAL:
						return 2 * std::min(col, row);
					case ANTIDIAGONAL:
						return 2 * std::min(row, (size - 1 - col));
					default:
						return 0;
				}
			}
			template<Direction Dir>
			uint64_t get_line_length(int idx) const noexcept
			{
				if constexpr (Dir <= VERTICAL)
				{
					assert(0 <= idx && idx < size);
					return 2 * size;
				}
				else
				{
					assert(-size < idx && idx < size);
					return 2 * (size - std::abs(idx));
				}
			}
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PATTERNS_RAWPATTERNS_HPP_ */
