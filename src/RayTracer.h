#ifndef __RAYTRACER_H__
#define __RAYTRACER_H__

// The main ray tracer.

#include <vector>

#include "scene/scene.h"
#include "scene/Ray.h"

class RayTracer
{
public:
    RayTracer();
    ~RayTracer();

    vec3f trace( Scene *scene, double x, double y );
	vec3f traceRay( Scene *scene, const Ray& r, const vec3f& thresh, int depth, const vec3f& curFactor );


	void getBuffer( unsigned char *&buf, int &w, int &h );
	double aspectRatio();
	void traceSetup( int w, int h, int maxDepth, const vec3f& threshold );
	void traceLines( int start = 0, int stop = 10000000 );
	void tracePixel( int i, int j );
	void setLightScale(double value);

	bool loadScene( char* fn );

	bool sceneLoaded();

	int ssaaSample{0};	// the exponent of 2
	bool ssaaJitter{false};

private:
	unsigned char *buffer;
	int buffer_width, buffer_height;
	int bufferSize;
	Scene *scene;

	bool m_bSceneLoaded;
	int maxDepth{0};
	vec3f threshold;

	static std::vector<std::vector<std::pair<int, int>>> msaaSamplePattern;
};

#endif // __RAYTRACER_H__
