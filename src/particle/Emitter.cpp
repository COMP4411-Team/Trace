#include "Emitter.h"
#include "../scene/material.h"
#include "../scene/scene.h"
#include "../SceneObjects/Sphere.h"

Emitter::Emitter(Scene* scene, Material* init, Material* end): scene(scene), initMaterial(init), endMaterial(end)
{
	palette.resize(paletteSize);
	Material offset(*end);
	offset += (-1.0 * *init);
	for (int i = 0; i < paletteSize; ++i)
	{
		auto* material = new Material(*init);
		*material += (static_cast<double>(i) / paletteSize) * offset;
		palette[i] = material;
	}
}

Emitter::~Emitter()
{
	delete initMaterial;
	delete endMaterial;
	for (auto* material : palette)
		delete material;
}

void Emitter::simulate(double timeStep, int tick)
{
	for (int i = 0; i < tick; ++i)
	{
		if (i < emissionCut)
			emit();
		advance(timeStep);
	}
	render();
}

void Emitter::advance(double t)
{
	while (particles.front().life > lifespan)
		particles.pop_front();
	for (auto& particle : particles)
	{
		particle.velocity += (gravity - drag * particle.velocity) / mass * t;
		particle.position += particle.velocity * t;
		particle.trajectory.push_back(particle.position);
		if (particle.trajectory.size() > tail)
			particle.trajectory.pop_front();
	}
}

void Emitter::emit()
{
	if (particles.size() > maxNumParticle)
		return;
	for (int i = 0; i < emissionRate; ++i)
	{
		vec3f velocity(uniformSampleSphere().normalize() * initialSpeed);
		if (velocity[1] < 0.0)
			velocity[1] = -velocity[1];
		particles.push_back({0.0, source, velocity});
	}
}

void Emitter::render()
{
	for (auto& particle : particles)
	{
		int idx = particle.life / lifespan * (paletteSize - 1);
		auto* sphere = new Sphere(scene, palette[idx]);
		auto* transform = scene->transformRoot.createChild(mat4f::translate(particle.position));
		sphere->setTransform(transform->createChild(mat4f::scale(vec3f(renderingRadius))));
		scene->add(sphere);

		for (int i = 0; i < particle.trajectory.size(); ++i)
		{
			int idx = (double) i / tail * (paletteSize - 1);
			auto* sphere = new Sphere(scene, palette[idx]);
			auto* transform = scene->transformRoot.createChild(mat4f::translate(particle.trajectory[i]));
			sphere->setTransform(transform->createChild(mat4f::scale(vec3f(renderingRadius))));
			scene->add(sphere);
		}
	}
}
