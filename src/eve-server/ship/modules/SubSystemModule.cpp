
 /**
  * @name SubSystemModule.cpp
  *   SubSystem module class
  * @Author:         Allan
  * @date:   10 June 2015   -UD/RW 02 April 2017
  */

#include "ship/modules/SubSystemModule.h"


SubSystemModule::SubSystemModule(ModuleItemRef mRef, ShipItemRef sRef)
: PassiveModule(mRef, sRef)
{
    // T3 subsystem modules (for Tech 3 ships like Loki, Tengu, etc.)
    // Not yet implemented — requires T3 ship system completion
}

int8 SubSystemModule::GetModulePowerLevel()
{
    return Module::Bank::Subsystem;
}
