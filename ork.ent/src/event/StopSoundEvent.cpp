////////////////////////////////////////////////////////////////
// Orkid Media Engine
// Copyright 1996-2012, Michael T. Mayers.
// Distributed under the Boost Software License - Version 1.0 - August 17, 2003
// see http://www.boost.org/LICENSE_1_0.txt
////////////////////////////////////////////////////////////////

#include <ork/pch.h>
#include <pkg/ent/AudioComponent.h>
#include <pkg/ent/event/StopSoundEvent.h>
#include <ork/reflect/RegisterProperty.h>
#include <ork/application/application.h>
///////////////////////////////////////////////////////////////////////////////
INSTANTIATE_TRANSPARENT_RTTI(ork::ent::event::StopSoundEvent, "StopSoundEvent");
///////////////////////////////////////////////////////////////////////////////
namespace ork { namespace ent { namespace event {
///////////////////////////////////////////////////////////////////////////////
void StopSoundEvent::Describe()
{
	ork::reflect::RegisterProperty("SoundName", &StopSoundEvent::mSoundName);
	ork::reflect::AnnotatePropertyForEditor<StopSoundEvent>( "SoundName",	"ged.userchoice.delegate", "AudioEventChoiceDelegate" );

}
///////////////////////////////////////////////////////////////////////////////
StopSoundEvent::StopSoundEvent(ork::PieceString name)
	: mSoundName(ork::AddPooledString(name))
{
}
Object* StopSoundEvent::Clone() const // final
{
    return new StopSoundEvent(mSoundName);
}
///////////////////////////////////////////////////////////////////////////////
} } } // namespace ork::ent::event
///////////////////////////////////////////////////////////////////////////////
