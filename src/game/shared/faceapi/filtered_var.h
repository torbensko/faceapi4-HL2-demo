/*

This code is provided under a Creative Commons Attribution license
http://creativecommons.org/licenses/by/3.0/
Essentially, you are free to use the code for any purpose as long as you remember 
to mention my name (Torben Sko) at some point.

Please also note that my code is provided AS IS with NO WARRANTY OF ANY KIND,
INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.

 */

#ifndef FACEAPI_FILTERED_VAR_H
#define FACEAPI_FILTERED_VAR_H

#include <map>
#include <vector>

#include "faceapi/tunable_var.h"

// -----------------------------------------
// Handles Time-based Smoothing. Able to smooth over two
// different time periods.
// -----------------------------------------
class Smoothable
{
public:
	Smoothable();

	void EnableSmoothing(TunableVar *duration) { m_duration = duration; }
	void Smooth(float &value, float now, float adpativeSmoothing = 1);
	void ResetSmoothing();

private:

	TunableVar *m_duration;

	std::map<float, float> m_timestampedValues;
	float m_sum;
	float m_lastTime;
};



// -----------------------------------------
// Handles Normalising
// -----------------------------------------
class Normalisable 
{
public:
	Normalisable();

	void EnableNormalising(TunableVar *range, TunableVar *min = NULL)
	{ 
		m_normaliseRange = range; 
		m_min = min;
	}

	void Normalise(float &value);

private:	
	TunableVar *m_min;
	TunableVar *m_normaliseRange;
};



// -----------------------------------------
// Handles Easing
// -----------------------------------------
class Easable 
{
public:
	Easable();

	// @param amount An integer percentage which determines by how
	//				 much the value should be eased e.g. 80 = 80%
	void EnableEasing(TunableVar *amount) { m_easeAmount = amount; }
	void Ease(float &normalisedValue);

private:
	TunableVar *m_easeAmount;
};



// -----------------------------------------
// Handles Neutralising
// -----------------------------------------
class Neutralisable
{
public:
	Neutralisable();

	// When enabled weighted average is calculated and used as an
	// offset. Each iteration, the average is updated using the current
	// value. Each value is weighted by its distance to the
	// average (closer == stronger)
	//
	// @param favourZero If this is true, the distance weighting is
	//					 computed relative to the zero point, not the
	//					 average.
	void ConfigureNeutralising(TunableVar *tendency, TunableVar *range, TunableVar *initialPeriod, bool favourZero = false);
	void Neutralise(float &value, unsigned int frameNum, float frameDuration);
	void ResetNeutralising();
	float GetNeutral() { return m_weightedAverage; }

private:
	float m_weightedSum;
	float m_collectiveTime;
	float m_weightedAverage;
	unsigned int m_lastFrameNumber;
	bool m_favourZero;

	TunableVar *m_tendency;
	TunableVar *m_range;
	TunableVar *m_initialPeriod;
};


// -----------------------------------------
// Handles Scaling
// -----------------------------------------
class Scaleable
{
public:
	Scaleable();

	void EnableScaling(TunableVar *scale1, TunableVar *scale2 = NULL) 
	{
		m_scale1 = scale1;
		m_scale2 = scale2;
	}

	void EnableFadeoutScaling(TunableVar *range) 
	{
		m_range = range;
	}

	void Scale(float &value);
private:
	TunableVar *m_scale1;
	TunableVar *m_scale2;

	TunableVar *m_range;
	TunableVar *m_power;
};


// -----------------------------------------
// Handles Fading In/Out (when tracking drops out)
// -----------------------------------------
class Fadable
{
public:
	Fadable();

	void EnableFading(TunableVar *duration);

	float FadeIn(float value, float now);
	float FadeOut(float now);
	void ResetFade();

private:
	float m_fadeoutTime;
	float m_fadeoutDuration;
	float m_fadeinTime;
	float m_fadeinDuration;

	float m_lastFadeOutValue;
	float m_lastFadeInValue;
	
	TunableVar *m_easeDurationOnDropout;
};



#endif