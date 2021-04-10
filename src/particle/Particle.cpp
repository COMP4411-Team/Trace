#include "Particle.h"

double Particle::mass = 0.01;
int Particle::maxNeighbourNum = 100;

SpacialGrid::SpacialGrid(int numCells, const vec3f& center, double cellSize):
	numCells(numCells), cellSize(cellSize)
{
	grid.resize(numCells * numCells * numCells);
	gridMin = center - vec3f(cellSize * numCells / 2.0);
	gridMax = center + vec3f(cellSize * numCells / 2.0);
	invCellSize = 1.0 / cellSize;
}

inline int SpacialGrid::getCellIndex(const vec3f& pos) const
{
	int x = (pos[0] - gridMin[0]) * invCellSize;
	int y = (pos[1] - gridMin[1]) * invCellSize;
	int z = (pos[2] - gridMin[2]) * invCellSize;
	return (z * numCells + y) * numCells + x;
}

void SpacialGrid::addParticles(const vector<Particle*>& particles)
{
	for (auto& cell : grid)
		cell.clear();

	for (auto* particle : particles)
	{
		grid[getCellIndex(particle->position)].push_back(particle);
	}
}

void SpacialGrid::getNeighbours(Particle& particle) const
{
	particle.neighbours.clear();
	int x = (particle.position[0] - gridMin[0]) * invCellSize;
	int y = (particle.position[1] - gridMin[1]) * invCellSize;
	int z = (particle.position[2] - gridMin[2]) * invCellSize;

	pushBackParticles(getCellIndex(x, y, z), particle);
	pushBackParticles(getCellIndex(x - 1, y, z), particle);
	pushBackParticles(getCellIndex(x, y - 1, z), particle);
	pushBackParticles(getCellIndex(x, y, z - 1), particle);
	pushBackParticles(getCellIndex(x, y - 1, z - 1), particle);
	pushBackParticles(getCellIndex(x - 1, y - 1, z), particle);
	pushBackParticles(getCellIndex(x - 1, y, z - 1), particle);
	pushBackParticles(getCellIndex(x - 1, y - 1, z - 1), particle);
}

void SpacialGrid::pushBackParticles(int index, Particle& particle) const
{
	if (index < 0)
		return;
	for (auto* neighbour : grid[index])
	{
		if (particle.neighbours.size() > Particle::maxNeighbourNum)
			return;
		particle.neighbours.push_back(neighbour);
	}
}

inline int SpacialGrid::getCellIndex(int x, int y, int z) const
{
	if (x < 0 || y < 0 || z < 0)
		return -1;
	return (z * numCells + y) * numCells + x;
}
