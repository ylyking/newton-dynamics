/* Copyright (c) <2003-2011> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dStdAfxNewton.h"
#include "dNewton.h"
#include "dNewtonBody.h"
#include "dNewtonCollision.h"


dNewtonBody::dNewtonBody()
	:m_posit0 (0.0f, 0.0f, 0.0f, 0.0f)
	,m_posit1 (0.0f, 0.0f, 0.0f, 0.0f)
	,m_rotat0()
	,m_rotat1()
	,m_body(NULL)
	,m_lock(0)
	,m_userData(NULL)
{
}

dNewtonBody::dNewtonBody (dNewton* const dWorld, dFloat mass, const dNewtonCollision* const collision, void* const userData, const dMatrix& location)
	:m_posit0 (location.m_posit)
	,m_posit1 (location.m_posit)
	,m_rotat0(dQuaternion(location))
	,m_rotat1(dQuaternion(location))
	,m_lock(0)
	,m_userData(userData)
{
	NewtonWorld* const world = dWorld->GetNewton ();
	NewtonBody* const body = NewtonCreateDynamicBody (world, collision->GetShape(), &location[0][0]);

	NewtonCollision* const shape = NewtonBodyGetCollision(body);
	NewtonBodySetMassProperties(body, mass, shape);

	SetBody (body);
}

dNewtonBody::~dNewtonBody()
{
	if (NewtonBodyGetDestructorCallback(m_body)) {
		NewtonBodySetDestructorCallback (m_body, NULL);
		NewtonDestroyBody(NewtonBodyGetWorld(m_body), m_body);
	}
}


NewtonBody* dNewtonBody::GetNewtonBody () const
{
	return m_body;
}

void dNewtonBody::SetUserData(void* const userData)
{
	m_userData = userData;
}

void* dNewtonBody::GetUserData() const
{
	return m_userData;
}

void dNewtonBody::SetMatrix (const dFloat* const matrix)
{
	NewtonBodySetMatrix(m_body, matrix);
}

void dNewtonBody::GetMatrix (dFloat* const matrix) const
{
	NewtonBodyGetMatrix(m_body, matrix);
}

void dNewtonBody::SetVeloc (const dFloat* const veloc)
{
	NewtonBodySetVelocity(m_body, veloc);
}

void dNewtonBody::GetVeloc (dFloat* const veloc) const
{
	NewtonBodyGetVelocity(m_body, veloc);
}

void dNewtonBody::SetOmega (const dFloat* const omega)
{
	NewtonBodySetOmega(m_body, omega);
}

void dNewtonBody::GetOmega (dFloat* const omega) const
{
	NewtonBodyGetOmega(m_body, omega);
}


void dNewtonBody::SetForce (const dFloat* const force)
{
	NewtonBodySetForce(m_body, force);
}

void dNewtonBody::SetTorque (const dFloat* const torque)
{
	NewtonBodySetTorque(m_body, torque);
}


void dNewtonBody::GetMassAndInertia (dFloat& mass, dFloat& Ixx, dFloat& Iyy, dFloat& Izz) const
{
	NewtonBodyGetMassMatrix(m_body, &mass, &Ixx, &Iyy, &Izz);
}

dNewton* dNewtonBody::GetNewton () const
{
	return (dNewton*) NewtonWorldGetUserData(NewtonBodyGetWorld(m_body));
}

void dNewtonBody::GetPointVeloc (const dFloat* const point, dFloat* const veloc) const
{
	NewtonBodyGetPointVelocity (m_body, &point[0], &veloc[0]);
}

void dNewtonBody::SetBody (NewtonBody* const body)
{
	m_body = body;
	NewtonBodySetUserData(m_body, this);
	NewtonBodySetTransformCallback (m_body, OnBodyTransform);
	NewtonBodySetDestructorCallback (m_body, OnBodyDestroy);
	NewtonBodySetForceAndTorqueCallback(m_body, OnForceAndTorque);
}

void dNewtonBody::ApplyImpulseToDesiredPointVeloc (const dFloat* const point, const dFloat* const desiredveloc)
{
	NewtonBodyAddImpulse (m_body, &desiredveloc[0], &point[0]);
}

void dNewtonBody::OnBodyDestroy (const NewtonBody* const body)
{
	dNewtonBody* const me = (dNewtonBody*) NewtonBodyGetUserData(body);
	if (me) {
		NewtonBodySetDestructorCallback (me->m_body, NULL);
		delete me;
	}
}

bool dNewtonBody::GetSleepState() const
{
	return NewtonBodyGetSleepState(m_body) ? true : false;
}

dMatrix dNewtonBody::GetVisualMatrix (dFloat param) const
{
	dVector p0 (0.0f, 0.0f,0.0f, 0.0f);
	dVector p1 (0.0f, 0.0f,0.0f, 0.0f);
	dQuaternion r0;
	dQuaternion r1;
	{
		dNewton::ScopeLock scopelock (&m_lock);
		p0 = m_posit0;
		p1 = m_posit1;
		r0 = m_rotat0;
		r1 = m_rotat1;
	}

	dVector posit (p0 + (p1 - p0).Scale (param));
	dQuaternion rotation (r0.Slerp(r1, param));
	return dMatrix (rotation, posit);
}


void dNewtonBody::OnBodyTransform (const dFloat* const matrix, int threadIndex)
{
	dMatrix transform (matrix);
	dQuaternion rot (transform);

	dNewton::ScopeLock scopelock (&m_lock);
	m_posit0 = m_posit1;
	m_rotat0 = m_rotat1;
	m_posit1 = transform.m_posit;
	m_rotat1 = rot;

	dFloat angle = m_rotat0.DotProduct(m_rotat1);
	if (angle < 0.0f) {
		m_rotat1.Scale(-1.0f);
	}
}

void dNewtonBody::OnBodyTransform (const NewtonBody* const body, const dFloat* const matrix, int threadIndex)
{
	dNewtonBody* const me = (dNewtonBody*) NewtonBodyGetUserData(body);
	dAssert (me);
	me->OnBodyTransform (matrix, threadIndex);
}

void dNewtonBody::OnForceAndTorque (const NewtonBody* body, dFloat timestep, int threadIndex)
{
	dNewtonBody* const me = (dNewtonBody*) NewtonBodyGetUserData(body);
	dAssert (me);
	me->OnForceAndTorque (timestep, threadIndex);
}


dNewtonCollision* dNewtonBody::GetCollision() const
{
	return (dNewtonCollisionScene*) NewtonCollisionGetUserData(NewtonBodyGetCollision (m_body));
}