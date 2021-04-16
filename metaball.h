#pragma once

#ifndef _METABALL_H_
#define _METABALL_H_

#include <list>
#include <vector>

#include "../scene/scene.h"


class Metaball
	:public MaterialSceneObject
{
public:
	Metaball(Scene* scene, Material* mat, float r, TransformNode* transform)
		: MaterialSceneObject(scene, mat)
	{
		radius = r;
		this->transform = transform;
	}

	~Metaball();
	void addBalls(vec3f b);
	float sphereSDF(vec3f p, vec3f b);
	float sceneSDF(vec3f p);
	virtual bool intersectLocal(const Ray& r, Isect& i) const;
	vec3f getNormal(vec3f p);

private:
	typedef vector<vec3f>Balls;
	Balls balls;
	float radius;
	int nBall{ 0 };
};

#endif 

