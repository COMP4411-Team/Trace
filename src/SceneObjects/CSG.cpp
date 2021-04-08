#include "CSG.h"

CSG::CSG(Scene* scene): Geometry(scene) { }

CSG::~CSG()
{
	delete left;
	delete right;
}

bool CSG::intersect(const Ray& ray, Isect& isect) const
{
	vector<Isect> l, r;
	getAllIsect(ray, l, left);
	getAllIsect(ray, r, right);

	if (l.empty() && r.empty())
		return false;
	if (!l.empty() && r.empty())
	{
		if (op == Operator::AND)
			return false;
		isect = l[0];
		return true;
	}
	if (l.empty() && !r.empty())
	{
		if (op == Operator::AND || op == Operator::SUB)
			return false;
		isect = r[0];
		return true;
	}

	bool stateL = ray.getDirection().dot(l[0].N) > 0;		// true for in, false for out
	bool stateR = ray.getDirection().dot(r[0].N) > 0;
	bool state = computeState(stateL, stateR);
	auto iterL = l.begin(), iterR = r.begin();
	while (iterL != l.end() && iterR != r.end())
	{
		if (iterL->t < iterR->t)
		{
			bool curState = computeState(!stateL, stateR);
			if (curState != state)
			{
				isect = *iterL;
				return true;
			}
			++iterL;
			stateL = !stateL;
			continue;
		}

		bool curState = computeState(stateL, !stateR);
		if (state != curState)
		{
			isect = *iterR;
			return true;
		}
		++iterR;
		stateR = !stateR;
	}

	while (iterL != l.end())
	{
		if (computeState(!stateL, stateR) != state)
		{
			isect = *iterL;
			return true;
		}
		++iterL;
		stateL = !stateL;
	}

	while (iterR != r.end())
	{
		if (computeState(stateL, !stateR) != state)
		{
			isect = *iterR;
			return true;
		}
		++iterR;
		stateR = !stateR;
	}

	return false;
}

void CSG::intersectCSG(const Ray& ray, vector<Isect>& list) const
{
	list.clear();
	vector<Isect> l, r;
	getAllIsect(ray, l, left);
	getAllIsect(ray, r, right);

	if (l.empty() && r.empty())
		return;
	if (!l.empty() && r.empty())
	{
		if (op == Operator::AND)
			return;
		list = std::move(l);
		return;
	}
	if (l.empty() && !r.empty())
	{
		if (op == Operator::AND || op == Operator::SUB)
			return;
		list = std::move(r);
		return;
	}

	bool stateL = ray.getDirection().dot(l[0].N) > 0;		// true for in, false for out
	bool stateR = ray.getDirection().dot(r[0].N) > 0;
	bool state = computeState(stateL, stateR);
	auto iterL = l.begin(), iterR = r.begin();
	while (iterL != l.end() && iterR != r.end())
	{
		if (iterL->t < iterR->t)
		{
			bool curState = computeState(!stateL, stateR);
			if (curState != state)
			{
				list.push_back(*iterL);
				state = curState;
			}
			++iterL;
			stateL = !stateL;
			continue;
		}

		bool curState = computeState(stateL, !stateR);
		if (state != curState)
		{
			list.push_back(*iterR);
			state = curState;
		}
		++iterR;
		stateR = !stateR;
	}

	// TODO: optimization
	while (iterL != l.end())
	{
		if (computeState(!stateL, stateR) != state)
		{
			list.push_back(*iterL);
			state = !state;
		}
		++iterL;
		stateL = !stateL;
	}

	while (iterR != r.end())
	{
		if (computeState(stateL, !stateR) != state)
		{
			list.push_back(*iterR);
			state = !state;
		}
		++iterR;
		stateR = !stateR;
	}
}

bool CSG::intersectLocal(const Ray& ray, Isect& isect) const
{
	return false;
}

bool CSG::hasBoundingBoxCapability() const
{
	return true;
}

void CSG::ComputeBoundingBox()
{
	left->ComputeBoundingBox();
	right->ComputeBoundingBox();
	const auto& l = left->getBoundingBox(), &r = right->getBoundingBox();
	if (op == Operator::SUB)
	{
		bounds = l;
		return;
	}
	// TODO: calculate the real AABB for intersection
	bounds.max = maximum(l.max, r.max);
	bounds.min = minimum(l.min, r.min);
}

void CSG::getAllIsect(const Ray& ray, std::vector<Isect>& list, Geometry* geometry) const
{
	if (typeid(*geometry) == typeid(CSG))
	{
		auto* csg = dynamic_cast<CSG*>(geometry);
		return csg->intersectCSG(ray, list);
	}
	
	Ray curRay(ray);
	Isect curIsect;
	double prevT = 0.0;
	while (geometry->intersect(curRay, curIsect))
	{
		curIsect.t += prevT;
		prevT = curIsect.t;
		list.push_back(curIsect);
		curRay = Ray(curRay.at(curIsect.t) + curRay.getDirection() * RAY_EPSILON, curRay.getDirection());
	}
}

inline bool CSG::computeState(bool l, bool r) const
{
	// True for in, false for out
	switch (op)
	{
	case Operator::AND:
		return l && r;
	case Operator::OR:
		return l || r;
	case Operator::SUB:
		return l && (!r);
	}
	return false;
}
