#include "runtime/function/animation/node.h"

#include "runtime/core/base/macro.h"

namespace Dao {
	Node::Node(const std::string name) {
		m_name = name;
	}

	Node::~Node() {

	}

	void Node::clear() {

	}

	Node* Node::getParent() const {
		return m_parent;
	}

	void Node::setParent(Node* parent) {
		m_parent = parent;
		setDirty();
	}

	void Node::update() {
		updateDerivedTransform();
		m_is_dirty = false;
	}
	
	void Node::updateDerivedTransform() {
		if (m_parent) {
			const Quaternion& parentOrientation = m_parent->getDerivedOrientation();
			m_derived_orientation = parentOrientation * m_orientation;
			m_derived_orientation.normalise();

			const Vector3& parentScale = m_parent->getDerivedScale();
			m_derived_scale = parentScale * m_scale;

			m_derived_position = parentOrientation * (parentScale * m_position);

			m_derived_position = m_derived_position + m_parent->getDerivedPosition();
		}
		else {
			m_derived_orientation = m_orientation;
			m_derived_position = m_position;
			m_derived_scale = m_scale;
		}
	}

	const Quaternion& Node::getOrientation() const {
		return m_orientation;
	}

	void Node::setOrientation(const Quaternion& q) {
		if (q.isNaN()) {
			m_orientation = Quaternion::IDENTITY;
		}
		else {
			m_orientation = q;
			m_orientation.normalise();
		}
		setDirty();
	}

	void Node::resetOrientation() {
		m_orientation = Quaternion::IDENTITY;
		setDirty();
	}

	void Node::setPosition(const Vector3& pos) {
		if (pos.isNaN()) {
			LOG_ERROR("pos is nan")
		}
		m_position = pos;
		setDirty();
	}

	const Vector3& Node::getPosition() const {
		return m_position;
	}

	void Node::translate(const Vector3& d, TransformSpace relativeTo) {
		switch (relativeTo)
		{
		case Dao::Node::TransformSpace::LOCAL:
			m_position = m_position + m_orientation * d;
			break;
		case Dao::Node::TransformSpace::PARENT:
			m_position = m_position + d;
			break;
		case Dao::Node::TransformSpace::OBJECT:
			if (m_parent) {
				m_position = m_position + (m_parent->getDerivedOrientation().inverse() * d) / m_parent->getDerivedScale();
			}
			else {
				m_position = m_position + d;
			}
			break;
		default:
			break;
		}
		setDirty();
	}

	void Node::rotate(const Quaternion& q, TransformSpace relativeTo) {
		Quaternion qnorm = q;
		qnorm.normalise();

		switch (relativeTo)
		{
		case Dao::Node::TransformSpace::LOCAL:
			m_orientation = m_orientation * qnorm;
			break;
		case Dao::Node::TransformSpace::PARENT:
			m_orientation = qnorm * m_orientation;
			break;
		case Dao::Node::TransformSpace::OBJECT:
			m_orientation = m_orientation * getDerivedOrientation().inverse() * qnorm * getDerivedOrientation();
			break;
		default:
			break;
		}
		setDirty();
	}

	const Quaternion& Node::getDerivedOrientation() const {
		return m_derived_orientation;
	}

	const Vector3& Node::getDerivedPosition() const {
		return m_derived_position;
	}

	const Vector3& Node::getDerivedScale() const {
		return m_derived_scale;
	}

	const Matrix4x4& Node::getInverseTpose() const {
		return m_inverse_Tpose;
	}

	void Node::setScale(const Vector3& scale) {
		if (scale.isNaN()) {
			LOG_ERROR("scale is nan")
		}
		m_scale = scale;
		setDirty();
	}

	const Vector3& Node::getScale() const {
		return m_scale;
	}

	void Node::scale(const Vector3& scale) {
		m_scale = m_scale * scale;
		setDirty();
	}

	const std::string& Node::getName() const {
		return m_name;
	}

	void Node::setAsInitialPose() {
		m_initial_position = m_position;
		m_initial_orientation = m_orientation;
		m_initial_scale = m_scale;
	}

	void Node::resetToInitialPose() {
		m_position = m_initial_position;
		m_orientation = m_initial_orientation;
		m_scale = m_initial_scale;
	}

	const Vector3& Node::getInitialPosition() const {
		return m_initial_position;
	}

	const Quaternion& Node::getInitialOrientation() const {
		return m_initial_orientation;
	}

	const Vector3& Node::getInitialScale() const {
		return m_initial_scale;
	}

	void Node::setDirty() {
		m_is_dirty = true;
	}

	bool Node::isDirty() const {
		return m_is_dirty;
	}
}