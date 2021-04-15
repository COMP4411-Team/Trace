#ifdef WIN32
#pragma warning( disable : 4786 )
#endif

#include <cmath>
#include <cstring>
#include <fstream>
#include <strstream>

#include <vector>

#include "read.h"

#include "bitmap.h"
#include "parse.h"

#include "../scene/scene.h"
#include "../SceneObjects/trimesh.h"
#include "../SceneObjects/Box.h"
#include "../SceneObjects/Cone.h"
#include "../SceneObjects/Cylinder.h"
#include "../SceneObjects/Sphere.h"
#include "../SceneObjects/Square.h"
#include "../scene/light.h"
#include "../SceneObjects/CSG.h"
#include "../particle/FluidSystem.h"
#include "../particle/Emitter.h"

typedef map<string,Material*> mmap;

static void processObject( Obj *obj, Scene *scene, mmap& materials );
static Obj *getColorField( Obj *obj );
static Obj *getField( Obj *obj, const string& name );
static bool hasField( Obj *obj, const string& name );
static vec3f tupleToVec( Obj *obj );
static TexCoords tupleToTexCoords(Obj *obj);
static Geometry* processGeometry(Obj* obj, Scene* scene,
                                 const mmap& materials, TransformNode* transform);
static Geometry* processGeometry(string name, Obj* child, Scene* scene,
                                 const mmap& materials, TransformNode* transform);
static void processTrimesh( string name, Obj *child, Scene *scene,
    const mmap& materials, TransformNode *transform );
static void processCamera( Obj *child, Scene *scene );
static Material *getMaterial( Obj *child, const mmap& bindings );
static Material *processMaterial( Obj *child, mmap *bindings = NULL );
static void processBindings(Obj* child, mmap* bindings, Material* mat);
static void verifyTuple( const mytuple& tup, size_t size );

static bool processHField(Scene* scene, TransformNode* transform);

static bool loadTexture(const string& filename, Texture& texture);
static bool processTexture(Obj* child, Geometry* geometry);
static bool processSkybox(Obj* child, Scene* scene);
static Microfacet* processMicrofacet(Obj* child, mmap* bindings);
static FresnelSpecular* processFresnelSpecular(Obj* child, mmap* bindings);
static CSG* processCSG(Obj* child, Scene* scene, const mmap& materials, TransformNode* transform);
static CSG* parseCSG(const mytuple& expression, const std::map<string, SceneObject*>& map, Scene* scene);
static void processFluidSystem(Obj* obj, Scene* scene);
static void processEmitter(Obj* obj, Scene* scene);

Scene *readScene( const string& filename, HFmap * hfm )
{
	ifstream ifs( filename.c_str() );
	if( !ifs ) {
		cerr << "Error: couldn't read scene file " << filename << endl;
		return NULL;
	}

	try {
		return readScene( ifs, hfm);
	} catch( ParseError& pe ) {
		cout << "Parse error: " << pe << endl;
		return NULL;
	}
}

Scene *readScene( istream& is, HFmap* hfm)
{
	Scene *ret = new Scene;
	
	// Extract the file header
	static const int MAXNAME = 80;
	char buf[ MAXNAME ];
	int ct = 0;

	while( ct < MAXNAME - 1 ) {
		char c;
		is.get( c );
		if( c == ' ' || c == '\t' || c == '\n' ) {
			break;
		}
		buf[ ct++ ] = c;
	}

	buf[ ct ] = '\0';

	if( strcmp( buf, "SBT-raytracer" ) ) {
		throw ParseError( string( "Input is not an SBT input file." ) );
	}

	float version;
	is >> version;

	if( version != 1.0 ) {
		ostrstream oss;
		oss << "Input is version " << version << ", need version 1.0" << ends;

		throw ParseError( string( oss.str() ) );
	}

	// vector<Obj*> result;
	mmap materials;

	while( true ) {
		Obj *cur = readFile( is );
		if( !cur ) {
			break;
		}
		processObject( cur, ret, materials );
		delete cur;
	}

	if (hfm!= nullptr && !ret->HFmapLoaded()) {
		delete[] ret->hfmap;
		ret->hfmap = hfm;
		processHField(ret, &ret->transformRoot);
	}

	return ret;
}

// Find a color field inside some object.  Now, I recognize that not
// everyone speaks the Queen's English, so I allow both spellings of
// color.  If you're composing scenes, you don't need to worry about
// spelling :^)
static Obj *getColorField( Obj *obj )
{
	if( obj == NULL ) {
		throw ParseError( string( "Object contains no color field" ) );
	}

	const dict& d = obj->getDict();
	dict::const_iterator i; 
	if( (i = d.find( "color" )) != d.end() ) {
		return (*i).second;
	}
	if( (i = d.find( "colour" )) != d.end() ) {
		return (*i).second;
	}

	throw ParseError( string( "Object contains no color field" ) );
}

// Get a named field out of a dictionary object.  Throw a parse
// error if the field isn't there.
static Obj *getField( Obj *obj, const string& name )
{
	if( obj == NULL ) {
		throw ParseError( string( "Object contains no field named \"" ) +
			name + "\"" );
	}

	const dict& d = obj->getDict();
	dict::const_iterator i; 
	if( (i = d.find( name )) != d.end() ) {
		return (*i).second;
	}

	throw ParseError( string( "Object contains no field named \"" ) + name + "\"" );
}

// Determine if the given dictionary object has the named field.
static bool hasField( Obj *obj, const string& name )
{
	if( obj == NULL ) {
		return false;
	}

	const dict& d = obj->getDict();
	return d.find( name ) != d.end();
}

// Turn a parsed tuple into a 3D point.
static vec3f tupleToVec( Obj *obj )
{
	const mytuple& t = obj->getTuple();
	verifyTuple( t, 3 );
	return vec3f( t[0]->getScalar(), t[1]->getScalar(), t[2]->getScalar() );
}

TexCoords tupleToTexCoords(Obj* obj)
{
	const mytuple& t = obj->getTuple();
	verifyTuple( t, 2 );
	return TexCoords(t[0]->getScalar(), t[1]->getScalar());
}

static Geometry* processGeometry(Obj* obj, Scene* scene,
                                 const mmap& materials, TransformNode* transform)
{
	string name;
	Obj *child; 

	if( obj->getTypeName() == "id" ) {
		name = obj->getID();
		child = NULL;
	} else if( obj->getTypeName() == "named" ) {
		name = obj->getName();
		child = obj->getChild();
	} else {
		ostrstream oss;
		oss << "Unknown input object ";
		obj->printOn( oss );

		throw ParseError( string( oss.str() ) );
	}
	return processGeometry( name, child, scene, materials, transform);
}

// Extract the named scalar field into ret, if it exists.
static bool maybeExtractField( Obj *child, const string& name, double& ret )
{
	const dict& d = child->getDict();
	if( hasField( child, name ) ) {
		ret = getField( child, name )->getScalar();
		return true;
	}

	return false;
}

// Extract the named boolean field into ret, if it exists.
static bool maybeExtractField( Obj *child, const string& name, bool& ret )
{
	const dict& d = child->getDict();
	if( hasField( child, name ) ) {
		ret = getField( child, name )->getBoolean();
		return true;
	}

	return false;
}

// Check that a tuple has the expected size.
static void verifyTuple( const mytuple& tup, size_t size )
{
	if( tup.size() != size ) {
		ostrstream oss;
		oss << "Bad tuple size " << tup.size() << ", expected " << size;

		throw ParseError( string( oss.str() ) );
	}
}

bool loadTexture(const string& filename, Texture& texture)
{
	auto* tex = readBMP(const_cast<char*>(filename.c_str()), texture.width, texture.height);
	if (tex == nullptr)
		return false;
	texture.data = tex;
	return true;
}

bool processTexture(Obj* child, Geometry* geometry)
{
	if (hasField(child, "bump_map"))
	{
		string filename = getField(child, "bump_map")->getString();
		if (!loadTexture(filename, geometry->bumpMap))
			return false;
		geometry->enableBumpMap = true;
		geometry->setEnableTexCoords(true);
	}
	if (hasField(child, "diffuse_map"))
	{
		string filename = getField(child, "diffuse_map")->getString();
		if (!loadTexture(filename, geometry->diffuseMap))
			return false;
		geometry->enableDiffuseMap = true;
		geometry->setEnableTexCoords(true);
	}
	if (hasField(child, "normal_map"))
	{
		string filename = getField(child, "normal_map")->getString();
		if (!loadTexture(filename, geometry->normalMap))
			return false;
		geometry->enableNormalMap = true;
		geometry->setEnableTexCoords(true);
	}
	if (hasField(child, "perlin_noise"))
	{
		Obj* obj = getField(child, "perlin_noise");
		double scale = 4;
		maybeExtractField(obj, "scale", scale);
		
		int depth = 7;
		if (hasField(obj, "turbulence_depth"))
			depth = getField(obj, "turbulence_depth")->getScalar();
		geometry->enableSolidTexture = true;
		geometry->solidTexture = new PerlinNoise(256, scale, depth);
	}
	if (hasField(child, "anisotropic_specular"))
	{
		string filename = getField(child, "anisotropic_specular")->getString();
		if (!loadTexture(filename, geometry->anisoSpecular))
			return false;
		geometry->enableAnisotropicSpecular = true;
		geometry->setEnableTexCoords(true);
	}
	return true;
}



bool processSkybox(Obj* child, Scene* scene)
{
	if (!hasField(child, "front") || !hasField(child, "back") || !hasField(child, "top") ||
		!hasField(child, "bottom") || !hasField(child, "left") || !hasField(child, "right"))
		return false;

	auto* skybox = new Skybox(scene, nullptr);
	bool result = loadTexture(getField(child, "front")->getString(), skybox->front) &&
		loadTexture(getField(child, "back")->getString(), skybox->back) &&
		loadTexture(getField(child, "top")->getString(), skybox->top) &&
		loadTexture(getField(child, "bottom")->getString(), skybox->bottom) &&
		loadTexture(getField(child, "left")->getString(), skybox->left) &&
		loadTexture(getField(child, "right")->getString(), skybox->right);
	
	if (result)
	{
		delete scene->skybox;
		scene->skybox = skybox;
		scene->useSkybox = true;
	}
	else
		delete skybox;
	
	return result;
}

Microfacet* processMicrofacet(Obj* child, mmap* bindings)
{
	if (!hasField(child, "albedo") || !hasField(child, "roughness") || !hasField(child, "metallic"))
		return nullptr;

	vec3f albedo = tupleToVec( getField( child, "albedo" ) );
	double roughness = getField( child, "roughness" )->getScalar();
	double metallic = getField( child, "metallic" )->getScalar();

	auto* material = new Microfacet(albedo, roughness, metallic);

	processBindings(child, bindings, material);
	
	return material;
}

FresnelSpecular* processFresnelSpecular(Obj* child, mmap* bindings)
{
	if (!hasField(child, "albedo") || !hasField(child, "metallic") 
		|| !hasField(child, "roughness") || !hasField(child, "translucency"))
		throw ParseError("parameters missing for FresnelSpecular material");

	vec3f albedo = tupleToVec(getField(child, "albedo"));
	double roughness = getField(child, "roughness")->getScalar();
	double metallic = getField(child, "metallic")->getScalar();
	double translucency = getField(child, "translucency")->getScalar();

	auto* material = new FresnelSpecular(albedo, roughness, metallic, translucency);

	if (hasField(child, "index"))
		material->index = getField(child, "index")->getScalar();

	if (hasField(child, "specular"))
		material->ks = tupleToVec(getField(child, "specular"));

	processBindings(child, bindings, material);

	return material;
}

CSG* processCSG(Obj* child, Scene* scene, const mmap& materials, TransformNode* transform)
{
	if (!hasField(child, "primitives"))
		throw ParseError("CSG must have primitives field");
	const auto& primitives = getField(child, "primitives")->getTuple();

	std::map<string, SceneObject*> map;
	
	for (auto* obj : primitives)
	{
		string name = obj->getString();
		if (!hasField(child, name))
			throw ParseError("CSG: primitive not found");
		Obj* primitive = getField(child, name);
		auto* geometry = dynamic_cast<SceneObject*>(processGeometry(primitive->getName(), primitive->getChild(), scene, materials, transform));
		if (geometry == nullptr)
			throw ParseError("CSG: unknown primitive. Note: mesh is not support in CSG");
		map[name] = geometry;
	}

	if (!hasField(child, "operation"))
		throw ParseError("CSG must have operation");
	
	try
	{
		return parseCSG(getField(child, "operation")->getTuple(), map, scene);
	}
	catch (...)
	{
		for (auto& item : map)
			delete item.second;
		throw;
	}
}

CSG* parseCSG(const mytuple& expression, const std::map<string, SceneObject*>& map, Scene* scene)
{
	if (expression.size() != 3)
		throw ParseError("CSG: invalid expression");

	auto* csg = new CSG(scene);
	SceneObject* left = nullptr, *right = nullptr;
	if (typeid(*expression[0]) == typeid(StringObj))
	{
		string name = expression[0]->getString();
		auto iter = map.find(name);
		if (iter == map.end())
		{
			delete csg;
			throw ParseError("CSG: unknown primitive name in expression");
		}
		left = iter->second;
	}
	else
		left = parseCSG(expression[0]->getTuple(), map, scene);

	if (typeid(*expression[2]) == typeid(StringObj))
	{
		string name = expression[2]->getString();
		auto iter = map.find(name);
		if (iter == map.end())
		{
			delete csg;
			throw ParseError("CSG: unknown primitive name in expression");
		}
		right = iter->second;
	}
	else
		right = parseCSG(expression[2]->getTuple(), map, scene);

	string op = expression[1]->getString();
	if (op == "and")
		csg->op = CSG::Operator::AND;
	else if (op == "or")
		csg->op = CSG::Operator::OR;
	else if (op == "sub")
		csg->op = CSG::Operator::SUB;
	else
	{
		delete csg;
		throw ParseError("CSG: unknown operator");
	}
	
	csg->left = left;
	csg->right = right;
	return csg;
}

void processFluidSystem(Obj* obj, Scene* scene)
{
	double size = getField(obj, "size")->getScalar();
	double kernelRadius = getField(obj, "kernel_radius")->getScalar();
	vec3f center = tupleToVec(getField(obj, "center"));
	int tick = getField(obj, "tick")->getScalar();
	
	FluidSystem fluid(scene, center, size, kernelRadius);
	const auto& blocks = getField(obj, "initial_blocks")->getTuple();
	for (auto* block : blocks)
	{
		vec3f min = tupleToVec(getField(block, "min"));
		vec3f max = tupleToVec(getField(block, "max"));
		double spacing = getField(block, "spacing")->getScalar();
		fluid.addParticles(min, max, spacing);
	}
	fluid.simulate(tick);
}

void processEmitter(Obj* obj, Scene* scene)
{
	auto* initMaterial = processMaterial(getField(obj, "initial_material"));
	auto* endMaterial = initMaterial;
	if (hasField(obj, "end_material"))
		endMaterial = processMaterial(getField(obj, "end_material"));
	Emitter* emitter = new Emitter(scene, initMaterial, endMaterial);

	emitter->emissionRate = getField(obj, "emission_rate")->getScalar();
	emitter->initialSpeed = getField(obj, "initial_speed")->getScalar();
	emitter->gravity = tupleToVec(getField(obj, "force"));
	emitter->lifespan = getField(obj, "lifespan")->getScalar();
	emitter->mass = getField(obj, "mass")->getScalar();
	emitter->maxNumParticle = getField(obj, "max_num_particles")->getScalar();
	emitter->renderingRadius = getField(obj, "particle_radius")->getScalar();
	emitter->source = tupleToVec(getField(obj, "source_position"));
	emitter->drag = getField(obj, "drag")->getScalar();
	emitter->tail = getField(obj, "tail")->getScalar();
	emitter->emissionCut = getField(obj, "emission_cut")->getScalar();

	double timeStep = getField(obj, "time_step")->getScalar();
	int tick = getField(obj, "tick")->getScalar(); 
	emitter->simulate(timeStep, tick);
	scene->emitter = emitter;
}

/*
bool loadHFMap(const string& filename, HFmap& hfmap)
{
	auto* h = readBMP(const_cast<char*>(filename.c_str()), hfmap.width, hfmap.height);
	if (h == nullptr)
		return false;
	hfmap.map = h;
	return true;
}
*/

static Geometry* processGeometry(string name, Obj* child, Scene* scene,
                                 const mmap& materials, TransformNode* transform)
{
	if( name == "translate" ) {
		const mytuple& tup = child->getTuple();
		verifyTuple( tup, 4 );
        return processGeometry( tup[3],
                         scene,
                         materials,
                         transform->createChild(mat4f::translate( vec3f(tup[0]->getScalar(), 
                                                                        tup[1]->getScalar(), 
                                                                        tup[2]->getScalar() ) ) ) );
	} else if( name == "rotate" ) {
		const mytuple& tup = child->getTuple();
		verifyTuple( tup, 5 );
		return processGeometry( tup[4],
		                 scene,
		                 materials,
		                 transform->createChild(mat4f::rotate( vec3f(tup[0]->getScalar(),
		                                                             tup[1]->getScalar(),
		                                                             tup[2]->getScalar() ),
		                                                       tup[3]->getScalar() ) ) );
	} else if( name == "scale" ) {
		const mytuple& tup = child->getTuple();
		if( tup.size() == 2 ) {
			double sc = tup[0]->getScalar();
			return processGeometry( tup[1],
			                 scene,
			                 materials,
			                 transform->createChild(mat4f::scale( vec3f( sc, sc, sc ) ) ) );
		} else {
			verifyTuple( tup, 4 );
			return processGeometry( tup[3],
			                 scene,
			                 materials,
			                 transform->createChild(mat4f::scale( vec3f(tup[0]->getScalar(),
			                                                            tup[1]->getScalar(),
			                                                            tup[2]->getScalar() ) ) ) );
		}
	} else if( name == "transform" ) {
		const mytuple& tup = child->getTuple();
		verifyTuple( tup, 5 );

		const mytuple& l1 = tup[0]->getTuple();
		const mytuple& l2 = tup[1]->getTuple();
		const mytuple& l3 = tup[2]->getTuple();
		const mytuple& l4 = tup[3]->getTuple();
		verifyTuple( l1, 4 );
		verifyTuple( l2, 4 );
		verifyTuple( l3, 4 );
		verifyTuple( l4, 4 );

		return processGeometry( tup[4],
		                 scene,
		                 materials,
		                 transform->createChild(mat4f(vec4f( l1[0]->getScalar(),
		                                                     l1[1]->getScalar(),
		                                                     l1[2]->getScalar(),
		                                                     l1[3]->getScalar() ),
		                                              vec4f( l2[0]->getScalar(),
		                                                     l2[1]->getScalar(),
		                                                     l2[2]->getScalar(),
		                                                     l2[3]->getScalar() ),
		                                              vec4f( l3[0]->getScalar(),
		                                                     l3[1]->getScalar(),
		                                                     l3[2]->getScalar(),
		                                                     l3[3]->getScalar() ),
		                                              vec4f( l4[0]->getScalar(),
		                                                     l4[1]->getScalar(),
		                                                     l4[2]->getScalar(),
		                                                     l4[3]->getScalar() ) ) ) );
	} else if( name == "trimesh" || name == "polymesh" ) { // 'polymesh' is for backwards compatibility
        processTrimesh( name, child, scene, materials, transform);
		return nullptr;
    } else {
		SceneObject *obj = NULL;
       	Material *mat = nullptr;
        
        //if( hasField( child, "material" ) )
		if (name != "skybox" && name != "sphere_light" && name != "csg")
		{
			mat = getMaterial(getField(child, "material"), materials);
		}
        //else
        //    mat = new Material();

		if( name == "sphere" ) {
			obj = new Sphere( scene, mat );
		} else if( name == "box" ) {
			obj = new Box( scene, mat );
		} else if( name == "cylinder" ) {
			bool capped = true;
			maybeExtractField(child, "capped", capped);
			obj = new Cylinder( scene, mat, capped );
		} else if( name == "cone" ) {
			double height = 1.0;
			double bottom_radius = 1.0;
			double top_radius = 0.0;
			bool capped = true;

			maybeExtractField( child, "height", height );
			maybeExtractField( child, "bottom_radius", bottom_radius );
			maybeExtractField( child, "top_radius", top_radius );
			maybeExtractField( child, "capped", capped );

			obj = new Cone( scene, mat, height, bottom_radius, top_radius, capped );
		} else if( name == "square" ) {
			obj = new Square( scene, mat );
		}
    		else if (name == "skybox")
    		{
    			if (!processSkybox(child, scene))
    				throw ParseError("Failed to load skybox.");
    			return nullptr;
    		}
		else if (name == "sphere_light")
		{
			auto* light = new SphereLight(scene, nullptr, tupleToVec(getField(child, "emission")));
			light->setTransform(transform);
			light->calInfo();
			return light;
		}
		else if (name == "moving_sphere")
		{
			double radius = 1.0, time0 = 0.0, time1 = 1.0;
			maybeExtractField(child, "radius", radius);
			maybeExtractField(child, "time0", time0);
			maybeExtractField(child, "time1", time1);
			vec3f start = tupleToVec(getField(child, "start"));
			vec3f end = tupleToVec(getField(child, "end"));
			obj = new movingSphere(scene, mat, start, end, radius, time0, time1);
		}
		else if (name == "csg")
		{
			return processCSG(child, scene, materials, transform);
		}

    		if (hasField(child, "has_tex_coords"))
			obj->setEnableTexCoords(true);

    		if (!processTexture(child, obj))
			throw ParseError("Failed to load texture, please check texture format or path.");

        obj->setTransform(transform);
		return obj;
	}
}

static void processTrimesh( string name, Obj *child, Scene *scene,
                                     const mmap& materials, TransformNode *transform )
{
    Material *mat;
    
    if( hasField( child, "material" ) )
        mat = getMaterial( getField( child, "material" ), materials );
    else
        mat = new Material();
    
    Trimesh *tmesh = new Trimesh( scene, mat, transform);

    const mytuple &points = getField( child, "points" )->getTuple();
    for( mytuple::const_iterator pi = points.begin(); pi != points.end(); ++pi )
        tmesh->addVertex( tupleToVec( *pi ) );
                
    const mytuple &faces = getField( child, "faces" )->getTuple();
    for( mytuple::const_iterator fi = faces.begin(); fi != faces.end(); ++fi )
    {
        const mytuple &pointids = (*fi)->getTuple();

        // triangulate here and now.  assume the poly is
        // concave and we can triangulate using an arbitrary fan
        if( pointids.size() < 3 )
            throw ParseError( "Faces must have at least 3 vertices." );

        mytuple::const_iterator i = pointids.begin();
        int a = (int) (*i++)->getScalar();
        int b = (int) (*i++)->getScalar();
        while( i != pointids.end() )
        {
            int c = (int) (*i++)->getScalar();
            if( !tmesh->addFace(a,b,c) )
                throw ParseError( "Bad face in trimesh." );
            b = c;
        }
    }

    bool generateNormals = false;
    maybeExtractField( child, "gennormals", generateNormals );
    if( generateNormals )
        tmesh->generateNormals();
            
    if( hasField( child, "materials" ) )
    {
        const mytuple &mats = getField( child, "materials" )->getTuple();
        for( mytuple::const_iterator mi = mats.begin(); mi != mats.end(); ++mi )
            tmesh->addMaterial( getMaterial( *mi, materials ) );
    }
	
    if( hasField( child, "normals" ) )
    {
        const mytuple &norms = getField( child, "normals" )->getTuple();
        for( mytuple::const_iterator ni = norms.begin(); ni != norms.end(); ++ni )
            tmesh->addNormal( tupleToVec( *ni ) );
    }

	if (hasField(child, "tex_coords"))
	{
		tmesh->setEnableTexCoords(true);
		const mytuple& coords = getField(child, "tex_coords")->getTuple();
		for(auto iter = coords.begin(); iter != coords.end(); ++iter)
			tmesh->addTexCoords(tupleToTexCoords(*iter));
	}

	if (hasField(child, "emission"))
	{
		tmesh->setEmission(tupleToVec(getField(child, "emission")));
	}

	if (!processTexture(child, tmesh))
		throw ParseError("Failed to load texture, please check texture format or path.");

    char *error;
    if( error = tmesh->doubleCheck() )
        throw ParseError( error );

    scene->add(tmesh);
}

static bool processHField(Scene* scene, TransformNode*transform) {
	Material* mat = new Material();
	Trimesh* tmesh = new Trimesh(scene, mat, transform);
	HFmap* thf = scene->hfmap;
/*
	const mytuple& points = getField(child, "points")->getTuple();
	for (mytuple::const_iterator pi = points.begin(); pi != points.end(); ++pi)
		tmesh->addVertex(tupleToVec(*pi));
*/
	for (double i = 0; i < thf->width; i++) {
		for (double j = 0; j < thf->height; j++) {
			double y = thf->getH(i, j) * thf->width / 255.0 / 100.0 - thf->width / 200.0 - 2;
			tmesh->addVertex(vec3f((i- thf->width / 2) / 100.0, y, (j - thf->height / 2) / 100.0 - 2));
			auto diffuse = thf->getC(i, j)/255.0;
			Material* tempmat = new Material();
			tempmat->kd = diffuse;
		//empmat->ks = vec3f(0.5, 0.5, 0.5);
		//empmat->shininess = 0.2;
			tmesh->addMaterial(tempmat);
		}
	}

	for (double i = 0; i < thf->width-1; i++) {
		for (double j = 0; j < thf->height-1; j++) {
			tmesh->addFace(j * thf->width + i,j* thf->width + i + 1, (j+1) * thf->width + i + 1);
			tmesh->addFace(j * thf->width + i, (j + 1) * thf->width + i+1, (j + 1) * thf->width + i );
		}
	}

	tmesh->generateNormals();

	scene->add(tmesh);
	return true;

	/*
	const mytuple& faces = getField(child, "faces")->getTuple();
	for (mytuple::const_iterator fi = faces.begin(); fi != faces.end(); ++fi)
	{
		const mytuple& pointids = (*fi)->getTuple();

		// triangulate here and now.  assume the poly is
		// concave and we can triangulate using an arbitrary fan
		if (pointids.size() < 3)
			throw ParseError("Faces must have at least 3 vertices.");

		mytuple::const_iterator i = pointids.begin();
		int a = (int)(*i++)->getScalar();
		int b = (int)(*i++)->getScalar();
		while (i != pointids.end())
		{
			int c = (int)(*i++)->getScalar();
			if (!tmesh->addFace(a, b, c))
				throw ParseError("Bad face in trimesh.");
			b = c;
		}
	}

	bool generateNormals = false;
	maybeExtractField(child, "gennormals", generateNormals);
	if (generateNormals)
		tmesh->generateNormals();

	if (hasField(child, "materials"))
	{
		const mytuple& mats = getField(child, "materials")->getTuple();
		for (mytuple::const_iterator mi = mats.begin(); mi != mats.end(); ++mi)
			tmesh->addMaterial(getMaterial(*mi, materials));
	}

	if (hasField(child, "normals"))
	{
		const mytuple& norms = getField(child, "normals")->getTuple();
		for (mytuple::const_iterator ni = norms.begin(); ni != norms.end(); ++ni)
			tmesh->addNormal(tupleToVec(*ni));
	}

	if (hasField(child, "tex_coords"))
	{
		tmesh->setEnableTexCoords(true);
		const mytuple& coords = getField(child, "tex_coords")->getTuple();
		for (auto iter = coords.begin(); iter != coords.end(); ++iter)
			tmesh->addTexCoords(tupleToTexCoords(*iter));
	}

	if (hasField(child, "emission"))
	{
		tmesh->setEmission(tupleToVec(getField(child, "emission")));
	}

	if (!processTexture(child, tmesh))
		throw ParseError("Failed to load texture, please check texture format or path.");

	char* error;
	if (error = tmesh->doubleCheck())
		throw ParseError(error);

	scene->add(tmesh);
	*/
}

static Material *getMaterial( Obj *child, const mmap& bindings )
{
	string tfield = child->getTypeName();
	if( tfield == "id" ) {
		mmap::const_iterator i = bindings.find( child->getID() );
		if( i != bindings.end() ) {
			return (*i).second;
		} 
	} else if( tfield == "string" ) {
		mmap::const_iterator i = bindings.find( child->getString() );
		if( i != bindings.end() ) {
			return (*i).second;
		} 
	} 
	// Don't allow binding.
	return processMaterial( child );
}

static void processBindings(Obj* child, mmap* bindings, Material* mat)
{
	if( bindings != NULL ) {
		// Want to bind, better have "name" field:
		if( hasField( child, "name" ) ) {
			Obj *field = getField( child, "name" );
			string tfield = field->getTypeName();
			string name;
			if( tfield == "id" ) {
				name = field->getID();
			} else {
				name = field->getString();
			}

			(*bindings)[ name ] = mat;
		} else {
			throw ParseError( 
				string( "Attempt to bind material with no name" ) );
		}
	}
}

static Material *processMaterial( Obj *child, mmap *bindings )
// Generate a material from a parse sub-tree
//
// child   - root of parse tree
// mmap    - bindings of names to materials (if non-null)
// defmat  - material to start with (if non-null)
{
	if (hasField(child, "type"))
	{
		string type = getField(child, "type")->getString();
		if (type == "microfacet")
			return processMicrofacet(child, bindings);
		if (type == "fresnel")
			return processFresnelSpecular(child, bindings);
	}
	
    Material *mat;
    mat = new Material();
	
    if( hasField( child, "emissive" ) ) {
        mat->ke = tupleToVec( getField( child, "emissive" ) );
    }
    if( hasField( child, "ambient" ) ) {
        mat->ka = tupleToVec( getField( child, "ambient" ) );
    }
    if( hasField( child, "specular" ) ) {
        mat->ks = tupleToVec( getField( child, "specular" ) );
    }
    if( hasField( child, "diffuse" ) ) {
        mat->kd = tupleToVec( getField( child, "diffuse" ) );
    }
    if( hasField( child, "reflective" ) ) {
        mat->kr = tupleToVec( getField( child, "reflective" ) );
    } else {
        mat->kr = mat->ks; // defaults to ks if none given.
    }
    if( hasField( child, "transmissive" ) ) {
        mat->kt = tupleToVec( getField( child, "transmissive" ) );
    }
    if( hasField( child, "index" ) ) { // index of refraction
        mat->index = getField( child, "index" )->getScalar();
    }
    if( hasField( child, "shininess" ) ) {
        mat->shininess = getField( child, "shininess" )->getScalar();
    }
	if (hasField(child, "absorb"))
	{
		mat->absorb = tupleToVec(getField(child, "absorb"));
	}
	if (hasField(child, "glossiness"))
	{
		mat->glossiness = getField(child, "glossiness")->getScalar();
	}

    processBindings(child, bindings, mat);

    return mat;
}

static void
processCamera( Obj *child, Scene *scene )
{
    if( hasField( child, "position" ) )
        scene->getCamera()->setEye( tupleToVec( getField( child, "position" ) ) );
    if( hasField( child, "quaternion" ) )
    {
        const mytuple &quat = getField( child, "quaternion" )->getTuple();
        if( quat.size() != 4 )
            throw( ParseError( "Bogus quaternion." ) );
        else
            scene->getCamera()->setLook( quat[0]->getScalar(),
                                         quat[1]->getScalar(),
                                         quat[2]->getScalar(),
                                         quat[3]->getScalar() );
    }
    if( hasField( child, "fov" ) )
        scene->getCamera()->setFOV( getField( child, "fov" )->getScalar() );
    if( hasField( child, "aspectratio" ) )
        scene->getCamera()->setAspectRatio( getField( child, "aspectratio" )->getScalar() );
    if( hasField( child, "viewdir" ) && hasField( child, "updir" ) )
    {
        scene->getCamera()->setLook( tupleToVec( getField( child, "viewdir" ) ).normalize(),
                                     tupleToVec( getField( child, "updir" ) ).normalize() );
    }
	if (hasField(child, "enable_motion_blur") && getField(child, "enable_motion_blur")->getBoolean())
	{
		double time0 = 0.0, time1 = 1.0;
		maybeExtractField(child, "time0", time0);
		maybeExtractField(child, "time1", time1);
		auto* camera = scene->getCamera();
		camera->enableMotionBlur = true;
		camera->time0 = time0;
		camera->time1 = time1;
	}
}

static void processObject( Obj *obj, Scene *scene, mmap& materials )
{
	// Assume the object is named.
	string name;
	Obj *child; 

	if( obj->getTypeName() == "id" ) {
		name = obj->getID();
		child = NULL;
	} else if( obj->getTypeName() == "named" ) {
		name = obj->getName();
		child = obj->getChild();
	} else {
		ostrstream oss;
		oss << "Unknown input object ";
		obj->printOn( oss );

		throw ParseError( string( oss.str() ) );
	}

	if( name == "directional_light" ) {
		if( child == NULL ) {
			throw ParseError( "No info for directional_light" );
		}

		scene->add( new DirectionalLight( scene, 
			tupleToVec( getField( child, "direction" ) ).normalize(),
			tupleToVec( getColorField( child ) ) ) );
	} else if( name == "point_light" ) {
		if( child == NULL ) {
			throw ParseError( "No info for point_light" );
		}

		auto* pointLight = new PointLight(scene, tupleToVec(getField(child, "position")), 
											tupleToVec(getColorField(child)));
		
		double c0{LIGHT_EPSILON}, c1{0.0}, c2{1.0};
		maybeExtractField(child, "constant_attenuation_coeff", c0);
		maybeExtractField(child, "linear_attenuation_coeff", c1);
		maybeExtractField(child, "quadratic_attenuation_coeff", c2);
		pointLight->setAttenuationCoeff(c0, c1, c2);
		
		scene->add( pointLight );
	}
	else if (name == "ambient_light")
	{
		if (child == nullptr)
			throw ParseError("No info for ambient_light");

		scene->add(new AmbientLight(scene, tupleToVec(getField(child, "color"))));
	}
	else if (name == "spot_light")
	{
		if (child == nullptr)
			throw ParseError("No info for spot_light");

		double cutoffDist = 50.0, penumbra = 30.0, coneAngle = 60.0;
		maybeExtractField(child, "cutoff_distance", cutoffDist);
		maybeExtractField(child, "penumbra", penumbra);
		maybeExtractField(child, "cone_angle", coneAngle);
		penumbra *= PI / 180.0;
		coneAngle *= PI / 180.0;

		auto* spotLight = new SpotLight(scene, tupleToVec(getField(child, "color")), 
			cutoffDist, penumbra, coneAngle);
		spotLight->setPos(tupleToVec(getField(child, "position")));
		spotLight->setTarget(tupleToVec(getField(child, "target")));
		scene->add(spotLight);
	}
	else if (name == "area_light")
	{
		if (child == nullptr)
			throw ParseError("No info for area_light");
		vec3f color = tupleToVec(getField(child, "color"));
		vec3f pos = tupleToVec(getField(child, "position"));
		vec3f u = tupleToVec(getField(child, "u"));
		vec3f v = tupleToVec(getField(child, "v"));
		auto* areaLight = new AreaLight(scene, color, pos, u, v);
		scene->add(areaLight);
	}
	else if (name == "fluid")
	{
		if (child == nullptr)
			throw ParseError("No info for fluid");
		processFluidSystem(child, scene);
	}
	else if (name == "emitter")
	{
		if (child == nullptr)
			throw ParseError("No info for emitter");
		processEmitter(child, scene);
	}
	else if( 	name == "sphere" ||
				name == "box" ||
				name == "cylinder" ||
				name == "cone" ||
				name == "square" ||
				name == "translate" ||
				name == "rotate" ||
				name == "scale" ||
				name == "transform" ||
                name == "trimesh" ||
                name == "polymesh" ||
				name == "moving_sphere" ||
				name == "sphere_light" ||
				name == "csg" ||
				name == "skybox") { // polymesh is for backwards compatibility.
		auto* geometry = processGeometry( name, child, scene, materials, &scene->transformRoot);
		if (geometry)
			scene->add(geometry);
		//scene->add( geo );
	} else if( name == "material" ) {
		processMaterial( child, &materials );
	}
	else if( name == "camera" ) {
		processCamera( child, scene );
	} 
	else {
		throw ParseError(string("Unrecognized object: ") + name);
	}
}
