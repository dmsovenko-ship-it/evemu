#ifndef __APIADMINMANAGER__H__INCL__
#define __APIADMINMANAGER__H__INCL__

#include "apiserver/APIServiceManager.h"

class APIAdminManager : public APIServiceManager
{
public:
    std::string ProcessCall(const std::string& handler,
                             const std::map<std::string, std::string>& params) override;

private:
    std::string ProcessAccounts(const std::string& handler,
                                const std::map<std::string, std::string>& params);
    std::string ProcessPetitions(const std::string& handler,
                                 const std::map<std::string, std::string>& params);
    std::string ProcessTimecodes(const std::string& handler,
                                 const std::map<std::string, std::string>& params);
    std::string ProcessItems(const std::string& handler,
                             const std::map<std::string, std::string>& params);
    std::string ProcessRoles(const std::string& handler,
                             const std::map<std::string, std::string>& params);

    // petition ownership/state helpers
    static bool PetitionOwnedBy(uint32 petitionID, uint32 accountID);
    static bool PetitionIsOpen(uint32 petitionID);
};

#endif // __APIADMINMANAGER__H__INCL__
