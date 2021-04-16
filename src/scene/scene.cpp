#include <cmath>

#include "scene.h"
#include "light.h"
#include "../ui/TraceUI.h"
extern TraceUI* traceUI;



void BoundingBox::operator=(const BoundingBox& target)
{
	min = target.min;
	max = target.max;
}

// Does this bounding box intersect the target?
bool BoundingBox::intersects(const BoundingBox &target) const
{
	return ((target.min[0] - RAY_EPSILON <= max[0]) && (target.max[0] + RAY_EPSILON >= min[0]) &&
			(target.min[1] - RAY_EPSILON <= max[1]) && (target.max[1] + RAY_EPSILON >= min[1]) &&
			(target.min[2] - RAY_EPSILON <= max[2]) && (target.max[2] + RAY_EPSILON >= min[2]));
}

// does the box contain this point?
bool BoundingBox::intersects(const vec3f& point) const
{
	return ((point[0] + RAY_EPSILON >= min[0]) && (point[1] + RAY_EPSILON >= min[1]) && (point[2] + RAY_EPSILON >= min[2]) &&
		 (point[0] - RAY_EPSILON <= max[0]) && (point[1] - RAY_EPSILON <= max[1]) && (point[2] - RAY_EPSILON <= max[2]));
}

// if the ray hits the box, put the "t" value of the intersection
// closest to the origin in tMin and the "t" value of the far intersection
// in tMax and return true, else return false.
// Using Kay/Kajiya algorithm.
bool BoundingBox::intersect(const Ray& r, double& tMin, double& tMax) const
{
	vec3f R0 = r.getPosition();
	vec3f Rd = r.getDirection();

	tMin = -1.0e308; // 1.0e308 is close to infinity... close enough for us!
	tMax = 1.0e308;
	double ttemp;
	
	for (int currentaxis = 0; currentaxis < 3; currentaxis++)
	{
		double vd = Rd[currentaxis];
		
		// if the ray is parallel to the face's plane (=0.0)
		if( vd == 0.0 )
			continue;

		double v1 = min[currentaxis] - R0[currentaxis];
		double v2 = max[currentaxis] - R0[currentaxis];

		// two slab intersections
		double t1 = v1/vd;
		double t2 = v2/vd;
		
		if ( t1 > t2 ) { // swap t1 & t2
			ttemp = t1;
			t1 = t2;
			t2 = ttemp;
		}

		if (t1 > tMin)
			tMin = t1;
		if (t2 < tMax)
			tMax = t2;

		if (tMin > tMax) // box is missed
			return false;
		if (tMax < 0.0) // box is behind ray
			return false;
	}
	return true; // it made it past all 3 axes.
}


bool Geometry::intersect(const Ray&r, Isect&i) const
{
    // Transform the ray into the object's local coordinate space
    vec3f pos = transform->globalToLocalCoords(r.getPosition());
    vec3f dir = transform->globalToLocalCoords(r.getPosition() + r.getDirection()) - pos;
    double length = dir.length();
    dir /= length;

    Ray localRay( pos, dir );

    if (intersectLocal(localRay, i)) {
        // Transform the intersection point & normal returned back into global space.
		i.N = transform->localToGlobalCoordsNormal(i.N);
		i.t /= length;
    		if (i.hasTexCoords)
    			i.tbn = transform->normi * i.tbn;
		return true;
    } else {
        return false;
    }
    
}

bool Geometry::intersectLocal( const Ray& r, Isect& i ) const
{
	return false;
}

bool Geometry::hasBoundingBoxCapability() const
{
	// by default, primitives do not have to specify a bounding box.
	// If this method returns true for a primitive, then either the ComputeBoundingBox() or
    // the ComputeLocalBoundingBox() method must be implemented.

	// If no bounding box capability is supported for an object, that object will
	// be checked against every single ray drawn.  This should be avoided whenever possible,
	// but this possibility exists so that new primitives will not have to have bounding
	// boxes implemented for them.
	
	return false;
}

Scene::~Scene()
{
    giter g;
    liter l;
    
	for( g = objects.begin(); g != objects.end(); ++g ) {
		delete (*g);
	}

	for( g = boundedobjects.begin(); g != boundedobjects.end(); ++g ) {
		delete (*g);
	}

	for( g = nonboundedobjects.begin(); g != nonboundedobjects.end(); ++g ) {
		delete (*g);
	}

	for( l = lights.begin(); l != lights.end(); ++l ) {
		delete (*l);
	}
	delete skybox;
	delete emitter;
}

// Get any intersection with an object.  Return information about the 
// intersection through the reference parameter.
bool Scene::intersect( const Ray& r, Isect& i ) const
{
	typedef list<Geometry*>::const_iterator iter;
	iter j;

	Isect cur;
	bool have_one = false;

	// try the non-bounded objects
	for( j = nonboundedobjects.begin(); j != nonboundedobjects.end(); ++j ) {
		if( (*j)->intersect( r, cur ) ) {
			if( !have_one || (cur.t < i.t) ) {
				i = cur;
				have_one = true;
			}
		}
	}

	// try the bounded objects
	for( j = boundedobjects.begin(); j != boundedobjects.end(); ++j ) {
		if( (*j)->intersect( r, cur ) ) {
			if( !have_one || (cur.t < i.t) ) {
				i = cur;
				have_one = true;
			}
		}
	}


	return have_one;
}

bool Scene::bvhIntersect(const Ray& ray, Isect& isect) const
{
	bool flag = bvh.intersect(ray, isect);
	Isect curIsect;

	for (auto* object : nonboundedobjects)
	{
		if (object->intersect(ray, curIsect))
		{
			if (!flag || curIsect.t < isect.t)
			{
				flag = true;
				isect = curIsect;
			}
		}
	}
	
	return flag;
}


void Scene::initScene()
{
	bool first_boundedobject = true;
	BoundingBox b;
	
	typedef list<Geometry*>::const_iterator iter;
	// split the objects into two categories: bounded and non-bounded
	for( iter j = objects.begin(); j != objects.end(); ++j ) {
		if( (*j)->hasBoundingBoxCapability() )
		{
			boundedobjects.push_back(*j);

			// widen the scene's bounding box, if necessary
			if (first_boundedobject) {
				sceneBounds = (*j)->getBoundingBox();
				first_boundedobject = false;
			}
			else
			{
				b = (*j)->getBoundingBox();
				sceneBounds.max = maximum(sceneBounds.max, b.max);
				sceneBounds.min = minimum(sceneBounds.min, b.min);
			}
		}
		else
			nonboundedobjects.push_back(*j);

		if ((*j)->hasEmission)
			emittingObjects.push_back(*j);
	}

	bvh.build(boundedobjects);
}

Ray Scene::uniformSampleOneLight(vec3f& emit, double& pdf)
{
	double emitAreaSum = 0.0;
	for (auto* light : emittingObjects)
		emitAreaSum += light->getArea();
	double prob = getRandomReal() * emitAreaSum;
	emitAreaSum = 0.0;
	for (auto* light : emittingObjects)
	{
		emitAreaSum += light->getArea();
		if (emitAreaSum >= prob)
			return light->sample(emit, pdf);
	}
	return Ray{vec3f(), vec3f()};
}


bool Scene::loadHFmap(const string& filename) {
	int h, w;
	auto* hf = readBMP(const_cast<char*>(filename.c_str()), w, h);
	if (hf == nullptr)
		return false;
	string grey = filename.substr(0, filename.length() - 4).append("grey_.bmp");
	auto* gf = readBMP(const_cast<char*>(filename.c_str()), w, h);
	if (gf == nullptr)
		return false;
	if (hfmap) delete hfmap;
	hfmap = new HFmap(hf, gf, h, w);
	return true;
}




Texture::~Texture()
{
	delete [] data;
}

vec3f Texture::sample(double u, double v) const
{
	u = _min(_max(0.0, u), 1.0);
	v = _min(_max(0.0, v), 1.0);

	int x = u * (width - 1);
	int y = v * (height - 1);
	unsigned char* pixel = data + (y * width + x) * 3;
	
	return vec3f(pixel[0], pixel[1], pixel[2]);
}

vec3f Texture::sample(const TexCoords& coords) const
{
	return sample(coords.u, coords.v);
}

//HFmap::HFmap(char* m, int h, int w) :map(m), height(h), weight(w) {}

HFmap::~HFmap() {
	map = nullptr;
	greymap = nullptr;
}

vec3f HFmap::getC(int x, int y) const {
	unsigned char* pixel = map+(y * width + x) * 3;

	return vec3f(pixel[0], pixel[1], pixel[2]);
}

double HFmap::getH(int x, int y) const {
	unsigned char* color = greymap + (y * width + x) * 3;
	return (static_cast<double>(color[0]) + color[1] + color[2]) / 3.0;
}

void BVH::build(const list<Geometry*>& objects)
{
	delete root;
	root = new BVHNode;
	vector<Geometry*> objs{std::begin(objects), std::end(objects)};
	buildHelper(root, objs);
}

void BVH::buildHelper(BVHNode* cur, const vector<Geometry*>& objects)
{
	BoundingBox maxBoundingBox = calMaxBoundingBox(objects);
	cur->aabb = maxBoundingBox;
	
	if (objects.size() < threshold)
	{
		cur->objects = objects;
		return;
	}
	
	vector<Geometry*> left, right;
	vector<pair<Geometry*, double>> coords;
	
	double xRange = maxBoundingBox.max[0] - maxBoundingBox.min[0];
	double yRange = maxBoundingBox.max[1] - maxBoundingBox.min[1];
	double zRange = maxBoundingBox.max[2] - maxBoundingBox.min[2];

	// Find the longest axis
	int axis = 0;
	if (yRange > xRange)
		axis = 1;
	if (zRange > xRange && zRange > yRange)
		axis = 2;
	
	for (auto* object : objects)
	{
		BoundingBox curBox = object->getBoundingBox();
		coords.emplace_back(object, (curBox.max[axis] + curBox.min[axis]) / 2.0);
	}

	sort(coords.begin(), coords.end(), 
		[](const pair<Geometry*, double>& a, const pair<Geometry*, double>& b) { return a.second < b.second; }
	);

	int mid = coords.size() / 2;
	for (int i = 0; i < mid; ++i)
		left.push_back(coords[i].first);
	for (int i = mid; i < coords.size(); ++i)
		right.push_back(coords[i].first);

	cur->left = new BVHNode;
	cur->right = new BVHNode;

	buildHelper(cur->left, left);
	buildHelper(cur->right, right);
}

BoundingBox BVH::calMaxBoundingBox(const vector<Geometry*>& objects)
{
	BoundingBox aabb;
	if (objects.empty())
		return aabb;
	vec3f curMin = objects[0]->getBoundingBox().min, curMax = objects[0]->getBoundingBox().max;
	for (auto* object : objects)
	{
		curMin = minimum(curMin, object->getBoundingBox().min);
		curMax = maximum(curMax, object->getBoundingBox().max);
	}
	
	aabb.max = curMax;
	aabb.min = curMin;
	return aabb;
}

bool BVH::intersect(const Ray& ray, Isect& isect) const
{
	return root->intersect(ray, isect);
}

bool BVHNode::intersect(const Ray& ray, Isect& isect) const
{
	double tMin, tMax;
	if (!aabb.intersect(ray, tMin, tMax))
		return false;

	if (left == nullptr)		// leaf node
	{
		bool flag = false;
		Isect curIsect;
		for (auto* object : objects)
		{
			if (object->intersect(ray, curIsect))
			{
				if (!flag || curIsect.t < isect.t)
				{
					flag = true;
					isect = curIsect;
				}
			}
		}
		return flag;
	}
	
	Isect leftIsect, rightIsect;
	bool leftSuccess = left->intersect(ray, leftIsect);
	bool rightSuccess = right->intersect(ray, rightIsect);
	if (leftSuccess)
	{
		if (rightSuccess && rightIsect.t < leftIsect.t)
			isect = rightIsect;
		else
			isect = leftIsect;
	}
	else if (rightSuccess)
		isect = rightIsect;
	
	return leftSuccess || rightSuccess;
}
