////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>

#include <ork/application/application.h>
#include <pkg/ent/entity.h>
#include <pkg/ent/entity.hpp>
#include <pkg/ent/scene.h>
#include <pkg/ent/drawable.h>
#include <ork/reflect/RegisterProperty.h>
#include <ork/reflect/AccessorObjectPropertyType.hpp>
#include <ork/rtti/downcast.h>
#include <pkg/ent/bullet.h>
#include <ork/reflect/DirectObjectVectorPropertyType.h>
#include <ork/reflect/DirectObjectVectorPropertyType.hpp>
#include <ork/reflect/DirectObjectMapPropertyType.h>
#include <ork/reflect/DirectObjectMapPropertyType.hpp>
#include <ork/rtti/RTTI.h>

#include <pkg/ent/event/DrawableEvent.h>
#include <pkg/ent/ReferenceArchetype.h>
#include <pkg/ent/PerfController.h>

#include <ork/rtti/Class.h>
#include <ork/kernel/orklut.hpp>
#include <pkg/ent/ParticleControllable.h>

///////////////////////////////////////////////////////////////////////////////

#include <pkg/ent/Compositor.h>
#include <pkg/ent/AudioAnalyzer.h>
#include <pkg/ent/ModelComponent.h>
#include <pkg/ent/SimpleCharacterArchetype.h>
#include <pkg/ent/ModelArchetype.h>
#include <pkg/ent/Lighting.h>
#include "ObserverCamera.h"
#include "EditorCamera.h"
#include "SpinnyCamera.h"
#include "Skybox.h"
#include "ProcTex.h"
#include "PerformanceAnalyzer.h"
#include "QuartzComposerTest.h"
#include <pkg/ent/PerfController.h>

///////////////////////////////////////////////////////////////////////////////

template class ork::reflect::DirectObjectMapPropertyType< ork::orklut< ork::PoolString, ork::Object*> >;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::Archetype,"Ent3dArchetype");
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::EntData,"Ent3dEntData");
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::Entity,"Ent3dEntity");

///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent {
///////////////////////////////////////////////////////////////////////////////

#if defined( ORK_CONFIG_EDITORBUILD )
class SceneDagObjectManipInterface : public ork::lev2::IManipInterface
{
	RttiDeclareConcrete(SceneDagObjectManipInterface,ork::lev2::IManipInterface);

public:

	SceneDagObjectManipInterface() {}

	virtual const TransformNode3D &GetTransform(rtti::ICastable *pobj)
	{
		SceneDagObject *pdago = rtti::autocast(pobj);
		return pdago->GetDagNode().GetTransformNode();
	}
	virtual void SetTransform(rtti::ICastable *pobj, const TransformNode3D &node)
	{
		SceneDagObject *pdago = rtti::autocast(pobj);
		pdago->GetDagNode().GetTransformNode() = node;
	}
};

void SceneDagObjectManipInterface::Describe()
{
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void EntData::Describe()
{
	ArrayString<64> arrstr;
	MutableString mutstr(arrstr);
	mutstr.format("EntData");
	GetClassStatic()->SetPreferredName(arrstr);

	reflect::AnnotateClassForEditor<EntData>( "editor.3dpickable", true );
	reflect::AnnotateClassForEditor<EntData>( "editor.3dxfable", true );
	reflect::AnnotateClassForEditor<EntData>( "editor.3dxfinterface", ConstString("SceneDagObjectManipInterface") );

	reflect::RegisterProperty( "Archetype", & EntData::ArchetypeGetter, & EntData::ArchetypeSetter );
	reflect::AnnotatePropertyForEditor<EntData>( "Archetype", "editor.choicelist", "archetype" );
	reflect::AnnotatePropertyForEditor<EntData>( "Archetype", "editor.factorylistbase", "Ent3dArchetype" );

	reflect::RegisterFunctor("SlotArchetypeDeleted", &EntData::SlotArchetypeDeleted);

	reflect::AnnotateClassForEditor< EntData >("editor.object.ops", ConstString("ArchDeRef:EntArchDeRef ArchReRef:EntArchReRef ArchSplit:EntArchSplit") );

	ork::reflect::RegisterMapProperty( "UserProperties", & EntData::mUserProperties );
}
///////////////////////////////////////////////////////////////////////////////
ConstString EntData::GetUserProperty(const ConstString& key) const
{
	ConstString rval("");
	orklut<ConstString, ConstString>::const_iterator it = mUserProperties.find(key);
	if(it != mUserProperties.end())
		rval = (*it).second;
	return rval;
}
///////////////////////////////////////////////////////////////////////////////
bool EntData::PostDeserialize(reflect::IDeserializer &)
{
	return true;
}
///////////////////////////////////////////////////////////////////////////////
void EntData::SetArchetype( const Archetype* parch )
{
	if(mArchetype != parch)
		mArchetype = parch;
}
///////////////////////////////////////////////////////////////////////////////

void EntData::SlotArchetypeDeleted( const ork::ent::Archetype *parch )
{
	if( GetArchetype() == parch )
	{
		SetArchetype(0);
	}
}

///////////////////////////////////////////////////////////////////////////////
EntData::EntData() : mArchetype(0)
{
}
///////////////////////////////////////////////////////////////////////////////
EntData::~EntData()
{
}
///////////////////////////////////////////////////////////////////////////////
void EntData::ArchetypeGetter(ork::rtti::ICastable*& val) const
{
	Archetype* nonconst = const_cast< Archetype* >( mArchetype );
	val = nonconst;
}
///////////////////////////////////////////////////////////////////////////////
void EntData::ArchetypeSetter( ork::rtti::ICastable* const & val)
{
	ork::rtti::ICastable* ptr = val;
	SetArchetype( (ptr==0) ? 0 : rtti::safe_downcast<Archetype*>(ptr) );
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::DeleteComponents()
{
	for( ComponentDataTable::LutType::const_iterator it=mComponentDatas.begin(); it!=mComponentDatas.end(); it++ )
	{
		ComponentData* pdata = it->second;
		delete pdata;
	}

	mComponentDataTable.Clear();
	mComponentDatas.clear();
}
///////////////////////////////////////////////////////////////////////////////
void Entity::Describe()
{
	reflect::RegisterProperty( "EntData", & Entity::EntDataGetter,
		static_cast<void (Entity::*)(ork::rtti::ICastable*const& val)>(0) );
	reflect::RegisterFunctor("Self", &Entity::Self);
	reflect::RegisterFunctor("Position", &Entity::GetEntityPosition);
	reflect::RegisterFunctor("PrintName", &Entity::PrintName);
	reflect::RegisterFunctor("GetComponentByClassName", &Entity::GetComponentByClassName);
}
///////////////////////////////////////////////////////////////////////////////
ComponentInst *Entity::GetComponentByClass(rtti::Class *clazz)
{
	const ComponentTable::LutType &lut = mComponentTable.GetComponents();
	for(ComponentTable::LutType::const_iterator it = lut.begin(); it != lut.end(); it++)
	{
		ComponentInst *cinst = (*it).second;
		if(cinst->GetClass()->IsSubclassOf(clazz))
			return cinst;
	}
	return NULL;
}
///////////////////////////////////////////////////////////////////////////////
ComponentInst *Entity::GetComponentByClassName(ork::PoolString classname)
{
	if(rtti::Class *clazz = rtti::Class::FindClass(classname))
		return GetComponentByClass(clazz);
	return NULL;
}
///////////////////////////////////////////////////////////////////////////////
void Entity::EntDataGetter( ork::rtti::ICastable* & ptr ) const
{
	EntData* pdata = const_cast<EntData*>( & mEntData );
	ptr = static_cast<ork::rtti::ICastable*>(pdata);
}
///////////////////////////////////////////////////////////////////////////////
Entity::Entity( const EntData& edata, SceneInst *inst )
	: mComponents(EKEYPOLICY_MULTILUT)
	, mEntData( edata )
	, mDagNode( edata.GetDagNode() )
	//, mDrawable( 0 )
	, mComponentTable( mComponents )
	, mSceneInst(inst)
{
	//mDrawable.reserve(4);
}
///////////////////////////////////////////////////////////////////////////////
Entity::~Entity()
{
	//printf( "Delete Entity<%p>\n", this );
	for( LayerMap::const_iterator itL=mLayerMap.begin(); itL!=mLayerMap.end(); itL++ )
	{
		DrawableVector* pldrawables = itL->second;
		
		for( DrawableVector::const_iterator it=pldrawables->begin(); it!=pldrawables->end(); it++ )
		{
			Drawable* pdrw = *it;
			delete pdrw;
		}
	}
	for( ComponentTable::LutType::const_iterator it=mComponents.begin(); it!=mComponents.end(); it++ )
	{
		ComponentInst* pinst = it->second;
		delete pinst;
	}
}
///////////////////////////////////////////////////////////////////////////////
CVector3 Entity::GetEntityPosition() const
{
	return GetDagNode().GetTransformNode().GetTransform()->GetPosition();
}
///////////////////////////////////////////////////////////////////////////////
void Entity::PrintName()
{
	orkprintf("EntityName:%s: \n",mEntData.GetName().c_str());

}
///////////////////////////////////////////////////////////////////////////////
bool Entity::DoNotify(const ork::event::Event *event)
{
    bool result = false;
	if(const event::DrawableEvent *drawable_event = rtti::autocast(event))
	{
		for( LayerMap::const_iterator itL=mLayerMap.begin(); itL!=mLayerMap.end(); itL++ )
		{
			DrawableVector* pldrawables = itL->second;
			
			for(ork::ent::Entity::DrawableVector::iterator it = pldrawables->begin(); it != pldrawables->end(); it++)
				result = static_cast<ork::Object*>(*it)->Notify(drawable_event->GetEvent()) || result;
		}
	}
	else if( const PerfSnapShotEvent* psse = rtti::autocast(event) )
	{
		ComponentTable::LutType& lut = mComponentTable.GetComponents();
		for( ComponentTable::LutType::const_iterator it=lut.begin(); it!=lut.end(); it++ )
		{
			ComponentInst* inst = (*it).second;
			const char* pshortname = inst->GetShortSelector();
			if( pshortname )
			{
				printf( " ent<%p>.component<%s>\n", this, pshortname );
				psse->PushNode( pshortname );
				((ork::Object*)inst)->Notify( psse );
				psse->PopNode();
			}
		}
		result = true;
	}
	else if( const PerfControlEvent* pce = rtti::autocast(event) )
	{
//		const FixedString<256>& tgt = pce->mTarget;
//		const FixedString<32>& val = pce->mValue;
		
		PerfControlEvent pce2 = *pce;
		std::string ComponentName = pce2.PopTargetNode();
		std::string KeyName = pce2.mTarget.c_str();

		printf( "Entity<%p> PerfControlEvent<%p> cname<%s> keyname<%s>\n", this, pce, ComponentName.c_str(), KeyName.c_str() );

		ComponentTable::LutType& lut = mComponentTable.GetComponents();
		for( ComponentTable::LutType::const_iterator it=lut.begin(); it!=lut.end(); it++ )
		{
			ComponentInst* inst = (*it).second;
			const char* pshortname = inst->GetShortSelector();
			if( pshortname )
			{
				printf( "testing shortname<%s>\n", pshortname );
				if( 0 == strcmp( pshortname, ComponentName.c_str() ) )
				{
					((ork::Object*)inst)->Notify( & pce2 );
				}
			}
		}
	}
	else
	{
		ComponentTable::LutType& lut = mComponentTable.GetComponents();
		for( ComponentTable::LutType::const_iterator it=lut.begin(); it!=lut.end(); it++ )
		{
			ComponentInst* inst = (*it).second;
			result = static_cast<ork::Object*>(inst)->Notify(event) || result;
		}
	}
	
    return result;
}
///////////////////////////////////////////////////////////////////////////////
ComponentTable& Entity::GetComponents()
{
	return mComponentTable;
}
///////////////////////////////////////////////////////////////////////////////
const ComponentTable& Entity::GetComponents() const
{
	return mComponentTable;
}
///////////////////////////////////////////////////////////////////////////////
void Entity::AddDrawable( const PoolString& layername, Drawable* pdrw )
{
	bool bDEFAULT = (0==strcmp(layername.c_str(),"Default"));

	PoolString actualLayerName = layername;
	
	if( bDEFAULT )
	{
		const ent::EntData& ED = GetEntData();
		ConstString layer = ED.GetUserProperty("DrawLayer");
		if( strlen( layer.c_str() ) != 0 )
		{
			actualLayerName = AddPooledString(layer.c_str());
		} 
	}

	SceneInst* psi = GetSceneInst();
	OrkAssert(psi);
	Layer* player = psi->GetLayer( actualLayerName );
	
	DrawableVector* pldrawables = GetDrawables(actualLayerName);
	
	if( 0 == pldrawables )
	{
		pldrawables = new DrawableVector;
		mLayerMap.AddSorted( actualLayerName, pldrawables );
	}
	
	pldrawables->push_back(pdrw);
}

Entity::DrawableVector* Entity::GetDrawables( const PoolString& layer )
{
	DrawableVector* pldrawables = 0;
	
	LayerMap::const_iterator itL=mLayerMap.find(layer);
	if( itL != mLayerMap.end() )
	{
		pldrawables = itL->second;
	}
	return pldrawables;
}
const Entity::DrawableVector* Entity::GetDrawables( const PoolString& layer ) const
{
	const DrawableVector* pldrawables = 0;
	
	LayerMap::const_iterator itL=mLayerMap.find(layer);
	if( itL != mLayerMap.end() )
	{
		pldrawables = itL->second;
	}
	return pldrawables;
}

///////////////////////////////////////////////////////////////////////////////
void ArchComposer::Register( ork::ent::ComponentData* pdata )
{
	if( pdata )
	{
		ork::object::ObjectClass* pclass = pdata->GetClass();
		mComponents.AddSorted( pclass,pdata );
	}
}

ArchComposer::ArchComposer(ork::ent::Archetype* parch,SceneComposer& scene_composer) 
	: mpArchetype(parch)
	, mComponents( ork::EKEYPOLICY_MULTILUT )
	, mSceneComposer(scene_composer)
{
}

ArchComposer::~ArchComposer()
{	mpArchetype->GetComponentDataTable().Clear();
	for( ork::orklut<ork::object::ObjectClass*,ork::Object*>::const_iterator it=mComponents.begin(); it!=mComponents.end(); it++ )
	{	ork::object::ObjectClass* pclass = it->first;
		ork::ent::ComponentData* pdata = ork::rtti::autocast(it->second);
		if( 0 == pdata )
		{
			OrkAssert( pclass->IsSubclassOf( ork::ent::ComponentData::GetClassStatic() ) );
			pdata = ork::rtti::autocast(pclass->CreateObject());
		}
		mpArchetype->GetComponentDataTable().AddComponent(pdata);
	}
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::Describe()
{
	ArrayString<64> arrstr;
	MutableString mutstr(arrstr);
	mutstr.format("/arch/Archetype");
	GetClassStatic()->SetPreferredName(arrstr);

	reflect::RegisterMapProperty( "Components", & Archetype::mComponentDatas );
	reflect::AnnotatePropertyForEditor<Archetype>("Components", "editor.map.policy.const", "true");
}
///////////////////////////////////////////////////////////////////////////////
Archetype::Archetype()
	: mComponentDatas( EKEYPOLICY_MULTILUT ) 
	, mComponentDataTable( mComponentDatas )
	, mpSceneData(0)
{
}
///////////////////////////////////////////////////////////////////////////////
bool Archetype::PostDeserialize(reflect::IDeserializer &)
{
	//Compose();
	return true;
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::ComposeEntity( Entity* pent ) const
{
	DoComposeEntity( pent );
}

void Archetype::DoComposeEntity( Entity *pent ) const
{
	const ent::ComponentDataTable::LutType& clut = GetComponentDataTable().GetComponents();
	for( ent::ComponentDataTable::LutType::const_iterator it = clut.begin(); it!= clut.end(); it++ )
	{	ent::ComponentData* pcompdata = it->second;
		if( pcompdata )
		{
			ent::ComponentInst* pinst = pcompdata->CreateComponent(pent);
			if( pinst )
			{	pent->GetComponents().AddComponent(pinst);
			}
		}
	}
}

void Archetype::LinkEntity(ork::ent::SceneInst *psi, ork::ent::Entity *pent) const
{
	if( GetClass() != ReferenceArchetype::GetClassStatic() )
	{
		const ComponentTable::LutType& lut = pent->GetComponents().GetComponents();
		for( ComponentTable::LutType::const_iterator it=lut.begin(); it!=lut.end(); it++ )
		{
			ComponentInst* inst = (*it).second;
			inst->Link(psi);
		}
	}

	DoLinkEntity( psi, pent );
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::UnLinkEntity(ork::ent::SceneInst *psi, ork::ent::Entity *pent) const
{
	if( GetClass() != ReferenceArchetype::GetClassStatic() )
	{
		const ComponentTable::LutType& lut = pent->GetComponents().GetComponents();
		for( ComponentTable::LutType::const_iterator it=lut.begin(); it!=lut.end(); it++ )
		{
			ComponentInst* inst = (*it).second;
			inst->UnLink(psi);
		}
	}

	DoUnLinkEntity( psi, pent );
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::DoLinkEntity( SceneInst* psi, Entity *pent ) const
{
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::DoUnLinkEntity( SceneInst* psi, Entity *pent ) const
{
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::DoDeComposeEntity(Entity *pent) const
{
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::StartEntity(SceneInst *psi, const CMatrix4 &world, Entity *pent) const
{
	StopEntity( psi, pent);

	pent->GetDagNode().GetTransformNode().GetTransform()->SetMatrix( world );

	if( GetClass() != ReferenceArchetype::GetClassStatic() )
	{
		const ComponentTable::LutType& lut = pent->GetComponents().GetComponents();
		for( ComponentTable::LutType::const_iterator it=lut.begin(); it!=lut.end(); it++ )
		{
			ComponentInst* inst = (*it).second;
			inst->Start(psi,world);
		}
	}
	DoStartEntity( psi, world, pent );

}
///////////////////////////////////////////////////////////////////////////////
void Archetype::StopEntity(SceneInst *psi, Entity *pent) const
{
	if( GetClass() != ReferenceArchetype::GetClassStatic() )
	{
		const ComponentTable::LutType& lut = pent->GetComponents().GetComponents();
		for( ComponentTable::LutType::const_iterator it=lut.begin(); it!=lut.end(); it++ )
		{
			ComponentInst* inst = (*it).second;
			inst->Stop(psi);
		}
	}
	DoStopEntity( psi, pent );
}
///////////////////////////////////////////////////////////////////////////////
void Archetype::Compose(SceneComposer& scene_composer)
{	
	mpSceneData = scene_composer.GetSceneData();

	ork::ent::ArchComposer arch_composer(this,scene_composer);
	DoCompose(arch_composer);

}
///////////////////////////////////////////////////////////////////////////////
void Archetype::DeCompose()
{
	DeleteComponents();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Init()
{
	Archetype::GetClassStatic();
	ork::ent::ModelArchetype::GetClassStatic();
	SkyBoxArchetype::GetClassStatic();
	ProcTexArchetype::GetClassStatic();
	ObserverCamArchetype::GetClassStatic();
	EditorCamArchetype::GetClassStatic();
	SequenceCamArchetype::GetClassStatic();
	BulletWorldArchetype::GetClassStatic();
	BulletObjectArchetype::GetClassStatic();
	PerfControllerArchetype::GetClassStatic();
	PerformanceAnalyzerArchetype::GetClassStatic();
	SimpleCharacterArchetype::GetClassStatic();
	EntData::GetClassStatic();
	SceneObject::GetClassStatic();
	SceneData::GetClassStatic();
	SceneInst::GetClassStatic();
    
	ork::ent::CompositingManagerComponentData::GetClassStatic();
	ork::ent::CompositingManagerComponentInst::GetClassStatic();
	ork::ent::CompositingComponentData::GetClassStatic();
	ork::ent::CompositingComponentInst::GetClassStatic();
	ork::ent::CompositingNode::GetClassStatic();
	ork::ent::NodeCompositingTechnique::GetClassStatic();
	ork::ent::Fx3CompositingTechnique::GetClassStatic();

	ork::psys::ParticleControllableData::GetClassStatic();
	ork::psys::ParticleControllableInst::GetClassStatic();

	ork::psys::ModularSystem::GetClassStatic();
	ork::psys::NovaParticleSystem::GetClassStatic();

	ork::psys::ModParticleItem::GetClassStatic();
	ork::psys::NovaParticleItemBase::GetClassStatic();

#if defined( ORK_CONFIG_EDITORBUILD )
	SceneDagObjectManipInterface::GetClassStatic();
#endif

#if defined(ORK_OSXX)
	ork::ent::AudioAnalysisManagerComponentData::GetClassStatic();
	ork::ent::AudioAnalysisManagerComponentInst::GetClassStatic();
	ork::ent::AudioAnalysisComponentData::GetClassStatic();
	ork::ent::AudioAnalysisComponentInst::GetClassStatic();
	AudioAnalysisArchetype::GetClassStatic();
	QuartzComposerArchetype::GetClassStatic();
#endif
}
///////////////////////////////////////////////////////////////////////////////
}}
///////////////////////////////////////////////////////////////////////////////
#if defined( ORK_CONFIG_EDITORBUILD )
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::SceneDagObjectManipInterface,"SceneDagObjectManipInterface");
#endif
