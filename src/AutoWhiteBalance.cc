/**
 * White Balance adjustment algorithms for some strange USB camera using libusb.
 *
 * Original author Simon Gustafsson (www.simong.eu/projects/dlc300)
 *
 * Copyright (c) 2012 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#include "AutoWhiteBalance.h"

#include <math.h>
#include <stdio.h>
#include <algorithm>


template <class T>
T coerce(const T& value, const T& min, const T& max)
{
	return std::max(std::min(value, max), min);
}


void AutoWhiteBalance::algoritm1(int sum_R, int sum_G, int sum_B, int& gain_red, int& gain_green, int& gain_blue)
{
	const double gain_max = 63;

	if (sum_R <= sum_G && sum_R <= sum_B)
	{
		gain_red   = round(gain_max);
		gain_green = round(gain_max*sum_R/sum_G);
		gain_blue  = round(gain_max*sum_R/sum_B);
	}
	else if (sum_G <= sum_R && sum_G <= sum_B)
	{
		gain_red   = round(gain_max*sum_G/sum_R);
		gain_green = round(gain_max);
		gain_blue  = round((gain_max*sum_G)/sum_B);
	}
	else if (sum_B <= sum_R && sum_B <= sum_G)
	{
		gain_red   = round(gain_max*sum_B/sum_R);
		gain_green = round(gain_max*sum_B/sum_G);
		gain_blue  = round(gain_max);
	}
}


/// Babysteps towards the goal...
void AutoWhiteBalance::algoritm2(int sum_R, int sum_G, int sum_B, int& gain_red, int& gain_green, int& gain_blue)
{
	long mean = (sum_R + sum_G + sum_B) / 3;

	const int gain_max = 63;

	if (sum_R > mean)
	{
		gain_red--;
	}
	else
	{
		if (gain_red == gain_max)
		{
			gain_green--;
			gain_blue--;
		}
		gain_red++;
	}

	if (sum_G > mean)
	{
		gain_green--;
	}
	else
	{
		if (gain_green == gain_max)
		{
			gain_red--;
			gain_blue--;
		}
		gain_green++;
	}

	if (sum_B > mean)
	{
		gain_blue--;
	}
	else
	{
		if (gain_blue == gain_max)
		{
			gain_red--;
			gain_green--;
		}
		gain_blue++;
	}

	gain_red   = coerce(gain_red,   0, gain_max);
	gain_green = coerce(gain_green, 0, gain_max);
	gain_blue  = coerce(gain_blue,  0, gain_max);

}


void AutoWhiteBalance::processCurrentSums(int sum_R, int sum_G, int sum_B)
{
	switch(state_)
	{
	case idle:
		break;

	case setting_equal_gains:
		state_ = setting_equal_gains_wait;
		gain_red_ = 63;
		gain_green_ = 63;
		gain_blue_ = 63;
		break;

	case setting_equal_gains_wait:
		state_ = coarse_tuning;
		break;

	case coarse_tuning:
		printf("AutoWhiteBalance: Coarse tuning\n");
		algoritm1(sum_R, sum_G, sum_B, gain_red_, gain_green_, gain_blue_);
		state_ = fine_tuning;
		fine_tune_iteration_ = 0;
		break;

	case fine_tuning:
		algoritm2(sum_R, sum_G, sum_B, gain_red_, gain_green_, gain_blue_);

		fine_tune_iteration_++;

		if (fine_tune_iteration_ > 10) {
			printf("AutoWhiteBalance: Stopping\n");
			state_ = idle;
		}
		break;
	}
}


void AutoWhiteBalance::setCurrentGains(int& gain_red, int& gain_green, int& gain_blue)
{
	gain_red_   = gain_red;
	gain_green_ = gain_green;
	gain_blue_  = gain_blue;
}


void AutoWhiteBalance::getCurrentGains(int& gain_red, int& gain_green, int& gain_blue)
{
	gain_red   = gain_red_;
	gain_green = gain_green_;
	gain_blue  = gain_blue_;
}


AutoWhiteBalance::AutoWhiteBalance(int& gain_red, int& gain_green, int& gain_blue)
:	gain_red_(gain_red),
 	gain_green_(gain_green),
 	gain_blue_(gain_blue),
 	fine_tune_iteration_(0),
 	state_(idle)
{

}

