/*****************************************************************************/
/*  Copyright (c) 2015, Alessandro Pieropan                                  */
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

#include "../include/model.h"

using namespace Eigen;
using namespace std;
using namespace cv;

namespace fato{


ObjectModel::ObjectModel() :
	m_center(),
	m_width(0),
	m_height(0),
	m_depth(0),
	m_isLearned(6, false),
	m_appearanceRatio(6, 0),
	m_eigNormals(6, 3)
{
	

	initNormals();

	m_cloudPoints = vector< vector<Point3f> >(6, vector<Point3f>());

	m_pointStatus = vector< vector<FatoStatus> >(6,vector<FatoStatus>());

	m_faceDescriptors = vector<Mat>(6, Mat());

	m_faceKeypoints = vector< vector<KeyPoint> >(6, vector<KeyPoint>());

	m_relativePointsPos = vector< vector<Point3f> >(6, vector<Point3f>());

}

ObjectModel::~ObjectModel()
{
}

void ObjectModel::restCube()
{
    initNormals();

    m_cloudPoints = vector< vector<Point3f> >(6, vector<Point3f>());

    m_pointStatus = vector< vector<FatoStatus> >(6,vector<FatoStatus>());

    m_faceDescriptors = vector<Mat>(6, Mat());

    m_faceKeypoints = vector< vector<KeyPoint> >(6, vector<KeyPoint>());

    m_relativePointsPos = vector< vector<Point3f> >(6, vector<Point3f>());
}

void ObjectModel::initNormals()
{
	m_eigNormals(FACE::LEFT, 0) = -1;
	m_eigNormals(FACE::LEFT, 1) = 0;
	m_eigNormals(FACE::LEFT, 2) = 0;

	m_eigNormals(FACE::RIGHT, 0) = 1;
	m_eigNormals(FACE::RIGHT, 1) = 0;
	m_eigNormals(FACE::RIGHT, 2) = 0;

	m_eigNormals(FACE::TOP, 0) = 0;
	m_eigNormals(FACE::TOP, 1) = -1;
	m_eigNormals(FACE::TOP, 2) = 0;

	m_eigNormals(FACE::DOWN, 0) = 0;
	m_eigNormals(FACE::DOWN, 1) = 1;
	m_eigNormals(FACE::DOWN, 2) = 0;

	m_eigNormals(FACE::FRONT, 0) = 0;
	m_eigNormals(FACE::FRONT, 1) = 0;
	m_eigNormals(FACE::FRONT, 2) = 1;

	m_eigNormals(FACE::BACK, 0) = 0;
	m_eigNormals(FACE::BACK, 1) = 0;
	m_eigNormals(FACE::BACK, 2) = -1;
	
	m_faceNormals = Mat(6, 3, CV_32FC1);
	m_faceNormals.at<float>(FACE::FRONT, 0) = 0;
	m_faceNormals.at<float>(FACE::FRONT, 1) = 0;
	m_faceNormals.at<float>(FACE::FRONT, 2) = 1;

	m_faceNormals.at<float>(FACE::RIGHT, 0) = -1;
	m_faceNormals.at<float>(FACE::RIGHT, 1) = 0;
	m_faceNormals.at<float>(FACE::RIGHT, 2) = 0;

	m_faceNormals.at<float>(FACE::LEFT, 0) = 1;
	m_faceNormals.at<float>(FACE::LEFT, 1) = 0;
	m_faceNormals.at<float>(FACE::LEFT, 2) = 0;

	m_faceNormals.at<float>(FACE::BACK, 0) = 0;
	m_faceNormals.at<float>(FACE::BACK, 1) = 0;
	m_faceNormals.at<float>(FACE::BACK, 2) = -1;

	m_faceNormals.at<float>(FACE::TOP, 0) = 0;
	m_faceNormals.at<float>(FACE::TOP, 1) = -1;
	m_faceNormals.at<float>(FACE::TOP, 2) = 0;

	m_faceNormals.at<float>(FACE::DOWN, 0) = 0;
	m_faceNormals.at<float>(FACE::DOWN, 1) = 1;
	m_faceNormals.at<float>(FACE::DOWN, 2) = 0;
	
}

void ObjectModel::initCube(Point3f& centroid, vector<Point3f>& front, vector<Point3f>& back)
{
	/*m_pointsFront = front;
	m_pointsBack = back;

	m_width = (front[1].x - front[0].x);
	m_height = (front[2].y - front[0].y);
	m_depth = (back[0].z - front[0].z);

	m_center = centroid;*/

	//m_faceNormalPoints = Mat(6, 3, CV_32FC1);

	/*m_faceNormalPoints.at<float>(FACE::FRONT, 0) = front[0].x + width;
	m_faceNormalPoints.at<float>(FACE::FRONT, 1) = front[0].y + height;
	m_faceNormalPoints.at<float>(FACE::FRONT, 2) = front[0].z;

	m_faceNormalPoints.at<float>(FACE::BACK, 0) = back[0].x + width;
	m_faceNormalPoints.at<float>(FACE::BACK, 1) = back[0].y + height;
	m_faceNormalPoints.at<float>(FACE::BACK, 2) = back[0].z;

	m_faceNormalPoints.at<float>(FACE::RIGHT, 0) = front[0].x;
	m_faceNormalPoints.at<float>(FACE::RIGHT, 1) = front[0].y + height;
	m_faceNormalPoints.at<float>(FACE::RIGHT, 2) = front[0].z + depth;

	m_faceNormalPoints.at<float>(FACE::LEFT, 0) = front[2].x;
	m_faceNormalPoints.at<float>(FACE::LEFT, 1) = front[0].y + height;
	m_faceNormalPoints.at<float>(FACE::LEFT, 2) = front[0].z + depth;

	m_faceNormalPoints.at<float>(FACE::TOP, 0) = front[0].x + width;
	m_faceNormalPoints.at<float>(FACE::TOP, 1) = front[0].y;
	m_faceNormalPoints.at<float>(FACE::TOP, 2) = front[0].z + depth;

	m_faceNormalPoints.at<float>(FACE::DOWN, 0) = front[0].x + width;
	m_faceNormalPoints.at<float>(FACE::DOWN, 1) = front[2].y;
	m_faceNormalPoints.at<float>(FACE::DOWN, 2) = front[0].z + depth;*/
}

vector<bool> ObjectModel::getVisibility(const Mat& pov)
{
	vector<bool> isVisible(6, false);
	/*
	Mat normals_T;
	transpose(m_faceNormals, normals_T);

	Mat res = pov * normals_T;

	for (size_t i = 0; i < 6; ++i)
	{
		if (res.at<float>(i) > 0)
			isVisible[i] = true;

		//if (val > 0)
		//	isVisible[i] = true;
	}
	*/
	return isVisible;
	
}

vector<Point3f> ObjectModel::getFacePoints(int face)
{
	vector<Point3f> tmp;

	if (face == FACE::FRONT)
	{
		tmp = m_pointsFront;
	}
	else if (face == FACE::BACK)
	{
		tmp = m_pointsBack;
	}
	else if (face == FACE::RIGHT)
	{
		tmp.push_back(m_pointsFront[0]);
		tmp.push_back(m_pointsBack[0]);
		tmp.push_back(m_pointsBack[3]);
		tmp.push_back(m_pointsFront[3]);
	}
	else if (face == FACE::LEFT)
	{
		tmp.push_back(m_pointsFront[1]);
		tmp.push_back(m_pointsBack[1]);
		tmp.push_back(m_pointsBack[2]);
		tmp.push_back(m_pointsFront[2]);
	}
	else if (face == FACE::DOWN)
	{
		tmp.push_back(m_pointsFront[0]);
		tmp.push_back(m_pointsBack[0]);
		tmp.push_back(m_pointsBack[1]);
		tmp.push_back(m_pointsFront[1]);
	}
	else if (face == FACE::TOP)
	{
		tmp.push_back(m_pointsFront[3]);
		tmp.push_back(m_pointsBack[3]);
		tmp.push_back(m_pointsBack[2]);
		tmp.push_back(m_pointsFront[2]);
	}

	return tmp;
}

} // end namespace
