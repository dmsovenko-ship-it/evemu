/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2021 The EVEmu Team
    For the latest information visit https://evemu.dev
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Zhur / Allan
    Updates:       shared portal_petitions backend (2026)
*/

#include "eve-server.h"

#include "Client.h"
#include "EVEServerConfig.h"
#include "admin/PetitionerService.h"
#include "python/classes/PyExceptions.h"
#include "TelegramBot.h"

#include <cctype>
#include <map>

// session language -> category table language code ('ru' or 'en-us')
static std::string CategoryLanguage(const std::string& sessionLang)
{
    std::string lang = sessionLang;
    for (auto& c : lang)
        c = static_cast<char>(::tolower(c));
    return lang.find("ru") != std::string::npos ? "ru" : "en-us";
}

// Escape a string for a SQL literal.
static std::string SqlEsc(const std::string& in)
{
    std::string out;
    sDatabase.DoEscapeString(out, in);
    return out;
}

// NULL-safe DB text for client strings (PyString must not receive nullptr).
static const char* SafeText(const char* s)
{
    return s != nullptr ? s : "";
}

// NULL-safe text as std::string (for PyWString, whose const char* ctor is
// ambiguous between PyString and std::string overloads).
static std::string SafeStr(const char* s)
{
    return std::string(s != nullptr ? s : "");
}

// The petition window requests the category tree once, then filters rows by the
// client's own languageID (with an English fallback if its language yields too
// few groups).  The DB stores one categoryID per logical category shared by all
// languages, which would collide in the dictionaries the client builds, so each
// (category, language-form) is emitted under its own "wire" id.  CCP clients may
// compare against the short lower-case code ('ru'/'en-us'), the two-letter
// language-table code ('RU'/'EN') or the numeric language id (1049/1033) — emit
// all forms so the wizard's filter always finds a group.  CreatePetition decodes
// any form back to the DB id.
struct CategoryLangForm {
    int32   offset;      // unique wire-id offset for this form
    int32   numericID;   // >0: emit a numeric languageID token (1049 etc.), else 0
    std::string code;    // string languageID token (ru / RU / en-us / EN ...)
};

static std::vector<CategoryLangForm> CategoryLanguageForms(const std::string& dbLang)
{
    std::vector<CategoryLangForm> out;
    if (dbLang == "ru") {
        out.push_back({0,      0,    "ru"});
        out.push_back({200000, 0,    "RU"});
        out.push_back({400000, 1049, ""});       // numeric russian
    } else if (dbLang == "en-us") {
        out.push_back({100000, 0,    "en-us"});
        out.push_back({300000, 0,    "EN"});
        out.push_back({500000, 1033, ""});       // numeric english
    } else {
        // unknown DB language: emit as-is plus lower-cased short form
        out.push_back({600000, 0, dbLang});
        std::string lower = dbLang;
        for (auto& c : lower) c = static_cast<char>(::tolower(c));
        if (lower != dbLang)
            out.push_back({700000, 0, lower});
    }
    return out;
}

static int32 WireCategoryID(int32 dbID, const CategoryLangForm& form)
{
    return dbID + form.offset;
}

static int32 DecodeCategoryID(int32 wireID)
{
    // strip whichever per-form offset is present (offsets are multiples of 100k)
    static const int32 kOffsets[] = { 700000, 600000, 500000, 400000, 300000, 200000, 100000, 0 };
    for (int32 off : kOffsets) {
        if (wireID >= off)
            return wireID - off;
    }
    return wireID;
}

PetitionerService::PetitionerService() :
    Service("petitioner", eAccessLevel_Character)
{
    // player + category
    this->Add("GetUserCatalogCountry", &PetitionerService::GetUserCatalogCountry);
    this->Add("GetCategories", &PetitionerService::GetCategories);
    this->Add("GetCategoryHierarchicalInfo", &PetitionerService::GetCategoryHierarchicalInfo);
    this->Add("GetCategoryProperties", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyRep*)>(&PetitionerService::GetCategoryProperties));
    this->Add("MayPetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyRep*, PyRep*)>(&PetitionerService::MayPetition));
    this->Add("PropertyPopulationInfo", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyRep*, PyRep*)>(&PetitionerService::PropertyPopulationInfo));
    this->Add("GetClientPickerInfo", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyRep*, PyRep*)>(&PetitionerService::GetClientPickerInfo));
    this->Add("CreatePetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyRep*, PyRep*, PyRep*, PyRep*, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>)>(&PetitionerService::CreatePetition));

    // my petitions / messages
    this->Add("GetMyPetitionsEx", &PetitionerService::GetMyPetitionsEx);
    this->Add("GetPetitionMessages", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::GetPetitionMessages));
    this->Add("GetUnreadMessages", &PetitionerService::GetUnreadMessages);
    this->Add("MarkAsRead", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::MarkAsRead));
    this->Add("PetitionerChat", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*, PyRep*)>(&PetitionerService::PetitionerChat));
    this->Add("PetitioneeChat", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*, PyRep*, PyRep*)>(&PetitionerService::PetitioneeChat));

    // actions
    this->Add("CancelPetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::CancelPetition));
    this->Add("ClosePetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::ClosePetition));
    this->Add("DeletePetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::DeletePetition));
    this->Add("ClaimPetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::ClaimPetition));
    this->Add("UnClaimPetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::UnClaimPetition));
    this->Add("EscalatePetition", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*, PyRep*)>(&PetitionerService::EscalatePetition));

    // GM views
    this->Add("GetQueues", &PetitionerService::GetQueues);
    this->Add("GetClaimedPetitions", &PetitionerService::GetClaimedPetitions);
    this->Add("GetPetitionQueue", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::GetPetitionQueue));
    this->Add("GetEvents", &PetitionerService::GetEvents);
    this->Add("GetLog", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, PyInt*)>(&PetitionerService::GetLog));
    this->Add("UpdatePetitionRating", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>)>(&PetitionerService::UpdatePetitionRating));
    this->Add("AddPetitionRating", static_cast<PyResult(PetitionerService::*)(PyCallArgs&, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>, std::optional<PyRep*>)>(&PetitionerService::AddPetitionRating));
}

PyResult PetitionerService::GetUserCatalogCountry(PyCallArgs& call)
{
    return PyStatic.NewZero();
}

// ------------------------------------------------------------------ helpers

static PyRep* PetitionToKeyVal(const DBResultRow& row)
{
    PyDict* p = new PyDict();
    p->SetItemString("petitionID", new PyInt((int32)row.GetUInt(0)));
    p->SetItemString("categoryID", new PyInt((int32)row.GetUInt(1)));
    p->SetItemString("subject",    new PyWString(SafeStr(row.GetText(2))));
    p->SetItemString("petition",   new PyWString(SafeStr(row.GetText(3))));   // body
    p->SetItemString("closed",     new PyBool(row.GetInt(4) == 0));        // status 1 open/0 closed
    p->SetItemString("claimed",    new PyBool(row.GetUInt(5) != 0));       // claimedBy
    p->SetItemString("deleted",    new PyBool(row.GetInt(6) != 0));
    p->SetItemString("updated",    new PyBool(row.GetInt(7) != 0));
    p->SetItemString("petitionerID", new PyInt((int32)row.GetUInt(10)));   // characterID
    p->SetItemString("email",      PyStatic.NewNone());
    p->SetItemString("properties", PyStatic.NewNone());
    p->SetItemString("rateable",   new PyInt(0));
    p->SetItemString("rating",     PyStatic.NewNone());
    p->SetItemString("escalatesTo",PyStatic.NewNone());
    // FILETIME from UNIX timestamps selected in SQL
    p->SetItemString("createDate", new PyLong((int64)(row.GetInt64(8) + 11644473600LL) * 10000000LL));
    p->SetItemString("touchDate",  new PyLong((int64)(row.GetInt64(9) + 11644473600LL) * 10000000LL));
    return new PyObject("util.KeyVal", p);
}

// SELECT shape shared by all "petition row" queries. Date columns are
// UNIX_TIMESTAMP(...) so we can build FILETIME values. touchDate is NOT the
// (often stale/NULL) column: the client's "last modified" must reflect the real
// last activity, so it's derived from the newest thread message (falling back to
// the creation time when a petition has no messages yet).
static const char* PetitionSelect =
    "SELECT petitionID, categoryID, subject, body, status, claimedBy, deleted, updated,"
    " UNIX_TIMESTAMP(createDate),"
    " COALESCE((SELECT UNIX_TIMESTAMP(MAX(m.sentDate)) FROM portal_petition_messages m"
    "           WHERE m.petitionID = portal_petitions.petitionID), UNIX_TIMESTAMP(createDate)),"
    " characterID"
    " FROM portal_petitions";

PyResult PetitionerService::GetCategories(PyCallArgs& call)
{
    // Flat list used by svc.PetitionSvc.GetC_String() to translate a petition's
    // categoryID -> displayName.  Petitions store the logical DB categoryID, so
    // return the calling client's language at those plain ids.
    std::string lang = CategoryLanguage(call.client->GetLanguageID());
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT categoryID, categoryName FROM portal_petition_categories"
        " WHERE languageID = '%s' AND parentCategoryID <> 0 ORDER BY sortOrder, categoryID",
        lang.c_str()))
        return nullptr;

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* c = new PyDict();
        c->SetItemString("categoryID", new PyInt(row.GetInt(0)));
        c->SetItemString("displayName", new PyWString(SafeStr(row.GetText(1))));
        list->AddItem(new PyObject("util.KeyVal", c));
    }
    return list;
}

PyResult PetitionerService::GetCategoryHierarchicalInfo(PyCallArgs& call)
{
    // The wizard asks for the tree once and filters by the client's own language
    // (falling back to English if its language yields too few groups).  Serve
    // every language form at once, using language-offset wire category ids so the
    // entries don't collide in the returned dictionaries.
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT categoryID, parentCategoryID, languageID, categoryName, description"
        " FROM portal_petition_categories ORDER BY languageID, sortOrder, categoryID"))
        return nullptr;

    PyDict* parentDict = new PyDict();   // {wireParentID: (name, lang)}
    PyDict* childDict  = new PyDict();   // {wireParentID: {wireChildID: (name, lang)}}
    PyDict* descDict   = new PyDict();   // {wireChildID: description}
    std::map<int32, PyDict*> childGroups;

    DBResultRow row;
    while (res.GetRow(row)) {
        int32 id        = row.GetInt(0);
        int32 par       = row.GetInt(1);
        std::string dbLang = SafeText(row.GetText(2));
        const char* name = row.GetText(3);
        const char* desc = row.GetText(4);

        std::vector<CategoryLangForm> forms = CategoryLanguageForms(dbLang);
        for (const CategoryLangForm& form : forms) {
            int32 wireID  = WireCategoryID(id, form);
            int32 wirePar = (par == 0) ? 0 : WireCategoryID(par, form);

            // languageID token the client compares against its own GetLanguageID()
            PyRep* langTok = form.numericID > 0
                           ? static_cast<PyRep*>(new PyInt(form.numericID))
                           : static_cast<PyRep*>(new PyString(form.code));

            if (par == 0) {
                PyTuple* t = new PyTuple(2);
                t->SetItem(0, new PyWString(SafeStr(name)));
                t->SetItem(1, langTok);
                parentDict->SetItem(new PyInt(wireID), t);
            } else {
                PyDict* group = nullptr;
                auto it = childGroups.find(wirePar);
                if (it != childGroups.end()) {
                    group = it->second;
                } else {
                    group = new PyDict();
                    childGroups[wirePar] = group;
                    childDict->SetItem(new PyInt(wirePar), group);
                }
                PyTuple* t = new PyTuple(2);
                t->SetItem(0, new PyWString(SafeStr(name)));
                t->SetItem(1, langTok);
                group->SetItem(new PyInt(wireID), t);
                descDict->SetItem(new PyInt(wireID), new PyWString(SafeStr(desc)));
            }
        }
    }

    PyTuple* result = new PyTuple(4);
    result->SetItem(0, parentDict);
    result->SetItem(1, childDict);
    result->SetItem(2, descDict);
    result->SetItem(3, new PyDict());   // billingCategories: none
    return result;
}

PyResult PetitionerService::GetCategoryProperties(PyCallArgs& call, PyRep* categoryID)
{
    return new PyList();
}

PyResult PetitionerService::MayPetition(PyCallArgs& call, PyRep* categoryID, PyRep* oocCharID)
{
    return PyStatic.NewZero();
}

PyResult PetitionerService::PropertyPopulationInfo(PyCallArgs& call, PyRep* propertyID, PyRep* oocCharID)
{
    return new PyList();
}

PyResult PetitionerService::GetClientPickerInfo(PyCallArgs& call, PyRep* filterString, PyRep* elementName)
{
    return new PyList();
}

// ------------------------------------------------------------------ create

PyResult PetitionerService::CreatePetition(PyCallArgs& call,
                                           PyRep* subjectRep, PyRep* petitionRep, PyRep* categoryRep, PyRep* retval,
                                           std::optional<PyRep*> oocCharID,
                                           std::optional<PyRep*> chatLog,
                                           std::optional<PyRep*> combatLog,
                                           std::optional<PyRep*> propertyList)
{
    int32 charID = call.client->GetCharacterID();
    int32 accountID = call.client->GetUserID();

    std::string subject = PyRep::StringContent(subjectRep);
    std::string body    = PyRep::StringContent(petitionRep);
    int32 categoryID    = DecodeCategoryID(static_cast<int32>(PyRep::IntegerValue(categoryRep)));

    if (subject.empty() || body.empty() || categoryID <= 0)
        return new PyBool(false);

    std::string author = SqlEsc(call.client->GetName());
    std::string eSubj  = SqlEsc(subject);
    std::string eBody  = SqlEsc(body);

    DBerror err;
    uint32 petitionID = 0;
    if (!sDatabase.RunQueryLID(err, petitionID,
        "INSERT INTO portal_petitions (accountID, characterID, authorName, categoryID, subject, body, status, deleted, updated, createDate, touchDate)"
        " VALUES (%u, %u, '%s', %u, '%s', '%s', 1, 0, 0, NOW(), NOW())",
        accountID, charID, author.c_str(), categoryID, eSubj.c_str(), eBody.c_str()))
    {
        sLog.Error("Petitioner", "CreatePetition insert failed: %s", err.c_str());
        return new PyBool(false);
    }

    if (!sDatabase.RunQuery(err,
        "INSERT INTO portal_petition_messages (petitionID, senderID, senderName, isGM, comment, text, sentDate)"
        " VALUES (%u, %u, '%s', 0, 0, '%s', NOW())",
        petitionID, charID, author.c_str(), eBody.c_str()))
    {
        sLog.Error("Petitioner", "CreatePetition message insert failed: %s", err.c_str());
        // roll the empty petition back so we never leave a thread-less row
        sDatabase.RunQuery(err, "DELETE FROM portal_petitions WHERE petitionID = %u", petitionID);
        return new PyBool(false);
    }

    sLog.Green("Petitioner", "%s(%u) filed petition #%u cat %u.", call.client->GetName(), charID, petitionID, categoryID);

    // Botting/RMT petitions (601/602) are admin-priority — notify the admin group.
    if (categoryID == 601 || categoryID == 602) {
        std::string tag = categoryID == 601 ? "BOTS/MULTIBOXING" : "RMT";
        TelegramBot::NotifyAdmin(tag + " petition #" + std::to_string(petitionID)
            + " by " + call.client->GetName() + ":\n" + subject);
    }
    return new PyBool(true);
}

// ------------------------------------------------------------------ list/read

PyResult PetitionerService::GetMyPetitionsEx(PyCallArgs& call)
{
    int32 charID = call.client->GetCharacterID();
    int32 accountID = call.client->GetUserID();

    DBQueryResult res;
    // A player sees petitions filed by this character; account-level portal
    // petitions (characterID = 0) filed on the same account are also included.
    if (!sDatabase.RunQuery(res,
        "%s WHERE deleted = 0 AND (characterID = %u OR (characterID = 0 AND accountID = %u))"
        " ORDER BY petitionID DESC",
        PetitionSelect, charID, accountID))
        return nullptr;

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row))
        list->AddItem(PetitionToKeyVal(row));
    return list;
}

PyResult PetitionerService::GetPetitionMessages(PyCallArgs& call, PyInt* petitionID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT messageID, senderID, senderName, comment, text, UNIX_TIMESTAMP(sentDate)"
        " FROM portal_petition_messages WHERE petitionID = %u ORDER BY sentDate ASC, messageID ASC",
        petitionID->value()))
        return nullptr;

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* m = new PyDict();
        m->SetItemString("messageID", new PyInt((int32)row.GetUInt(0)));
        m->SetItemString("senderID",  new PyInt((int32)row.GetUInt(1)));
        m->SetItemString("senderName",new PyWString(SafeStr(row.GetText(2))));
        m->SetItemString("comment",   new PyBool(row.GetInt(3) != 0));
        m->SetItemString("text",      new PyWString(SafeStr(row.GetText(4))));
        m->SetItemString("sentDate",  new PyLong((int64)(row.GetInt64(5) + 11644473600LL) * 10000000LL));
        list->AddItem(new PyObject("util.KeyVal", m));
    }
    return list;
}

PyResult PetitionerService::GetUnreadMessages(PyCallArgs& call)
{
    int32 charID = call.client->GetCharacterID();
    int32 accountID = call.client->GetUserID();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "SELECT m.messageID, m.petitionID, m.text"
        " FROM portal_petition_messages m"
        " JOIN portal_petitions p ON p.petitionID = m.petitionID"
        " WHERE (p.characterID = %u OR (p.characterID = 0 AND p.accountID = %u))"
        "   AND m.isGM = 1 AND p.status = 1 AND p.deleted = 0"
        " ORDER BY m.sentDate DESC LIMIT 20", charID, accountID))
        return nullptr;

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row)) {
        PyDict* m = new PyDict();
        m->SetItemString("messageID", new PyInt((int32)row.GetUInt(0)));
        m->SetItemString("petitionID", new PyInt((int32)row.GetUInt(1)));
        m->SetItemString("text", new PyWString(SafeStr(row.GetText(2))));
        list->AddItem(new PyObject("util.KeyVal", m));
    }
    return list;
}

PyResult PetitionerService::MarkAsRead(PyCallArgs& call, PyInt* messageID)
{
    return nullptr;
}

// ------------------------------------------------------------------ chat

PyResult PetitionerService::PetitionerChat(PyCallArgs& call, PyInt* petitionID, PyRep* message)
{
    int32 charID = call.client->GetCharacterID();
    std::string text = PyRep::StringContent(message);
    if (text.empty())
        return new PyBool(false);

    DBQueryResult chk;
    if (!sDatabase.RunQuery(chk, "SELECT status, deleted FROM portal_petitions WHERE petitionID = %u", petitionID->value()))
        return new PyBool(false);
    DBResultRow r;
    if (!chk.GetRow(r) || r.GetInt(0) != 1 || r.GetInt(1) != 0)
        throw UserError("PetitionClosed");   // client shows 'MessageNotSentPetitionAlreadyClosed'

    std::string author = SqlEsc(call.client->GetName());
    std::string eText  = SqlEsc(text);
    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT INTO portal_petition_messages (petitionID, senderID, senderName, isGM, comment, text, sentDate)"
        " VALUES (%u, %u, '%s', 0, 0, '%s', NOW())",
        petitionID->value(), charID, author.c_str(), eText.c_str());
    // Player has seen the thread again — clear the "GM replied since you looked"
    // marker and touch the row.
    sDatabase.RunQuery(err, "UPDATE portal_petitions SET updated = 0, touchDate = NOW() WHERE petitionID = %u",
        petitionID->value());
    return new PyBool(true);
}

PyResult PetitionerService::PetitioneeChat(PyCallArgs& call, PyInt* petitionID, PyRep* message, PyRep* comment)
{
    std::string text = PyRep::StringContent(message);
    if (text.empty())
        return new PyBool(false);
    bool isComment = (comment != nullptr) && !comment->IsNone() && comment->IsBool() && comment->AsBool()->value();

    DBQueryResult chk;
    if (!sDatabase.RunQuery(chk, "SELECT deleted FROM portal_petitions WHERE petitionID = %u", petitionID->value()))
        return new PyBool(false);
    DBResultRow r;
    if (!chk.GetRow(r) || r.GetInt(0) != 0)
        throw UserError("PetitionClosed");   // client shows 'MessageNotSentPetitionAlreadyClosed'

    std::string author = SqlEsc(call.client->GetName());
    std::string eText  = SqlEsc(text);
    DBerror err;
    sDatabase.RunQuery(err,
        "INSERT INTO portal_petition_messages (petitionID, senderID, senderName, isGM, comment, text, sentDate)"
        " VALUES (%u, %u, '%s', 1, %u, '%s', NOW())",
        petitionID->value(), call.client->GetCharacterID(), author.c_str(), isComment ? 1 : 0, eText.c_str());
    if (!isComment)
        sDatabase.RunQuery(err, "UPDATE portal_petitions SET updated = 1, touchDate = NOW() WHERE petitionID = %u",
            petitionID->value());
    return new PyBool(true);
}

// ------------------------------------------------------------------ actions

PyResult PetitionerService::CancelPetition(PyCallArgs& call, PyInt* petitionID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE portal_petitions SET status = 0 WHERE petitionID = %u AND claimedBy = 0",
        petitionID->value());
    return new PyBool(true);
}

PyResult PetitionerService::ClosePetition(PyCallArgs& call, PyInt* petitionID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE portal_petitions SET status = 0 WHERE petitionID = %u",
        petitionID->value());
    return new PyBool(true);
}

PyResult PetitionerService::DeletePetition(PyCallArgs& call, PyInt* petitionID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE portal_petitions SET deleted = 1 WHERE petitionID = %u",
        petitionID->value());
    return new PyBool(true);
}

PyResult PetitionerService::ClaimPetition(PyCallArgs& call, PyInt* petitionID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE portal_petitions SET claimedBy = %u, updated = 0 WHERE petitionID = %u AND claimedBy = 0",
        call.client->GetCharacterID(), petitionID->value());
    DBQueryResult chk;
    if (!sDatabase.RunQuery(chk, "SELECT claimedBy FROM portal_petitions WHERE petitionID = %u", petitionID->value()))
        return PyStatic.NewZero();
    DBResultRow r;
    if (chk.GetRow(r) && r.GetUInt(0) == (uint32)call.client->GetCharacterID())
        return new PyInt(1);
    return PyStatic.NewZero();
}

PyResult PetitionerService::UnClaimPetition(PyCallArgs& call, PyInt* petitionID)
{
    DBerror err;
    sDatabase.RunQuery(err,
        "UPDATE portal_petitions SET claimedBy = 0 WHERE petitionID = %u AND claimedBy = %u",
        petitionID->value(), call.client->GetCharacterID());
    return nullptr;
}

PyResult PetitionerService::EscalatePetition(PyCallArgs& call, PyInt* petitionID, PyRep* queueID)
{
    return nullptr;
}

// ------------------------------------------------------------------ GM views

PyResult PetitionerService::GetQueues(PyCallArgs& call)
{
    PyList* list = new PyList();
    PyDict* q = new PyDict();
    q->SetItemString("queueID", new PyInt(1));
        q->SetItemString("queueName", new PyWString(std::string("General")));
    list->AddItem(new PyObject("util.KeyVal", q));
    return list;
}

PyResult PetitionerService::GetClaimedPetitions(PyCallArgs& call)
{
    int32 charID = call.client->GetCharacterID();
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "%s WHERE claimedBy = %u AND deleted = 0 ORDER BY petitionID DESC",
        PetitionSelect, charID))
        return nullptr;

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row))
        list->AddItem(PetitionToKeyVal(row));
    return list;
}

PyResult PetitionerService::GetPetitionQueue(PyCallArgs& call, PyInt* queueID)
{
    DBQueryResult res;
    if (!sDatabase.RunQuery(res,
        "%s WHERE claimedBy = 0 AND status = 1 AND deleted = 0 ORDER BY petitionID DESC",
        PetitionSelect))
        return nullptr;

    PyList* list = new PyList();
    DBResultRow row;
    while (res.GetRow(row))
        list->AddItem(PetitionToKeyVal(row));
    return list;
}

PyResult PetitionerService::GetEvents(PyCallArgs& call)
{
    return new PyList();
}

PyResult PetitionerService::GetLog(PyCallArgs& call, PyInt* petitionID)
{
    return new PyList();
}

PyResult PetitionerService::UpdatePetitionRating(PyCallArgs& call, std::optional<PyRep*> petitionID, std::optional<PyRep*> a, std::optional<PyRep*> b, std::optional<PyRep*> c, std::optional<PyRep*> comment)
{
    return nullptr;
}

PyResult PetitionerService::AddPetitionRating(PyCallArgs& call, std::optional<PyRep*> petitionID, std::optional<PyRep*> a, std::optional<PyRep*> b, std::optional<PyRep*> c, std::optional<PyRep*> comment, std::optional<PyRep*> ratingTime)
{
    return nullptr;
}
