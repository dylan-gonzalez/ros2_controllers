/*********************************************************************
 * Copyright (c) 2023, Stogl Robotics Consulting UG (haftungsbeschränkt)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *********************************************************************/

/*
 * Author: dr. sc. Tomislav Petkovic
 * Author: Dr. Ing. Denis Stogl
 */

#include "steering_controllers_library/steering_odometry.hpp"

#include <cmath>
#include <iostream>

namespace steering_odometry
{
SteeringOdometry::SteeringOdometry(size_t velocity_rolling_window_size)
: timestamp_(0.0),
  x_(0.0),
  y_(0.0),
  heading_(0.0),
  linear_(0.0),
  angular_(0.0),
  wheel_track_(0.0),
  wheelbase_(0.0),
  wheel_radius_(0.0),
  traction_wheel_old_pos_(0.0),
  velocity_rolling_window_size_(velocity_rolling_window_size),
  linear_acc_(velocity_rolling_window_size),
  angular_acc_(velocity_rolling_window_size)
{
}

void SteeringOdometry::init(const rclcpp::Time & time)
{
  // Reset accumulators and timestamp:
  reset_accumulators();
  timestamp_ = time;
}

bool SteeringOdometry::update_odometry(
  const double linear_velocity, const double angular, const double dt)
{
  /// Integrate odometry:
  SteeringOdometry::integrate_exact(linear_velocity * dt, angular);

  /// We cannot estimate the speed with very small time intervals:
  if (dt < 0.0001)
  {
    return false;  // Interval too small to integrate with
  }

  /// Estimate speeds using a rolling mean to filter them out:
  linear_acc_.accumulate(linear_velocity);
  angular_acc_.accumulate(angular / dt);

  linear_ = linear_acc_.getRollingMean();
  angular_ = angular_acc_.getRollingMean();

  return true;
}

bool SteeringOdometry::update_from_position(
  const double traction_wheel_pos, const double steer_pos, const double dt)
{
  /// Get current wheel joint positions:
  const double traction_wheel_cur_pos = traction_wheel_pos * wheel_radius_;
  const double traction_wheel_est_pos_diff = traction_wheel_cur_pos - traction_wheel_old_pos_;

  /// Update old position with current:
  traction_wheel_old_pos_ = traction_wheel_cur_pos;

  /// Compute linear and angular diff:
  const double linear_velocity = traction_wheel_est_pos_diff / dt;
  steer_pos_ = steer_pos;
  const double angular = tan(steer_pos) * linear_velocity / wheelbase_;

  return update_odometry(linear_velocity, angular, dt);
}

bool SteeringOdometry::update_from_position(
  const double traction_right_wheel_pos, const double traction_left_wheel_pos,
  const double steer_pos, const double dt)
{
  /// Get current wheel joint positions:
  const double traction_right_wheel_cur_pos = traction_right_wheel_pos * wheel_radius_;
  const double traction_left_wheel_cur_pos = traction_left_wheel_pos * wheel_radius_;

  const double traction_right_wheel_est_pos_diff =
    traction_right_wheel_cur_pos - traction_right_wheel_old_pos_;
  const double traction_left_wheel_est_pos_diff =
    traction_left_wheel_cur_pos - traction_left_wheel_old_pos_;

  /// Update old position with current:
  traction_right_wheel_old_pos_ = traction_right_wheel_cur_pos;
  traction_left_wheel_old_pos_ = traction_left_wheel_cur_pos;

  const double linear_velocity =
    (traction_right_wheel_est_pos_diff + traction_left_wheel_est_pos_diff) * 0.5 / dt;
  steer_pos_ = steer_pos;
  const double angular = tan(steer_pos_) * linear_velocity / wheelbase_;

  return update_odometry(linear_velocity, angular, dt);
}

bool SteeringOdometry::update_from_position(
  const double traction_right_wheel_pos, const double traction_left_wheel_pos,
  const double right_steer_pos, const double left_steer_pos, const double dt)
{
  /// Get current wheel joint positions:
  const double traction_right_wheel_cur_pos = traction_right_wheel_pos * wheel_radius_;
  const double traction_left_wheel_cur_pos = traction_left_wheel_pos * wheel_radius_;

  const double traction_right_wheel_est_pos_diff =
    traction_right_wheel_cur_pos - traction_right_wheel_old_pos_;
  const double traction_left_wheel_est_pos_diff =
    traction_left_wheel_cur_pos - traction_left_wheel_old_pos_;

  /// Update old position with current:
  traction_right_wheel_old_pos_ = traction_right_wheel_cur_pos;
  traction_left_wheel_old_pos_ = traction_left_wheel_cur_pos;

  /// Compute linear and angular diff:
  const double linear_velocity =
    (traction_right_wheel_est_pos_diff + traction_left_wheel_est_pos_diff) * 0.5 / dt;
  steer_pos_ = (right_steer_pos + left_steer_pos) * 0.5;
  const double angular = tan(steer_pos_) * linear_velocity / wheelbase_;

  return update_odometry(linear_velocity, angular, dt);
}

bool SteeringOdometry::update_four_steering(
  const double fr_speed, const double fl_speed, const double rr_speed,
  const double rl_speed, const double front_steering, const double rear_steering, const double dt)
{
  const double front_tmp = cos(front_steering) * (tan(front_steering) - tan(rear_steering)) / wheelbase_;

  const double front_left_tmp = front_tmp / sqrt(1 - wheel_track_ * front_tmp * cos(front_steering)
                                              +pow(wheel_track_*front_tmp / 2, 2));
  const double front_right_tmp = front_tmp / sqrt(1 + wheel_track_ * front_tmp * cos(front_steering)
                                              +pow(wheel_track_*front_tmp / 2, 2));

  const double fl_speed_tmp = fl_speed * (1 / (1 - y_steering_offset_ * front_left_tmp));
  const double fr_speed_tmp = fr_speed * (1 / (1 - y_steering_offset_ * front_right_tmp));

  const double front_linear_speed = wheel_radius_ * copysign(1.0, fl_speed_tmp + fr_speed_tmp) *
      sqrt((pow(fl_speed, 2) + pow(fr_speed, 2)) / (2 + pow(wheel_track_ * front_tmp, 2) / 2.0));

  const double rear_tmp = cos(rear_steering) * (tan(front_steering) - tan(rear_steering)) / wheelbase_;

  const double rear_left_tmp = rear_tmp / sqrt(1 - wheel_track_ * rear_tmp * cos(rear_steering)
                                              + pow(wheel_track_ * rear_tmp/ 2, 2));
  const double rear_right_tmp = rear_tmp / sqrt(1 + wheel_track_ * rear_tmp * cos(rear_steering)
                                              +pow(wheel_track_ * rear_tmp/ 2, 2));

  const double rl_speed_tmp = rl_speed * (1 / (1 - y_steering_offset_ * rear_left_tmp));
  const double rr_speed_tmp = rr_speed * (1 / (1 - y_steering_offset_ * rear_right_tmp));

  const double rear_linear_speed = wheel_radius_ * copysign(1.0, rl_speed_tmp + rr_speed_tmp)*
      sqrt((pow(rl_speed_tmp, 2) + pow(rr_speed_tmp, 2)) / (2 + pow(wheel_track_ * rear_tmp, 2) / 2.0));

  angular_ = (front_linear_speed * front_tmp + rear_linear_speed * rear_tmp) / 2.0;

  const double linear_x_ = (front_linear_speed * cos(front_steering) + rear_linear_speed * cos(rear_steering)) / 2.0;
  const double linear_y_ = (front_linear_speed * sin(front_steering) - wheelbase_ * angular_ / 2.0
              + rear_linear_speed * sin(rear_steering) + wheelbase_ * angular_ / 2.0) / 2.0;

  const double linear_velocity =  copysign(1.0, rear_linear_speed)*sqrt(pow(linear_x_,2)+pow(linear_y_,2));

  /// Integrate odometry:
  // integrateXY(linear_x_*dt, linear_y_*dt, angular_*dt);

  // linear_accel_acc_((linear_vel_prev_ - linear_velocity)/dt);
  // linear_vel_prev_ = linear_velocity;
  // linear_jerk_acc_((linear_accel_prev_ - bacc::rolling_mean(linear_accel_acc_))/dt);
  // linear_accel_prev_ = bacc::rolling_mean(linear_accel_acc_);
  // front_steer_vel_acc_((front_steer_vel_prev_ - front_steering)/dt);
  // front_steer_vel_prev_ = front_steering;
  // rear_steer_vel_acc_((rear_steer_vel_prev_ - rear_steering)/dt);
  // rear_steer_vel_prev_ = rear_steering;
  return update_odometry(linear_velocity, angular_, dt);
}

bool SteeringOdometry::update_from_velocity(
  const double traction_wheel_vel, const double steer_pos, const double dt)
{
  steer_pos_ = steer_pos;
  double linear_velocity = traction_wheel_vel * wheel_radius_;
  const double angular = tan(steer_pos) * linear_velocity / wheelbase_;

  return update_odometry(linear_velocity, angular, dt);
}

bool SteeringOdometry::update_from_velocity(
  const double right_traction_wheel_vel, const double left_traction_wheel_vel,
  const double steer_pos, const double dt)
{
  double linear_velocity =
    (right_traction_wheel_vel + left_traction_wheel_vel) * wheel_radius_ * 0.5;
  steer_pos_ = steer_pos;

  const double angular = tan(steer_pos_) * linear_velocity / wheelbase_;

  return update_odometry(linear_velocity, angular, dt);
}

bool SteeringOdometry::update_from_velocity(
  const double right_traction_wheel_vel, const double left_traction_wheel_vel,
  const double right_steer_pos, const double left_steer_pos, const double dt)
{
  steer_pos_ = (right_steer_pos + left_steer_pos) * 0.5;
  double linear_velocity =
    (right_traction_wheel_vel + left_traction_wheel_vel) * wheel_radius_ * 0.5;
  const double angular = steer_pos_ * linear_velocity / wheelbase_;

  return update_odometry(linear_velocity, angular, dt);
}

void SteeringOdometry::update_open_loop(const double linear, const double angular, const double dt)
{
  /// Save last linear and angular velocity:
  linear_ = linear;
  angular_ = angular;

  /// Integrate odometry:
  SteeringOdometry::integrate_exact(linear * dt, angular * dt);
}

void SteeringOdometry::set_wheel_params(double wheel_radius, double wheelbase, double wheel_track)
{
  wheel_radius_ = wheel_radius;
  wheelbase_ = wheelbase;
  wheel_track_ = wheel_track;
}

void SteeringOdometry::set_wheel_params(double wheel_radius, double wheelbase, double wheel_track, double y_steering_offset)
{
  wheel_radius_ = wheel_radius;
  wheelbase_ = wheelbase;
  wheel_track_ = wheel_track;
  y_steering_offset_ = y_steering_offset;
}

void SteeringOdometry::set_velocity_rolling_window_size(size_t velocity_rolling_window_size)
{
  velocity_rolling_window_size_ = velocity_rolling_window_size;

  reset_accumulators();
}

void SteeringOdometry::set_odometry_type(const unsigned int type) { config_type_ = type; }

double SteeringOdometry::convert_trans_rot_vel_to_steering_angle(double Vx, double theta_dot)
{
  if (theta_dot == 0 || Vx == 0)
  {
    return 0;
  }
  return std::atan(theta_dot * wheelbase_ / Vx);
}

std::tuple<std::vector<double>, std::vector<double>> SteeringOdometry::get_commands(
  double Vx, double angular, bool from_twist)
{
  // desired velocity and steering angle of the middle of traction and steering axis
  double Ws, alpha;

  if (from_twist)
  {
    if (Vx == 0 && angular != 0)
    {
      //turning on the spot
      alpha = angular > 0 ? M_PI_2 : -M_PI_2;
      Ws = abs(angular) * wheelbase_ / wheel_radius_;
    }
    else
    {
      alpha = SteeringOdometry::convert_trans_rot_vel_to_steering_angle(Vx, angular);
      Ws = Vx / (wheel_radius_ * std::cos(steer_pos_));
    }
  }
  else
  {
    alpha = angular;
  }


  if (config_type_ == BICYCLE_CONFIG)
  {
    std::vector<double> traction_commands = {Ws};
    std::vector<double> steering_commands = {alpha};
    return std::make_tuple(traction_commands, steering_commands);
  }
  else if (config_type_ == TRICYCLE_CONFIG)
  {
    std::vector<double> traction_commands;
    std::vector<double> steering_commands;
    if (fabs(steer_pos_) < 1e-6)
    {
      traction_commands = {Ws, Ws};
    }
    else
    {
      double turning_radius = wheelbase_ / std::tan(steer_pos_);
      double Wr = Ws * (turning_radius + wheel_track_ * 0.5) / turning_radius;
      double Wl = Ws * (turning_radius - wheel_track_ * 0.5) / turning_radius;
      traction_commands = {Wr, Wl};
    }
    steering_commands = {alpha};
    return std::make_tuple(traction_commands, steering_commands);
  }
  else if (config_type_ == ACKERMANN_CONFIG)
  {
    std::vector<double> traction_commands;
    std::vector<double> steering_commands;
    if (fabs(steer_pos_) < 1e-6)
    {
      traction_commands = {Ws, Ws};
      steering_commands = {alpha, alpha};
    }
    else
    {
      double turning_radius = wheelbase_ / std::tan(steer_pos_);
      double Wr = Ws * (turning_radius + wheel_track_ * 0.5) / turning_radius;
      double Wl = Ws * (turning_radius - wheel_track_ * 0.5) / turning_radius;
      traction_commands = {Wr, Wl};

      double numerator = 2 * wheelbase_ * std::sin(alpha);
      double denominator_first_member = 2 * wheelbase_ * std::cos(alpha);
      double denominator_second_member = wheel_track_ * std::sin(alpha);

      double alpha_r = std::atan2(numerator, denominator_first_member - denominator_second_member);
      double alpha_l = std::atan2(numerator, denominator_first_member + denominator_second_member);
      steering_commands = {alpha_r, alpha_l};
    }
    return std::make_tuple(traction_commands, steering_commands);
  }
  else if (config_type_ == FOUR_STEERING_CONFIG) 
  {
    std::vector<double> traction_commands;
    std::vector<double> steering_commands;
    
    double steering_track = wheel_track_- 2 * y_steering_offset_;
    double vel_steering_offset = (alpha * y_steering_offset_) / wheel_radius_;
    double sign = copysign(1.0, Ws);
    double vel_left_front  = sign * std::hypot((Ws - alpha * steering_track / 2),
                                        (wheelbase_ * alpha / 2.0)) / wheel_radius_
                      - vel_steering_offset;
    double vel_right_front = sign * std::hypot((Ws + alpha * steering_track / 2),
                                        (wheelbase_ * alpha / 2.0)) / wheel_radius_
                      + vel_steering_offset;
    double vel_left_rear = sign * std::hypot((Ws - alpha * steering_track / 2),
                                      (wheelbase_ * alpha / 2.0)) / wheel_radius_
                    - vel_steering_offset;
    double vel_right_rear = sign * std::hypot((Ws + alpha * steering_track / 2),
                                        (wheelbase_ * alpha / 2.0)) / wheel_radius_
                      + vel_steering_offset;
    traction_commands = {vel_left_front, vel_right_front, vel_left_rear, vel_right_rear};

    double front_left_steering = 0.0;
    double front_right_steering = 0.0;

    // Compute steering angles
    if(fabs(2.0 * Ws) > fabs(alpha * steering_track))
    {
      front_left_steering = atan(alpha * wheelbase_ /
                                  (2.0 * Ws - alpha * steering_track));
      front_right_steering = atan(alpha * wheelbase_ /
                                    (2.0 * Ws + alpha * steering_track));
    }
    else if(fabs(Ws) > 0.001)
    {
      front_left_steering = copysign(M_PI_2, alpha);
      front_right_steering = copysign(M_PI_2, alpha);
    }

    double rear_left_steering = -front_left_steering;
    double rear_right_steering = -front_right_steering;
    steering_commands = {front_left_steering, front_right_steering, rear_left_steering, rear_right_steering};

    return std::make_tuple(traction_commands, steering_commands);
  }
  else
  {
    throw std::runtime_error("Config not implemented");
  }
}

void SteeringOdometry::reset_odometry()
{
  x_ = 0.0;
  y_ = 0.0;
  heading_ = 0.0;
  reset_accumulators();
}

void SteeringOdometry::integrate_runge_kutta_2(double linear, double angular)
{
  const double direction = heading_ + angular * 0.5;

  /// Runge-Kutta 2nd order integration:
  x_ += linear * cos(direction);
  y_ += linear * sin(direction);
  heading_ += angular;
}

/**
 * \brief Other possible integration method provided by the class
 * \param linear
 * \param angular
 */
void SteeringOdometry::integrate_exact(double linear, double angular)
{
  if (fabs(angular) < 1e-6)
  {
    integrate_runge_kutta_2(linear, angular);
  }
  else
  {
    /// Exact integration (should solve problems when angular is zero):
    const double heading_old = heading_;
    const double r = linear / angular;
    heading_ += angular;
    x_ += r * (sin(heading_) - sin(heading_old));
    y_ += -r * (cos(heading_) - cos(heading_old));
  }
}

void SteeringOdometry::reset_accumulators()
{
  linear_acc_ = rcppmath::RollingMeanAccumulator<double>(velocity_rolling_window_size_);
  angular_acc_ = rcppmath::RollingMeanAccumulator<double>(velocity_rolling_window_size_);
}

}  // namespace steering_odometry
