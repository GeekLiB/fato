/*****************************************************************************/
/*  Copyright (c) 2016, Alessandro Pieropan                                  */
/*  All rights reserved.                                                     */
/*                                                                           */
/*  Redistribution and use in source and binary forms, with or without       */
/*  modification, are permitted provided that the following conditions       */
/*  are met:                                                                 */
/*                                                                           */
/*  1. Redistributions of source code must retain the above copyright        */
/*  notice, this list of conditions and the following disclaimer.            */
/*                                                                           */
/*  2. Redistributions in binary form must reproduce the above copyright     */
/*  notice, this list of conditions and the following disclaimer in the      */
/*  documentation and/or other materials provided with the distribution.     */
/*                                                                           */
/*  3. Neither the name of the copyright holder nor the names of its         */
/*  contributors may be used to endorse or promote products derived from     */
/*  this software without specific prior written permission.                 */
/*                                                                           */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS      */
/*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT        */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR    */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT     */
/*  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   */
/*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT         */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,    */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY    */
/*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT      */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     */
/*****************************************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/core/core.hpp>
#include <message_filters/time_synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <image_transport/image_transport.h>
#include <image_transport/subscriber_filter.h>
#include <sensor_msgs/CameraInfo.h>
#include <message_filters/subscriber.h>
#include <math.h>

#include <device_1d.h>
#include <hdf5_file.h>
#include <utilities.h>
#include "../../fato_rendering/include/multiple_rigid_models_ogre.h"
#include "../../fato_rendering/include/windowless_gl_context.h"
#include <utility_kernels.h>
#include <utility_kernels_pose.h>

using namespace std;

ros::Publisher pose_publisher, rendering_publisher;

cv::Mat camera_image;
string img_topic, info_topic, model_name, obj_file;

std::vector<Eigen::Vector3f> translations;
std::vector<Eigen::Vector3f> rotations;

auto downloadRenderedImg = [](pose::MultipleRigidModelsOgre &model_ogre,
                              std::vector<uchar4> &h_texture) {
  util::Device1D<uchar4> d_texture(480 * 640);
  vision::convertFloatArrayToGrayRGBA(d_texture.data(), model_ogre.getTexture(),
                                      640, 480, 1.0, 2.0);
  h_texture.resize(480 * 640);
  d_texture.copyTo(h_texture);
};

class SyntheticRenderer {
 public:
  void rgbCallback(const sensor_msgs::ImageConstPtr &rgb_msg,
                   const sensor_msgs::CameraInfoConstPtr &camera_info_msg) {
    cv_bridge::CvImageConstPtr pCvImage;
    pCvImage = cv_bridge::toCvShare(rgb_msg, rgb_msg->encoding);
    pCvImage->image.copyTo(camera_image);
    // ROS_INFO("Img received ");

    if (!initialized_) {
      cv::Mat cam =
          cv::Mat(3, 4, CV_64F, (void *)camera_info_msg->P.data()).clone();
      rendering_engine = unique_ptr<pose::MultipleRigidModelsOgre>(
          new pose::MultipleRigidModelsOgre(
              camera_image.cols, camera_image.rows, cam.at<double>(0, 0),
              cam.at<double>(1, 1), cam.at<double>(0, 2), cam.at<double>(1, 2),
              0.01, 10.0));
      rendering_engine->addModel(obj_file);

      initialized_ = true;
    }

    image_updated = true;

  }

  void initRGB(ros::NodeHandle &nh) {
    rgb_it_.reset(new image_transport::ImageTransport(nh));
    info_sub_.subscribe(nh, info_topic, 1);

    img_sub_.subscribe(*rgb_it_, img_topic, 1,
                       image_transport::TransportHints("compressed"));

    sync_rgb_.reset(new SynchronizerRGB(SyncPolicyRGB(5), img_sub_, info_sub_));

    sync_rgb_->registerCallback(
        boost::bind(&SyntheticRenderer::rgbCallback, this, _1, _2));
  }

  void run() {
    ros::Rate r(100);

    int frame_counter = 0;
    int fst_id = 0;
    int scd_id = 1;

    float t_inc = 0.02 / 30.0;
    Eigen::Vector3f pos(0,0,0.5);
    Eigen::Vector3f inc(t_inc,0,0);

    while (ros::ok()) {

      Eigen::Vector3f t_inc;

      if(image_updated)
      {
          frame_counter++;
          pos += inc;

          if(frame_counter % 240 == 0)
          {
              inc = inc * -1.0;
          }

          double T[] = {pos[0], pos[1], pos[2]};
          double R[] = {M_PI, 0, 0};

          std::vector<pose::TranslationRotation3D> TR(1);
          TR.at(0) = pose::TranslationRotation3D(T, R);
          rendering_engine->render(TR);

          std::vector<uchar4> h_texture(camera_image.rows * camera_image.cols);
          downloadRenderedImg(*rendering_engine, h_texture);

          cv::Mat img_rgba(camera_image.rows, camera_image.cols, CV_8UC4,
                           h_texture.data());

          cv::Mat res;
          cv::cvtColor(img_rgba, res, CV_RGBA2BGR);

          for (auto i = 0; i < camera_image.rows; ++i) {
            for (auto j = 0; j < camera_image.cols; ++j) {
              cv::Vec3b &pixel = res.at<cv::Vec3b>(i, j);
              if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
                cv::Vec3b &cp = camera_image.at<cv::Vec3b>(i, j);
                pixel[0] = cp[2];
                pixel[1] = cp[1];
                pixel[2] = cp[0];
              }
            }
          }

          cv_bridge::CvImage cv_img, cv_rend;
          cv_img.image = res;
          cv_img.encoding = sensor_msgs::image_encodings::BGR8;
          rendering_publisher.publish(cv_img.toImageMsg());

          image_updated = false;
      }


      r.sleep();
      ros::spinOnce();
    }
  }

 private:
  message_filters::Subscriber<sensor_msgs::CameraInfo> info_sub_;
  image_transport::SubscriberFilter img_sub_;
  boost::shared_ptr<image_transport::ImageTransport> rgb_it_;

  typedef message_filters::sync_policies::ApproximateTime<
      sensor_msgs::Image, sensor_msgs::CameraInfo> SyncPolicyRGB;

  typedef message_filters::Synchronizer<SyncPolicyRGB> SynchronizerRGB;
  boost::shared_ptr<SynchronizerRGB> sync_rgb_;

  bool initialized_, image_updated;
  unique_ptr<pose::MultipleRigidModelsOgre> rendering_engine;
};

void simpleMovement() {
  translations.push_back(Eigen::Vector3f(0, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.02, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.04, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.06, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.08, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.10, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.08, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.06, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.04, 0, 0.5));
  translations.push_back(Eigen::Vector3f(0.02, 0, 0.5));

  float deg2rad = M_PI / 180;

  rotations.resize(10, Eigen::Vector3f(0, 90 * deg2rad, 0));
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "fato_synthetic_scene");

  ros::NodeHandle nh;

  // Create dummy GL context before cudaGL init
  render::WindowLessGLContext dummy(10, 10);

  if (!ros::param::get("fato/camera/image_topic", img_topic)) {
    throw std::runtime_error("cannot read h5 file param");
  }

  if (!ros::param::get("fato_camera/info_topic", info_topic)) {
    throw std::runtime_error("cannot read obj file param");
  }

  if (!ros::param::get("fato/model/h5_file", model_name)) {
    throw std::runtime_error("cannot read h5 file param");
  }

  if (!ros::param::get("fato/model/obj_file", obj_file)) {
    throw std::runtime_error("cannot read obj file param");
  }

  cout << "img_topic " << img_topic << endl;
  cout << "info_topic " << info_topic << endl;

  rendering_publisher =
      nh.advertise<sensor_msgs::Image>("fato_tracker/synthetic", 1);

  SyntheticRenderer sr;

  sr.initRGB(nh);

  sr.run();

  ros::shutdown();

  return 0;
}
