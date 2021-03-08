/*
 * launcher.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: maciek
 */

#include <alphagomoku/selfplay/Game.hpp>
#include <alphagomoku/selfplay/GameBuffer.hpp>
#include <chrono>
#include <iostream>

using namespace ag;

double get_time()
{
	return std::chrono::system_clock::now().time_since_epoch().count() * 1.0e-9;
}
int main()
{
	std::string path = "/home/maciek/alphagomoku/";
	std::string name = "standard_15x15_correct";
	int number = 99;
	for (int i = 0; i <= number; i++)
	{
		std::cout << "train buffer " << i << "/" << number << '\n';
		GameBuffer buff(path + name + "/train_buffer/buffer_" + std::to_string(i) + ".bin");
		buff.save("/home/maciek/gomoku_datasets/" + name + "/train_buffer/buffer_" + std::to_string(i) + ".zip");
	}

	for (int i = 0; i <= number; i++)
	{
		std::cout << "valid buffer " << i << "/" << number << '\n';
		GameBuffer buff(path + name + "/valid_buffer/buffer_" + std::to_string(i) + ".bin");
		buff.save("/home/maciek/gomoku_datasets/" + name + "/valid_buffer/buffer_" + std::to_string(i) + ".zip");
	}

//	GameBuffer buff("/home/maciek/Desktop/test_buffer.bin");
//	for (int i = 0; i < buff.getFromBuffer(0).length(); i++)
//		buff.getFromBuffer(0).printSample(i);

	return 0;
}

