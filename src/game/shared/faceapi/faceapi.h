/*

This code is provided under a Creative Commons Attribution license
http://creativecommons.org/licenses/by/3.0/
Essentially, you are free to use the code for any purpose as long as you remember 
to mention my name (Torben Sko) at some point.

Please also note that my code is provided AS IS with NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.

 */

#ifndef FACEAPI_H
#define FACEAPI_H

#include <string>
#include <sstream>
#include <map>
//#include <boost/scoped_ptr.hpp>

#include "sm_api.h"
#include "sm_api_cxx.h"

enum FaceAPIDataName {
	UNDEFINED,
	// head rotation
	HEADPOS_YAW,
	HEADPOS_PITCH,	
	HEADPOS_ROLL,
	// head position
	HEAD_VERT,	
	HEAD_HOR,	
	HEAD_DEPTH,
	// winking
	L_EYE_CLOSE,
	R_EYE_CLOSE,
	// eyebrows
	L_EB_INNER,
	L_EB_MIDDLE,
	L_EB_OUTER,
	R_EB_INNER,
	R_EB_MIDDLE,
	R_EB_OUTER,
	// mouth: horizontal
	R_MO_CORN_X,
	R_MO_CORN_Y,
	L_MO_CORN_X,
	L_MO_CORN_Y,
	// mouth: vertical
	MO_TOP,
	MO_BOTTOM,
	// mouth: size
	MO_SEPERATION_VERT,
	MO_SEPERATION_HOR
};


class FaceAPIData
{
public:
	FaceAPIData();

	std::map<int,float>	h_data;
	float				h_confidence;
	float				h_frameDuration;
	unsigned int		h_frameNum;
};

class FaceAPI
{
public:
	FaceAPI();
	void			Init();
	void			Shutdown();
	void			GetCameraDetails(char *modelBuf, int bufLen, int &framerate, int &resWidth, int &resHeight);
	void			RestartTracking();

	FaceAPIData		GetHeadData(); // not a halting function
	float			GetTrackingConf();
	
	void			InternalDataFetch();

	bool			m_shuttingDown;

protected:
	//boost::scoped_ptr<sm::faceapi::Camera>		camera;
	//boost::scoped_ptr<sm::faceapi::HeadTrackerV2>	engine;
	//boost::scoped_ptr<sm::faceapi::VideoDisplay>	video_display;
	sm::faceapi::Camera			*faceapi_camera;
	sm::faceapi::HeadTrackerV2	*faceapi_engine;

	FaceAPIData		m_data[3];
	int				m_currData;
	int				m_nextData;
	float			m_lastUpdate;

	int				m_lastFrame;
};

FaceAPI* GetFaceAPI();

#endif FACEAPI_H
