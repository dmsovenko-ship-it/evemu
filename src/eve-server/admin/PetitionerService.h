#ifndef __PETITIONER_SERVICE_H_INCL__
#define __PETITIONER_SERVICE_H_INCL__

#include "services/Service.h"

class PetitionerService : public Service<PetitionerService> {
public:
    PetitionerService();

protected:
    // -- categories / filing --
    PyResult GetUserCatalogCountry(PyCallArgs& call);
    PyResult GetCategories(PyCallArgs& call);
    PyResult GetCategoryHierarchicalInfo(PyCallArgs& call);
    PyResult GetCategoryProperties(PyCallArgs& call, PyRep* categoryID);
    PyResult MayPetition(PyCallArgs& call, PyRep* categoryID, PyRep* oocCharID);
    PyResult PropertyPopulationInfo(PyCallArgs& call);
    PyResult GetClientPickerInfo(PyCallArgs& call);
    // CreatePetition(subject, petition, categoryID, retval, OocCharacterID, chatLog, combatLog, propertyList)
    // args 0-4 always present (some None), 5-7 optional/None — accept generically.
    PyResult CreatePetition(PyCallArgs& call,
                             PyRep* subject, PyRep* petition, PyRep* categoryID, PyRep* retval,
                             std::optional<PyRep*> oocCharID,
                             std::optional<PyRep*> chatLog,
                             std::optional<PyRep*> combatLog,
                             std::optional<PyRep*> propertyList);

    // -- my petitions / messages --
    PyResult GetMyPetitionsEx(PyCallArgs& call);
    PyResult GetPetitionMessages(PyCallArgs& call, PyInt* petitionID);
    PyResult GetUnreadMessages(PyCallArgs& call);
    PyResult MarkAsRead(PyCallArgs& call, PyInt* messageID);
    PyResult PetitionerChat(PyCallArgs& call, PyInt* petitionID, PyRep* message);     // player adds msg
    PyResult PetitioneeChat(PyCallArgs& call, PyInt* petitionID, PyRep* message, PyRep* comment); // GM replies

    // -- actions --
    PyResult CancelPetition(PyCallArgs& call, PyInt* petitionID);
    PyResult ClosePetition(PyCallArgs& call, PyInt* petitionID);
    PyResult DeletePetition(PyCallArgs& call, PyInt* petitionID);
    PyResult ClaimPetition(PyCallArgs& call, PyInt* petitionID);
    PyResult UnClaimPetition(PyCallArgs& call, PyInt* petitionID);
    PyResult EscalatePetition(PyCallArgs& call, PyInt* petitionID, PyRep* queueID);

    // -- GM views (lightweight) --
    PyResult GetQueues(PyCallArgs& call);
    PyResult GetClaimedPetitions(PyCallArgs& call);
    PyResult GetPetitionQueue(PyCallArgs& call, PyInt* queueID);
    PyResult GetEvents(PyCallArgs& call);
    PyResult GetLog(PyCallArgs& call, PyInt* petitionID);
    PyResult UpdatePetitionRating(PyCallArgs& call, std::optional<PyRep*> petitionID, std::optional<PyRep*> a, std::optional<PyRep*> b, std::optional<PyRep*> c, std::optional<PyRep*> comment);
    PyResult AddPetitionRating(PyCallArgs& call, std::optional<PyRep*> petitionID, std::optional<PyRep*> a, std::optional<PyRep*> b, std::optional<PyRep*> c, std::optional<PyRep*> comment, std::optional<PyRep*> ratingTime);
};

#endif
