#include "metaball.h"

const int MAX_MARCHING_STEPS = 10;
const float EPSILON = 0.001;

// polynomial smooth min (k = 0.7);
float smin(float a, float b, float k = 0.7) {
	float h = 0.5 + (a - b) / 2 / k;
	if (h < 0.0) h = 0.0;
	if (h > 1.0) h = 1.0;
	return a * (1 - h) + b * h - k * h * (1 - h);
}

void Metaball::addBalls(vec3f b) {
	balls.push_back(b);
	nBall++;
}

float Metaball::sphereSDF(vec3f p, vec3f b) const{
	return (p - b).length() - radius;
}

float Metaball::sceneSDF(vec3f p ) const {
	float sdf = 100;
	for (int i = 0; i < nBall; i++) {
		sdf = smin(sdf, sphereSDF(p, balls[i]));
	}
	return sdf;
}

bool Metaball::intersectLocal(const Ray& r, Isect& i) const {
	i.t = 0;
	bool transparent{ false };
	for (int j = 0; j < MAX_MARCHING_STEPS; j++) {
		float dist = sceneSDF(r.at(i.t));
		if (abs(dist) < EPSILON &&j!=0) {
			i.obj = this;
			i.N = getNormal(r.at(i.t));
			return true;
		}
		if (abs(dist) < EPSILON && getNormal(r.at(i.t)).dot(r.getDirection())<0&& j == 0) {
			transparent = true;
			i.t += radius;
		}
		if (abs(dist) < EPSILON && getNormal(r.at(i.t)).dot(r.getDirection()) > 0 && j == 0) {
			return false;
		}
		if(transparent) i.t -= dist;
		else i.t += dist;
	}
	return false;
}

vec3f Metaball::getNormal(vec3f p) const {
	return vec3f(
		sceneSDF(vec3f(p[0] + EPSILON, p[1], p[2])) - sceneSDF(vec3f(p[0] - EPSILON, p[1], p[2])),
		sceneSDF(vec3f(p[0], p[1] + EPSILON, p[2])) - sceneSDF(vec3f(p[0], p[1] - EPSILON, p[2])),
		sceneSDF(vec3f(p[0], p[1], p[2] + EPSILON)) - sceneSDF(vec3f(p[0], p[1], p[2] - EPSILON))
	).normalize();
}

