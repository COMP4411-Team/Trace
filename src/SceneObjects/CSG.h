#pragma once

#include "../scene/scene.h"

class CSG : public Geometry
{
public:
	enum class Operator
	{
		AND, OR, SUB
	};

	CSG(Scene* scene);
	~CSG();
	bool intersect(const Ray& ray, Isect& isect) const override;		// only need to return when finding the first intersection
	void intersectCSG(const Ray& ray, std::vector<Isect>& list) const;	// return the whole Roth diagram to avoid redundant computation
	bool intersectLocal(const Ray& ray, Isect& isect) const override;
	bool hasBoundingBoxCapability() const override;
	void ComputeBoundingBox() override;

	Geometry* left{nullptr}, *right{nullptr};
	Operator op;

private:
	void getAllIsect(const Ray& ray, std::vector<Isect>& list, Geometry* geometry) const;
	bool computeState(bool l, bool r) const;
};

