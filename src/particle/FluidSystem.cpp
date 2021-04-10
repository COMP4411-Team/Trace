#include "FluidSystem.h"

#include <cassert>

#include "../SceneObjects/Sphere.h"

Poly6Kernel::Poly6Kernel(double radius): h(radius), h2(radius * radius),
	invH2(1.0 / h2), factor(315.0 / (64.0 * PI * h2 * h)) { }

inline double Poly6Kernel::apply(double dist) const
{
	double dist2 = dist * dist;
	if (dist2 >= h2)
		return 0.0;
	double x = 1.0 - dist2 * invH2;
	return factor * x * x * x;
}

SpikyKernel::SpikyKernel(double radius): h(radius), invH(1.0 / h), factor1(-45.0 / (PI * h * h * h * h)),
	factor2(factor1 * -2 / h) { }

inline double SpikyKernel::derivative(double dist) const
{
	if (dist >= h)
		return 0.0;
	double x = 1.0 - dist * invH;
	return factor1 * x * x;
}

inline double SpikyKernel::secondOrderDerivative(double dist) const
{
	if (dist >= h)
		return 0.0;
	return factor2 * (1.0 - dist * invH);
}

FluidSystem::FluidSystem(Scene* scene, const vec3f& center, double size, double kernelRadius):
	grid(int(size / (2.0 * kernelRadius)), center, 2.0 * kernelRadius), scene(scene),
	kernelRadius(kernelRadius), poly6(kernelRadius), spiky(kernelRadius), bounds(scene, nullptr)
{
	auto* transform = scene->transformRoot.createChild(mat4f::scale(vec3f(size)));
	transform = transform->createChild(mat4f::translate(center));
	bounds.setTransform(transform);
}

FluidSystem::~FluidSystem()
{
	for (auto* particle : particles)
		delete particle;
}

void FluidSystem::reset()
{
	particles.clear();
}

void FluidSystem::addParticles(const vec3f& min, const vec3f& max, double spacing)
{
	vec3f num = (max - min) / spacing;
	int x = num[0], y = num[1], z = num[2];
	for (int i = 0; i < x; ++i)
		for (int j = 0; j < y; ++j)
			for (int k = 0; k < z; ++k)
			{
				auto* particle = new Particle;
				particle->position = vec3f(min[0] + i * spacing, min[1] + j * spacing, min[2] + k * spacing);
				particles.push_back(particle);
			}
}

void FluidSystem::addParticlesToScene() const
{
	auto* water = new Material;
	water->kd = vec3f(0.7);
	double radius = kernelRadius * 0.1;

	for (auto* particle : particles)
	{
		auto* sphere = new Sphere(scene, water);
		auto* transform = scene->transformRoot.createChild(mat4f::translate(particle->position));
		sphere->setTransform(transform->createChild(mat4f::scale(vec3f(radius))));
		scene->add(sphere);
	}
}

void FluidSystem::updateDensities()
{
	for (auto* particle : particles)
	{
		grid.getNeighbours(*particle);
		double sum = 0.0;
		for (auto* neighbour : particle->neighbours)
			sum += poly6.apply((particle->position - neighbour->position).length());
		particle->density = Particle::mass * sum;
	}
}

void FluidSystem::calculateViscosity()
{
	double mass2 = Particle::mass * Particle::mass;
	double factor = viscosityCoeff * mass2;
	for (auto* particle : particles)
	{
		particle->force = Particle::mass * gravity;
		for (auto* neighbour : particle->neighbours)
		{
			double dist = (neighbour->position - particle->position).length();
			particle->force += (factor * spiky.secondOrderDerivative(dist) / neighbour->density)
				* (neighbour->velocity - particle->velocity);
		}
	}
}

void FluidSystem::calculatePressure()
{
	double scale = targetDensity * soundSpeed * soundSpeed / gamma;
	for (auto* particle : particles)
	{
		double pressure = scale * (pow(particle->density / targetDensity, gamma) - 1.0);
		particle->pressure = pressure < 0.0 ? 0.0 : pressure;
	}
}

void FluidSystem::calculatePressureForce()
{
	double mass2 = Particle::mass * Particle::mass;
	for (auto* particle : particles)
	{
		double term = particle->pressure / (particle->density * particle->density);
		for (auto* neighbour : particle->neighbours)
		{
			vec3f offset(neighbour->position - particle->position);
			double dist = offset.length();
			if (dist > 0.0)
			{
				vec3f gradient = -spiky.derivative(dist) * (offset / dist);
				particle->force -= mass2 * (term + neighbour->pressure / (neighbour->density * neighbour->density)) * gradient;
				assert(!isnan(particle->force[0]));
			}
		}
	}
}

void FluidSystem::integrate(double t)
{
	double invMass = 1.0 / Particle::mass;
	for (auto* particle : particles)
	{
		vec3f newVel = particle->velocity + t * particle->force * invMass;
		vec3f newPos = particle->position + t * newVel;
		assert(!isnan(newPos[0]));
		resolveCollision(*particle, newPos, newVel);
		particle->velocity = newVel;
		particle->position = newPos;
	}
}

void FluidSystem::resolveCollision(Particle& particle, vec3f& newPos, vec3f& newVel)
{
	if (!isOutOfBounds(newPos))
		return;

	Ray ray(particle.position, newPos - particle.position);
	Isect isect;
	bounds.intersect(ray, isect);

	vec3f isectPos = ray.at(isect.t) - RAY_EPSILON * ray.getDirection();
	double vDotN = isect.N.dot(newVel);
	vec3f vPerp = vDotN * isect.N, vProj = newVel - vPerp;

	if (vDotN > 0.0)
	{
		vec3f deltaVPerp = (-restitutionCoeff - 1.0) * vPerp;
		vPerp *= -restitutionCoeff;
		if (vProj.length_squared() > 0.0)
		{
			double scale = _max(0.0, 1.0 - frictionCoeff * deltaVPerp.length() / vProj.length());
			vProj *= scale;
		}
		newVel = vPerp + vProj;
	}

	newPos = isectPos;
}

double FluidSystem::nextTimeStep() const
{
	double maxF = 0.0;
	for (auto* particle : particles)
		maxF = _max(maxF, particle->force.length());
	double limitBySpeed = lambdaV * kernelRadius / soundSpeed;
	double limitByForce = lambdaF * sqrt(kernelRadius * Particle::mass / maxF);
	return _min(limitByForce, limitBySpeed);
}

void FluidSystem::simulate(int totalTick)
{
	double curTimeStep = 0.03;
	for (int i = 0; i < totalTick; ++i)
	{
		grid.addParticles(particles);
		updateDensities();
		calculateViscosity();
		calculatePressure();
		calculatePressureForce();
		integrate(curTimeStep);
		// curTimeStep = nextTimeStep();
	}

	addParticlesToScene();
}

inline bool FluidSystem::isOutOfBounds(const vec3f& pos) const
{
	vec3f diff = pos - grid.gridMin;
	if (diff[0] < 0.0 || diff[1] < 0.0 || diff[2] < 0.0)
		return true;
	diff = grid.gridMax;
	if (diff[0] < 0.0 || diff[1] < 0.0 || diff[2] < 0.0)
		return true;
	return false;
}
