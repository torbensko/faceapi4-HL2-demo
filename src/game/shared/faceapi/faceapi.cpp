/*

This code is provided under a Creative Commons Attribution license
http://creativecommons.org/licenses/by/3.0/
Essentially, you are free to use the code for any purpose as long as you remember 
to mention my name (Torben Sko) at some point.

Please also note that my code is provided AS IS with NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.


This file was originally adapted from the 'TestAppConsole C' sample program provided
by Seeing Machines' faceAPI. 

 */


#include "cbase.h"

#include <windows.h>
#include <iostream>

#include "convar.h"
#include "faceapi.h"

#include "sm_api.h"

//#define BROADCAST_OSC

#ifdef BROADCAST_OSC
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#define OSC_SEPERATOR "|"

ConVar oscIP		= ConVar("oscIP", "127.0.0.1", FCVAR_ARCHIVE);
ConVar oscPort		= ConVar("oscPort", "7000", FCVAR_ARCHIVE);
#endif


using namespace sm::faceapi;
using namespace std;

const bool DISABLE_GUI = false;

#define THROW_ON_ERROR(x) \
{ \
    smReturnCode result = (x); \
	DevMsg("faceAPI: %d\n", result); \
}

FaceAPIData::FaceAPIData()
{
	h_confidence = 0.0f;
	h_frameDuration = 0.0f;
	h_frameNum = 0;
}

bool		_faceapi_fetcher_running;
FaceAPI*	_faceapi;

void __cdecl FaceAPI_dataFetcher(void *)
{
	_faceapi_fetcher_running = true;
	while(_faceapi->m_shuttingDown) 
	{
		_faceapi->InternalDataFetch();
	}
	_faceapi_fetcher_running = false;
	_endthread();
}

FaceAPI* GetFaceAPI() 
{
	if(_faceapi == NULL)
	{
		_faceapi = new FaceAPI();
		_faceapi->Init();
	}
	return _faceapi;
}


FaceAPI::FaceAPI() 
{
	// too early to initialise the actual tracking
	_faceapi = this;
	m_lastUpdate = 0.0f;
}

// The main function: setup a tracking engine and show a video window, then loop on the keyboard.
void FaceAPI::Init()
{
    smLoggingSetFileOutputEnable(true);

	// Initialize faceAPI
	faceapi_api = new faceAPIScope();

    // Register windows webcam camera type
    CameraInfoList::registerType(SM_API_CAMERA_TYPE_WDM);

	if (!faceAPIScope::isNonCommercialLicense())
	{
        // Get list of connected cameras
        // (For non-commercial license this will contain at most 1 item)
        CameraInfoList camera_info_list;
        if (camera_info_list.numCameras() == 0)
        {
            return;
        }
        // Create a camera
        const int camera_index = 0;
        const int format_index = 0;
        faceapi_camera = new LiveCamera(SM_API_DEFAULT_APPROX_HFOV, camera_index, format_index);
        // Create engine
        faceapi_engine = new HeadTrackerV2(*faceapi_camera);
	}
	else
	{
        // Create engine using first available webcam
        faceapi_engine = new HeadTrackerV2();
	}

	// Configure engine
    if (!faceAPIScope::isNonCommercialLicense())
    {
        faceapi_engine->setLipTrackingEnabled(true);
        faceapi_engine->setEyebrowTrackingEnabled(true);
        faceapi_engine->setEyeClosureTrackingEnabled(true);
        //if (faceapi_engine->getNumGPUs() > 0)
        //{
        //    faceapi_engine->setGPUAcceleration(true);
        //}
    }

    // Start tracking
	faceapi_engine->start();

    // start up the  data fetcher
	m_shuttingDown = false;
	m_currData = 0;
	m_nextData = 1;
	(HANDLE)_beginthread(FaceAPI_dataFetcher, 0, (void *) 0);
}

void FaceAPI::Shutdown() 
{
	// wait for the fetcher thread to die
	m_shuttingDown = true;
	while(_faceapi_fetcher_running)
		Sleep(100);

    faceapi_engine->stop();

	delete faceapi_engine;
	delete faceapi_api;
}

void FaceAPI::RestartTracking()
{
	DevMsg("restart tracker request - not yet implemented\n");
}

void FaceAPI::InternalDataFetch()
{
	float now = gpGlobals->curtime;
	
	EngineData engine_data;
    const int timeout_ms = 1000;
    // Waits for the next measurement from the engine
	const bool is_ok = faceapi_engine->waitNext(engine_data, timeout_ms);

	if( !is_ok )
	{
		DevMsg("error fetching faceAPI data\n");
		return;
	}

	if( !engine_data.isValid() )
    {
        DevMsg("invalid engine data received\n");
        return;
    }

	smEngineData* sm_engine_data = engine_data.get();
	
	if(sm_engine_data->num_people == 0)
		return; // no data to get

	// just use the data from the first person
	const smEnginePersonData &person_data = sm_engine_data->people[0];

	if(person_data.head_pose_data)
	{
		m_data[m_nextData].h_confidence				= person_data.head_pose_data->confidence;
		m_data[m_nextData].h_data[HEAD_HOR]			= person_data.head_pose_data->head_pos.x;
		m_data[m_nextData].h_data[HEAD_VERT]		= person_data.head_pose_data->head_pos.y;
		m_data[m_nextData].h_data[HEAD_DEPTH]		= person_data.head_pose_data->head_pos.z;
		m_data[m_nextData].h_data[HEADPOS_PITCH]	= person_data.head_pose_data->head_rot.x_rads;
		m_data[m_nextData].h_data[HEADPOS_YAW]		= person_data.head_pose_data->head_rot.y_rads;
		m_data[m_nextData].h_data[HEADPOS_ROLL]		= person_data.head_pose_data->head_rot.z_rads;
	}

	if(person_data.eye_data)
	{
		m_data[m_nextData].h_data[L_EYE_CLOSE]		= person_data.eye_data->closure_data->closure[SM_API_LEFT_EYE];
		m_data[m_nextData].h_data[R_EYE_CLOSE]		= person_data.eye_data->closure_data->closure[SM_API_RIGHT_EYE];
	}

	smEngineFaceData* facePoints = person_data.face_data;
	if(facePoints)
	{
		float mLeft = 0.0f;
		float mRight = 0.0f;
		float mTop = 0.0f;
		float mBottom = 0.0f;

		bool mLeftSet	= false;
		bool mRightSet	= false;
		bool mTopSet	= false;
		bool mBottomSet	= false;

		for(int i = 0; i < facePoints->num_landmarks; i++)
		{
			if(facePoints->landmarks[i].fc.x != 0.0f && facePoints->landmarks[i].fc.y != 0.0f)
			{
				//DevMsg("id:%d x:%.2f y:%.2f\n", 
				//		facePoints->landmarks[i].id, 
				//		facePoints->landmarks[i].fc.x, 
				//		facePoints->landmarks[i].fc.y);
				switch(facePoints->landmarks[i].id)
				{
					// right eyebrow:
				case 302:
					m_data[m_nextData].h_data[R_EB_INNER]	= facePoints->landmarks[i].fc.y;
					break;
				case 301:
					m_data[m_nextData].h_data[R_EB_MIDDLE]	= facePoints->landmarks[i].fc.y;
					break;
				case 300:
					m_data[m_nextData].h_data[R_EB_OUTER]	= facePoints->landmarks[i].fc.y;
					break;

					// left eyebrow:
				case 400:
					m_data[m_nextData].h_data[L_EB_INNER]	= facePoints->landmarks[i].fc.y;
					break;
				case 401:
					m_data[m_nextData].h_data[L_EB_MIDDLE]	= facePoints->landmarks[i].fc.y;
					break;
				case 402:
					m_data[m_nextData].h_data[L_EB_OUTER]	= facePoints->landmarks[i].fc.y;
					break;

					// mouth corners
				case 200:
					m_data[m_nextData].h_data[L_MO_CORN_X]	= mLeft = facePoints->landmarks[i].fc.x;
					m_data[m_nextData].h_data[L_MO_CORN_Y]	= facePoints->landmarks[i].fc.y;
					mLeftSet = true;
					break;
				case 204:
					m_data[m_nextData].h_data[R_MO_CORN_X]	= mRight = facePoints->landmarks[i].fc.x;
					m_data[m_nextData].h_data[R_MO_CORN_Y]	= facePoints->landmarks[i].fc.y;
					mRightSet = true;
					break;

				// lip seperation
				case 202:
					m_data[m_nextData].h_data[MO_TOP]		= mTop = facePoints->landmarks[i].fc.y;
					mTopSet = true;
					break;
				case 206:
					m_data[m_nextData].h_data[MO_BOTTOM]	= mBottom = facePoints->landmarks[i].fc.y;
					mBottomSet = true;
					break;
				}
			}
		}
		if(mLeftSet && mRightSet)
			m_data[m_nextData].h_data[MO_SEPERATION_VERT] = abs(mLeft - mRight);
		if(mTopSet && mBottomSet)
			m_data[m_nextData].h_data[MO_SEPERATION_VERT] = abs(mTop - mBottom);
	}

	
	m_data[m_nextData].h_frameNum			= sm_engine_data->video_frame.frame_num;
	m_data[m_nextData].h_frameDuration		= now - m_lastUpdate;

	m_currData = m_nextData;
	m_nextData = (m_nextData + 1) % 3;
	
	m_lastUpdate = now;

#ifdef BROADCAST_OSC

#define OUTPUT_BUFFER_SIZE 1024

	// send this position via OSC
	UdpTransmitSocket transmitSocket( IpEndpointName( oscIP.GetString(), oscPort.GetInt() ) );

	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
	
	p << osc::BeginBundleImmediate 
			<< osc::BeginMessage( "/hor" )		<< engine_data.head_pose_data->head_pos.x		<< osc::EndMessage 
			<< osc::BeginMessage( "/vert" )		<< engine_data.head_pose_data->head_pos.y		<< osc::EndMessage 
			<< osc::BeginMessage( "/depth" )	<< engine_data.head_pose_data->head_pos.z		<< osc::EndMessage 
			<< osc::BeginMessage( "/pitch" )	<< engine_data.head_pose_data->head_rot.x_rads	<< osc::EndMessage 
			<< osc::BeginMessage( "/yaw" )		<< engine_data.head_pose_data->head_rot.y_rads	<< osc::EndMessage 
			<< osc::BeginMessage( "/roll" )		<< engine_data.head_pose_data->head_rot.z_rads	<< osc::EndMessage 
			<< osc::BeginMessage( "/conf" )		<< engine_data.head_pose_data->confidence		<< osc::EndMessage 
			<< osc::EndBundle;
	transmitSocket.Send( p.Data(), p.Size() );
#endif BROADCAST_OSC
}

FaceAPIData FaceAPI::GetHeadData()
{
	int curr = m_currData;
	return m_data[curr];
}

float FaceAPI::GetTrackingConf()
{
	return m_data[m_currData].h_confidence;
}

void FaceAPI::GetCameraDetails(char *modelBuf, int bufLen, int &framerate, int &resWidth, int &resHeight)
{
	DevMsg("camera details requested - not yet implemented\n");

	//smCameraVideoFormat video_format;
	//smStringHandle model_name;
	//smCameraHandle camera_handle;
	//int length;
	//wchar_t *camera_model;
	//smStringCreate(&model_name);
	//smEngineGetCamera(engine_handle, &camera_handle);
	//smCameraGetCurrentFormat(camera_handle, &video_format);
	//smCameraGetModelName(camera_handle, model_name);
	//smStringGetBufferW(model_name, &camera_model, &length);
	//V_snprintf(modelBuf, bufLen, "%ls", camera_model);
	//if(strlen(modelBuf) > 0)
	//{
	//	framerate = (int)video_format.framerate;
	//	resWidth = video_format.res.w;
	//	resHeight = video_format.res.h;
	//}
	//else
	//{
	//	framerate = 0;
	//	resWidth = 0;
	//	resHeight = 0;
	//}
	//smStringDestroy(&model_name);
}