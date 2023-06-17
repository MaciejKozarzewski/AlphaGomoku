/*
 * selfcheck.cpp
 *
 *  Created on: Jun 13, 2023
 *      Author: Maciej Kozarzewski
 */

#include <alphagomoku/utils/selfcheck.hpp>
#include <alphagomoku/utils/matrix.hpp>
#include <alphagomoku/utils/file_util.hpp>
#include <alphagomoku/game/Board.hpp>
#include <alphagomoku/patterns/PatternCalculator.hpp>
#include <alphagomoku/version.hpp>

#include <minml/core/Device.hpp>
#include <minml/utils/selfcheck.hpp>

#include <iostream>
#include <fstream>
#include <functional>
#include <cstring>

#if defined(__linux__)
#  include <stdio.h>
#  include <stdlib.h>
#  include <unistd.h>
#  include <sys/wait.h>
#elif defined(_WIN32)
#  include <windows.h>
#endif

#define CHECK_ERROR(x, msg) if ((x) == -1) { std::cerr << (msg) << std::endl; exit(1); }

namespace
{
	using namespace ag;

	class Redirect
	{
			std::streambuf *_coutbuf;
			std::ostream &_outf;
		public:
			explicit Redirect(std::ostream &stream) :
					_coutbuf(std::cout.rdbuf()),   // save original rdbuf
					_outf(stream)
			{
				std::cout.rdbuf(_outf.rdbuf()); // replace cout's rdbuf with the file's rdbuf
			}
			~Redirect()
			{
				std::cout << std::flush; // restore cout's rdbuf to the original
				std::cout.rdbuf(_coutbuf);
			}
	};

	bool run_and_redirect_stream(std::ostream &stream, std::function<void()> func)
	{
#if defined(__linux__)
		int fd[2] = { 0, 0 }; /* pipe ends */
		int tmp = pipe(fd);
		CHECK_ERROR(tmp, "Failed to create pipes");

		const pid_t pid = fork();
		CHECK_ERROR(pid, "Failed to create subprocess");

		if (pid == 0)
		{
			tmp = dup2(fd[1], 1);
			CHECK_ERROR(tmp, "Failed to dup2() in child");
			tmp = close(fd[0]);
			CHECK_ERROR(tmp, "Failed to close pipe[0] in child");
			tmp = close(fd[1]);
			CHECK_ERROR(tmp, "Failed to close pipe[1] in child");

			func();
			exit(0);
		}
		else
		{
			tmp = close(fd[1]);
			CHECK_ERROR(tmp, "Failed to close pipe[1] in parent");

			char buffer[1025];
			for (;;)
			{
				std::memset(buffer, 0, 1025);
				const int num = read(fd[0], buffer, 1024);
				if (num <= 0)
					break;
				stream << buffer;
			}

			tmp = close(fd[0]);
			CHECK_ERROR(tmp, "Failed to close pipe[0] in parent");

			int status;
			waitpid(pid, &status, 0);
			stream << "----Exit status = " << std::to_string(status) << std::endl << std::endl;
			return status == 0;
		}
#elif defined(_WIN32)
		Redirect tmp(stream);
		func();
		return true;
#endif
	}


	class PrintAndSave
	{
			std::ostream &_outf;
		public:
			explicit PrintAndSave(std::ostream &stream) :
					_outf(stream)
			{
			}
			void write(const std::string &s)
			{
				_outf << s << std::endl;
				std::cout << s << std::endl;
			}
	};

	void check_ml_backed()
	{
		std::cout << "Detected following devices:\n" << ml::Device::hardwareInfo() << std::endl;

		std::vector<ml::Device> devices = { ml::Device::cpu() };
		for (int i = 0; i < ml::Device::numberOfCudaDevices(); i++)
			devices.push_back(ml::Device::cuda(i));

		for (size_t i = 0; i < devices.size(); i++)
		{
			ml::checkDeviceDetection(devices[i]);
			ml::checkMatrixMultiplication(devices[i]);
			ml::checkWinogradTransforms(devices[i]);
			ml::checkActivationFuncion(devices[i]);
		}
	}
	void check_patterns()
	{
// @formatter:off
		const matrix<Sign> board = Board::fromString(
				/*        a b c d e f g h i j k l m n o          */
				/*  0 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  0 */
				/*  1 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  1 */
				/*  2 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  2 */
				/*  3 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /*  3 */
				/*  4 */" _ _ _ _ _ X X X O _ _ _ _ _ _\n" /*  4 */
				/*  5 */" _ _ _ _ _ X O X X _ _ _ _ _ _\n" /*  5 */
				/*  6 */" _ _ _ _ _ X O O O O X _ _ _ _\n" /*  6 */
				/*  7 */" _ _ _ _ _ O O _ _ _ _ _ _ _ _\n" /*  7 */
				/*  8 */" _ _ _ _ X O _ _ O _ _ _ _ _ _\n" /*  8 */
				/*  9 */" _ _ _ _ _ _ X _ _ _ _ _ _ _ _\n" /*  9 */
				/* 10 */" _ _ _ _ _ _ _ X O _ _ O _ _ _\n" /* 10 */
				/* 11 */" _ _ _ _ _ _ _ O X _ _ _ _ _ _\n" /* 11 */
				/* 12 */" _ _ _ _ _ X X _ _ _ _ _ _ _ _\n" /* 12 */
				/* 13 */" _ _ _ _ _ _ O _ _ _ _ _ _ _ _\n" /* 13 */
				/* 14 */" _ _ _ _ _ _ _ _ _ _ _ _ _ _ _\n" /* 14 */
				/*        a b c d e f g h i j k l m n o          */
				);
// @formatter:on
		const Sign sign_to_move = Sign::CROSS;

		const GameConfig game_config(GameRules::STANDARD, 15, 15);

		std::cout << "Checking pattern calculator" << std::endl;
		PatternCalculator calculator(game_config);
		std::cout << "  Created" << std::endl;
		calculator.setBoard(board, sign_to_move);
		std::cout << "  Board set and patterns initialized" << std::endl;

		const Move move("Xf2");

		calculator.addMove(move);
		std::cout << "  addMove() works" << std::endl;
		calculator.undoMove(move);
		std::cout << "  undoMove() works" << std::endl;

		std::cout << "Checking defensive moves table" << std::endl;
		const NormalPattern pattern(69632u);
		[[maybe_unused]] const BitMask1D<uint16_t> moves1 = getOpenThreePromotionMoves(pattern);
		std::cout << "  getOpenThreePromotionMoves() works" << std::endl;

		std::cout << "Checking threat histogram" << std::endl;
		ThreatHistogram hist;
		const Location loc(0, 0);
		hist.add(ThreatType::FIVE, loc);
		std::cout << "  add() works" << std::endl;
		hist.remove(ThreatType::FIVE, loc);
		std::cout << "  remove() works" << std::endl;
	}
}

namespace ag
{
	void checkMLBackend(std::ostream &stream)
	{
		PrintAndSave printer(stream);
		printer.write("----ML-backend-check-starts----");

		const bool success = run_and_redirect_stream(stream, check_ml_backed);
		if (success)
			printer.write("----ML-backend-check-completed----");
		else
			printer.write("----ML-backend-check-crashed----");
	}
	void checkNeuralNetwork(std::ostream &stream)
	{
	}
	void checkPatternCalculation(std::ostream &stream)
	{
		PrintAndSave printer(stream);
		printer.write("----Patterns-check-starts----");

		const bool success = run_and_redirect_stream(stream, check_patterns);
		if (success)
			printer.write("----Patterns-check-completed----");
		else
			printer.write("----Patterns-check-crashed----");
	}

	void checkConfigFile(std::ostream &stream, const std::string &pathToConfig)
	{
		PrintAndSave printer(stream);

		if (not pathExists(pathToConfig))
		{
			printer.write("path '" + pathToConfig + "' does not exists");
			return;
		}

		try
		{
			FileLoader fl(pathToConfig);
			const Json config = fl.getJson();
			if (config["version"].getString() != ProgramInfo::version())
				printer.write("You are using outdated version of the configuration file.");


		} catch (std::exception &e)
		{
			printer.write("Specified json file could not be loaded. Include it when sending a bug report.");
			return;
		}
	}
	void checkIntegrity(std::ostream &stream, const std::string &launchPath)
	{
	}

} /* namespace ag */

