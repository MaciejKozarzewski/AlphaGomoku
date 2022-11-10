/*
 * RawPatternCalculator.hpp
 *
 *  Created on: Nov 7, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PATTERNS_RAWPATTERNCALCULATOR_HPP_
#define ALPHAGOMOKU_PATTERNS_RAWPATTERNCALCULATOR_HPP_

#include <alphagomoku/patterns/RawPattern.hpp>
#include <alphagomoku/patterns/common.hpp>
#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <cinttypes>
#include <vector>
#include <cassert>

namespace ag
{
	class RawPatternCalculator
	{
			static constexpr uint64_t padding = 6;
			static constexpr uint64_t base_shift = 2 * padding;
			static constexpr uint64_t out_of_board = ones<uint64_t, base_shift>();
			static constexpr uint64_t extended_mask = ones<uint64_t, 2 * ExtendedPattern::length>();
			static constexpr uint64_t normal_mask = ones<uint64_t, 2 * NormalPattern::length>();

			std::vector<uint64_t> m_data;
			int size = 0;
			uint64_t *horizontal = nullptr;
			uint64_t *vertical = nullptr;
			uint64_t *diagonal = nullptr;
			uint64_t *antidiagonal = nullptr;
		public:
			RawPatternCalculator() noexcept = default;
			RawPatternCalculator(int rows, int cols) :
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

			NormalPattern getNormalPatternAt(int row, int col, Direction dir) const noexcept
			{
				switch (dir)
				{
					case HORIZONTAL:
						return NormalPattern(extract<HORIZONTAL, false>(row, col));
					case VERTICAL:
						return NormalPattern(extract<VERTICAL, false>(row, col));
					case DIAGONAL:
						return NormalPattern(extract<DIAGONAL, false>(row, col));
					case ANTIDIAGONAL:
						return NormalPattern(extract<ANTIDIAGONAL, false>(row, col));
					default:
						return NormalPattern();
				}
			}
			ExtendedPattern getExtendedPatternAt(int row, int col, Direction dir) const noexcept
			{
				switch (dir)
				{
					case HORIZONTAL:
						return ExtendedPattern(extract<HORIZONTAL, true>(row, col));
					case VERTICAL:
						return ExtendedPattern(extract<VERTICAL, true>(row, col));
					case DIAGONAL:
						return ExtendedPattern(extract<DIAGONAL, true>(row, col));
					case ANTIDIAGONAL:
						return ExtendedPattern(extract<ANTIDIAGONAL, true>(row, col));
					default:
						return ExtendedPattern();
				}
			}
			void set(const matrix<Sign> &board) noexcept
			{
				assert(board.isSquare());
				assert(board.rows() == static_cast<int>(size));
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
						const uint64_t tmp = static_cast<uint64_t>(board.at(row, col));
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

			template<typename T>
			static DirectionGroup<T> getPatternsAt(const matrix<Sign> &board, Move move) noexcept
			{
				assert(board.isSquare());
				static_assert(std::is_same<T, NormalPattern>::value or std::is_same<T, ExtendedPattern>::value, "");
				const auto get = [](const matrix<Sign> &board, int row, int col)
				{	return board.isInside(row, col) ? static_cast<uint32_t>(board.at(row, col)) : 3u;};

				constexpr int Pad = (T::length - 1) / 2;
				DirectionGroup<uint32_t> result;
				uint32_t shift = 0;
				for (int i = -Pad; i <= Pad; i++, shift += 2)
				{
					result.horizontal |= (get(board, move.row, move.col + i) << shift);
					result.vertical |= (get(board, move.row + i, move.col) << shift);
					result.diagonal |= (get(board, move.row + i, move.col + i) << shift);
					result.antidiagonal |= (get(board, move.row + i, move.col - i) << shift);
				}
				if (board.at(move.row, move.col) != Sign::NONE)
				{ // the patterns must have central spot empty
					const uint32_t mask = ~(3u << static_cast<uint32_t>(2 * Pad));
					result.horizontal &= mask;
					result.vertical &= mask;
					result.diagonal &= mask;
					result.antidiagonal &= mask;
				}
				return DirectionGroup<T>( { T(result.horizontal), T(result.vertical), T(result.diagonal), T(result.antidiagonal) });
			}
			static bool isStraightFourAt(const matrix<Sign> &board, Move move, Direction direction)
			{
				assert(board.isSquare());
				assert(board.at(move.row, move.col) == Sign::NONE);
				assert(move.sign == Sign::CROSS); // this method is intended only for cross (or black) in renju rule
				const auto get = [](const matrix<Sign> &board, int row, int col)
				{	return board.isInside(row, col) ? static_cast<uint32_t>(board.at(row, col)) : 3u;};

				constexpr int Pad = (NormalPattern::length - 1) / 2;
				uint32_t result = static_cast<uint32_t>(move.sign) << (2 * Pad);
				uint32_t shift = 0;
				switch (direction)
				{
					case HORIZONTAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (get(board, move.row, move.col + i) << shift);
						break;
					case VERTICAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (get(board, move.row + i, move.col) << shift);
						break;
					case DIAGONAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (get(board, move.row + i, move.col + i) << shift);
						break;
					case ANTIDIAGONAL:
						for (int i = -Pad; i <= Pad; i++, shift += 2)
							result |= (get(board, move.row + i, move.col - i) << shift);
						break;
				}
				for (int i = 0; i < (2 * Pad + 1 - 4); i++, result /= 4)
					if ((result & 255u) == 85u)
						return true;
				return false;
			}
		private:
			template<Direction Dir, bool Extended>
			uint32_t extract(int row, int col) const noexcept
			{
				assert(0 <= row && row < size && 0 <= col && col < size);
				const uint64_t tmp = data<Dir>()[get_line_index<Dir>(row, col)] >> get_shift_in_line<Dir>(row, col);
				return Extended ? static_cast<uint32_t>(tmp & extended_mask) : static_cast<uint32_t>((tmp >> 2) & normal_mask);
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

#endif /* ALPHAGOMOKU_PATTERNS_RAWPATTERNCALCULATOR_HPP_ */
