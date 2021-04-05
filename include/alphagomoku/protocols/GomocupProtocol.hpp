/*
 * GomocupProtocol.hpp
 *
 *  Created on: 5 kwi 2021
 *      Author: maciek
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_GOMOCUPPROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_GOMOCUPPROTOCOL_HPP_

#include <alphagomoku/protocols/Protocol.hpp>

namespace ag
{

	class GomocupProtocol: public Protocol
	{
		private:
			std::vector<Move> list_of_moves;

		public:
			GomocupProtocol() = default;

			ProtocolType getType() const noexcept;
			Message processInput(InputListener &listener);
			bool processOutput(const Message &msg, OutputSender &sender);

		private:
			Sign get_sign_to_move() const noexcept;

			Message INFO(InputListener &listener);

			Message START(InputListener &listener);
			Message RECTSTART(InputListener &listener);
			Message RESTART(InputListener &listener);

			// opening rules
			Message PROBOARD(InputListener &listener);
			Message LONGPROBOARD(InputListener &listener);
			Message SWAPBOARD(InputListener &listener);
			Message SWAP2BOARD(InputListener &listener);

			Message BEGIN(InputListener &listener);
			Message BOARD(InputListener &listener);
			Message TURN(InputListener &listener);
			Message TAKEBACK(InputListener &listener);
			Message END(InputListener &listener);

			Message PLAY(InputListener &listener);
			Message ABOUT(InputListener &listener);
			Message UNKNOWN(InputListener &listener);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_GOMOCUPPROTOCOL_HPP_ */
