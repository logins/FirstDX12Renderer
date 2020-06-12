
#ifndef Geometry3DGEP_h__
#define Geometry3DGEP_h__

#include <Eigen/Geometry>


namespace GEPUtils {
	namespace Geometry {


		/// @brief Returns a perspective transformation matrix like the one from gluPerspective
		/// @see http://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
		/// @see glm::perspective
		template<typename Scalar>
		Eigen::Matrix<Scalar, 4, 4> Perspective(Scalar InZNear, Scalar InZFar, Scalar InAspectRatio, Scalar InFovYRad);


		/// @brief Returns a view transformation matrix like the one from glu's lookAt
		/// @see http://www.opengl.org/sdk/docs/man2/xhtml/gluLookAt.xml
		/// @see glm::lookAt
		template<typename Derived>
		Eigen::Matrix<typename Derived::Scalar, 4, 4> LookAt(Derived const& eye, Derived const& center, Derived const& up);

	}
}

#endif // Geometry3DGEP_h__
