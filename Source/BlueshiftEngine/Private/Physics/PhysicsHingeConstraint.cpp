// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Physics/Physics.h"
#include "PhysicsInternal.h"

BE_NAMESPACE_BEGIN

PhysHingeConstraint::PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA) : 
    PhysConstraint(bodyA, nullptr) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    btHingeConstraint *hingeConstraint = new btHingeConstraint(*bodyA->GetRigidBody(), frameA);
    hingeConstraint->setUserConstraintPtr(this);

    constraint = hingeConstraint;
}

PhysHingeConstraint::PhysHingeConstraint(PhysRigidBody *bodyA, const Vec3 &anchorInA, const Mat3 &axisInA, PhysRigidBody *bodyB, const Vec3 &anchorInB, const Mat3 &axisInB) : 
    PhysConstraint(bodyA, bodyB) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);
    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    btHingeConstraint *hingeConstraint = new btHingeConstraint(*bodyA->GetRigidBody(), *bodyB->GetRigidBody(), frameA, frameB);
    hingeConstraint->setUserConstraintPtr(this);

    constraint = hingeConstraint;
}

void PhysHingeConstraint::SetFrameA(const Vec3 &anchorInA, const Mat3 &axisInA) {
    Vec3 anchorInACentroid = anchorInA - bodyA->centroid;

    btTransform frameA = ToBtTransform(axisInA, anchorInACentroid);

    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);
    hingeConstraint->setFrames(frameA, hingeConstraint->getFrameOffsetB());
}

void PhysHingeConstraint::SetFrameB(const Vec3 &anchorInB, const Mat3 &axisInB) {
    Vec3 anchorInBCentroid = anchorInB - bodyB->centroid;

    btTransform frameB = ToBtTransform(axisInB, anchorInBCentroid);

    btHingeConstraint *hingeConstraint = static_cast<btHingeConstraint *>(constraint);
    hingeConstraint->setFrames(hingeConstraint->getFrameOffsetA(), frameB);
}

void PhysHingeConstraint::SetLimit(float lowerLimit, float upperLimit) {
    this->lowerLimit = lowerLimit;
    this->upperLimit = upperLimit;
}

void PhysHingeConstraint::EnableLimit(bool enable) {
    if (enable) {
        ((btHingeConstraint *)constraint)->setLimit(lowerLimit, upperLimit, 0.9f, 0.3f);
    } else {
        ((btHingeConstraint *)constraint)->setLimit(1.0f, -1.0f);
    }
}

void PhysHingeConstraint::EnableMotor(bool enable) {
    btScalar motorSpeed = ((btHingeConstraint *)constraint)->getMotorTargetVelosity();
    btScalar maxMotorImpulse = ((btHingeConstraint *)constraint)->getMaxMotorImpulse();
    ((btHingeConstraint *)constraint)->enableAngularMotor(enable, motorSpeed, maxMotorImpulse);
}

void PhysHingeConstraint::SetMotor(float motorSpeed, float maxMotorImpulse) {
    bool enabled = ((btHingeConstraint *)constraint)->getEnableAngularMotor();
    ((btHingeConstraint *)constraint)->enableAngularMotor(enabled, motorSpeed, maxMotorImpulse);
}

BE_NAMESPACE_END
