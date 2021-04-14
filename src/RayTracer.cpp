// The main ray tracer.

#include <Fl/fl_ask.h>

#include "RayTracer.h"

#include <cassert>

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
	if (!enablePathTracing)
		return traceRay( scene, r, threshold, 0, {1.0, 1.0, 1.0} ).clamp();

	//vec3f color;
	//for (int i = 0; i < SPP; ++i)
	//	color += tracePath(scene, r, 0).clamp();
	//return color / SPP;
	return tracePath(scene, r, 0).clamp();
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

		vec3f reflective = m.fresnelReflective(-r.getDirection(), i.N);
		vec3f directIllumination = m.shade(scene, r, i);
		vec3f indirectIllumination, factor(1.0);

		if (!reflective.iszero())
		{
			Ray reflection = r.reflect(i);
			if (enableDistributed)
			{
				vec3f normal = i.N.dot(r.getDirection()) < 0.0 ? i.N : -i.N;
				vec3f tmp, position = r.at(i.t) + normal * DISPLACEMENT_EPSILON;
				for (int j = 0; j < numChildRay; ++j)
				{
					reflection = Ray(position, m.randomReflect(r.getDirection(), i.N));
					tmp += prod(traceRay(scene, reflection, thresh, depth + 1, prod(curFactor, m.kr)), reflective);
				}
				tmp /= numChildRay;
				indirectIllumination += tmp;
			}
			else
				indirectIllumination += prod(traceRay(scene, reflection, thresh, depth + 1, prod(curFactor, m.kr)), reflective);
		}

		Ray refraction{vec3f(), vec3f()};
		if (!m.kt.iszero() && r.refract(i, refraction))
		{
			indirectIllumination += prod(traceRay(scene, refraction, thresh, depth + 1, prod(curFactor, m.kt)), m.kt);
		}

		// Travel inside a transparent object, apply Beer's law
		if (i.N.dot(r.getDirection()) > 0.0 && !m.absorb.iszero())
		{
			double distTraveled = i.t;
			factor = -m.absorb * distTraveled;
			factor[0] = exp(factor[0]);
			factor[1] = exp(factor[1]);
			factor[2] = exp(factor[2]);
		}

		// So far we don't have a uniform model in Whitted ray tracing to
		// distinguish diffuse and specular surfaces
		if (enablePM && m.kr.iszero() && m.kt.iszero())
			indirectIllumination += prod(gatherPhoton(r.at(i.t)), m.kd);

		return prod(directIllumination + indirectIllumination, factor);
	
	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
		
		if (!scene->useSkybox)
			return vec3f( 0.0, 0.0, 0.0 );
		
		scene->skybox->intersect(r, i);
		return scene->skybox->getColor(r, i) / 255.0;
	}
}

vec3f RayTracer::tracePath(Scene* scene, const Ray& ray, int depth)
{
	vec3f radiance, beta(1.0, 1.0, 1.0);
	Isect isect;
	Ray curRay(ray);

	for (int bounce = 0; bounce < ptMaxDepth; ++bounce)
	{
		if (beta.iszero())
			break;
		if (!scene->bvhIntersect(curRay, isect))
		{
			if (scene->useSkybox)
			{
				scene->skybox->intersect(curRay, isect);
				radiance += prod(beta, scene->skybox->getColor(curRay, isect) / 255.0);
			}
			break;
		}
		
		if (isect.obj->hasEmission)
		{
			radiance += prod(isect.obj->getEmission(), beta);
			break;
		}

		const Material& material = isect.getMaterial();
		vec3f pos = curRay.at(isect.t) + isect.N * DISPLACEMENT_EPSILON;
		if (!material.isTransmissive)
		{
			vec3f emission;
			double lightPdf;
			Ray directLight = scene->uniformSampleOneLight(emission, lightPdf);

			vec3f lightDir = (directLight.getPosition() - pos).normalize();
			Ray lightRay(pos, lightDir);
			double distance = (directLight.getPosition() - pos).length();

			Isect shadowRay;

			// Direct illumination part
			if (!scene->bvhIntersect(lightRay, shadowRay) || shadowRay.t > distance - RAY_EPSILON)
			{
				vec3f bxdf = material.bxdf(lightDir, -curRay.getDirection(), isect.N);
				// From sampling solid angle to sampling light area
				double coeff = directLight.getDirection().dot(-lightDir) / (distance * distance * lightPdf);
				coeff *= isect.N.dot(lightDir);
				
				if (coeff > 0.0)
				{
					emission *= coeff;
					emission = prod(bxdf, emission);
					radiance += prod(beta, emission);
				}
			}
		}

		double rr = getRandomReal(); // Russian roulette
		if (rr > rrThresh)
			break;

		double bxdfPdf;
		vec3f wo = -curRay.getDirection();
		vec3f wi;
		vec3f bxdf = material.sampleF(wo, wi, isect.N, bxdfPdf);	// sample new direction and get the BxDF
		if (bxdf.iszero())
			break;
		wi = wi.normalize();
		Ray newRay(pos, wi);

		double coeff = _abs(isect.N.dot(wi)) / (bxdfPdf * rrThresh);
		beta = prod(bxdf, beta) * coeff;
		curRay = newRay;
	}
	return radiance;
}

void RayTracer::buildPhotonMap()
{
	if (!m_bSceneLoaded)
		return;
	photonMap.clear();
	delete kdTree;
	
	vector<Light*> lights;
	for (auto* light : scene->lights)
	{
		if (typeid(*light) != typeid(SpotLight) && typeid(*light) != typeid(AmbientLight))
			lights.push_back(light);
		light->buildProjectionMap();
	}

	// TODO: emit photons according to the importance of each light
	while (photonMap.size() < numPhotons)
	{
		for (auto* light : lights)
			tracePhoton(light->emitPhoton());
	}

	kdTree = new KdTree(photonMap, 3);
}

void RayTracer::tracePhoton(Photon* cur)
{
	int bounce = 0;
	bool hitSpecular = false;
	Ray ray(cur->position, cur->direction);
	while (bounce < maxBounce)
	{
		Isect isect;
		// TODO: add projection maps
		if (!scene->bvhIntersect(ray, isect))
			break;

		Material material = isect.getMaterial();
		//double refractProb = _max(material.kt[0], _max(material.kt[1], material.kt[2]));
		//double reflectProb = 1.0 - refractProb;
		//reflectProb *= _max(material.kr[0], _max(material.kr[1], material.kr[2]));

		// Only trace caustic photons so far
		// double roll = getRandomReal();

		if (!material.kt.iszero())
		{
			Ray tmp(ray);
			if (!ray.refract(isect, tmp))
				break;
			hitSpecular = true;
			cur->power = prod(cur->power, material.kt);
			ray = tmp;
			++bounce;
			continue;
		}
		
		if (!material.kr.iszero())
		{
			ray = ray.reflect(isect);
			hitSpecular = true;
			cur->power[0] *= material.kr[0];
			cur->power[1] *= material.kr[1];
			cur->power[2] *= material.kr[2];
			++bounce;
			continue;
		}

		if (hitSpecular)
		{
			auto* photon = new Photon{ray.at(isect.t), ray.getDirection(), cur->power};
			photonMap.push_back(photon);
		}
		break;
	}
	delete cur;
}

vec3f RayTracer::gatherPhoton(const vec3f& pos)
{
	vec3f flux;
	double maxDist2 = 0.0;
	auto nearestPhotons = kdTree->getKnn(pos, numNeighbours, maxDist);
	if (nearestPhotons.empty())
		return vec3f();
	for (auto* photon : nearestPhotons)
	{
		double curDist2 = (photon->position - pos).length_squared();
		if (curDist2 > maxDist2)
			maxDist2 = curDist2;
		flux += photon->power;
	}
	return prod(flux, totalFlux) / (numPhotons * PI * maxDist2 + RAY_EPSILON);
}

RayTracer::RayTracer()
{
	buffer = NULL;
	backBuffer = nullptr;
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

void RayTracer::swapBuffer()
{
	swap(buffer, backBuffer);
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
		scene = readScene( fn, hfmap);
	}
	catch( ParseError pe )
	{
		fl_alert( "ParseError: %s\n", pe );
		m_bSceneLoaded = false;
		return false;
	}

	if( !scene )
	{
		m_bSceneLoaded = false;
		return false;
	}
	
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
	scene->enableDistributed = enableDistributed;

	return true;
}


bool RayTracer::loadHFmap(const string& filename) {
	int h, w;
	auto* hf = readBMP(const_cast<char*>(filename.c_str()), w, h);
	if (hf == nullptr)
		return false;
	string grey = filename.substr(0, filename.length() - 5).append("grey_.bmp");
	auto* gf = readBMP(const_cast<char*>(filename.c_str()), w, h);
	if (gf == nullptr)
		return false;
	if (hfmap) delete hfmap;
	hfmap = new HFmap(hf, gf, h, w);
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

		delete [] backBuffer;
		backBuffer = new unsigned char[bufferSize];
	}
	memset( buffer, 0, w*h*3 );
	memset(backBuffer, 0, bufferSize);
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
			tracePixel(i, j, 1);
}

void RayTracer::tracePixel( int i, int j, int iter )
{
	vec3f col;

	if( !scene )
		return;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	if (!enableMotionBlur)
	{
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
	}
	else
		col = tracePixelMotionBlur(i, j, iter);
	
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;
	if (!enablePathTracing)
	{
		pixel[0] = (int)( 255.0 * col[0]);
		pixel[1] = (int)( 255.0 * col[1]);
		pixel[2] = (int)( 255.0 * col[2]);
	}
	else
	{
		double weight = (double)(iter - 1) / iter;
		auto* backPixel = backBuffer + ( i + j * buffer_width ) * 3;
		backPixel[0] = (int)( 255.0 * col[0] / iter + pixel[0] * weight );
		backPixel[1] = (int)( 255.0 * col[1] / iter + pixel[1] * weight );
		backPixel[2] = (int)( 255.0 * col[2] / iter + pixel[2] * weight );
	}
}

vec3f RayTracer::tracePixelMotionBlur(int i, int j, int iter)
{
	vec3f col;
	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);
	
	for (int i = 0; i < motionBlurSPP; ++i)
		col += trace( scene, x, y );
	return col / motionBlurSPP;
}

void RayTracer::pathTrace(int iter)
{
	for (int i = 0; i < buffer_height; ++i)
	{
		#pragma omp parallel for num_threads(4)
		for (int j = 0; j < buffer_width; ++j)
		{
			tracePixel(i, j, iter);
		}
	}
}

void RayTracer::setLightScale(double value)
{
	scene->lightScale = value;
}

void RayTracer::enableDof(bool value)
{
	if (!m_bSceneLoaded)
		return;
	scene->getCamera()->setEnableDof(value);
}

void RayTracer::setAperture(double aperture)
{
	if (!m_bSceneLoaded)
		return;
	scene->getCamera()->setAperture(aperture);
}

void RayTracer::setFocalLength(double focal)
{
	if (!m_bSceneLoaded)
		return;
	scene->getCamera()->setFocalLength(focal);
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
