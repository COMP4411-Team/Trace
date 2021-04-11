#include "KdTree.h"
#include <algorithm>
#include <cmath>

using namespace std;

Photon* KdTree::getMedian(int l, int r, int dimension)
{
	nth_element(photons.begin() + l, photons.begin() + (l + r) / 2, photons.begin() + r, 
		[dimension] (const Photon* a, const Photon* b) { return a->position[dimension] < b->position[dimension]; });
	return photons[(l + r) / 2];
}

vector<Photon*> KdTree::getKnn(const vec3f& pos, int k, double maxDist)
{
	knn = priority_queue<Pair>{less<Pair>(), vector<Pair>{(unsigned)k, {nullptr, numeric_limits<double>::lowest()}}};
	traverse(pos, root, maxDist);
	vector<Photon*> result;
	while (!knn.empty())
	{
		auto cur = knn.top();
		knn.pop();
		if (cur.photon != nullptr) 
			result.push_back(cur.photon);
	}
	return result;
}

// TODO: add optimization: do not build the heap before the required number of photons are collected
void KdTree::traverse(const vec3f& pos, Node* cur, double maxDist)
{
	if (cur == nullptr)	return;
	double dist = pos[cur->dimension] - cur->photon->position[cur->dimension];

	if (dist < 0) traverse(pos, cur->left, maxDist);
	else traverse(pos, cur->right, maxDist);
	
	double curDist2 = (pos - cur->photon->position).length_squared();

	if (sqrt(curDist2) <= maxDist)
	{
		knn.push({cur->photon, -curDist2});
		knn.pop();
	}
	
	if (-knn.top().distance > std::abs(dist))
	{
		if (dist < 0) traverse(pos, cur->right, maxDist);
		else traverse(pos, cur->left, maxDist);
	}
}

double KdTree::getRange(int l, int r, int dimension)
{
	double minV = photons[l]->position[dimension], maxV = minV;
	for (auto iter = photons.begin() + l, end = photons.begin() + r; iter != end; ++iter)
	{
		minV = min(minV, (*iter)->position[dimension]);
		maxV = max(maxV, (*iter)->position[dimension]);
	}
	return maxV - minV;
}

KdTree::KdTree(vector<Photon*>& photons, int dimension): photons(photons), totDim(dimension)
{
	root = build(0, photons.size());
}

KdTree::~KdTree()
{
	delete root;
}

KdTree::Node* KdTree::build(int l, int r)
{
	if (l >= r)	return nullptr;
	int dimension = 0;
	double range = getRange(l, r, dimension);

	for (int i = 1; i < totDim; ++i)
	{
		double tmp = getRange(l, r, i);
		if (tmp > range)
		{
			range = tmp;
			dimension = i;
		}
	}
	
	Node* cur = new Node(getMedian(l, r, dimension), dimension);
	cur->left = build(l, (l + r) / 2);
	cur->right = build((l + r) / 2 + 1, r);
	return cur;
}
