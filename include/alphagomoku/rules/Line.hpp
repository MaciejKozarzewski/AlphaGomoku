/*
 * Line.hpp
 *
 *  Created on: Jul 27, 2021
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_RULES_LINE_HPP_
#define ALPHAGOMOKU_RULES_LINE_HPP_

#include <inttypes.h>
#include <string>

namespace ag
{
	enum class GameRules
	;
} /* namespace ag */

namespace ag
{

	class Line
	{
		private:
			struct Data
			{
					bool was_evaluated = false;
					bool value;
			};
			uint32_t line = 0;
			GameRules rules;

			Data cross_five; // five here means winning five, so overlines (for renju or standard) does not count here, as well as blocked five (in caro)
			Data cross_open_four; // open four is a pattern that can become five (defined above) in more than one way
			Data cross_four; // four is a pattern that can become five (defined above) in only one way
			Data cross_open_three; // open four is a pattern that can become an open four (defined above) in more than one way

		public:
			Line() = default;
			Line(const std::string &text, GameRules rules);

			bool is_cross_five() noexcept;
			bool is_cross_open_four() noexcept;
			bool is_cross_four() noexcept;
			bool is_cross_inline_4x4() noexcept;
			bool is_cross_open_three() noexcept;

			std::string toString() const;
			std::string toBinaryString() const;
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_RULES_LINE_HPP_ */
