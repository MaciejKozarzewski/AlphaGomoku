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
			bool is_renju_rule = false;
			int rows = 0;
			int columns = 0;

		public:
			ExtendedGomocupProtocol(MessageQueue &queueIN, MessageQueue &queueOUT);

			void reset();
			ProtocolType getType() const noexcept;
			void processInput(InputListener &listener);
			void processOutput(OutputSender &sender);
		private:
			void START(InputListener &listener);
			void RECTSTART(InputListener &listener);
			void INFO(InputListener &listener);

			void PLAY(InputListener &listener);
			void PONDER(InputListener &listener);
			void STOP(InputListener &listener);
			void SHOWFORBID(InputListener &listener);
			void BALANCE(InputListener &listener);
			void CLEARHASH(InputListener &listener);
			void PROTOCOLVERSION(InputListener &listener);

			/*
			 * opening rules
			 */
			void PROBOARD(InputListener &listener);
			void LONGPROBOARD(InputListener &listener);
			void SWAPBOARD(InputListener &listener);
			void SWAP2BOARD(InputListener &listener);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_EXTENDEDGOMOCUPPROTOCOL_HPP_ */
