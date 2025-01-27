/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "BulletCollision/CollisionDispatch/btCompoundCollisionAlgorithm.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/BroadphaseCollision/btDbvt.h"
#include "LinearMath/btIDebugDraw.h"
#include "LinearMath/btAabbUtil2.h"

btCompoundCollisionAlgorithm::btCompoundCollisionAlgorithm( const btCollisionAlgorithmConstructionInfo& ci,btCollisionObject* body0,btCollisionObject* body1,bool isSwapped)
:btActivatingCollisionAlgorithm(ci,body0,body1),
m_isSwapped(isSwapped),
m_sharedManifold(ci.m_manifold)
{
	m_ownsManifold = false;

	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;
	assert (colObj->getCollisionShape()->isCompound());
	
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->getCollisionShape());
	int numChildren = compoundShape->getNumChildShapes();
	int i;
	
	m_childCollisionAlgorithms.resize(numChildren);
	for (i=0;i<numChildren;i++)
	{
		if (compoundShape->getDynamicAabbTree())
		{
			m_childCollisionAlgorithms[i] = 0;
		} else
		{
			btCollisionShape* tmpShape = colObj->getCollisionShape();
			btCollisionShape* childShape = compoundShape->getChildShape(i);
			colObj->internalSetTemporaryCollisionShape( childShape );
			m_childCollisionAlgorithms[i] = ci.m_dispatcher1->findAlgorithm(colObj,otherObj,m_sharedManifold);
			colObj->internalSetTemporaryCollisionShape( tmpShape );
		}
	}
}


btCompoundCollisionAlgorithm::~btCompoundCollisionAlgorithm()
{
	int numChildren = m_childCollisionAlgorithms.size();
	int i;
	for (i=0;i<numChildren;i++)
	{
		if (m_childCollisionAlgorithms[i])
		{
			m_childCollisionAlgorithms[i]->~btCollisionAlgorithm();
			m_dispatcher->freeCollisionAlgorithm(m_childCollisionAlgorithms[i]);
		}
	}
}




struct	btCompoundLeafCallback : btDbvt::ICollide
{

public:

	btCollisionObject* m_compoundColObj;
	btCollisionObject* m_otherObj;
	btDispatcher* m_dispatcher;
	const btDispatcherInfo& m_dispatchInfo;
	btManifoldResult*	m_resultOut;
	btCollisionAlgorithm**	m_childCollisionAlgorithms;
	btPersistentManifold*	m_sharedManifold;




	btCompoundLeafCallback (btCollisionObject* compoundObj,btCollisionObject* otherObj,btDispatcher* dispatcher,const btDispatcherInfo& dispatchInfo,btManifoldResult*	resultOut,btCollisionAlgorithm**	childCollisionAlgorithms,btPersistentManifold*	sharedManifold)
		:m_compoundColObj(compoundObj),m_otherObj(otherObj),m_dispatcher(dispatcher),m_dispatchInfo(dispatchInfo),m_resultOut(resultOut),
		m_childCollisionAlgorithms(childCollisionAlgorithms),
		m_sharedManifold(sharedManifold)
	{

	}


	void	ProcessChildShape(btCollisionShape* childShape,int index)
	{
		
		btCompoundShape* compoundShape = static_cast<btCompoundShape*>(m_compoundColObj->getCollisionShape());


		//backup
		btTransform	orgTrans = m_compoundColObj->getWorldTransform();
		btTransform	orgInterpolationTrans = m_compoundColObj->getInterpolationWorldTransform();
		const btTransform& childTrans = compoundShape->getChildTransform(index);
		btTransform	newChildWorldTrans = orgTrans*childTrans ;

		//perform an AABB check first
		btVector3 aabbMin0,aabbMax0,aabbMin1,aabbMax1;
		childShape->getAabb(newChildWorldTrans,aabbMin0,aabbMax0);
		m_otherObj->getCollisionShape()->getAabb(m_otherObj->getWorldTransform(),aabbMin1,aabbMax1);

		if (TestAabbAgainstAabb2(aabbMin0,aabbMax0,aabbMin1,aabbMax1))
		{

			m_compoundColObj->setWorldTransform( newChildWorldTrans);
			m_compoundColObj->setInterpolationWorldTransform(newChildWorldTrans);

			//the contactpoint is still projected back using the original inverted worldtrans
			btCollisionShape* tmpShape = m_compoundColObj->getCollisionShape();
			m_compoundColObj->internalSetTemporaryCollisionShape( childShape );

			if (!m_childCollisionAlgorithms[index])
				m_childCollisionAlgorithms[index] = m_dispatcher->findAlgorithm(m_compoundColObj,m_otherObj,m_sharedManifold);

			m_childCollisionAlgorithms[index]->processCollision(m_compoundColObj,m_otherObj,m_dispatchInfo,m_resultOut);
			if (m_dispatchInfo.m_debugDraw && (m_dispatchInfo.m_debugDraw->getDebugMode() & btIDebugDraw::DBG_DrawAabb))
			{
				btVector3 worldAabbMin,worldAabbMax;
				m_dispatchInfo.m_debugDraw->drawAabb(aabbMin0,aabbMax0,btVector3(1,1,1));
				m_dispatchInfo.m_debugDraw->drawAabb(aabbMin1,aabbMax1,btVector3(1,1,1));
			}
			
			//revert back transform
			m_compoundColObj->internalSetTemporaryCollisionShape( tmpShape);
			m_compoundColObj->setWorldTransform(  orgTrans );
			m_compoundColObj->setInterpolationWorldTransform(orgInterpolationTrans);
		}
	}
	void		Process(const btDbvtNode* leaf)
	{
		int index = leaf->dataAsInt;

		btCompoundShape* compoundShape = static_cast<btCompoundShape*>(m_compoundColObj->getCollisionShape());
		btCollisionShape* childShape = compoundShape->getChildShape(index);
		if (m_dispatchInfo.m_debugDraw && (m_dispatchInfo.m_debugDraw->getDebugMode() & btIDebugDraw::DBG_DrawAabb))
		{
			btVector3 worldAabbMin,worldAabbMax;
			btTransform	orgTrans = m_compoundColObj->getWorldTransform();
			btTransformAabb(leaf->volume.Mins(),leaf->volume.Maxs(),0.,orgTrans,worldAabbMin,worldAabbMax);
			m_dispatchInfo.m_debugDraw->drawAabb(worldAabbMin,worldAabbMax,btVector3(1,0,0));
		}
		ProcessChildShape(childShape,index);

	}
};






void btCompoundCollisionAlgorithm::processCollision (btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{
	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;

	assert (colObj->getCollisionShape()->isCompound());
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->getCollisionShape());

	btDbvt* tree = compoundShape->getDynamicAabbTree();
	//use a dynamic aabb tree to cull potential child-overlaps
	btCompoundLeafCallback  callback(colObj,otherObj,m_dispatcher,dispatchInfo,resultOut,&m_childCollisionAlgorithms[0],m_sharedManifold);


	if (tree)
	{

		btVector3 localAabbMin,localAabbMax;
		btTransform otherInCompoundSpace;
		otherInCompoundSpace = colObj->getWorldTransform().inverse() * otherObj->getWorldTransform();
		otherObj->getCollisionShape()->getAabb(otherInCompoundSpace,localAabbMin,localAabbMax);

		const ATTRIBUTE_ALIGNED16(btDbvtVolume)	bounds=btDbvtVolume::FromMM(localAabbMin,localAabbMax);
		//process all children, that overlap with  the given AABB bounds
		tree->collideTV(tree->m_root,bounds,callback);

	} else
	{
		//iterate over all children, perform an AABB check inside ProcessChildShape
		int numChildren = m_childCollisionAlgorithms.size();
		int i;
		for (i=0;i<numChildren;i++)
		{
			callback.ProcessChildShape(compoundShape->getChildShape(i),i);
		}
	}

	{
				//iterate over all children, perform an AABB check inside ProcessChildShape
		int numChildren = m_childCollisionAlgorithms.size();
		int i;
		btManifoldArray	manifoldArray;

		for (i=0;i<numChildren;i++)
		{
			if (m_childCollisionAlgorithms[i])
			{
				btCollisionShape* childShape = compoundShape->getChildShape(i);
			//if not longer overlapping, remove the algorithm
				btTransform	orgTrans = colObj->getWorldTransform();
				btTransform	orgInterpolationTrans = colObj->getInterpolationWorldTransform();
				const btTransform& childTrans = compoundShape->getChildTransform(i);
				btTransform	newChildWorldTrans = orgTrans*childTrans ;

				//perform an AABB check first
				btVector3 aabbMin0,aabbMax0,aabbMin1,aabbMax1;
				childShape->getAabb(newChildWorldTrans,aabbMin0,aabbMax0);
				otherObj->getCollisionShape()->getAabb(otherObj->getWorldTransform(),aabbMin1,aabbMax1);

				if (!TestAabbAgainstAabb2(aabbMin0,aabbMax0,aabbMin1,aabbMax1))
				{
					m_childCollisionAlgorithms[i]->~btCollisionAlgorithm();
					m_dispatcher->freeCollisionAlgorithm(m_childCollisionAlgorithms[i]);
					m_childCollisionAlgorithms[i] = 0;
				}

			}
			
		}

		

	}
}

btScalar	btCompoundCollisionAlgorithm::calculateTimeOfImpact(btCollisionObject* body0,btCollisionObject* body1,const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut)
{

	btCollisionObject* colObj = m_isSwapped? body1 : body0;
	btCollisionObject* otherObj = m_isSwapped? body0 : body1;

	assert (colObj->getCollisionShape()->isCompound());
	
	btCompoundShape* compoundShape = static_cast<btCompoundShape*>(colObj->getCollisionShape());

	//We will use the OptimizedBVH, AABB tree to cull potential child-overlaps
	//If both proxies are Compound, we will deal with that directly, by performing sequential/parallel tree traversals
	//given Proxy0 and Proxy1, if both have a tree, Tree0 and Tree1, this means:
	//determine overlapping nodes of Proxy1 using Proxy0 AABB against Tree1
	//then use each overlapping node AABB against Tree0
	//and vise versa.

	btScalar hitFraction = btScalar(1.);

	int numChildren = m_childCollisionAlgorithms.size();
	int i;
	for (i=0;i<numChildren;i++)
	{
		//temporarily exchange parent btCollisionShape with childShape, and recurse
		btCollisionShape* childShape = compoundShape->getChildShape(i);

		//backup
		btTransform	orgTrans = colObj->getWorldTransform();
	
		const btTransform& childTrans = compoundShape->getChildTransform(i);
		//btTransform	newChildWorldTrans = orgTrans*childTrans ;
		colObj->setWorldTransform( orgTrans*childTrans );

		btCollisionShape* tmpShape = colObj->getCollisionShape();
		colObj->internalSetTemporaryCollisionShape( childShape );
		btScalar frac = m_childCollisionAlgorithms[i]->calculateTimeOfImpact(colObj,otherObj,dispatchInfo,resultOut);
		if (frac<hitFraction)
		{
			hitFraction = frac;
		}
		//revert back
		colObj->internalSetTemporaryCollisionShape( tmpShape);
		colObj->setWorldTransform( orgTrans);
	}
	return hitFraction;

}


