#pragma once

#include "runtime/core/math/math.h"
#include "runtime/core/math/transform.h"
#include "runtime/resource/res_type/data/skeleton_data.h"

namespace Dao {

    class Node {
    public:
        // Enumeration denoting the spaces which a transform can be relative to.
        enum class TransformSpace
        {
            /// Transform is relative to the local space
            LOCAL,
            /// Transform is relative to the space of the parent pNode
            PARENT,
            /// Transform is relative to object space
            OBJECT
        };

    public:
        Node(const std::string name);
        virtual ~Node();
        void clear();
        const std::string& getName() const;
        virtual Node* getParent() const;

        virtual const Quaternion& getOrientation() const;

        virtual void setOrientation(const Quaternion& q);
        virtual void resetOrientation();

        virtual void setPosition(const Vector3& pos);
        virtual const Vector3& getPosition() const;

        virtual void setScale(const Vector3& scale);
        virtual const Vector3& getScale() const;

        virtual void scale(const Vector3& scale);

        // Triggers the pNode to update it's combined transforms.
        virtual void updateDerivedTransform();

        virtual void translate(const Vector3& d, TransformSpace relativeTo = TransformSpace::PARENT);

        // Rotate the pNode around an aritrary axis using a Quarternion.
        virtual void rotate(const Quaternion& q, TransformSpace relativeTo = TransformSpace::LOCAL);

        // Gets the orientation of the pNode as derived from all parents.
        virtual const Quaternion& getDerivedOrientation() const;
        virtual const Vector3& getDerivedPosition() const;
        virtual const Vector3& getDerivedScale() const;
        virtual const Matrix4x4& getInverseTpose() const;

        // dirty and update
        virtual bool isDirty() const;
        virtual void setDirty();
        virtual void update();

        virtual void setAsInitialPose();
        virtual void resetToInitialPose();

        virtual const Vector3& getInitialPosition() const;
        virtual const Quaternion& getInitialOrientation() const;
        virtual const Vector3& getInitialScale() const;

    protected:
        /// Only available internally - notification of parent.
        virtual void setParent(Node* parent);

#ifdef _DEBUG
    public:
#else
    protected:
#endif
        Node* m_parent{ nullptr };

        std::string m_name;

        /// Stores the orientation/position/scale of the pNode relative to it's parent.
        Quaternion m_orientation{ Quaternion::IDENTITY };
        Vector3 m_position{ Vector3::ZERO };
        Vector3 m_scale{ Vector3::UNIT_SCALE };

        // Cached combined orientation/position/scale.
        Quaternion m_derived_orientation{ Quaternion::IDENTITY };
        Vector3 m_derived_position{ Vector3::ZERO };
        Vector3 m_derived_scale{ Vector3::UNIT_SCALE };

        /// The position/orientation/scale to use as a base for keyframe animation
        Quaternion m_initial_orientation{ Quaternion::IDENTITY };
        Vector3 m_initial_position{ Vector3::ZERO };
        Vector3 m_initial_scale{ Vector3::UNIT_SCALE };

        Matrix4x4 m_inverse_Tpose;

        bool m_is_dirty{ true };
    };
}