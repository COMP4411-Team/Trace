#pragma once

#include "Particle.h"
#include "../SceneObjects/Box.h"

class Poly6Kernel
{
public:
	Poly6Kernel(double radius);
	double apply(double dist) const;

private:
	double h, h2;		// kernel radius
	double invH2;		// inverse of h^2
	double factor;		// 315 / (64 * pi * h^3)
};

class SpikyKernel
{
public:
	SpikyKernel(double radius);
	double derivative(double dist) const;
	double secondOrderDerivative(double dist) const;

private:
	double h, invH;
	double factor1;		// -45 / (pi * h^4)
	double factor2;		// 90 / (pi * h^5)
};

class FluidSystem
{
public:
	FluidSystem(Scene* scene, const vec3f& center, double size, double kernelRadius = 0.1);
	~FluidSystem();

	void reset();
	void addParticles(const vec3f& min, const vec3f& max, double spacing);
	void addParticlesToScene() const;
	
	void updateDensities();
	void calculateViscosity();
	void calculatePressure();
	void calculatePressureForce();
	void integrate(double t);
	void resolveCollision(Particle& particle, vec3f& newPos, vec3f& newVel);
	double nextTimeStep() const;
	void simulate(int totalTick);

	vector<Particle*> particles;
	SpacialGrid grid;
	Box bounds;
	Scene* scene;
	
	double kernelRadius{0.1};
	Poly6Kernel poly6;
	SpikyKernel spiky;

	// Tait¡¯s equation
	double targetDensity{100.0};
	double soundSpeed{88.5};
	double gamma{7.0};
	double lambdaV{0.4}, lambdaF{0.25};
	
	double viscosityCoeff{0.3};
	double restitutionCoeff{0.9};
	double frictionCoeff{0.2};
	vec3f gravity{0.0, -9.81, 0.0};

private:
	bool isOutOfBounds(const vec3f& pos) const;
};

