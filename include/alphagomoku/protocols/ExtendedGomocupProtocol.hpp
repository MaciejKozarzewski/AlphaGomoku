/*
 * ExtendedGomocupProtocol.hpp
 *
 *  Created on: Feb 11, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_EXTENDEDGOMOCUPPROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_EXTENDEDGOMOCUPPROTOCOL_HPP_

#include <alphagomoku/protocols/GomocupProtocol.hpp>

namespace ag
{

	class ExtendedGomocupProtocol: public GomocupProtocol
	{
		private:
			bool is_in_analysis_mode = false;
			bool expects_play_command = false;
			bool is_renju_rule = false;

		public:
			ExtendedGomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT);

			void reset();
			ProtocolType getType() const noexcept;
		private:
			/*
			 * Output processors
			 */
			void best_move(OutputSender &sender);
			/*
			 * Input processors
			 */
			void info_evaluate(InputListener &listener);
			void info_rule(InputListener &listener);
			void info_analysis_mode(InputListener &listener);
			void info_max_depth(InputListener &listener);
			void info_max_node(InputListener &listener);
			void info_time_increment(InputListener &listener);
			void info_style(InputListener &listener);
			void info_auto_pondering(InputListener &listener);
			void info_protocol_lag(InputListener &listener);
			void info_thread_num(InputListener &listener);
			void play(InputListener &listener);
			void turn(InputListener &listener);
			void takeback(InputListener &listener);

			void ponder(InputListener &listener);
			void stop(InputListener &listener);
			void showforbid(InputListener &listener);
			void balance(InputListener &listener);
			void clearhash(InputListener &listener);
			void protocolversion(InputListener &listener);
			/*
			 * opening rules
			 */
			void proboard(InputListener &listener);
			void longproboard(InputListener &listener);
			void swapboard(InputListener &listener);
			void swap2board(InputListener &listener);

//			void PONDER(InputListener &listener);
//			void STOP(InputListener &listener);
//			void SHOWFORBID(InputListener &listener);
//			void BALANCE(InputListener &listener);
//			void CLEARHASH(InputListener &listener);
//			void PROTOCOLVERSION(InputListener &listener);

//			/*
//			 * opening rules
//			 */
//			void PROBOARD(InputListener &listener);
//			void LONGPROBOARD(InputListener &listener);
//			void SWAPBOARD(InputListener &listener);
//			void SWAP2BOARD(InputListener &listener);

//			void INFO(InputListener &listener);
//
//			void PONDER(InputListener &listener);
//			void STOP(InputListener &listener);
//			void SHOWFORBID(InputListener &listener);
//			void BALANCE(InputListener &listener);
//			void CLEARHASH(InputListener &listener);
//			void PROTOCOLVERSION(InputListener &listener);
//
//			/*
//			 * opening rules
//			 */
//			void PROBOARD(InputListener &listener);
//			void LONGPROBOARD(InputListener &listener);
//			void SWAPBOARD(InputListener &listener);
//			void SWAP2BOARD(InputListener &listener);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_EXTENDEDGOMOCUPPROTOCOL_HPP_ */
