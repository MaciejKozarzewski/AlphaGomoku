/*
 * SearchData.hpp
 *
 *  Created on: Apr 7, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef SELFPLAY_SEARCHDATA_HPP_
#define SELFPLAY_SEARCHDATA_HPP_

#include <alphagomoku/game/Move.hpp>
#include <alphagomoku/mcts/Node.hpp>
#include <alphagomoku/rules/game_rules.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/configs.hpp>
#include <libml/utils/serialization.hpp>

#include <vector>
#include <inttypes.h>

namespace ag
{

	class SearchData
	{
		private:
			struct internal_data
			{
					uint32_t sign :2;
					uint32_t pv :2;
					uint32_t prior :18;
					uint32_t win :14;
					uint32_t draw :14;
					uint32_t loss :14;
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
			void setActionProvenValues(const matrix<ProvenValue> &provenValues) noexcept;
			void setPolicy(const matrix<float> &policy) noexcept;
			void setActionValues(const matrix<Value> &actionValues) noexcept;
			void setMinimaxValue(Value minimaxValue) noexcept;
			void setProvenValue(ProvenValue pv) noexcept;
			void setOutcome(GameOutcome gameOutcome) noexcept;
			void setMove(Move m) noexcept;

			void getBoard(matrix<Sign> &board) const noexcept;
			void getActionProvenValues(matrix<ProvenValue> &provenValues) const noexcept;
			void getPolicy(matrix<float> &policy) const noexcept;
			void getActionValues(matrix<Value> &actionValues) const noexcept;
			Value getMinimaxValue() const noexcept;
			ProvenValue getProvenValue() const noexcept;
			GameOutcome getOutcome() const noexcept;
			Move getMove() const noexcept;

			void serialize(SerializedObject &binary_data) const;
			void print() const;
			bool isCorrect() const noexcept;
	};

} /* namespace ag */

#endif /* SELFPLAY_SEARCHDATA_HPP_ */
