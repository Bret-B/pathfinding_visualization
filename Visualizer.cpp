#include "Visualizer.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <limits>

#define SFML_DEFINE_DISCRETE_GPU_PREFERENCE

Visualizer::Visualizer(int width, int height, int squarePix, int padding, int fps, int frameskip, bool diagonal) : 
	rng(std::random_device()()),
	window(sf::VideoMode(width, height), "Pathfinding Visualization", sf::Style::Titlebar | sf::Style::Close) {

	desiredFps = fps;
	window.setFramerateLimit(fps);
	this->frameskip = frameskip;
	tileSpacing = padding;
	moveDiagonal = diagonal;
	moveVal = static_cast<float>(squarePix + padding);
	rowCount = static_cast<int>(height / moveVal);
	colCount = static_cast<int>(width / moveVal);
	start = 0;
	end = (rowCount - 1) * colCount + colCount - 1;
	tileVertices.setPrimitiveType(sf::Quads);
	tileVertices.resize(rowCount * colCount * 4);
	tiles.resize(rowCount * colCount, false);
	blankTileVertices();
}

void Visualizer::run() {
	bool m1Down = false;
	bool m2Down = false;
	bool drawing = true;

	// Function to clear any left over colored squares when user starts drawing
	auto startDrawingifNeeded = [&]() {
		if (!drawing) {
			drawing = true;
			blankTileVertices();
		}
	};

	// Main loop: handle events such as mouse clicks and keypresses
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left) {
					m1Down = true;
				}
				else if (event.mouseButton.button == sf::Mouse::Right) {
					m2Down = true;
				}
				break;
			case sf::Event::MouseButtonReleased:
				if (event.mouseButton.button == sf::Mouse::Left) {
					m1Down = false;
				}
				else if (event.mouseButton.button == sf::Mouse::Right) {
					m2Down = false;
				}
				break;
			case sf::Event::KeyPressed:
				switch (event.key.code) {
				case sf::Keyboard::Space:
					blankTileVertices();
					aStar();
					drawing = false;
					break;
				case sf::Keyboard::R:
					setAllTiles(false);
					blankTileVertices();
					drawing = true;
					break;
				case sf::Keyboard::S:
					setStart(getHoveredSquare());
					startDrawingifNeeded();
					break;
				case sf::Keyboard::E:
					setEnd(getHoveredSquare());
					startDrawingifNeeded();
					break;
				case sf::Keyboard::M:
					mazeDepthFirstSearch();
					break;
				case sf::Keyboard::N:
					mazeWilsons();
					break;
				}
				break;
			}
		}

		// Handle mouse input if any (but not when both buttons are pressed)
		if (m1Down ^ m2Down) {
			// Clear any left over colors if user is drawing on tiles again
			startDrawingifNeeded();
			handleClicks(m1Down, m2Down);
		}

		// Re-draw everything
		draw();
	}
}

// Update the screen 
void Visualizer::draw() {
	if (++frame < frameskip) { return; }

	frame = 0;
	window.clear(COLORGRAY);
	window.draw(tileVertices);
	window.display();
}

// Keep window responsive and respond to quit
bool Visualizer::checkQuit() {
	sf::Event event;
	while (window.pollEvent(event)) {
		if (event.type == sf::Event::Closed) {
			window.close();
			return true;
		}
		else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
			return true;
		}
	}
	return false;
}

// Check if 2 tiles are perfectly diagonal (at any distance) by comparing
// their row and column difference magnitudes
bool Visualizer::isDiagonal(int from, int to) {
	int rowFrom = from / colCount;
	int colFrom = from % colCount;
	int rowTo = to / colCount;
	int colTo = to % colCount;
	return std::abs(rowFrom - rowTo) == std::abs(colFrom - colTo);
}

bool Visualizer::isDiagonal(int rowFrom, int colFrom, int rowTo, int colTo) {
	return std::abs(rowFrom - rowTo) == std::abs(colFrom - colTo);
}

// Check if 2 tiles are in a perfectly horizontal/vertical line
bool Visualizer::isStraight(int from, int to) {
	int rowFrom = from / colCount;
	int colFrom = from % colCount;
	int rowTo = to / colCount;
	int colTo = to % colCount;
	return (rowFrom == rowTo || colFrom == colTo);
}

bool Visualizer::isStraight(int rowFrom, int colFrom, int rowTo, int colTo) {
	return (rowFrom == rowTo || colFrom == colTo);
}

// Color a specific tile by index with a provided color
void Visualizer::colorQuad(int tile, sf::Color color) {
	sf::Vertex* quad = &tileVertices[tile * 4];
	for (int i = 0; i < 4; i++) {
		quad[i].color = color;
	}
}

// Reset all open tiles to white and all blocked tiles to black
// Start and end get their own special colors
void Visualizer::blankTileVertices() {
	for (int row = 0; row < rowCount; row++) {
		for (int col = 0; col < colCount; col++) {
			sf::Vertex* quad = &tileVertices[(row * colCount + col) * 4];
			quad[0].position = sf::Vector2f(col * moveVal, row * moveVal);
			quad[1].position = sf::Vector2f((col + 1) * moveVal - tileSpacing, row * moveVal);
			quad[2].position = sf::Vector2f((col + 1) * moveVal - tileSpacing, (row + 1) * moveVal - tileSpacing);
			quad[3].position = sf::Vector2f(col * moveVal, (row + 1) * moveVal - tileSpacing);
			sf::Color tileColor = (tiles[row * colCount + col]) ? sf::Color::Black : sf::Color::White;
			for (int i = 0; i < 4; i++) {
				quad[i].color = tileColor;
			}
		}
	}

	sf::Vertex* st = &tileVertices[start * 4];
	sf::Vertex* fn = &tileVertices[end * 4];
	for (int i = 0; i < 4; i++) {
		st[i].color = sf::Color::Cyan;
		fn[i].color = sf::Color::Magenta;
	}
}

// Unblock or block all nodes/tiles
void Visualizer::setAllTiles(bool blocked) {
	for (int i = 0; i < rowCount * colCount; i++) {
		tiles[i] = blocked;
	}
}

// Takes vector of window mouse pos and returns an int representing square index
// Note: coordinates on the "border" for a tile will still count
int Visualizer::getHoveredSquare() {
	sf::Vector2i coordinates = sf::Mouse::getPosition(window);
	// Clamps to a valid square
	int col = std::clamp(static_cast<int>(coordinates.x / moveVal), 0, colCount - 1);
	int row = std::clamp(static_cast<int>(coordinates.y / moveVal), 0, rowCount - 1);
	return row * colCount + col;
}

// Changes the open/closed status of a tile and recolors it based on that
void Visualizer::setBlocked(int tile, bool blocked) {
	tiles[tile] = blocked;
	colorQuad(tile, blocked ? sf::Color::Black : sf::Color::White);
}

// Modify tile spaces with current mouse position given clicks (except start/end)
void Visualizer::handleClicks(bool leftClick, bool rightClick) {
	int square = getHoveredSquare();
	if (square == start || square == end) { return; }

	if (leftClick) {
		setBlocked(square, true);
	}
	else if (rightClick) {
		setBlocked(square, false);
	}
}

// Sets the starting square if it would not overwrite the end (and colors it)
void Visualizer::setStart(int tile, bool blockOriginal) {
	if (tile != end) {
		setBlocked(start, blockOriginal);
		setBlocked(tile, false);
		start = tile;
		colorQuad(start, sf::Color::Cyan);
	}
}

// Sets the ending square if it would not overwrite the start (and colors it)
void Visualizer::setEnd(int tile, bool blockOriginal) {
	if (tile != start) {
		setBlocked(end, blockOriginal);
		setBlocked(tile, false);
		end = tile;
		colorQuad(end, sf::Color::Magenta);
	}
}

// Manhattan distance heuristic
float Visualizer::heuristic(int from, int to) {
	int rowFrom = from / colCount;
	int colFrom = from % colCount;
	int rowTo = to / colCount;
	int colTo = to % colCount;
	return static_cast<float>(std::abs(rowFrom - rowTo) + std::abs(colFrom - colTo));
}

// Clears neighbors and modifies it to contain neighbors surrounding the given tile
// int distance determines how far away it will look for neighbors
// bool blocked determines which tiles it should get (true = blocked, false = open)
void Visualizer::getNeighbors(std::vector<int>& neighbors, int tile, bool includeStraight, bool includeDiag, bool blocked, int distance) {
	neighbors.clear();
	// Avoid looking outside the vector bounds when the tile is next to an edge
	int row = tile / colCount;
	int col = tile % colCount;
	int rowStart = std::max(row - distance, 0);
	int rowEnd = std::min(row + distance, rowCount - 1);
	int colStart = std::max(col - distance, 0);
	int colEnd = std::min(col + distance, colCount - 1);

	for (int rowi = rowStart; rowi <= rowEnd; rowi++) {
		for (int coli = colStart; coli <= colEnd; coli++) {
			if (rowi == row && coli == col) { continue; } // Skips provided tile

			// Skip adding the neighbor if it does not meet the requirements (includeStraight/includeDiag)
			if (!((includeStraight && isStraight(row, col, rowi, coli)) || (includeDiag && isDiagonal(row, col, rowi, coli)))) { continue; }

			// If the new tile matches desired blocked status, include it
			int newTile = rowi * colCount + coli;
			if (tiles[newTile] == blocked) {
				neighbors.push_back(newTile);
			}
		}
	}

	// Forces alternating movements for a cleaner looking path
	// see https://www.redblobgames.com/pathfinding/a-star/implementation.html#troubleshooting-ugly-path
	if ((row + col) % 2 == 0) {
		std::reverse(neighbors.begin(), neighbors.end());
	}
}

void Visualizer::getNeighbors(std::vector<int>& neighbors, int tile, bool blocked) {
	getNeighbors(neighbors, tile, true, moveDiagonal, blocked);
}

// Set everything to walls and then place open tiles separated by one wall
void Visualizer::initMaze() {
	setAllTiles(true);
	blankTileVertices();

	setBlocked(start, true);
	setBlocked(end, true);

	for (int row = 1; row < rowCount; row += 2) {
		for (int col = 1; col < colCount; col += 2) {
			tiles[row * colCount + col] = false;
		}
	}
}

// Attempt to find and place start and end squares in open spaces
void Visualizer::createStartEnd() {
	bool startPlaced = false;
	for (int row = 0; row < rowCount; ++row) {
		if (startPlaced) { break; }
		for (int col = 0; col < colCount; ++col) {
			int tile = row * colCount + col;
			if (!tiles[tile]) {
				setStart(tile, tiles[start]);
				startPlaced = true;
				break;
			}
		}
	}

	bool endPlaced = false;
	for (int row = rowCount - 1; row >= 0; --row) {
		if (endPlaced) { break; }
		for (int col = colCount - 1; col >= 0; --col) {
			int tile = row * colCount + col;
			if (!tiles[tile]) {
				setEnd(tile, tiles[end]);
				endPlaced = true;
				break;
			}
		}
	}
}

// Draws the creation of a maze using Wilson's Algorithm
// see https://en.wikipedia.org/wiki/Maze_generation_algorithm
void Visualizer::mazeWilsons() {
	initMaze();
	draw();

	std::vector<int> neighbors;
	std::unordered_set<int> cellsInMaze;
	std::vector<int> cellsToExplore;
	for (int row = 1; row < rowCount; row += 2) {
		for (int col = 1; col < colCount; col += 2) {
			cellsToExplore.push_back(row * colCount + col);
		}
	}

	// Pick 1 random cell, make it the initial target cell, and move it from cellsToExplore -> cellsInMaze
	int index = std::uniform_int_distribution<int>(0, cellsToExplore.size() - 1)(rng);
	int tile = cellsToExplore[index];
	cellsToExplore.erase(cellsToExplore.begin() + index);
	cellsInMaze.insert(tile);
	colorQuad(tile, sf::Color::Green);

	while (!cellsToExplore.empty()) {
		std::unordered_map<int, int> previous;

		// Choose a random position to start a random walk from
		int walkStart = cellsToExplore[std::uniform_int_distribution<int>(0, cellsToExplore.size() - 1)(rng)];
		int current = walkStart;
		cellsToExplore.erase(std::remove(cellsToExplore.begin(), cellsToExplore.end(), current), cellsToExplore.end());

		// Point the previous tile for the start of the walk to itself for ease of processing
		previous[current] = current;

		while (true) {
			// Avoid moving directly back into the previous path
			getNeighbors(neighbors, current, true, false, false, 2);
			if (previous.find(current) != previous.end()) {
				neighbors.erase(std::remove(neighbors.begin(), neighbors.end(), previous[current]), neighbors.end());
			}
			int newTile = neighbors[std::uniform_int_distribution<int>(0, neighbors.size() - 1)(rng)];

			// The reached cell is in the maze, so add the entire walked path to cellsInMaze and remove them from the nodes to explore
			if (cellsInMaze.count(newTile) == 1) {
				previous[newTile] = current;
				int loopCurrent = newTile;
				// Include the walk's starting cell
				setBlocked(walkStart, false);
				cellsInMaze.insert(walkStart);

				for (; previous[loopCurrent] != loopCurrent; loopCurrent = previous[loopCurrent]) {
					cellsInMaze.insert(loopCurrent);
					setBlocked(loopCurrent, false);

					// Remove the cell from list of cells to explore in the future
					cellsToExplore.erase(std::remove(cellsToExplore.begin(), cellsToExplore.end(), loopCurrent), cellsToExplore.end());

					int wall = (loopCurrent + previous[loopCurrent]) / 2;
					setBlocked(wall, false);
				}

				// This path is finished, break out of the loop to choose a new cell to start a walk from
				break;
			}

			// The reached cell is a part of the walked path, so remove the part of the path that loops
			if (previous.count(newTile) == 1) {
				int loopCurrent = current;
				// Iterate through the walk until the previous tile would be newTile, removing the loop path from previous in the process
				while (loopCurrent != newTile) {
					colorQuad(loopCurrent, sf::Color::Black);

					int wall = (loopCurrent + previous[loopCurrent]) / 2;
					colorQuad(wall, sf::Color::Black);

					int old = loopCurrent;
					loopCurrent = previous[loopCurrent];
					previous.erase(old);
				}

				// newTile, the tile from which the loop came, becomes the new starting tile to continue the randomly walked path
				current = newTile;
				continue;
			}

			// Otherwise, it's just another addition to the random walk, so the previous position is recorded and the new part of the path is colored
			previous[newTile] = current;
			colorQuad(current, sf::Color::Magenta);
			int wall = (current + newTile) / 2;
			colorQuad(wall, sf::Color::Magenta);

			current = newTile;

			// Keep window responsive, respond to quit, and draw to the screen
			if (checkQuit()) { return; }
			draw();
		}
	}

	// Find, set, and unblock start and end squares
	createStartEnd();
	draw();
}

// Draws the creation of a maze using randomized depth first search
// see https://en.wikipedia.org/wiki/Maze_generation_algorithm
void Visualizer::mazeDepthFirstSearch() {
	initMaze();

	std::deque<int> stack;
	std::unordered_set<int> visited;
	std::vector<int> neighbors;
	int start = std::uniform_int_distribution<int>(0, rowCount * colCount - 1)(rng);
	if (tiles[start]) {
		getNeighbors(neighbors, start, true, true, false, 2);
		start = neighbors[0];
	}
	setBlocked(start, false);
	stack.push_back(start);
	visited.insert(start);
	
	while (!stack.empty()) {
		int current = stack.back();
		stack.pop_back();

		// Filter out already visited neighbors
		getNeighbors(neighbors, current, true, false, false, 2);
		neighbors.erase(std::remove_if(neighbors.begin(), neighbors.end(), [&visited](int tile) {return visited.count(tile) == 1;}), neighbors.end());
		if (neighbors.size() == 0) { continue; }

		// Choose a random neighbor, and remove the wall between chosen and current
		stack.push_back(current);
		int chosen = neighbors[std::uniform_int_distribution<int>(0, neighbors.size() - 1)(rng)];
		setBlocked(chosen, false);
		stack.push_back(chosen);
		visited.insert(chosen);
		int wall = (current + chosen) / 2;
		setBlocked(wall, false);
		visited.insert(wall);

		// Keep window responsive, respond to quit, and draw to the screen
		if (checkQuit()) { return; }
		draw();
	}

	// Find, set, and unblock start and end squares
	createStartEnd();
	draw();
}

// Runs A* on the grid and draws progress simultaneously
void Visualizer::aStar() {
	// Elements in the open heap (min priority queue) are stored as a
	// tuple (float fScore, unsigned int accumulator, int tile)
	// and are sorted by their float fScores
	typedef std::tuple<float, unsigned int, int> node;
	std::priority_queue<node, std::vector<node>, std::greater<node>> openHeap;
	std::unordered_set<int> closedSet;         // For fast membership checking of already explored nodes
	std::unordered_map<int, int> prevConnect;  // Maps a tile to their parent
	std::unordered_map<int, float> gScores;    // G score for nodes
	std::vector<int> neighbors;  // Holds the neighbors of the single tile being looked at
	gScores[start] = 0.0f;

	// For use in tiebreakers (nodes added first are explored first)
	unsigned int accumulator = std::numeric_limits<unsigned int>::max();
	openHeap.push(node(0.0f, accumulator, start));

	// For drawing/undrawing last path from start looked at (every x frames)
	std::vector<int> lastPath;
	int curPath = start;
	int frame = 0;

	// Dictates how many frames to wait inbetween path display
	int pathEvery = desiredFps == 0 ? 240 : desiredFps / 30;
	pathEvery += frameskip * 5;

	// Function to color the previous looked at path to already explored color and clear it
	auto clearPrevious = [&]() {
		for (int tile : lastPath) {
			colorQuad(tile, COLORRED);
		}
		lastPath.clear();
	};

	// Function to draw current path being looked at in path color
	auto drawPath = [&]() {
		while (prevConnect.find(curPath) != prevConnect.end()) {
			lastPath.push_back(curPath);
			colorQuad(curPath, COLORBLUE);
			curPath = prevConnect[curPath];
		}
	};

	while (!openHeap.empty()) {
		int current = std::get<2>(openHeap.top());
		openHeap.pop();
		if (closedSet.count(current) == 1) { continue; }

		closedSet.insert(current);

		// Draw the path that was found and return
		if (current == end) {
			clearPrevious();
			curPath = prevConnect[current];
			drawPath();
			return;
		}

		// Color the current path and reset it for the future
		if (++frame > pathEvery) {
			frame = 0;
			clearPrevious();
			curPath = current;
			drawPath();
		}

		if (current != start) {
			colorQuad(current, COLORRED);
		}

		getNeighbors(neighbors, current);
		for (int neighbor : neighbors) {
			if (closedSet.count(neighbor) == 1) { continue; }  // Skip already explored nodes
			float distance = (isDiagonal(current, neighbor)) ? SQRT2 : 1.0f;
			float newG = gScores[current] + distance;
			// If this is the first time this node has been seen or if the current
			// travel cost to it is lower than previously seen, record the new
			// lowest score, set the prevConnect for this neighbor to current, and add it to the nodes to explore.
			// (This may add duplicate tile entries, but it is rare and does not break admissibility)
			if (gScores.find(neighbor) == gScores.end() || newG < gScores[neighbor]) {
				prevConnect[neighbor] = current;
				gScores[neighbor] = newG;
				openHeap.push(node(newG + heuristic(neighbor, end), --accumulator, neighbor));
				if (neighbor != end) {
					colorQuad(neighbor, sf::Color::Green);
				}
			}
		}

		// Keep window responsive, respond to quit, and draw to the screen
		if (checkQuit()) { return; }
		draw();
	}
	// No path found, so clear the last shown path
	clearPrevious();

	return;
}
