/*
 GEPUtilsGeometry.h

 First DX12 Renderer - https://github.com/logins/FirstDX12Renderer

 MIT License - Copyright (c) 2021 Riccardo Loggini
*/

#ifndef GEPUtilsGeometry_h__
#define GEPUtilsGeometry_h__

#include <Eigen/Geometry>

namespace GEPUtils {
	namespace Geometry {

		Eigen::Matrix4f Perspective(float InZNear, float InZFar, float InAspectRatio, float InFovYRad);

		Eigen::Matrix4f LookAt(const Eigen::Vector3f& eye, const Eigen::Vector3f& center, const Eigen::Vector3f& up);

	}
}

#endif // Geometry3DGEP_h__
