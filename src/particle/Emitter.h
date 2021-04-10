#pragma once

#include <deque>
#include <vector>

#include "../vecmath/vecmath.h"

class Scene;
class Material;

class Emitter
{
public:
	class EmitterParticle
	{
	public:
		double life;
		vec3f position;
		vec3f velocity;
		deque<vec3f> trajectory;
	};

	Emitter(Scene* scene, Material* init, Material* end);
	~Emitter();
	
	void simulate(double timeStep, int tick);
	void advance(double t);
	void emit();
	void render();

	deque<EmitterParticle> particles;
	vector<Material*> palette;

	Scene* scene;
	Material* initMaterial;
	Material* endMaterial;
	vec3f source;
	vec3f gravity{0.0, -9.81, 0.0};

	int paletteSize{256};
	int emissionRate;
	int maxNumParticle;
	int tail;
	int emissionCut;
	double mass;
	double lifespan;
	double renderingRadius;
	double initialSpeed;
	double drag;
};

