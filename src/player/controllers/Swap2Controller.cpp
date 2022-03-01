/*
 * Swap2Controller.cpp
 *
 *  Created on: Feb 10, 2022
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/player/controllers/Swap2Controller.hpp>
#include <alphagomoku/player/SearchEngine.hpp>
#include <alphagomoku/utils/Logger.hpp>

namespace ag
{
	Swap2Controller::Swap2Controller(const EngineSettings &settings, TimeManager &manager, SearchEngine &engine) :
			EngineController(settings, manager, engine)
	{
	}
	void Swap2Controller::control(MessageQueue &outputQueue)
	{

	}
	/*
	 * private
	 */
	void Swap2Controller::load_opening_book()
	{
//		Logger::write("Placing 3 initial stones");
//		if (std::filesystem::exists(engine_settings.) == false)
//			throw std::logic_error("No swap2 opening book");
//		else
//		{
//			FileLoader fl(static_cast<std::string>(config["swap2_openings_file"]));
//			FIXME later
//			switch to using text format - "Xa0"
//			Json json = fl.getJson();
//			int r = randInt(json.size());
//		}
//		std::vector<Move> moves;
//		for (int i = 0; i < json[r].size(); i++)
//			moves.push_back(Move(json[r][i]));
//		return Message(MessageType::BEST_MOVE, moves);
	}
} /* namespace ag */

