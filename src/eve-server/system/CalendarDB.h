/*
 *  CalendarDB.h
 *
 *  Allan
*/


#ifndef EVEMU_SYSTEM_CALENDARDB_H_
#define EVEMU_SYSTEM_CALENDARDB_H_


#include "ServiceDB.h"



class CalendarDB {
  public:

      static void DeleteEvent(uint32 eventID);
      static uint32 SaveSystemEvent(uint32 ownerID, uint32 creatorID, int64 startDateTime, uint8 autoEventType,
                                     std::string title, std::string description, bool important=false);
      static void SaveEventResponse(uint32 charID, uint32 eventID, uint32 response);
      static void UpdateEventParticipants(uint32 eventID, PyList* charsToAdd, PyList* charsToRemove);
      static void UpdateEvent(uint32 eventID, int64 dateTime, int32 duration,
                              std::string title, std::string description, int32 important);

      static PyRep* SaveNewEvent(uint32 ownerID, Call_CreateEventWithInvites &args);
      static PyRep* SaveNewEvent(uint32 ownerID, uint32 creatorID, Call_CreateEvent &args);
      static PyRep* GetEventList(uint32 ownerID, uint32 month, uint32 year);
      static PyRep* GetEventDetails(uint32 eventID);

      static PyRep* GetResponsesToEvent(uint32 eventID);
      static PyRep* GetResponsesForCharacter(uint32 charID);

  protected:

};

#endif // EVEMU_SYSTEM_CALENDARDB_H_
