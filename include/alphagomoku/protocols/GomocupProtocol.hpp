/*
 * GomocupProtocol.hpp
 *
 *  Created on: Apr 5, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_GOMOCUPPROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_GOMOCUPPROTOCOL_HPP_

#include <alphagomoku/protocols/Protocol.hpp>

namespace ag
{

	class GomocupProtocol: public Protocol
	{
		protected:
			mutable std::recursive_mutex protocol_mutex;
			std::vector<Move> list_of_moves;
			int rows = 0;
			int columns = 0;
			bool use_suggest = false;
			bool expects_play_command = false;

		public:
			GomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT);

			virtual void reset();
			virtual ProtocolType getType() const noexcept;
			virtual void processInput(InputListener &listener);
			virtual void processOutput(OutputSender &sender);

		protected:
			Sign get_sign_to_move() const noexcept;
			void add_new_move(Move move);
			void check_move_validity(Move move, const std::vector<Move> &playedMoves) const;
			std::string parse_search_summary(const SearchSummary &summary) const;
			std::vector<Move> parse_list_of_moves(InputListener &listener, const std::string &ending) const;

			void INFO(InputListener &listener);

			void START(InputListener &listener);
			void RECTSTART(InputListener &listener);
			void RESTART(InputListener &listener);

			void BEGIN(InputListener &listener);
			void BOARD(InputListener &listener);
			void TURN(InputListener &listener);
			void TAKEBACK(InputListener &listener);
			void PLAY(InputListener &listener);
			void END(InputListener &listener);

			void ABOUT(InputListener &listener);
			void UNKNOWN(InputListener &listener);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_GOMOCUPPROTOCOL_HPP_ */
