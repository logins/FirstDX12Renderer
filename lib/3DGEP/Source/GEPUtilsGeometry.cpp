#include "GEPUtilsGeometry.h"

namespace GEPUtils {
	namespace Geometry {

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

		Eigen::Matrix4f Perspective(float InZNear, float InZFar, float InAspectRatio, float InFovYRad) // TODO Change this to a non-template function
		{
			// FROM: https://docs.microsoft.com/en-us/windows/win32/direct3d9/projection-transform
			// Note: Using the Perspective and LookAt from OpenGL in D3D will render the back faces !!!
			Eigen::Transform<float, 3, Eigen::Projective> tr;
			tr.matrix().setZero();
			assert(InZFar > InZNear);
			float h = 1.0f / std::tan(InFovYRad / 2.f);
			float w = 1.0f / (InAspectRatio * std::tan(InFovYRad / 2.f));
			float Q = InZFar / (InZFar - InZNear);
			tr(0, 0) = w;
			tr(1, 1) = h;
			tr(2, 2) = Q;
			tr(3, 2) = 1.0f;
			tr(2, 3) = -Q * InZNear;
			
			return tr.matrix();
		}

		Eigen::Matrix4f LookAt(const Eigen::Vector3f& eye, const Eigen::Vector3f& center, const Eigen::Vector3f& up)
		{
			// FROM: https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
			// Note: Using the Perspective and LookAt from OpenGL in D3D will render the back faces !!!
			typedef Eigen::Matrix<float, 4, 4> Matrix4;
			typedef Eigen::Matrix<float, 3, 1> Vector3;
			Matrix4 mat = Matrix4::Zero();
			Vector3 zAxis = (center - eye).normalized();
			Vector3 xAxis = up.cross(zAxis).normalized();
			Vector3 yAxis = zAxis.cross(xAxis).normalized();
			mat(0, 0) = xAxis.x();
			mat(0, 1) = xAxis.y();
			mat(0, 2) = xAxis.z();
			mat(0, 3) = -xAxis.dot(eye);
			mat(1, 0) = yAxis.x();
			mat(1, 1) = yAxis.y();
			mat(1, 2) = yAxis.z();
			mat(1, 3) = -yAxis.dot(eye);
			mat(2, 0) = zAxis.x();
			mat(2, 1) = zAxis.y();
			mat(2, 2) = zAxis.z();
			mat(2, 3) = -zAxis.dot(eye);
			mat.row(3) << 0, 0, 0, 1;
			return mat;
		}

		// Note: before these functions were templated, with an explicit template instantiation.
		// Explicit template instantiation, this makes possible defining templated function bodies in .cpp files
		// https://stackoverflow.com/questions/115703/storing-c-template-function-definitions-in-a-cpp-file
		// The downside is that we need to instantiate the template for each type we need.

	}
}

