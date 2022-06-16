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
			std::vector<Move> list_of_moves;
			int rows = 0;
			int columns = 0;

		public:
			GomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT);

			virtual void reset();
			virtual ProtocolType getType() const noexcept;
		protected:
			std::string extract_command_data(InputListener &listener, const std::string &command) const;
			Sign get_sign_to_move() const noexcept;
			void add_new_move(Move move);
			void check_move_validity(Move move, const std::vector<Move> &playedMoves) const;
			std::string parse_search_summary(const SearchSummary &summary) const;
			std::vector<Move> parse_list_of_moves(InputListener &listener, const std::string &ending) const;
			/*
			 * Output processing
			 */
			void best_move(OutputSender &sender);
			void plain_string(OutputSender &sender);
			void unknown_command(OutputSender &sender);
			void error(OutputSender &sender);
			void info_message(OutputSender &sender);
			void about_engine(OutputSender &sender);
			/*
			 * Input processing
			 */
			void info_timeout_turn(InputListener &listener);
			void info_timeout_match(InputListener &listener);
			void info_time_left(InputListener &listener);
			void info_max_memory(InputListener &listener);
			void info_game_type(InputListener &listener);
			void info_rule(InputListener &listener);
			void info_evaluate(InputListener &listener);
			void info_folder(InputListener &listener);
			void start(InputListener &listener);
			void rectstart(InputListener &listener);
			void restart(InputListener &listener);
			void begin(InputListener &listener);
			void board(InputListener &listener);
			void turn(InputListener &listener);
			void takeback(InputListener &listener);
			void end(InputListener &listener);
			void about(InputListener &listener);
			void unknown(InputListener &listener);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_GOMOCUPPROTOCOL_HPP_ */
