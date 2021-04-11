#pragma once

#include <vector>
#include <queue>
#include "Photon.h"

class KdTree
{
public:
	// TODO: memory optimization: use a heap-like structure instead of storing child pointers
	class Node
	{
	public:
		Node(Photon* photon, int dimension): photon(photon), dimension(dimension) { }
		~Node() { delete left; delete right; }
		
		Photon* photon;
		Node* left{nullptr}, *right{nullptr};
		int dimension;
	};

	class Pair
	{
	public:
		Photon* photon;
		double distance;
		friend bool operator<(const Pair& a, const Pair& b) { return a.distance > b.distance; }
	};

	KdTree(std::vector<Photon*>& photons, int dimension);
	~KdTree();
	
	Node* build(int l, int r);
	Photon* getMedian(int l, int r, int dimension);
	std::vector<Photon*> getKnn(const vec3f& pos, int k, double maxDist);
	
	std::vector<Photon*>& photons;

private:
	void traverse(const vec3f& pos, Node* cur, double maxDist);
	double getRange(int l, int r, int dimension);

	std::priority_queue<Pair> knn;
	Node* root{nullptr};
	int totDim;
};

