#include "GEPUtilsGeometry.h"

namespace GEPUtils {
	namespace Geometry {

		// Note: these are the OpenGL implementations, maybe they can be optimized
		// https://eigen.tuxfamily.org/bz/show_bug.cgi?id=17

		// Note: Direct3D and OpenGL BOTH set the Matrix Packing Order for Uniform Parameters to Column-Major.
		// https://docs.microsoft.com/en-gb/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math?redirectedfrom=MSDN
		//
		// According to this source: https://antiagainst.github.io/post/hlsl-for-vulkan-matrices/#hlsl-matrices
		// HLSL will change the majorness if requested, ONLY for external data, the one read from GPU memory (so the one uploaded by our program), 
		// while the local variables will be always stored as Row Major.
		// Majorness only matters for external initialized matrices, 
		// because it controls how they transform from the storage to the mathematical form.
		//
		// The difference is: HLSL (D3D shader language) and DXMATH (DX Math Library) identify vectors as Row Vectors.
		//
		// Heigen, glm and Spir-V libraries store data in Column-Major by default! And every Vector is a Column by default.
		//
		// That does NOT impact matrix multiplication oder of operand.
		//
		// Instead, that DOES impact how we do algebra operations in our code, 
		// such as the vector Base Change operation: with Eigen, every change of base will need to be 
		// V1 = M V0 where M is the base change matrix, V0 the initial vector and V1 the vector in the new base.
		// That also means, Model(Mm) View(Mv) Projection(Mp) base change will need to be acted as: V1 = Mp Mv Mm V0.
		//

		// Note: Since we are going to use these functions with Derived = Eigen::Vector4f and Scalar = float, we might as well
		// make these not templated anymore.
		template<typename Scalar>
		Eigen::Matrix<Scalar, 4, 4>
			Perspective(Scalar fovy, Scalar aspect, Scalar zNear, Scalar zFar)
		{
			Transform<Scalar, 3, Projective> tr;
			tr.matrix().setZero();
			assert(aspect > 0);
			assert(zFar > zNear);
			Scalar radf = M_PI * fovy / 180.0;
			Scalar tan_half_fovy = std::tan(radf / 2.0);
			tr(0, 0) = 1.0 / (aspect * tan_half_fovy);
			tr(1, 1) = 1.0 / (tan_half_fovy);
			tr(2, 2) = -(zFar + zNear) / (zFar - zNear);
			tr(3, 2) = -1.0;
			tr(2, 3) = -(2.0 * zFar * zNear) / (zFar - zNear);
			return tr.matrix();
		}

		// TODO change this to a not-templated case with Vector3f inputs
		template<typename Derived>
		Eigen::Matrix<typename Derived::Scalar, 4, 4>
			LookAt(Derived const& eye, Derived const& center, Derived const& up)
		{
			typedef Eigen::Matrix<typename Derived::Scalar, 4, 4> Matrix4;
			typedef Eigen::Matrix<typename Derived::Scalar, 3, 1> Vector3;
			Vector3 f = (center - eye).normalized().head<3>();
			Vector3 u = up.normalized().head<3>();
			Vector3 s = f.cross(u).normalized().head<3>();
			u = s.cross(f);
			Matrix4 mat = Matrix4::Zero();
			mat(0, 0) = s.x();
			mat(0, 1) = s.y();
			mat(0, 2) = s.z();
			mat(0, 3) = -s.dot(eye.head<3>());
			mat(1, 0) = u.x();
			mat(1, 1) = u.y();
			mat(1, 2) = u.z();
			mat(1, 3) = -u.dot(eye.head<3>());
			mat(2, 0) = -f.x();
			mat(2, 1) = -f.y();
			mat(2, 2) = -f.z();
			mat(2, 3) = f.dot(eye.head<3>());
			mat.row(3) << 0, 0, 0, 1;
			return mat;
		}

		// Explicit template instantiation, this makes possible defining templated function bodies in .cpp files
		// https://stackoverflow.com/questions/115703/storing-c-template-function-definitions-in-a-cpp-file
		// The downside is that we need to instantiate the template for each type we need.
		template Eigen::Matrix<Eigen::Vector4f::Scalar, 4, 4> LookAt(const Eigen::Vector4f& eye, const Eigen::Vector4f& center, const Eigen::Vector4f& up);

	}
}

