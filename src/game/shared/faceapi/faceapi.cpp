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

#include "sm_api.h"
#include "faceapi.h"

#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

using namespace std;

#define OSC_SEPERATOR "|"

ConVar oscIP		= ConVar("oscIP", "127.0.0.1", FCVAR_ARCHIVE);
ConVar oscPort		= ConVar("oscPort", "7000", FCVAR_ARCHIVE);

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
	while(_faceapi->InternalDataFetch()) {}
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
    // Log API debugging information to a file (good for tech support)
    THROW_ON_ERROR(smLoggingSetFileOutputEnable(SM_API_TRUE));

#   ifdef _DEBUG
    // Hook up log message callback
    THROW_ON_ERROR(smLoggingRegisterCallback(0,receiveLogMessage));
#   endif

    // Get the version
    //int major, minor, maint;
    THROW_ON_ERROR(smAPIVersion(&m_versionMajor, &m_versionMinor, &m_versionMaintenance));
	DevMsg("faceAPI version: %d.%d.%d\n", m_versionMajor, m_versionMinor, m_versionMaintenance);
    
	// Print detailed license info
    char *buff;
    int size;
    THROW_ON_ERROR(smAPILicenseInfoString(0,&size,SM_API_TRUE));
    buff = new char[size];
    THROW_ON_ERROR(smAPILicenseInfoString(buff,&size,SM_API_TRUE));
	DevMsg("faceAPI license: %s\n", buff);
	delete [] buff;
    
	// Initialize the API
    THROW_ON_ERROR(smAPIInit());

    // Register the WDM category of cameras
    THROW_ON_ERROR(smCameraRegisterType(SM_API_CAMERA_TYPE_WDM));

    // Create a new Head-Tracker engine that uses the camera
    THROW_ON_ERROR(smEngineCreate(SM_API_ENGINE_LATEST_HEAD_TRACKER,&engine_handle));

    // Check license for particular engine version (always ok for non-commercial license)
    const bool engine_licensed = smEngineIsLicensed(engine_handle) == SM_API_OK;

#	ifdef SHOW_FACEAPI_WINDOW
	// Create and show a video-display window
    smVideoDisplayHandle video_display_handle = 0;
    THROW_ON_ERROR(smVideoDisplayCreate(engine_handle,&video_display_handle,0,TRUE));

    // Setup the VideoDisplay
	unsigned short g_overlay_flags(SM_API_VIDEO_DISPLAY_HEAD_MESH);
    THROW_ON_ERROR(smVideoDisplaySetFlags(video_display_handle,g_overlay_flags));

    smWindowHandle win_handle = 0;
    THROW_ON_ERROR(smVideoDisplayGetWindowHandle(video_display_handle,&win_handle));    
    SetWindowText(win_handle, _T(""));
	SetWindowPos(win_handle, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
	//SetWindowLong(win_handle, GWL_STYLE, GetWindowLong(win_handle, GWL_STYLE) & ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME));
    //SetWindowLong(win_handle, GWL_EXSTYLE, GetWindowLong(win_handle, GWL_EXSTYLE) & ~WS_EX_DLGMODALFRAME);
#	endif

	smHTV2SetHeadPoseFilterLevel(engine_handle, 1);
	//THROW_ON_ERROR(smHTV2SetStrictFaceDetection(engine_handle, true));
	// Enable lip tracking
    THROW_ON_ERROR(smHTSetLipTrackingEnabled(engine_handle, SM_API_TRUE));
    // Enable eyebrow tracking
    THROW_ON_ERROR(smHTSetEyebrowTrackingEnabled(engine_handle, SM_API_TRUE));
    // Enable eye closure tracking
    THROW_ON_ERROR(smHTSetEyeClosureTrackingEnabled(engine_handle, SM_API_TRUE));
	THROW_ON_ERROR(smHTSetEyeClosureEyeIndependence(engine_handle, SM_API_TRUE));
    // Enable GPU processing
	int num_gpus = 0;
    THROW_ON_ERROR(smHTV2GetNumGPUs(engine_handle,&num_gpus));
    if (num_gpus > 0)
    	THROW_ON_ERROR(smHTV2SetGPUAccel(engine_handle,SM_API_TRUE));
    
    // Start tracking
    THROW_ON_ERROR(smEngineStart(engine_handle));

	// start up the fetcher
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

	// Destroy engine
    THROW_ON_ERROR(smEngineDestroy(&engine_handle));
	THROW_ON_ERROR(smAPIQuit());
}

void FaceAPI::GetVersion(int &major, int &minor, int &maintenance)
{
	major		= m_versionMajor;
	minor		= m_versionMinor;
	maintenance = m_versionMaintenance;
}

void FaceAPI::RestartTracking()
{
	THROW_ON_ERROR(smEngineStart(engine_handle));
}

bool FaceAPI::InternalDataFetch()
{
#	define OUTPUT_BUFFER_SIZE 1024

	if(!engine_handle || m_shuttingDown)
		return false;

	float now = gpGlobals->curtime;

	smEngineData enginedata;
	smReturnCode result = smEngineDataWaitNext(engine_handle, &enginedata, 5000);

	if(result != SM_API_OK)
	{
		DevMsg("error fetching faceAPI data\n");
	}
	else
	{
		m_data[m_nextData].h_confidence = enginedata.head_pose_data->confidence;
		
		m_data[m_nextData].h_data[HEAD_HOR]			= enginedata.head_pose_data->head_pos.x;
		m_data[m_nextData].h_data[HEAD_VERT]		= enginedata.head_pose_data->head_pos.y;
		m_data[m_nextData].h_data[HEAD_DEPTH]		= enginedata.head_pose_data->head_pos.z;
		m_data[m_nextData].h_data[HEADPOS_PITCH]	= enginedata.head_pose_data->head_rot.x_rads;
		m_data[m_nextData].h_data[HEADPOS_YAW]		= enginedata.head_pose_data->head_rot.y_rads;
		m_data[m_nextData].h_data[HEADPOS_ROLL]		= enginedata.head_pose_data->head_rot.z_rads;

		// if we are using the non-commercial license then we won't be able to access this stuff
		if(enginedata.eye_data != NULL && enginedata.eye_data->closure_data != NULL)
		{
			m_data[m_nextData].h_data[L_EYE_CLOSE]	= enginedata.eye_data->closure_data->closure[SM_API_LEFT_EYE];
			m_data[m_nextData].h_data[R_EYE_CLOSE]	= enginedata.eye_data->closure_data->closure[SM_API_RIGHT_EYE];
		}

		smEngineFaceData* facePoints = enginedata.face_data;
		if(facePoints)
		{
			float mLeft = 0.0f,	mRight = 0.0f;
			float mTop = 0.0f,	mBottom = 0.0f;
			bool mLeftSet	= false, mRightSet	= false;
			bool mTopSet	= false, mBottomSet	= false;

			for(int i = 0; i < facePoints->num_landmarks; i++)
			{
				if(facePoints->landmarks[i].fc.x != 0.0f && facePoints->landmarks[i].fc.y != 0.0f)
				{
					//DevMsg("id:%d x:%.2f y:%.2f\n", facePoints->landmarks[i].id, facePoints->landmarks[i].fc.x, facePoints->landmarks[i].fc.y);
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

		
		m_data[m_nextData].h_frameNum						= enginedata.video_frame.frame_num;
		m_data[m_nextData].h_frameDuration					= now - m_lastUpdate;

		m_currData = m_nextData;
		m_nextData = (m_nextData + 1) % 3;
		
		m_lastUpdate = now;

		// send this position via OSC
		UdpTransmitSocket transmitSocket( IpEndpointName( oscIP.GetString(), oscPort.GetInt() ) );
    
		char buffer[OUTPUT_BUFFER_SIZE];
		osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );
		
		p << osc::BeginBundleImmediate 
				<< osc::BeginMessage( "/hor" )		<< enginedata.head_pose_data->head_pos.x		<< osc::EndMessage 
				<< osc::BeginMessage( "/vert" )		<< enginedata.head_pose_data->head_pos.y		<< osc::EndMessage 
				<< osc::BeginMessage( "/depth" )	<< enginedata.head_pose_data->head_pos.z		<< osc::EndMessage 
				<< osc::BeginMessage( "/pitch" )	<< enginedata.head_pose_data->head_rot.x_rads	<< osc::EndMessage 
				<< osc::BeginMessage( "/yaw" )		<< enginedata.head_pose_data->head_rot.y_rads	<< osc::EndMessage 
				<< osc::BeginMessage( "/roll" )		<< enginedata.head_pose_data->head_rot.z_rads	<< osc::EndMessage 
				<< osc::BeginMessage( "/conf" )		<< enginedata.head_pose_data->confidence		<< osc::EndMessage 
				<< osc::EndBundle;
		transmitSocket.Send( p.Data(), p.Size() );
	}
	
	smEngineDataDestroy(&enginedata);
	return true;
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
	smCameraVideoFormat video_format;
	smStringHandle model_name;
	smCameraHandle camera_handle;
	int length;
	wchar_t *camera_model;
	smStringCreate(&model_name);
	smEngineGetCamera(engine_handle, &camera_handle);
	smCameraGetCurrentFormat(camera_handle, &video_format);
	smCameraGetModelName(camera_handle, model_name);
	smStringGetBufferW(model_name, &camera_model, &length);
	V_snprintf(modelBuf, bufLen, "%ls", camera_model);
	if(strlen(modelBuf) > 0)
	{
		framerate = (int)video_format.framerate;
		resWidth = video_format.res.w;
		resHeight = video_format.res.h;
	}
	else
	{
		framerate = 0;
		resWidth = 0;
		resHeight = 0;
	}
	smStringDestroy(&model_name);
}