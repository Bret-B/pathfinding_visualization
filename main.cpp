#include <SFML/Graphics.hpp>
#include "Visualizer.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cstddef>

int main(int argc, char* argv[]) {
	bool diagonal = false;
	int width = 620;
	int height = 620;
	int tileSize = 19;
	int fps = 120;
	int frameskip = 0;
	int padding = 1;
	std::vector<std::string> arguments(argv + 1, argv + argc);

	// Attempt to parse command line arguments to be used instead of defaults
	for (std::size_t index = 0; index < arguments.size(); ++index) {
		if (arguments[index] == "-diagonal") { diagonal = true; }

		// Read the argument and its value
		if (index >= arguments.size() - 1) { continue; }
		try {
			std::string current = arguments[index];
			std::string next = arguments[index + 1];
			if (current == "-width") {
				width = std::clamp(std::stoi(next), 400, 1600);
			}
			else if (current == "-height") {
				height = std::clamp(std::stoi(next), 400, 1600);
			}
			else if (current == "-tile") {
				tileSize = std::clamp(std::stoi(next), 1, 39);
			}
			else if (current == "-fps") {
				fps = std::clamp(std::stoi(next), 0, 10000);
			}
			else if (current == "-padding") {
				padding = std::clamp(std::stoi(next), 0, 4);
			}
			else if (current == "-skip") {
				frameskip = std::clamp(std::stoi(next), 0, 5000);
			}
		}
		catch (...) {}
	}

	Visualizer visualizer(width, height, tileSize, padding, fps, frameskip, diagonal);
	visualizer.run();

	return 0;
}

