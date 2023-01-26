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
#include <alphagomoku/mcts/Value.hpp>
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
			static constexpr float p_scale = 1048575.0f;
			static constexpr float q_scale = 16383.0f;

			struct internal_data
			{
					uint32_t sign :2;
					uint32_t proven_value :2;
					uint32_t visit_count :12;
					uint32_t policy_prior :20;
					uint32_t win :14;
					uint32_t draw :14;
			};
			matrix<internal_data> actions;
			Value minimax_value;
			ProvenValue proven_value = ProvenValue::UNKNOWN;
			GameOutcome game_outcome = GameOutcome::UNKNOWN;
			Move played_move;
		public:
			SearchData(const SerializedObject &so, size_t &offset);
			SearchData(int rows, int cols);

			int rows() const noexcept;
			int cols() const noexcept;

			void setBoard(const matrix<Sign> &board) noexcept;
			void setSearchResults(const Node &node) noexcept;
			void setOutcome(GameOutcome gameOutcome) noexcept;
			void setMove(Move m) noexcept;

			Sign getSign(int row, int col) const noexcept;
			ProvenValue getActionProvenValue(int row, int col) const noexcept;
			int getVisitCount(int row, int col) const noexcept;
			float getPolicyPrior(int row, int col) const noexcept;
			Value getActionValue(int row, int col) const noexcept;
			Value getMinimaxValue() const noexcept;
			ProvenValue getProvenValue() const noexcept;
			GameOutcome getOutcome() const noexcept;
			Move getMove() const noexcept;

			void serialize(SerializedObject &binary_data) const;
			void print() const;
			bool isCorrect() const noexcept;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_SELFPLAY_SEARCHDATA_HPP_ */
