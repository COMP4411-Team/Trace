//
// TraceUI.h
//
// Handles FLTK integration and other user interface tasks
//
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <omp.h>

#include <FL/fl_ask.h>

#include "TraceUI.h"
#include "../RayTracer.h"

static bool done;

//------------------------------------- Help Functions --------------------------------------------
TraceUI* TraceUI::whoami(Fl_Menu_* o)	// from menu item back to UI itself
{
	return ( (TraceUI*)(o->parent()->user_data()) );
}

TraceUI* TraceUI::whoami(Fl_Widget* o)
{
	return ( (TraceUI*)(o->user_data()) );
}

Fl_Value_Slider* TraceUI::createSlider(int x, int y, int w, int h, const char* l,
                                       double minValue, double maxValue, double step, double value,
                                       void (*callback)(Fl_Widget*, void*))
{
	auto* slider = new Fl_Value_Slider(x, y, w, h, l);
	slider->user_data(this);
	slider->type(FL_HOR_NICE_SLIDER);
	slider->labelfont(FL_COURIER);
	slider->labelsize(12);
	slider->minimum(minValue);
	slider->maximum(maxValue);
	slider->step(step);
	slider->value(value);
	slider->align(FL_ALIGN_RIGHT);
	slider->callback(callback);
	return slider;
}

//--------------------------------- Callback Functions --------------------------------------------
void TraceUI::cb_load_scene(Fl_Menu_* o, void* v) 
{
	TraceUI* pUI=whoami(o);
	
	char* newfile = fl_file_chooser("Open Scene?", "*.ray", NULL );

	if (newfile != NULL) {
		char buf[256];

		if (pUI->raytracer->loadScene(newfile)) {
			sprintf(buf, "Ray <%s>", newfile);
			done=true;	// terminate the previous rendering
		} else{
			sprintf(buf, "Ray <Not Loaded>");
		}

		pUI->m_mainWindow->label(buf);
	}
}

void TraceUI::cb_load_hf(Fl_Menu_* o, void* v)
{
	TraceUI* pUI = whoami(o);

	char* newfile = fl_file_chooser("Open HFmap?", "*.bmp", NULL);

	if (newfile != NULL) {
		char buf[256];

		if (pUI->raytracer->loadHFmap(newfile)) {
			sprintf(buf, "HFmap <%s>", newfile);
			done = true;	// terminate the previous rendering
		}
		else {
			sprintf(buf, "HFmap <Not Loaded>");
		}

		pUI->m_mainWindow->label(buf);
	}
}

void TraceUI::cb_save_image(Fl_Menu_* o, void* v) 
{
	TraceUI* pUI=whoami(o);
	
	char* savefile = fl_file_chooser("Save Image?", "*.bmp", "save.bmp" );
	if (savefile != NULL) {
		pUI->m_traceGlWindow->saveImage(savefile);
	}
}

void TraceUI::cb_exit(Fl_Menu_* o, void* v)
{
	TraceUI* pUI=whoami(o);

	// terminate the rendering
	done=true;

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
}

void TraceUI::cb_exit2(Fl_Widget* o, void* v) 
{
	TraceUI* pUI=(TraceUI *)(o->user_data());
	
	// terminate the rendering
	done=true;

	pUI->m_traceGlWindow->hide();
	pUI->m_mainWindow->hide();
}

void TraceUI::cb_about(Fl_Menu_* o, void* v) 
{
	fl_message("RayTracer Project, FLTK version for CS 341 Spring 2002. Latest modifications by Jeff Maurer, jmaurer@cs.washington.edu");
}

void TraceUI::cb_sizeSlides(Fl_Widget* o, void* v)
{
	TraceUI* pUI=(TraceUI*)(o->user_data());
	
	pUI->m_nSize=int( ((Fl_Slider *)o)->value() ) ;
	int	height = (int)(pUI->m_nSize / pUI->raytracer->aspectRatio() + 0.5);
	pUI->m_traceGlWindow->resizeWindow( pUI->m_nSize, height );
}

void TraceUI::cb_depthSlides(Fl_Widget* o, void* v)
{
	((TraceUI*)(o->user_data()))->m_nDepth=int( ((Fl_Slider *)o)->value() ) ;
}

void TraceUI::cb_lightScaleSlides(Fl_Widget* o, void* v)
{
	auto* ui = ((TraceUI*)(o->user_data()));
	ui->lightScale = double( ((Fl_Slider *)o)->value() );
	ui->raytracer->setLightScale(ui->lightScale);
}

void TraceUI::cb_threshSlides(Fl_Widget* o, void* v)
{
	double value = double( ((Fl_Slider *)o)->value() );
	((TraceUI*)(o->user_data()))->threshold = vec3f(value, value, value);
}

void TraceUI::cb_ssaaLevelSlides(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->ssaaSample = int( ((Fl_Slider *)o)->value() );
}

void TraceUI::cb_ssaaJitterButton(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->ssaaJitter = bool( ((Fl_Light_Button*)o)->value() );
}

void TraceUI::cb_pathTracingButton(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->enablePathTracing = bool( ((Fl_Light_Button*)o)->value() );
	ui->raytracer->enablePathTracing = bool( ((Fl_Light_Button*)o)->value() );
}

void TraceUI::cb_fasterShadow(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->enableFasterShadow = bool(((Fl_Light_Button*)o)->value());
	ui->raytracer->setFasterShadow(bool(((Fl_Light_Button*)o)->value()));
}

void TraceUI::cb_enableDof(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->enableDof(bool( ((Fl_Light_Button*)o)->value() ));
}

void TraceUI::cb_aperture(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->setAperture(double( ((Fl_Slider*)o)->value() ));
}

void TraceUI::cb_focalLength(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->setFocalLength(double( ((Fl_Slider*)o)->value() ));
}

void TraceUI::cb_enableMotionBlur(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->enableMotionBlur = (bool( ((Fl_Light_Button*)o)->value() ));
}

void TraceUI::cb_motionBlurSPP(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->motionBlurSPP = (int( ((Fl_Slider*)o)->value() ));
}

void TraceUI::cb_distributed(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->enableDistributed = (bool( ((Fl_Light_Button*)o)->value() ));
	ui->raytracer->scene->enableDistributed = (bool( ((Fl_Light_Button*)o)->value() ));
}

void TraceUI::cb_childRay(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->numChildRay = (int( ((Fl_Slider*)o)->value() ));
	ui->raytracer->scene->numChildRay = (int( ((Fl_Slider*)o)->value() ));
}

void TraceUI::cb_multiThread(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->enableMultiThread = (bool( ((Fl_Light_Button*)o)->value() ));
}

void TraceUI::cb_adaptiveSS(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->enableAdaptiveSS = (bool( ((Fl_Light_Button*)o)->value() ));
}

void TraceUI::cb_visualizeSS(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->visualizeAdaptiveSS = (bool( ((Fl_Light_Button*)o)->value() ));
}

void TraceUI::cb_adaptiveThresh(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->adaptiveThresh = (double( ((Fl_Slider*)o)->value() ));
}

void TraceUI::cb_buildPM(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->buildPhotonMap();
	fl_alert("Photon Map Construction Done.");
}

void TraceUI::cb_enablePM(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->enablePM = bool( ((Fl_Light_Button*)o)->value() );
}

void TraceUI::cb_photonNum(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->numPhotons = 1000 * (int( ((Fl_Slider*)o)->value() ));
}

void TraceUI::cb_neighbourNum(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->numNeighbours = (int( ((Fl_Slider*)o)->value() ));
}

void TraceUI::cb_flux(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->totalFlux = vec3f((double( ((Fl_Slider*)o)->value() )));
}

void TraceUI::cb_render(Fl_Widget* o, void* v)
{
	TraceUI* pUI=((TraceUI*)(o->user_data()));
	
	if (pUI->enableMultiThread && !pUI->enableAdaptiveSS)
		return cb_renderParallel(o, v);
	
	char buffer[256];
	
	if (pUI->raytracer->sceneLoaded()) {
		int width=pUI->getSize();
		int	height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
		pUI->m_traceGlWindow->resizeWindow( width, height );

		pUI->m_traceGlWindow->show();

		pUI->raytracer->traceSetup(width, height, pUI->getDepth(), pUI->threshold);

		if (pUI->debug)
		{
			pUI->raytracer->renderHFMap();
			pUI->m_traceGlWindow->refresh();
			return;
		}

		if (pUI->enableAdaptiveSS)
		{
			pUI->raytracer->adaptiveTrace();
			pUI->m_traceGlWindow->refresh();
			return;
		}
		if (pUI->visualizeAdaptiveSS)
		{
			pUI->raytracer->visualizeSamples();
			pUI->m_traceGlWindow->refresh();
			return;
		}
		
		// Save the window label
		const char *old_label = pUI->m_traceGlWindow->label();

		// start to render here	
		done=false;
		clock_t prev, now;
		prev=clock();
		
		pUI->m_traceGlWindow->refresh();
		Fl::check();
		Fl::flush();

		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				if (done) break;
				
				// current time
				now = clock();

				// check event every 1/2 second
				if (((double)(now-prev)/CLOCKS_PER_SEC)>0.5) {
					prev=now;

					if (Fl::ready()) {
						// refresh
						pUI->m_traceGlWindow->refresh();
						// check event
						Fl::check();

						if (Fl::damage()) {
							Fl::flush();
						}
					}
				}

				pUI->raytracer->tracePixel( x, y, 1 );
		
			}
			if (done) break;

			// flush when finish a row
			if (Fl::ready()) {
				// refresh
				pUI->m_traceGlWindow->refresh();

				if (Fl::damage()) {
					Fl::flush();
				}
			}
			// update the window label
			sprintf(buffer, "(%d%%) %s", (int)((double)y / (double)height * 100.0), old_label);
			pUI->m_traceGlWindow->label(buffer);
			
		}
		done=true;
		pUI->m_traceGlWindow->refresh();

		// Restore the window label
		pUI->m_traceGlWindow->label(old_label);		
	}
}

void TraceUI::cb_renderParallel(Fl_Widget* o, void* v)
{
	char buffer[256];

	TraceUI* pUI=((TraceUI*)(o->user_data()));
	
	if (!pUI->raytracer->sceneLoaded())
		return;

	int width = pUI->getSize();
	int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
	pUI->m_traceGlWindow->resizeWindow(width, height);
	pUI->m_traceGlWindow->show();
	pUI->raytracer->traceSetup(width, height, pUI->getDepth(), pUI->threshold);

	// Save the window label
	const char* old_label = pUI->m_traceGlWindow->label();

	// start to render here	
	done = false;
	clock_t prev, now;
	prev = clock();

	pUI->m_traceGlWindow->refresh();
	Fl::check();
	Fl::flush();

	for (int y = 0; y < height; y++)
	{
		#pragma omp parallel for num_threads(4)
		for (int x = 0; x < width; x++)
		{
			pUI->raytracer->tracePixel(x, y, 1);
		}
		if (done) break;

		// flush when finish a row
		if (Fl::ready())
		{
			// refresh
			pUI->m_traceGlWindow->refresh();
			Fl::check();

			if (Fl::damage())
			{
				Fl::flush();
			}
		}
		// update the window label
		sprintf(buffer, "(%d%%) %s", (int)((double)y / (double)height * 100.0), old_label);
		pUI->m_traceGlWindow->label(buffer);
	}
	done = true;
	pUI->m_traceGlWindow->refresh();

	// Restore the window label
	pUI->m_traceGlWindow->label(old_label);		
}

void TraceUI::cb_renderPt(Fl_Widget* o, void* v)
{
	char buffer[256];
	auto* pUI = ((TraceUI*)(o->user_data()));
	if (!pUI->raytracer->sceneLoaded())
		return;
	int width = pUI->getSize();
	int height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
	pUI->m_traceGlWindow->resizeWindow(width, height);
	pUI->m_traceGlWindow->show();
	pUI->raytracer->traceSetup(width, height, pUI->getDepth(), pUI->threshold);

	// Save the window label
	const char* old_label = pUI->m_traceGlWindow->label();

	// start to render here	
	done = false;
	clock_t prev, now;
	prev = clock();

	pUI->m_traceGlWindow->refresh();
	Fl::check();
	Fl::flush();

	for (int i = 1; i <= pUI->maxIter; ++i)
	{
		pUI->raytracer->pathTrace(i);
		Fl::check();
		if (done) break;
		pUI->m_traceGlWindow->refresh();
		sprintf(buffer, "Iter %d %s", i, old_label);
		pUI->m_traceGlWindow->label(buffer);
		pUI->raytracer->swapBuffer();
	}

	done = true;
	pUI->m_traceGlWindow->refresh();
	pUI->m_traceGlWindow->label(old_label);
}

void TraceUI::cb_stop(Fl_Widget* o, void* v)
{
	done=true;
}

void TraceUI::show()
{
	m_mainWindow->show();
}

void TraceUI::setRayTracer(RayTracer *tracer)
{
	raytracer = tracer;
	m_traceGlWindow->setRayTracer(tracer);
}

int TraceUI::getSize()
{
	return m_nSize;
}

int TraceUI::getDepth()
{
	return m_nDepth;
}

// menu definition
Fl_Menu_Item TraceUI::menuitems[] = {
	{ "&File",		0, 0, 0, FL_SUBMENU },
		{ "&Load Scene...",	FL_ALT + 'l', (Fl_Callback *)TraceUI::cb_load_scene },
		{ "&Load HFmap...",	FL_ALT + 'h', (Fl_Callback*)TraceUI::cb_load_hf },
		{ "&Save Image...",	FL_ALT + 's', (Fl_Callback *)TraceUI::cb_save_image },
		{ "&Exit",			FL_ALT + 'e', (Fl_Callback *)TraceUI::cb_exit },
		{ 0 },

	{ "&Help",		0, 0, 0, FL_SUBMENU },
		{ "&About",	FL_ALT + 'a', (Fl_Callback *)TraceUI::cb_about },
		{ 0 },

	{ 0 }
};

TraceUI::TraceUI() {
	// init.
	m_nDepth = 0;
	m_nSize = 150;
	m_mainWindow = new Fl_Window(100, 40, 330, 540, "Ray <Not Loaded>");
		m_mainWindow->user_data((void*)(this));	// record self to be used by static callback functions
		// install menu bar
		m_menubar = new Fl_Menu_Bar(0, 0, 330, 25);
		m_menubar->menu(menuitems);

		// install slider depth
		m_depthSlider = new Fl_Value_Slider(10, 30, 180, 20, "Depth");
		m_depthSlider->user_data((void*)(this));	// record self to be used by static callback functions
		m_depthSlider->type(FL_HOR_NICE_SLIDER);
        m_depthSlider->labelfont(FL_COURIER);
        m_depthSlider->labelsize(12);
		m_depthSlider->minimum(0);
		m_depthSlider->maximum(10);
		m_depthSlider->step(1);
		m_depthSlider->value(m_nDepth);
		m_depthSlider->align(FL_ALIGN_RIGHT);
		m_depthSlider->callback(cb_depthSlides);

		// install slider size
		m_sizeSlider = new Fl_Value_Slider(10, 55, 180, 20, "Size");
		m_sizeSlider->user_data((void*)(this));	// record self to be used by static callback functions
		m_sizeSlider->type(FL_HOR_NICE_SLIDER);
        m_sizeSlider->labelfont(FL_COURIER);
        m_sizeSlider->labelsize(12);
		m_sizeSlider->minimum(64);
		m_sizeSlider->maximum(1024);
		m_sizeSlider->step(1);
		m_sizeSlider->value(m_nSize);
		m_sizeSlider->align(FL_ALIGN_RIGHT);
		m_sizeSlider->callback(cb_sizeSlides);

		m_lightScaleSlider = createSlider(10, 80, 180, 20, "Light Scale",
			1, 50, 1, lightScale, cb_lightScaleSlides);

		m_threshSlider = createSlider(10, 105, 180, 20, "Threshold",
			0, 1, 0.01, threshold[0], cb_threshSlides);

		m_ssaaLevelSlider = createSlider(10, 130, 180, 20, "SSAA Sample Level",
			0, 5, 1, ssaaSampleLevel, cb_ssaaLevelSlides);

		m_ssaaJitterButton = new Fl_Light_Button(10, 155, 150, 25, "SSAA Sample Jitter");
		m_ssaaJitterButton->user_data(this);
		m_ssaaJitterButton->value(0);
		m_ssaaJitterButton->callback(cb_ssaaJitterButton);

		// DOF
		m_enableDofButton = new Fl_Light_Button(170, 155, 150, 25, "Depth of Field");
		m_enableDofButton->user_data(this);
		m_enableDofButton->value(0);
		m_enableDofButton->callback(cb_enableDof);

		m_apertureSlider = createSlider(10, 185, 180, 20, "Aperture",
			0, 2, 0.01, 0, cb_aperture);

		m_focalLengthSlider = createSlider(10, 210, 180, 20, "Focal Length",
			0, 20, 0.01, 5, cb_focalLength);

		// Motion blur
		m_motionBlurButton = new Fl_Light_Button(10, 260, 150, 25, "Motion Blur");
		m_motionBlurButton->user_data(this);
		m_motionBlurButton->value(0);
		m_motionBlurButton->callback(cb_enableMotionBlur);

		m_motionBlurSPPSlider = createSlider(10, 235, 180, 20, "Motion Blur SPP",
			1, 1000, 1, 100, cb_motionBlurSPP);

		m_fasterShadow = new Fl_Light_Button(150, 370, 170, 25, "Shadow Acceleration");
		m_fasterShadow->user_data(this);
		m_fasterShadow->value(0);
		m_fasterShadow->callback(cb_fasterShadow);

		// Distributed RT
		m_distributedButton = new Fl_Light_Button(170, 260, 150, 25, "Distributed RT");
		m_distributedButton->user_data(this);
		m_distributedButton->value(0);
		m_distributedButton->callback(cb_distributed);

		m_childRaySlider = createSlider(10, 290, 180, 20, "Child Ray Num",
			1, 50, 1, 10, cb_childRay);

		m_multiThreadButton = new Fl_Light_Button(10, 370, 130, 25, "Multi Thread");
		m_multiThreadButton->user_data(this);
		m_multiThreadButton->value(0);
		m_multiThreadButton->callback(cb_multiThread);

		// Adaptive supersampling
		m_adaptiveSSButton = new Fl_Light_Button(10, 315, 130, 25, "Adaptive SS");
		m_adaptiveSSButton->user_data(this);
		m_adaptiveSSButton->value(0);
		m_adaptiveSSButton->callback(cb_adaptiveSS);

		m_adaptiveVisButton = new Fl_Light_Button(150, 315, 170, 25, "Visualize Adaptive SS");
		m_adaptiveVisButton->user_data(this);
		m_adaptiveVisButton->value(0);
		m_adaptiveVisButton->callback(cb_visualizeSS);

		m_adaptiveThreshSlider = createSlider(10, 345, 180, 20, "Adaptive SS Thresh",
			0.0, 1.0, 0.01, 0.25, cb_adaptiveThresh);

		// Photon mapping
		m_buildPMButton = new Fl_Button(10, 400, 120, 25, "Build Photon Map");
		m_buildPMButton->user_data(this);
		m_buildPMButton->callback(cb_buildPM);

		m_enablePMButton = new Fl_Light_Button(140, 400, 180, 25, "Enable Photon Mapping");
		m_enablePMButton->user_data(this);
		m_enablePMButton->value(0);
		m_enablePMButton->callback(cb_enablePM);

		m_numPhotonSlider = createSlider(10, 430, 180, 20, "Photon Num(k)", 10, 100, 1, 50, cb_photonNum);

		m_numNeighbourSlider = createSlider(10, 455, 180, 20, "Neighbour Num", 10, 100, 1, 20, cb_neighbourNum);

		m_fluxSlider = createSlider(10, 480, 180, 20, "Total Flux", 0.001, 0.1, 0.001, 0.03, cb_flux);

		// Path tracing
		m_pathTracingButton = new Fl_Light_Button(10, 510, 150, 25, "Enable Path Tracing");
		m_pathTracingButton->user_data(this);
		m_pathTracingButton->value(0);
		m_pathTracingButton->callback(cb_pathTracingButton);
	
		m_renderButton = new Fl_Button(220, 510, 100, 25, "Render PT");
		m_renderButton->user_data((void*)(this));
		m_renderButton->callback(cb_renderPt);

		m_renderButton = new Fl_Button(240, 27, 80, 25, "&Render");
		m_renderButton->user_data((void*)(this));
		m_renderButton->callback(cb_render);

		m_stopButton = new Fl_Button(240, 55, 80, 25, "&Stop");
		m_stopButton->user_data((void*)(this));
		m_stopButton->callback(cb_stop);

		m_mainWindow->callback(cb_exit2);
		m_mainWindow->when(FL_HIDE);
    m_mainWindow->end();

	// image view
	m_traceGlWindow = new TraceGLWindow(100, 150, m_nSize, m_nSize, "Rendered Image");
	m_traceGlWindow->end();
	m_traceGlWindow->resizable(m_traceGlWindow);
}