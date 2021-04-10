#pragma once

#include <vector>
#include "../vecmath/vecmath.h"

class Particle
{
public:
	vec3f position;
	vec3f velocity;
	vec3f force;
	double density;
	double pressure;
	vector<Particle*> neighbours;

	static double mass;
	static int maxNeighbourNum;
};

class SpacialGrid
{
public:
	SpacialGrid(int numCells, const vec3f& center, double cellSize);
	int getCellIndex(const vec3f& pos) const;
	void addParticles(const vector<Particle*>& particles);
	void getNeighbours(Particle& particle) const;

	vector<vector<Particle*>> grid;
	int numCells;			// per dimension
	double cellSize, invCellSize;
	vec3f gridMin, gridMax;

private:
	void pushBackParticles(int index, Particle& particle) const;
	int getCellIndex(int x, int y, int z) const;
};