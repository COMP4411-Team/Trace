//
// TraceUI.h
//
// Handles FLTK integration and other user interface tasks
//
#include <stdio.h>
#include <time.h>
#include <string.h>

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

void TraceUI::cb_pbrButton(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->raytracer->enablePBR = bool( ((Fl_Light_Button*)o)->value() );
}

void TraceUI::cb_pathTracingButton(Fl_Widget* o, void* v)
{
	auto* ui = whoami(o);
	ui->enablePathTracing = bool( ((Fl_Light_Button*)o)->value() );
	ui->raytracer->enablePathTracing = bool( ((Fl_Light_Button*)o)->value() );
}

void TraceUI::cb_render(Fl_Widget* o, void* v)
{
	char buffer[256];

	TraceUI* pUI=((TraceUI*)(o->user_data()));
	
	if (pUI->raytracer->sceneLoaded()) {
		int width=pUI->getSize();
		int	height = (int)(width / pUI->raytracer->aspectRatio() + 0.5);
		pUI->m_traceGlWindow->resizeWindow( width, height );

		pUI->m_traceGlWindow->show();

		pUI->raytracer->traceSetup(width, height, pUI->getDepth(), pUI->threshold);
		
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
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				if (done) break;
				now = clock();
				if (((double)(now - prev) / CLOCKS_PER_SEC) > 0.5)
				{
					prev = now;
					if (Fl::ready())
					{
						// refresh
						pUI->m_traceGlWindow->refresh();
						// check event
						Fl::check();

						if (Fl::damage())
							Fl::flush();
					}
				}
				pUI->raytracer->tracePixel(x, y, i);
			}
			if (done) break;

			if (Fl::ready())
			{
				// refresh
				pUI->m_traceGlWindow->refresh();
				if (Fl::damage())
					Fl::flush();
			}
		}
		// update the window label
		sprintf(buffer, "Iter %d %s", i, old_label);
		pUI->m_traceGlWindow->label(buffer);

		pUI->raytracer->swapBuffer();
	}

	done = true;
	pUI->m_traceGlWindow->refresh();
	// Restore the window label
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
	m_mainWindow = new Fl_Window(100, 40, 320, 400, "Ray <Not Loaded>");
		m_mainWindow->user_data((void*)(this));	// record self to be used by static callback functions
		// install menu bar
		m_menubar = new Fl_Menu_Bar(0, 0, 320, 25);
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

		m_pbrButton = new Fl_Light_Button(10, 185, 150, 25, "Enable PBR");
		m_pbrButton->user_data(this);
		m_pbrButton->value(0);
		m_pbrButton->callback(cb_pbrButton);

		m_pathTracingButton = new Fl_Light_Button(10, 215, 150, 25, "Enable Path Tracing");
		m_pathTracingButton->user_data(this);
		m_pathTracingButton->value(0);
		m_pathTracingButton->callback(cb_pathTracingButton);

		m_renderButton = new Fl_Button(220, 215, 90, 25, "Render PT");
		m_renderButton->user_data((void*)(this));
		m_renderButton->callback(cb_renderPt);

		m_renderButton = new Fl_Button(240, 27, 70, 25, "&Render");
		m_renderButton->user_data((void*)(this));
		m_renderButton->callback(cb_render);

		m_stopButton = new Fl_Button(240, 55, 70, 25, "&Stop");
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