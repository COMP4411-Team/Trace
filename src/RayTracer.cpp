// The main ray tracer.

#include <Fl/fl_ask.h>

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/Ray.h"
#include "fileio/read.h"
#include "fileio/parse.h"
#include "SceneObjects/Box.h"

// Trace a top-level ray through normalized window coordinates (x,y)
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
vec3f RayTracer::trace( Scene *scene, double x, double y )
{
    Ray r( vec3f(0,0,0), vec3f(0,0,0) );

	scene->getCamera()->rayThrough( x, y, r );
	return traceRay( scene, r, threshold, 0, {1.0, 1.0, 1.0} ).clamp();
}

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
vec3f RayTracer::traceRay( Scene *scene, const Ray& r, 
	const vec3f& thresh, int depth, const vec3f& curFactor )
{
	if (depth > maxDepth)
		return vec3f();

	// Adaptive termination
	if (curFactor[0] < thresh[0] && curFactor[1] < thresh[1] && curFactor[2] < thresh[2])
		return vec3f();
	
	Isect i;

	if( scene->bvhIntersect( r, i ) ) {
		// YOUR CODE HERE

		// An intersection occured!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.  

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.

		const Material& m = i.getMaterial();
		vec3f directIllumination = m.shade(scene, r, i);

		vec3f indirectIllumination;

		// Intersects at outer surface or the material is translucent
		if (!m.kr.iszero())
		{
			Ray reflection = r.reflect(i);
			indirectIllumination += prod(traceRay(scene, reflection, thresh, depth + 1, prod(curFactor, m.kr)), m.kr);
		}

		Ray refraction{vec3f(), vec3f()};
		if (!m.kt.iszero() && r.refract(i, refraction))
		{
			indirectIllumination += prod(traceRay(scene, refraction, thresh, depth + 1, prod(curFactor, m.kt)), m.kt);
		}

		return directIllumination + indirectIllumination;
	
	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
		
		if (!scene->useSkybox)
			return vec3f( 0.0, 0.0, 0.0 );
		
		scene->skybox->intersect(r, i);

		if (isnan(r.getDirection()[0]))
			return {0, 0, 0};
		
		return scene->skybox->getColor(r, i) / 255.0;
	}
}

RayTracer::RayTracer()
{
	buffer = NULL;
	buffer_width = buffer_height = 256;
	scene = NULL;

	m_bSceneLoaded = false;
}


RayTracer::~RayTracer()
{
	delete [] buffer;
	delete scene;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return scene ? scene->getCamera()->getAspectRatio() : 1;
}

bool RayTracer::sceneLoaded()
{
	return m_bSceneLoaded;
}

bool RayTracer::loadScene( char* fn )
{
	try
	{
		scene = readScene( fn );
	}
	catch( ParseError pe )
	{
		fl_alert( "ParseError: %s\n", pe );
		return false;
	}

	if( !scene )
		return false;
	
	buffer_width = 256;
	buffer_height = (int)(buffer_width / scene->getCamera()->getAspectRatio() + 0.5);

	bufferSize = buffer_width * buffer_height * 3;
	buffer = new unsigned char[ bufferSize ];
	
	// separate objects into bounded and unbounded and calculate BVH
	scene->initScene();
	
	// Add any specialized scene loading code here
	
	m_bSceneLoaded = true;

	if (scene->useSkybox)
		scene->skybox->initialize(buffer_width);

	return true;
}

void RayTracer::traceSetup( int w, int h, int maxDepth, const vec3f& threshold )
{
	if( buffer_width != w || buffer_height != h )
	{
		buffer_width = w;
		buffer_height = h;

		bufferSize = buffer_width * buffer_height * 3;
		delete [] buffer;
		buffer = new unsigned char[ bufferSize ];
	}
	memset( buffer, 0, w*h*3 );
	this->maxDepth = maxDepth;
	this->threshold = threshold;
}

void RayTracer::traceLines( int start, int stop )
{
	vec3f col;
	if( !scene )
		return;

	if( stop > buffer_height )
		stop = buffer_height;

	for( int j = start; j < stop; ++j )
		for( int i = 0; i < buffer_width; ++i )
			tracePixel(i,j);
}

void RayTracer::tracePixel( int i, int j )
{
	vec3f col;

	if( !scene )
		return;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	// SSAA, in fact, MSAA
	int sampleNum = pow(2, ssaaSample);
	auto pattern = msaaSamplePattern[ssaaSample];
	double unitWidth = 0.0625 / double(buffer_width);		// 1/16
	double unitHeight = 0.0625 / double(buffer_height);

	if (!ssaaJitter)
	{
		for (int i = 0; i < sampleNum; ++i)
		{
			auto offset = pattern[i];
			col += trace( scene, x + offset.first * unitWidth, y + offset.second * unitHeight );
		}
	}
	else
	{
		double widthFactor = 1.0 / (double(RAND_MAX) * buffer_width);
		double heightFactor = 1.0 / (double(RAND_MAX) * buffer_height);
		for (int i = 0; i < sampleNum; ++i)
		{
			double xOffset = rand() * widthFactor, yOffset = rand() * heightFactor;
			col += trace(scene, x + xOffset, y + yOffset);
		}
	}
	
	col /= sampleNum;
	
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
}

void RayTracer::setLightScale(double value)
{
	scene->lightScale = value;
}

// D3D11_STANDARD_MULTISAMPLE_QUALITY_LEVELS enumeration
std::vector<std::vector<std::pair<int, int>>> RayTracer::msaaSamplePattern =
{
	{
		{ 0, 0 }
	},
	{
		{ 4, 4 }, { - 4, - 4 }
	},
	{
		{ - 2, - 6 }, { 6, - 2 }, { - 6, 2 }, { 2, 6 }
	},
	{
		{ 1, - 3 }, { - 1, 3 }, { 5, 1 }, { - 3, - 5 },
		{ - 5, 5 }, { - 7, - 1 }, { 3, 7 }, { 7, - 7 }
	},
	{
		{ 1, 1 }, { - 1, - 3 }, { - 3, 2 }, { 4, - 1 },
		{ - 5, - 2 }, { 2, 5 }, { 5, 3 }, { 3, - 5 },
		{ - 2, 6 }, { 0, - 7 }, { - 4, - 6 }, { - 6, 4 },
		{ - 8, 0 }, { 7, - 4 }, { 6, 7 }, { - 7, - 8 }
	},
	{
		{ - 4, - 7 }, { - 7, - 5 }, { - 3, - 5 }, { - 5, - 4 },
		{ - 1, - 4 }, { - 2, - 2 }, { - 6, - 1 }, { - 4, 0 },
		{ - 7, 1 }, { - 1, 2 }, { - 6, 3 }, { - 3, 3 },
		{ - 7, 6 }, { - 3, 6 }, { - 5, 7 }, { - 1, 7 },
		{ 5, - 7 }, { 1, - 6 }, { 6, - 5 }, { 4, - 4 },
		{ 2, - 3 }, { 7, - 2 }, { 1, - 1 }, { 4, - 1 },
		{ 2, 1 }, { 6, 2 }, { 0, 4 }, { 4, 4 },
		{ 2, 5 }, { 7, 5 }, { 5, 6 }, { 3, 7 }
	}
};
