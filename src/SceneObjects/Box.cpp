#include <cmath>
#include <assert.h>

#include "Box.h"

const double PI = 3.14159265358979323846;

bool Box::intersectLocal( const Ray& r, Isect& i ) const
{
	// YOUR CODE HERE:
    // Add box intersection code here.
	// it currently ignores all boxes and just returns false.

	vec3f pos = r.getPosition(), dir = r.getDirection();
	vec3f invDir = {1.0 / dir[0], 1.0 / dir[1], 1.0 / dir[2]};	// multiplication will be faster
	
	double xMin = (-0.5 - pos[0]) * invDir[0];
	double xMax = (0.5 - pos[0]) * invDir[0];
	double yMin = (-0.5 - pos[1]) * invDir[1];
	double yMax = (0.5 - pos[1]) * invDir[1];
	double zMin = (-0.5 - pos[2]) * invDir[2];
	double zMax = (0.5 - pos[2]) * invDir[2];

	if (dir[0] < 0.0)
		_swap(xMin, xMax);
	if (dir[1] < 0.0)
		_swap(yMin, yMax);
	if (dir[2] < 0.0)
		_swap(zMin, zMax);

	double tMin = _max(_max(xMin, yMin), zMin);		// use self-defined functions for performance
	double tMax = _min(_min(xMax, yMax), zMax);

	if (tMax < 0.0 || tMin > tMax)
		return false;

	i.obj = this;
	i.t = tMin < 0.0 ? tMax : tMin;
	i.N = vec3f(-1.0, 0.0, 0.0);
	vec3f isectPos = r.at(i.t);
	double delta = _abs(isectPos[0] + 0.5);
	
	if (_abs(isectPos[0] - 0.5) < delta)
	{
		delta = _abs(isectPos[0] - 0.5);
		i.N = vec3f(1.0, 0.0, 0.0);
	}
	if (_abs(isectPos[1] + 0.5) < delta)
	{
		delta = _abs(isectPos[1] + 0.5);
		i.N = vec3f(0.0, -1.0, 0.0);
	}
	if (_abs(isectPos[1] - 0.5) < delta)
	{
		delta = _abs(isectPos[1] - 0.5);
		i.N = vec3f(0.0, 1.0, 0.0);
	}
	if (_abs(isectPos[2] + 0.5) < delta)
	{
		delta = _abs(isectPos[2] + 0.5);
		i.N = vec3f(0.0, 0.0, -1.0);
	}
	if (_abs(isectPos[2] - 0.5) < delta)
	{
		i.N = vec3f(0.0, 0.0, 1.0);
	}

	if (enableTexCoords)
	{
		i.hasTexCoords = true;
		if (i.N[0] != 0.0)
			i.texCoords = {isectPos[1] + 0.5, isectPos[2] + 0.5};
		else if (i.N[1] != 0.0)
			i.texCoords = {isectPos[0] + 0.5, isectPos[2] + 0.5};
		else
			i.texCoords = {isectPos[0] + 0.5, isectPos[1] + 0.5};
	}
	
	return true;
}

void Box::setEnableTexCoords(bool value)
{
	enableTexCoords = value;
}

Skybox::Skybox(Scene* scene, Material* material): Box(scene, material) { enableTexCoords = true; }

vec3f Skybox::getColor(const Ray& ray, const Isect& isect) const
{
	TexCoords uv = isect.texCoords;
	if (isect.N[0] < 0.0)
		return right.sample(1.0 - uv.v, uv.u);
	if (isect.N[0] > 0.0)
		return left.sample(uv.v, uv.u);
	if (isect.N[1] > 0.0)
		return top.sample(1.0 - uv.u, 1.0 - uv.v);
	if (isect.N[1] < 0.0)
		return bottom.sample(1.0 - uv.u, uv.v);
	if (isect.N[2] > 0.0)
		return front.sample(1.0 - uv.u, uv.v);
	return back.sample(uv);
}

void Skybox::initialize(double width)
{
	transform = &(scene->transformRoot);
	double scaling = width / tan(scene->getCamera()->getFov() / 360.0 * PI);
	
	transform = transform->createChild(mat4f::translate({scene->getCamera()->getEye()}));
	transform = transform->createChild(mat4f::scale({scaling, scaling, scaling}));
	initialized = true;
}
