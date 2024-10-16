#include "runtime/function/render/render_camera.h"

namespace Dao {

	void RenderCamera::setCurrentCameraType(RenderCameraType type) {
		std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex);
		m_current_render_camera_type = type;
	}

	void RenderCamera::setMainViewMatrix(const Matrix4x4& view_matrix, RenderCameraType type) {
		std::lock_guard<std::mutex> loc_guard(m_view_matrix_mutex);
		m_current_render_camera_type = type;
		m_view_matrices[MAIN_VIEW_MATRIX_INDEX] = view_matrix;
		Vector3 s = Vector3(view_matrix[0][0], view_matrix[0][1], view_matrix[0][2]);
		Vector3 u = Vector3(view_matrix[1][0], view_matrix[1][1], view_matrix[1][2]);
		Vector3 f = Vector3(view_matrix[2][0], view_matrix[2][1], view_matrix[2][2]);
		m_position = s * (-view_matrix[0][3]) + u * (-view_matrix[1][3]) + f * (-view_matrix[2][3]);
	}

	void RenderCamera::move(Vector3 delta) {
		m_position += delta;
	}

	void RenderCamera::rotate(Vector2 delta) {
		delta = Vector2(Radian(Degree(delta.x)).valueRadians(), Radian(Degree(delta.y)).valueRadians());
		float dot = m_up_axis.dotProduct(forward());
		if ((dot < -0.99f && delta.x>0.0f) || (dot > 0.99f && delta.x < 0.0f)) {
			delta.x = 0.0f;
		}
		Quaternion pitch, yaw;
		pitch.fromAngleAxis(Radian(delta.x), X);
		yaw.fromAngleAxis(Radian(delta.y), Z);
		
		m_rotation = pitch * m_rotation * yaw;
		m_inv_rotation = m_rotation.conjugate();
	}

	void RenderCamera::zoom(float offset) {
		m_fovx = Math::clamp(m_fovx - offset, MIN_FOV, MAX_FOV);
	}

	void RenderCamera::lookAt(const Vector3& position, const Vector3& target, const Vector3& up) {
		m_position = position;
		Vector3 forward = (target - position).normalisedCopy();
		m_rotation = forward.getRotationTo(Y);
		Vector3 right = forward.crossProduct(up.normalisedCopy()).normalisedCopy();
		Vector3 orth_up = right.crossProduct(forward);
		Quaternion up_rotation = (m_rotation * orth_up).getRotationTo(Z);
		m_rotation = Quaternion(up_rotation) * m_rotation;
		m_inv_rotation = m_rotation.conjugate();
	}

	Matrix4x4 RenderCamera::getViewMatrix() {
		std::lock_guard<std::mutex> lock_guard(m_view_matrix_mutex);
		auto view_matrix = Matrix4x4::IDENTITY;
		
		switch (m_current_render_camera_type)
		{
		case Dao::RenderCameraType::EDITOR:
			view_matrix = Math::makeLookAtMatrix(position(), position() + forward(), up());
			break;
		case Dao::RenderCameraType::MOTOR:
			view_matrix = m_view_matrices[MAIN_VIEW_MATRIX_INDEX];
			break;
		default:
			break;
		}
		
		return view_matrix;
	}

	Matrix4x4 RenderCamera::getPersProjMatrix() const {
		Matrix4x4 fix_mat(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
		Matrix4x4 proj_mat = fix_mat * Math::makePerspectiveMatrix(Radian(Degree(m_fovy)), m_aspect, m_znear, m_zfar);
		return proj_mat;
	}

	void RenderCamera::setAspect(float aspect) {
		m_aspect = aspect;
		m_fovy = Radian(Math::atan(Math::tan(Radian(Degree(m_fovx) * 0.5f)) / m_aspect) * 2.0f).valueDegrees();
	}
}