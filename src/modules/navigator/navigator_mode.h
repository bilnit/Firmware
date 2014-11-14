/****************************************************************************
 *
 *   Copyright (c) 2014 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/**
 * @file navigator_mode.h
 *
 * Base class for different modes in navigator
 *
 * @author Julian Oes <julian@oes.ch>
 * @author Anton Babushkin <anton.babushkin@me.com>
 */

#ifndef NAVIGATOR_MODE_H
#define NAVIGATOR_MODE_H

#include <drivers/drv_hrt.h>

#include <controllib/blocks.hpp>
#include <controllib/block/BlockParam.hpp>

#include <dataman/dataman.h>

#include <uORB/topics/position_setpoint_triplet.h>
#include <uORB/topics/vehicle_command.h>

class Navigator;

class NavigatorMode : public control::SuperBlock
{
public:
	/**
	 * Constructor
	 */
	NavigatorMode(Navigator *navigator, const char *name);

	/**
	 * Destructor
	 */
	virtual ~NavigatorMode();

	void run(bool active, bool parameters_updated);

	/**
	 * This function is called while the mode is inactive
	 */
	virtual void on_inactive();

	/**
	 * This function is called one time when mode become active, poos_sp_triplet must be initialized here
	 */
	virtual void on_activation();

	/**
	 * This function is called while the mode is active
	 */
	virtual void on_active();

	/*
	 * This function defines how vehicle reacts on commands in
	 * current navigator mode
	 */
	virtual void execute_vehicle_command();

	/*
	 * Check if vehicle command subscription has been updated
	 * if it has it updates _vcommand structure and returns true
	 */
	bool update_vehicle_command();

	void point_camera_to_target(position_setpoint_s *sp);

	void updateParameters();
	void updateParamValues();
	void updateParamHandles();

    void land();
    void takeoff();
    void disarm();

	struct {
		float takeoff_alt;
		float takeoff_acceptance_radius;
		float acceptance_radius;
		float loiter_step;
		float velocity_lpf;

		int afol_rep_target_alt;
		int afol_use_cam_pitch;

		float loi_step_len;
		float loi_min_alt;

		float rtl_ret_alt;

        float airdog_dst_inv; 
        float airdog_init_pos_dst;
        int airdog_init_pos_use; 

        float a_yaw_ignore_radius;

	} _parameters;		


	struct {
		param_t takeoff_alt;
		param_t takeoff_acceptance_radius;
		param_t acceptance_radius;
		param_t velocity_lpf;

		param_t afol_rep_target_alt;
		param_t afol_use_cam_pitch;

		param_t loi_step_len;
		param_t loi_min_alt;

		param_t rtl_ret_alt;

        param_t airdog_dst_inv;
        param_t airdog_init_pos_dst; 
        param_t airdog_init_pos_use;

        param_t a_yaw_ignore_radius;

	} _parameter_handles;



protected:
	Navigator *_navigator;

	struct position_setpoint_triplet_s 	*pos_sp_triplet;

	struct target_global_position_s 	*target_pos;
	struct vehicle_global_position_s 	*global_pos;
	struct home_position_s 				*home_pos;

	struct vehicle_command_s _vcommand;

	int		_mavlink_fd;			/**< the file descriptor to send messages over mavlink */

	bool check_current_pos_sp_reached();
    void go_to_intial_position();



private:

	bool _first_run;

	/*
	 * This class has ptr data members, so it should not be copied,
	 * consequently the copy constructors are private.
	 */
	NavigatorMode(const NavigatorMode&);
	NavigatorMode operator=(const NavigatorMode&);

};

#endif
