/*
 * SearchData.hpp
 *
 *  Created on: Apr 7, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_SELFPLAY_SEARCHDATA_HPP_
#define ALPHAGOMOKU_SELFPLAY_SEARCHDATA_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/game/rules.hpp>
#include <alphagomoku/search/Value.hpp>
#include <alphagomoku/search/Score.hpp>
#include <alphagomoku/utils/matrix.hpp>

#include <cinttypes>

class SerializedObject;
namespace ag
{
	class Node;
}
namespace ag
{

	class SearchData
	{
		private:
			static constexpr float scale = 65535.0f;

			struct internal_data
			{
					uint16_t sign_and_visit_count = 0;
					uint16_t policy_prior = 0;
					Score score;
					uint16_t win_rate = 0;
					uint16_t draw_rate = 0;
			};
			matrix<internal_data> actions;
			Value minimax_value;
			Score minimax_score;
			GameOutcome game_outcome = GameOutcome::UNKNOWN;
			Move played_move;
		public:
			SearchData(const SerializedObject &so, size_t &offset);
			SearchData(int rows, int cols);

			int rows() const noexcept;
			int cols() const noexcept;
			void getBoard(matrix<Sign> &board) const noexcept;

			void setBoard(const matrix<Sign> &board) noexcept;
			void setSearchResults(const Node &node) noexcept;
			void setOutcome(GameOutcome gameOutcome) noexcept;
			void setMove(Move m) noexcept;

			Sign getSign(int row, int col) const noexcept;
			int getVisitCount(int row, int col) const noexcept;
			float getPolicyPrior(int row, int col) const noexcept;
			Score getActionScore(int row, int col) const noexcept;
			Value getActionValue(int row, int col) const noexcept;

			Score getMinimaxScore() const noexcept;
			Value getMinimaxValue() const noexcept;
			GameOutcome getOutcome() const noexcept;
			Move getMove() const noexcept;

			void serialize(SerializedObject &binary_data) const;
			void print() const;
			bool isCorrect() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_SEARCHDATA_HPP_ */
