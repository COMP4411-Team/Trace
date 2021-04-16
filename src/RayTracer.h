#ifndef __RAYTRACER_H__
#define __RAYTRACER_H__

// The main ray tracer.

#include <vector>

#include "fileio/bitmap.h"
#include "scene/scene.h"
#include "scene/Ray.h"
#include "photon_map/Photon.h"
#include "photon_map/KdTree.h"

class RayTracer
{
	friend class TraceUI;
public:
    RayTracer();
    ~RayTracer();

    vec3f trace( Scene *scene, double x, double y );
	vec3f traceRay( Scene *scene, const Ray& r, const vec3f& thresh, int depth, const vec3f& curFactor );
	vec3f tracePath(Scene* scene, const Ray& ray, int depth);		// path tracing
	
	void buildPhotonMap();
	void tracePhoton(Photon* cur);
	vec3f gatherPhoton(const vec3f& pos);

	void getBuffer( unsigned char *&buf, int &w, int &h );
	void swapBuffer();
	double aspectRatio();
	void traceSetup( int w, int h, int maxDepth, const vec3f& threshold );
	void traceLines( int start = 0, int stop = 10000000 );
	void tracePixel( int i, int j, int iter );

	// Adaptive supersampling
	void adaptiveTrace();
	vec3f subdivide(double x1, double y1, double x2, double y2,
		const vec3f& color1, const vec3f& color2, int& newSampleNum, int depth);
	void visualizeSamples();
	
	vec3f tracePixelMotionBlur(int i, int j);
	void pathTrace(int iter);
	void setLightScale(double value);

	// Depth of field
	void enableDof(bool value);
	void setAperture(double aperture);
	void setFocalLength(double focal);

	bool loadScene( char* fn );
	Scene* getScene() { return scene; }
	bool sceneLoaded();

	
	bool loadHFmap(const string& filename);	
	bool HFmapLoaded() { if (hfmap != nullptr) return true; else return false; }
	HFmap* getHFmap() { return hfmap; }
	void renderHFMap();

	void setFasterShadow(bool i) { scene->enableFasterShadow = i; }

	int ssaaSample{0};	// the exponent of 2
	bool ssaaJitter{false};

	// Adaptive supersampling
	double adaptiveThresh{0.25};
	int maxSubdivisionDepth{3};
	std::vector<std::vector<int>> sampleNum;

	bool enableMotionBlur{false};
	int motionBlurSPP{100};

	bool enableDistributed{false};
	int numChildRay{10};

	// Parameters for path tracing
	bool enablePathTracing{false};
	int SPP{64};		// sample per pixel
	double rrThresh{0.7};	// russian roulette threshold

	// Photon mapping
	bool enablePM{false};
	vec3f totalFlux{0.03};
	int numPhotons{50000};	// total num of photons emitted
	int numNeighbours{20};	// num of nearest neighbours
	int maxBounce{16};
	double maxDist{0.01};

private:
	void setPixel(int x, int y, const vec3f& color);
	
	unsigned char *buffer;
	unsigned char* backBuffer;		// used for progressive path tracing
	int buffer_width, buffer_height;
	int bufferSize;
	Scene *scene;

	KdTree* kdTree{nullptr};
	std::vector<Photon*> photonMap;

	HFmap* hfmap{ nullptr };

	bool m_bSceneLoaded;
	int maxDepth{0};
	int ptMaxDepth{32};
	vec3f threshold;

	static std::vector<std::vector<std::pair<int, int>>> msaaSamplePattern;
};

#endif // __RAYTRACER_H__
