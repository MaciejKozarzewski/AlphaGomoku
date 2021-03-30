/*
 * launcher.cpp
 *
 *  Created on: Mar 30, 2021
 *      Author: maciek
 */

#include <iostream>

int main(int argc, char *argv[])
{
	std::cout << "MESSAGE compiled on " << __DATE__ << " at " << __TIME__ << std::endl;

//	std::string localLaunchPath(argv[0]);
//	int last_slash = -1;
//	for (int i = 0; i < (int) localLaunchPath.length(); i++)
//		if (localLaunchPath[i] == '\\' || localLaunchPath[i] == '/')
//			last_slash = i + 1;
//	localLaunchPath = localLaunchPath.substr(0, last_slash);
//	std::ofstream ofs(localLaunchPath + "\\logfile.log");
//	std::clog.rdbuf(ofs.rdbuf());
//	std::clog << "Compiled on " << __DATE__ << " at " << __TIME__ << std::endl;
//	std::clog << "Launched in " << localLaunchPath << std::endl;

//	GomocupPlayer player(localLaunchPath);
//	player.run();

	return 0;
}

