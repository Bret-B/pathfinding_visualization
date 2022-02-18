#pragma once

#include <SFML/Graphics.hpp>
#include <random>

class Visualizer {
public:
	Visualizer(int width, int height, int squarePix, int spacing, int fps, int frameskip, bool diagonal);

	void run();

private:
	void draw();
	void initMaze();
	void mazeDepthFirstSearch();
	void mazeWilsons();
	void aStar();
	void blankTileVertices();
	void createStartEnd();
	void handleClicks(bool leftClick, bool rightClick);
	void setStart(int tile, bool blockOriginal = false);
	void setEnd(int tile, bool blockOriginal = false);
	void setAllTiles(bool blocked = false);
	void colorQuad(int tile, sf::Color color);
	void setBlocked(int tile, bool blocked);
	void getNeighbors(std::vector<int>& neighbors, int tile, bool includeStraight, bool includeDiag, bool blocked = false, int distance = 1);
	void getNeighbors(std::vector<int>& neighbors, int tile, bool blocked = false);
	int getHoveredSquare();
	float heuristic(int from, int to);
	bool checkQuit();
	bool isDiagonal(int from, int to);
	bool isDiagonal(int rowFrom, int colFrom, int rowTo, int colTo);
	bool isStraight(int from, int to);
	bool isStraight(int rowFrom, int colFrom, int rowTo, int colTo);

	const sf::Color COLORGRAY = sf::Color(35, 35, 35, 255);
	const sf::Color COLORRED = sf::Color(255, 70, 50, 255);
	const sf::Color COLORBLUE = sf::Color(55, 120, 255, 255);
	const float SQRT2 = 1.4142135f;

	float moveVal;
	int rowCount;
	int colCount;
	int width;
	int height;
	int start;
	int end;
	int tileSpacing;
	int desiredFps;
	int frameskip;
	int frame;
	bool moveDiagonal;
	std::mt19937 rng;
	std::vector<bool> tiles;
	sf::VertexArray tileVertices;
	sf::RenderWindow window;
};