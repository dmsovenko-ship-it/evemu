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
    Author:     caytchen
*/

#include "eve-server.h"

#include "LiveUpdateDB.h"
#include <fstream>

PyList* LiveUpdateDB::GenerateUpdates()
{
    const char* query = "SELECT"
            " updateID,"
            " updateName,"
            " description,"
            " machoVersionMin,"
            " machoVersionMax,"
            " buildNumberMin,"  //5
            " buildNumberMax,"
            " methodName,"
            " objectID,"
            " codeType,"
            " code"             //10
            " FROM liveupdates";
    DBQueryResult res;

    if (!sDatabase.RunQuery(res, query))
    {
        codelog(DATABASE__ERROR, "Couldn't get live updates from database: %s", res.error.c_str());
        return nullptr;
    }

    // setup the descriptor
    DBRowDescriptor* header = new DBRowDescriptor();
    header->AddColumn("updateID", DBTYPE_I4);
    header->AddColumn("updateName", DBTYPE_WSTR);
    header->AddColumn("description", DBTYPE_WSTR);
    header->AddColumn("machoVersionMin", DBTYPE_I4);
    header->AddColumn("machoVersionMax", DBTYPE_I4);
    header->AddColumn("buildNumberMin", DBTYPE_I4);
    header->AddColumn("buildNumberMax", DBTYPE_I4);
    header->AddColumn("code", DBTYPE_STR);

    // count rows + 2 for news ticker patch (holoscreen patch + RPC fallback)
    uint32 rowCount = res.GetRowCount();
    PyList* list = new PyList(rowCount + 2);
    int listIndex = 0;
    DBResultRow row;
    while (res.GetRow(row))
    {
        PyPackedRow* packedRow = new PyPackedRow(header);
        for (int i = 0; i < 7; i++)
            packedRow->SetField(i, DBColumnToPyRep(row, i));

        LiveUpdateInner inner;
        // binary data so we can't expect strlen to get it right
        inner.code = std::string(row.GetText(10), row.ColumnLength(10));
        inner.codeType = row.GetText(9);
        inner.objectID = row.GetText(8);
        inner.methodName = row.GetText(7);
        packedRow->SetField(static_cast<uint32>(7) /* code */, inner.Encode());

        list->SetItem(listIndex++, packedRow);
    }

    // add news ticker live update with latest commit title
    {
        // read latest commit message
        std::string commitMsg = "Welcome to EVEmu Crucible";
        std::ifstream gitHead("/src/.git/HEAD");
        if (gitHead.is_open()) {
            std::string ref;
            std::getline(gitHead, ref);
            gitHead.close();
            if (ref.compare(0, 5, "ref: ") == 0) {
                std::string refPath = "/src/.git/" + ref.substr(5);
                std::ifstream gitRef(refPath.c_str());
                if (gitRef.is_open()) {
                    std::string sha;
                    std::getline(gitRef, sha);
                    gitRef.close();
                    std::string logCmd = "git log -1 --format=\"%s\" " + sha + " 2>/dev/null";
                    FILE* fp = popen(logCmd.c_str(), "r");
                    if (fp) {
                        char buf[256];
                        if (fgets(buf, sizeof(buf), fp))
                            commitMsg = std::string(buf);
                        while (commitMsg.size() > 0 && (commitMsg.back() == '\n' || commitMsg.back() == '\r'))
                            commitMsg.pop_back();
                        pclose(fp);
                    }
                }
            }
        }

        // escape for Python string
        std::string safeMsg;
        for (char c : commitMsg) {
            if (c == '\'') safeMsg += "\\'";
            else if (c == '\\') safeMsg += "\\\\";
            else if (c == '\n') safeMsg += "\\n";
            else safeMsg += c;
        }

        std::string pyCode =
            "import service\n"
            "import util\n"
            "try:\n"
            "    # Patch the client-side holoscreen service to call server RPC for news\n"
            "    svc = sm.GetService('holoscreen')\n"
            "    if svc:\n"
            "        orig = svc.GetNewsTickerData\n"
            "        def _patched_news(self):\n"
            "            try:\n"
            "                from xml.dom.minidom import parseString\n"
            "                xml = sm.RemoteSvc('holoscreenMgr').GetNewsTickerData()\n"
            "                if xml and len(xml):\n"
            "                    return parseString(xml)\n"
            "            except:\n"
            "                pass\n"
            "            return orig()\n"
            "        svc.GetNewsTickerData = _patched_news.__get__(svc, type(svc))\n"
            "except:\n"
            "    pass\n";
        // Fallback: also replace the holoscreenMgr RPC method
        std::string rpcCode =
            "from xml.dom.minidom import parseString\n"
            "import blue\n"
            "msg = '" + safeMsg + "'\n"
            "now = blue.os.GetWallclockTime()\n"
            "xml = '<?xml version=\"1.0\"?>'\n"
            "xml += '<news><item>'\n"
            "xml += '<title>EVEmu Crucible</title>'\n"
            "xml += '<text>' + msg + '</text>'\n"
            "xml += '<date>' + str(blue.os.GetTimeParts(now)[:3]) + '</date>'\n"
            "xml += '</item></news>'\n"
            "return parseString(xml)\n";

        uint32 now = static_cast<uint32>(GetFileTimeNow() / 10000000LL - 11644473600LL);

        // Row 1: Patch client-side holoscreen service to call server RPC for news
        {
            PyPackedRow* packedRow = new PyPackedRow(header);
            packedRow->SetField(uint32(0), static_cast<PyRep*>(new PyInt(998)));
            packedRow->SetField(uint32(1), static_cast<PyRep*>(new PyWString(std::string("HoloscreenPatch"))));
            packedRow->SetField(uint32(2), static_cast<PyRep*>(new PyWString(std::string("Patches holoscreenSvc.GetNewsTickerData to call server RPC"))));
            packedRow->SetField(uint32(3), static_cast<PyRep*>(PyStatic.NewInt(0)));
            packedRow->SetField(uint32(4), static_cast<PyRep*>(new PyInt(999999)));
            packedRow->SetField(uint32(5), static_cast<PyRep*>(PyStatic.NewInt(0)));
            packedRow->SetField(uint32(6), static_cast<PyRep*>(new PyInt(999999)));

            LiveUpdateInner inner;
            inner.code = pyCode;
            inner.codeType = "";  // unrecognized → client exec's it as raw Python
            inner.objectID = "";
            inner.methodName = "";
            packedRow->SetField(uint32(7), inner.Encode());
            list->SetItem(listIndex++, packedRow);
        }

        // Row 2: RPC method fallback (keeps holoscreenMgr.GetNewsTickerData working)
        {
            PyPackedRow* packedRow = new PyPackedRow(header);
            packedRow->SetField(uint32(0), static_cast<PyRep*>(new PyInt(999)));
            packedRow->SetField(uint32(1), static_cast<PyRep*>(new PyWString(std::string("NewsTicker"))));
            packedRow->SetField(uint32(2), static_cast<PyRep*>(new PyWString(std::string("Server news ticker data provider"))));
            packedRow->SetField(uint32(3), static_cast<PyRep*>(PyStatic.NewInt(0)));
            packedRow->SetField(uint32(4), static_cast<PyRep*>(new PyInt(999999)));
            packedRow->SetField(uint32(5), static_cast<PyRep*>(PyStatic.NewInt(0)));
            packedRow->SetField(uint32(6), static_cast<PyRep*>(new PyInt(999999)));

            LiveUpdateInner inner;
            inner.code = rpcCode;
            inner.codeType = "method";
            inner.objectID = "holoscreenMgr";
            inner.methodName = "GetNewsTickerData";
            packedRow->SetField(7, inner.Encode());
            list->SetItem(listIndex++, packedRow);
        }
    }

    list->Dump(NET__PRES_DEBUG, "    ");

    return list;
}
