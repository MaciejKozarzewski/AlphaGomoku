/*
 * YixinBoardProtocol.hpp
 *
 *  Created on: Jun 13, 2022
 *      Author: Maciej Kozarzewski
 */

#ifndef ALPHAGOMOKU_PROTOCOLS_YIXINBOARDPROTOCOL_HPP_
#define ALPHAGOMOKU_PROTOCOLS_YIXINBOARDPROTOCOL_HPP_

#include <alphagomoku/protocols/GomocupProtocol.hpp>

namespace ag
{

	class YixinBoardProtocol: public GomocupProtocol
	{
		private:
			bool is_renju_rule = false;
			bool show_realtime_info = false;
			bool opening_swap2 = false;
			bool opening_soosorv = false;
			int search_type = 0;
			int nbest = 0;

		public:
			YixinBoardProtocol(MessageQueue &queueIN, MessageQueue &queueOUT);

			void reset();
			ProtocolType getType() const noexcept;
		private:
			void best_move(OutputSender &sender);

			void info_max_depth(InputListener &listener);
			void info_max_node(InputListener &listener);
			void info_time_increment(InputListener &listener);
			void info_caution_factor(InputListener &listener);
			void info_pondering(InputListener &listener);
			void info_usedatabase(InputListener &listener);
			void info_thread_num(InputListener &listener);
			void info_nbest_sym(InputListener &listener);
			void info_checkmate(InputListener &listener);
			void info_hash_size(InputListener &listener);
			void info_thread_split_depth(InputListener &listener);
			void info_rule(InputListener &listener);
			void info_show_detail(InputListener &listener);

			void start(InputListener &listener);
			void yxboard(InputListener &listener);
			void yxstop(InputListener &listener);
			void yxshowforbid(InputListener &listener);
			void yxbalance(InputListener &listener);
			void yxnbest(InputListener &listener);
			/*
			 * hashtable
			 */
			void yxhashclear(InputListener &listener);
			void yxhashdump(InputListener &listener);
			void yxhashload(InputListener &listener);
			void yxshowhashusage(InputListener &listener);
			/*
			 * opening rules
			 */
			void yxswap2(InputListener &listener);
			void yxsoosorv(InputListener &listener);
			/*
			 * search
			 */
			void yxdraw(InputListener &listener);
			void yxresign(InputListener &listener);
			void yxshowinfo(InputListener &listener);
			void yxprintfeature(InputListener &listener);
			void yxblockpathreset(InputListener &listener);
			void yxblockpathundo(InputListener &listener);
			void yxblockpath(InputListener &listener);
			void yxblockreset(InputListener &listener);
			void yxblockundo(InputListener &listener);
			void yxsearchdefend(InputListener &listener);
			/*
			 * database
			 */
			void yxsetdatabase(InputListener &listener);
			void yxquerydatabaseall(InputListener &listener);
			void yxquerydatabaseone(InputListener &listener);
			void yxeditlabeldatabase(InputListener &listener);
			void yxedittvddatabase(InputListener &listener);
			void yxdeletedatabaseone(InputListener &listener);
			void yxdeletedatabaseall(InputListener &listener);
			void yxsetbestmovedatabase(InputListener &listener);
			void yxclearbestmovedatabase(InputListener &listener);
			void yxdbtopos(InputListener &listener);
			void yxdbtotxt(InputListener &listener);
			void yxtxttodb(InputListener &listener);
			void yxdbcheck(InputListener &listener);
			void yxdbfix(InputListener &listener);
	};

} /* namespace ag */

#endif /* ALPHAGOMOKU_PROTOCOLS_YIXINBOARDPROTOCOL_HPP_ */
