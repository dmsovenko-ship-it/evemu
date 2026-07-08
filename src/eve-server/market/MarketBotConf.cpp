
/**
 * @name MarketBotConf.h
 *   system for automating/emulating buy and sell orders on the market.
 * base config code taken from EVEServerConfig
 * idea and some code taken from AuctionHouseBot - Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * @Author:         Allan
 * @date:   10 August 2016
 * @version:  0.15
 */



#include <sstream>
#include <string>
#include <vector>

#include "market/MarketBotConf.h"


MarketBotConf::MarketBotConf()
{
    // register needed parsers
    AddMemberParser( "marketBot", &MarketBotConf::ProcessBotConf );

    // Set sane defaults
    // items with a "N" behind them are NOT implemented
    // items with a "P" behind them are PARTIALLY implemented
    // items with /*x*/ behind them denote time idetifier, with x = (s=seconds, m=minutes, etc)

    // main
    main.EnableRegional = false;//N
    main.EnableConst = false;//N
    main.DataRefreshTime = 15/*m*/;//N
    main.OrderLifetime = 5/*d*/;//N
    main.OrdersPerRefresh = 10;//N
    main.MaxISKPerOrder = 1500000000;//N
    main.QuantityLargeMin = 1000;
    main.QuantityLargeMax = 1000000;
    main.QuantitySmallMin = 10;
    main.QuantitySmallMax = 500;

    // buy
    buy.RegionJumps = 10;//N
    buy.ConstJumps = 8;//N
    buy.SystemJumps = 5;//N
    buy.OrdersPerRegion = 35;//N
    buy.OrdersPerConst = 20;//N
    buy.OrdersPerSystem = 10;//N
    buy.DupeOrdersPerRegion = 10;//N
    buy.DupeOrdersPerConst = 5;//N
    buy.DupeOrdersPerSystem = 2;//N
    buy.MinBuyAmount = 1;//N
    buy.PriceMultiplierMin = 0.8f;
    buy.PriceMultiplierMax = 1.1f;
    buy.QuantityMin = 1000;
    buy.QuantityMax = 1000000;

    // sell
    sell.SellNamedItem = false;//N
    sell.OrdersPerRegion = 20;//N
    sell.OrdersPerConst = 10;//N
    sell.OrdersPerSystem = 2;//N
    sell.DupeOrdersPerRegion = 5;//N
    sell.DupeOrdersPerConst = 3;//N
    sell.DupeOrdersPerSystem = 1;//N
    sell.SellItemMetaLevelMin = 0;//N
    sell.SellItemMetaLevelMax = 4;//N
    sell.MinSellAmount = 1;//N
    sell.PriceMultiplierMin = 1.0f;
    sell.PriceMultiplierMax = 1.3f;
    sell.QuantityMin = 10;
    sell.QuantityMax = 500;

    // default valid groups (minerals, ores, ammo, charges, probes, boosters)
    uint32 defaultGroups[] = {18,83,84,85,86,87,88,89,90,92,372,373,374,375,376,377,
        384,385,386,387,388,389,390,391,392,393,394,395,396,648,653,654,655,656,657,772,
        450,451,452,453,454,455,456,457,458,459,460,461,462,465,466,467,468,469,
        479,482,492,538,548,663,303};
    validGroups.assign(defaultGroups, defaultGroups + sizeof(defaultGroups)/sizeof(uint32));
}

bool MarketBotConf::ProcessBotConf(const TiXmlElement* ele)
{
    // entering element, extend allowed syntax
    AddMemberParser( "main",      &MarketBotConf::ProcessMain );
    AddMemberParser( "buy",       &MarketBotConf::ProcessBuy );
    AddMemberParser( "sell",      &MarketBotConf::ProcessSell );
    AddMemberParser( "groups",    &MarketBotConf::ProcessGroups );

    // parse the element
    const bool result = ParseElementChildren( ele );

    // leaving element, reduce allowed syntax
    RemoveParser( "main" );
    RemoveParser( "buy" );
    RemoveParser( "sell" );
    RemoveParser( "groups" );

    // return status of parsing
    return result;
}

bool MarketBotConf::ProcessGroups(const TiXmlElement* ele)
{
    const char* text = ele->GetText();
    if (text == nullptr)
        return true;

    validGroups.clear();
    std::string groupsStr(text);
    std::stringstream ss(groupsStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // trim whitespace
        token.erase(0, token.find_first_not_of(" \t"));
        token.erase(token.find_last_not_of(" \t") + 1);
        if (!token.empty()) {
            try {
                uint32 groupID = static_cast<uint32>(std::stoul(token));
                if (groupID > 0)
                    validGroups.push_back(groupID);
            } catch (...) {
                // skip invalid tokens
            }
        }
    }
    return true;
}

bool MarketBotConf::ProcessMain(const TiXmlElement* ele)
{
    AddValueParser( "EnableRegional",           main.EnableRegional );
    AddValueParser( "EnableConst",              main.EnableConst );
    AddValueParser( "DataRefreshTime",          main.DataRefreshTime );
    AddValueParser( "OrderLifetime",            main.OrderLifetime );
    AddValueParser( "OrdersPerRefresh",         main.OrdersPerRefresh );
    AddValueParser( "MaxISKPerOrder",           main.MaxISKPerOrder );
    AddValueParser( "QuantityLargeMin",          main.QuantityLargeMin );
    AddValueParser( "QuantityLargeMax",          main.QuantityLargeMax );
    AddValueParser( "QuantitySmallMin",          main.QuantitySmallMin );
    AddValueParser( "QuantitySmallMax",          main.QuantitySmallMax );

    const bool result = ParseElementChildren( ele );

    RemoveParser( "EnableRegional" );
    RemoveParser( "EnableConst" );
    RemoveParser( "DataRefreshTime" );
    RemoveParser( "OrderLifetime" );
    RemoveParser( "OrdersPerRefresh" );
    RemoveParser( "MaxISKPerOrder" );
    RemoveParser( "QuantityLargeMin" );
    RemoveParser( "QuantityLargeMax" );
    RemoveParser( "QuantitySmallMin" );
    RemoveParser( "QuantitySmallMax" );

    return result;
}

bool MarketBotConf::ProcessBuy(const TiXmlElement* ele)
{
    AddValueParser( "MinBuyAmount",             buy.MinBuyAmount );
    AddValueParser( "RegionJumps",              buy.RegionJumps );
    AddValueParser( "ConstJumps",               buy.ConstJumps );
    AddValueParser( "SystemJumps",              buy.SystemJumps );
    AddValueParser( "OrdersPerRegion",          buy.OrdersPerRegion );
    AddValueParser( "OrdersPerConst",           buy.OrdersPerConst );
    AddValueParser( "OrdersPerSystem",          buy.OrdersPerSystem );
    AddValueParser( "DupeOrdersPerRegion",      buy.DupeOrdersPerRegion );
    AddValueParser( "DupeOrdersPerConst",       buy.DupeOrdersPerConst );
    AddValueParser( "DupeOrdersPerSystem",      buy.DupeOrdersPerSystem );
    AddValueParser( "PriceMultiplierMin",        buy.PriceMultiplierMin );
    AddValueParser( "PriceMultiplierMax",        buy.PriceMultiplierMax );
    AddValueParser( "QuantityMin",              buy.QuantityMin );
    AddValueParser( "QuantityMax",              buy.QuantityMax );

    const bool result = ParseElementChildren( ele );

    RemoveParser( "MinBuyAmount" );
    RemoveParser( "RegionJumps" );
    RemoveParser( "ConstJumps" );
    RemoveParser( "SystemJumps" );
    RemoveParser( "OrdersPerRegion" );
    RemoveParser( "OrdersPerConst" );
    RemoveParser( "OrdersPerSystem" );
    RemoveParser( "DupeOrdersPerRegion" );
    RemoveParser( "DupeOrdersPerConst" );
    RemoveParser( "DupeOrdersPerSystem" );
    RemoveParser( "PriceMultiplierMin" );
    RemoveParser( "PriceMultiplierMax" );
    RemoveParser( "QuantityMin" );
    RemoveParser( "QuantityMax" );

    return result;
}

bool MarketBotConf::ProcessSell(const TiXmlElement* ele)
{
    AddValueParser( "SellNamedItem",            sell.SellNamedItem );
    AddValueParser( "MinSellAmount",            sell.MinSellAmount );
    AddValueParser( "OrdersPerRegion",          sell.OrdersPerRegion );
    AddValueParser( "OrdersPerConst",           sell.OrdersPerConst );
    AddValueParser( "OrdersPerSystem",          sell.OrdersPerSystem );
    AddValueParser( "DupeOrdersPerRegion",      sell.DupeOrdersPerRegion );
    AddValueParser( "DupeOrdersPerConst",       sell.DupeOrdersPerConst );
    AddValueParser( "DupeOrdersPerSystem",      sell.DupeOrdersPerSystem );
    AddValueParser( "SellItemMetaLevelMin",     sell.SellItemMetaLevelMin );
    AddValueParser( "SellItemMetaLevelMax",     sell.SellItemMetaLevelMax );
    AddValueParser( "PriceMultiplierMin",        sell.PriceMultiplierMin );
    AddValueParser( "PriceMultiplierMax",        sell.PriceMultiplierMax );
    AddValueParser( "QuantityMin",              sell.QuantityMin );
    AddValueParser( "QuantityMax",              sell.QuantityMax );

    const bool result = ParseElementChildren( ele );

    RemoveParser( "SellNamedItem" );
    RemoveParser( "MinSellAmount" );
    RemoveParser( "OrdersPerRegion" );
    RemoveParser( "OrdersPerConst" );
    RemoveParser( "OrdersPerSystem" );
    RemoveParser( "DupeOrdersPerRegion" );
    RemoveParser( "DupeOrdersPerConst" );
    RemoveParser( "DupeOrdersPerSystem" );
    RemoveParser( "SellItemMetaLevelMax" );
    RemoveParser( "SellItemMetaLevelMin" );
    RemoveParser( "PriceMultiplierMin" );
    RemoveParser( "PriceMultiplierMax" );
    RemoveParser( "QuantityMin" );
    RemoveParser( "QuantityMax" );

    return result;
}

