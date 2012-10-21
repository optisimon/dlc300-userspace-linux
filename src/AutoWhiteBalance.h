/**
 * White Balance adjustment algorithms for some strange USB camera using libusb.
 *
 * Original author Simon Gustafsson (www.simong.eu/projects/dlc300)
 *
 * Copyright (c) 2012 Simon Gustafsson (www.simong.eu)
 * Do whatever you like with this code, but please refer to me as the original author.
 */

#ifndef AUTOWHITEBALANCE_H_
#define AUTOWHITEBALANCE_H_


/**
 * White balance is set iteratively. An instance of this class should be fed sums of red, green, and blue
 * components in the white balance region continuously when adjusting white balance. This class should in
 * turn be queried for the gain values to be sent to the camera.
 *
 * @note A white or gray object should be used when setting white balance using this class, and
 * exposure should not be set in such a way that the image is over-exposed.
 */
class AutoWhiteBalance {
	int gain_red_;
	int gain_green_;
	int gain_blue_;

	int fine_tune_iteration_;

	enum stateEnum {
		idle = 0,
		setting_equal_gains,
		setting_equal_gains_wait,
		coarse_tuning,
		fine_tuning
	};

	stateEnum state_;

	void algoritm1(int sum_R, int sum_G, int sum_B, int& gain_red, int& gain_green, int& gain_blue);

	/// Babysteps towards the goal...
	void algoritm2(int sum_R, int sum_G, int sum_B, int& gain_red, int& gain_green, int& gain_blue);

public:
	AutoWhiteBalance(int& gain_red, int& gain_green, int& gain_blue);

	void processCurrentSums(int sum_R, int sum_G, int sum_B);

	void stop() { state_ = idle; }

	void start() {	state_ = setting_equal_gains; }

	bool isRunning() { return state_ != idle; }

	void setCurrentGains(int& gain_red, int& gain_green, int& gain_blue);

	void getCurrentGains(int& gain_red, int& gain_green, int& gain_blue);

};


#endif /* AUTOWHITEBALANCE_H_ */
