#pragma once

#include "../scene/scene.h"

class TorusKnot : public MaterialSceneObject
{
public:
	TorusKnot(Scene* scene, Material* material, double radius, double tube, double p, double q);
	double getDistance(const vec3f& pos) const;
	bool intersectLocal(const Ray& r, Isect& i) const override;
	vec3f getNormal(const vec3f& pos) const;

private:
	double p, q;
	double radius, tube;

	double epsilon{1e-3};
	int maxIter{64};
};

