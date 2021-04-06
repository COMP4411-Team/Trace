//
// rayUI.h
//
// The header file for the UI part
//

#ifndef __rayUI_h__
#define __rayUI_h__

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>

#include <FL/fl_file_chooser.H>		// FLTK file chooser

#include "TraceGLWindow.h"

class TraceUI {
public:
	TraceUI();

	// The FLTK widgets
	Fl_Window*			m_mainWindow;
	Fl_Menu_Bar*		m_menubar;

	Fl_Slider*			m_sizeSlider;
	Fl_Slider*			m_depthSlider;
	Fl_Slider* m_lightScaleSlider;
	Fl_Slider* m_threshSlider;
	Fl_Slider* m_ssaaLevelSlider;

	Fl_Button*			m_renderButton;
	Fl_Button*			m_stopButton;
	Fl_Button* m_renderPtButton;
	Fl_Light_Button* m_ssaaJitterButton;
	Fl_Light_Button* m_pathTracingButton;

	Fl_Light_Button* m_fasterShadow;

	Fl_Light_Button* m_motionBlurButton;
	Fl_Slider* m_motionBlurSPPSlider;

	Fl_Light_Button* m_distributedButton;
	Fl_Slider* m_childRaySlider;

	Fl_Light_Button* m_multiThreadButton;

	Fl_Button* m_buildPMButton;
	Fl_Light_Button* m_enablePMButton;
	
	Fl_Light_Button* m_enableDofButton;
	Fl_Slider* m_apertureSlider;
	Fl_Slider* m_focalLengthSlider;

	TraceGLWindow*		m_traceGlWindow;

	// member functions
	void show();

	void		setRayTracer(RayTracer *tracer);

	int			getSize();
	int			getDepth();

private:
	Fl_Value_Slider* createSlider(int x, int y, int w, int h, const char* l,
	                              double minValue, double maxValue, double step, double value,
	                              void (*callback)(Fl_Widget*, void*));
	
	RayTracer*	raytracer;

	int			m_nSize;
	int			m_nDepth;
	double lightScale{10};
	int ssaaSampleLevel{0};
	bool enablePathTracing{false};
	bool enableFasterShadow{ false };
	bool enableMultiThread{false};
	int maxIter{10000};
	vec3f threshold;

// static class members
	static Fl_Menu_Item menuitems[];

	static TraceUI* whoami(Fl_Menu_* o);

	static TraceUI* whoami(Fl_Widget* o);

	static void cb_load_scene(Fl_Menu_* o, void* v);
	static void cb_load_hf(Fl_Menu_* o, void* v);
	static void cb_save_image(Fl_Menu_* o, void* v);
	static void cb_exit(Fl_Menu_* o, void* v);
	static void cb_about(Fl_Menu_* o, void* v);

	static void cb_exit2(Fl_Widget* o, void* v);

	static void cb_sizeSlides(Fl_Widget* o, void* v);
	static void cb_depthSlides(Fl_Widget* o, void* v);
	static void cb_lightScaleSlides(Fl_Widget* o, void* v);
	static void cb_threshSlides(Fl_Widget* o, void* v);
	static void cb_ssaaLevelSlides(Fl_Widget* o, void* v);
	static void cb_ssaaJitterButton(Fl_Widget* o, void* v);
	static void cb_pathTracingButton(Fl_Widget* o, void* v);
	static void cb_fasterShadow(Fl_Widget* o, void* v);

	// Depth of field
	static void cb_enableDof(Fl_Widget* o, void* v);
	static void cb_aperture(Fl_Widget* o, void* v);
	static void cb_focalLength(Fl_Widget* o, void* v);

	// Motion Blur
	static void cb_enableMotionBlur(Fl_Widget* o, void* v);
	static void cb_motionBlurSPP(Fl_Widget* o, void* v);

	// Distributed RT
	static void cb_distributed(Fl_Widget* o, void* v);
	static void cb_childRay(Fl_Widget* o, void* v);

	static void cb_multiThread(Fl_Widget* o, void* v);

	// Photon mapping
	static void cb_buildPM(Fl_Widget* o, void* v);
	static void cb_enablePM(Fl_Widget* o, void* v);

	static void cb_render(Fl_Widget* o, void* v);
	static void cb_renderParallel(Fl_Widget* o, void* v);
	static void cb_renderPt(Fl_Widget* o, void* v);
	static void cb_stop(Fl_Widget* o, void* v);
};

#endif
