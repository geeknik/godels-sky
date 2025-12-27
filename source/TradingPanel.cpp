/* TradingPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "TradingPanel.h"

#include "Color.h"
#include "Command.h"
#include "EconomicState.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "Government.h"
#include "Information.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Screen.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <string>

using namespace std;

namespace {
	const string TRADE_LEVEL[] = {
		"(very low)",
		"(low)",
		"(medium)",
		"(high)",
		"(very high)"
	};

	const int NAME_X = 20;
	const int PRICE_X = 140;
	const int LEVEL_X = 180;
	const int PROFIT_X = 260;
	const int BUY_X = 310;
	const int SELL_X = 370;
	const int HOLD_X = 430;
}



TradingPanel::TradingPanel(PlayerInfo &player)
	: player(player), system(*player.GetSystem()), COMMODITY_COUNT(GameData::Commodities().size())
{
	SetTrapAllEvents(false);
}



TradingPanel::~TradingPanel()
{
	if(profit)
	{
		string message = "You sold " + Format::CargoString(tonsSold, "cargo ");

		if(profit < 0)
			message += "at a loss of " + Format::CreditString(-profit) + ".";
		else
			message += "for a total profit of " + Format::CreditString(profit) + ".";

		Messages::Add({message, GameData::MessageCategories().Get("normal")});
	}
}



void TradingPanel::Step()
{
	DoHelp("trading");
}



void TradingPanel::Draw()
{
	const Interface *tradeUi = GameData::Interfaces().Get(Screen::Width() < 1280 ? "trade (small screen)" : "trade");
	const Rectangle box = tradeUi->GetBox("content");
	const int MIN_X = box.Left();
	const int FIRST_Y = box.Top();

	const Color &back = *GameData::Colors().Get("faint");
	int selectedRow = player.MapColoring();
	if(selectedRow >= 0 && selectedRow < COMMODITY_COUNT)
	{
		const Point center(MIN_X + box.Width() / 2, FIRST_Y + 20 * selectedRow + 33);
		const Point dimensions(box.Width() - 20., 20.);
		FillShader::Fill(center, dimensions, back);
	}

	const Font &font = FontSet::Get(14);
	const Color &unselected = *GameData::Colors().Get("medium");
	const Color &selected = *GameData::Colors().Get("bright");

	int y = FIRST_Y;
	font.Draw("Commodity", Point(MIN_X + NAME_X, y), selected);
	font.Draw("Price", Point(MIN_X + PRICE_X, y), selected);

	string mod = "x " + to_string(Modifier());
	font.Draw(mod, Point(MIN_X + BUY_X, y), unselected);
	font.Draw(mod, Point(MIN_X + SELL_X, y), unselected);

	font.Draw("In Hold", Point(MIN_X + HOLD_X, y), selected);

	y += 5;
	int lastY = y + 20 * COMMODITY_COUNT + 25;
	font.Draw("free:", Point(MIN_X + SELL_X + 5, lastY), selected);
	font.Draw(to_string(player.Cargo().Free()), Point(MIN_X + HOLD_X, lastY), selected);

	int outfits = player.Cargo().OutfitsSize();
	int missionCargo = player.Cargo().MissionCargoSize();
	sellOutfits = false;
	if(player.Cargo().HasOutfits() || missionCargo)
	{
		bool hasOutfits = false;
		bool hasMinables = false;
		for(const auto &it : player.Cargo().Outfits())
			if(it.second)
			{
				bool isMinable = it.first->Get("minable");
				(isMinable ? hasMinables : hasOutfits) = true;
			}
		sellOutfits = (hasOutfits && !hasMinables);

		string str = Format::MassString(outfits + missionCargo) + " of ";
		if(hasMinables && missionCargo)
			str += "mission cargo and other items.";
		else if(hasOutfits && missionCargo)
			str += "outfits and mission cargo.";
		else if(hasOutfits && hasMinables)
			str += "outfits and special commodities.";
		else if(hasOutfits)
			str += "outfits.";
		else if(hasMinables)
			str += "special commodities.";
		else
			str += "mission cargo.";
		font.Draw(str, Point(MIN_X + NAME_X, lastY), unselected);
	}

	const SystemEconomy &economy = GameData::GetEconomicManager().GetSystemEconomy(&system);
	const Government *gov = system.GetGovernment();
	double reputation = gov ? gov->Reputation() : 0.0;
	bool isBlackMarket = economy.IsBlackMarketOnly();

	// Display economic state indicator (for non-STABLE states).
	EconomicStateType state = economy.GetState();
	if(state != EconomicStateType::STABLE && state != EconomicStateType::LOCKDOWN)
	{
		const Color *stateColor = nullptr;
		switch(state)
		{
			case EconomicStateType::BOOM:
				stateColor = GameData::Colors().Get("escort selected");
				break;
			case EconomicStateType::BUST:
				stateColor = GameData::Colors().Get("dead");
				break;
			case EconomicStateType::SHORTAGE:
				stateColor = GameData::Colors().Get("available job");
				break;
			case EconomicStateType::SURPLUS:
				stateColor = GameData::Colors().Get("active mission");
				break;
			default:
				stateColor = GameData::Colors().Get("medium");
				break;
		}
		string stateText = "Economy: " + economy.GetStateDescription();
		font.Draw(stateText, Point(MIN_X + NAME_X, y + 20), *stateColor);
		y += 20;
	}

	if(isBlackMarket)
	{
		const Color &warning = *GameData::Colors().Get("dim");
		font.Draw("BLACK MARKET - Trade at your own risk!", Point(MIN_X + NAME_X, y + 20), warning);
		y += 20;
	}

	int i = 0;
	bool canSell = false;
	bool canBuy = false;
	bool showProfit = false;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		y += 20;
		int basePrice = system.Trade(commodity.name);
		double buyMod, sellMod;
		if(isBlackMarket)
		{
			buyMod = economy.GetBlackMarketModifier(true);
			sellMod = economy.GetBlackMarketModifier(false);
		}
		else
		{
			double econBuyMod = economy.GetPriceModifier(commodity.name, true);
			double econSellMod = economy.GetPriceModifier(commodity.name, false);
			double repBuyMod = economy.GetReputationModifier(reputation, true);
			double repSellMod = economy.GetReputationModifier(reputation, false);
			buyMod = econBuyMod * repBuyMod;
			sellMod = econSellMod * repSellMod;
		}
		int buyPrice = static_cast<int>(basePrice * buyMod);
		int sellPrice = static_cast<int>(basePrice * sellMod);
		int hold = player.Cargo().Get(commodity.name);

		bool isSelected = (i++ == selectedRow);
		const Color &color = (isSelected ? selected : unselected);
		font.Draw(commodity.name, Point(MIN_X + NAME_X, y), color);

		if(basePrice)
		{
			canBuy |= isSelected;
			font.Draw(to_string(buyPrice), Point(MIN_X + PRICE_X, y), color);

			int basis = player.GetBasis(commodity.name);
			if(basis && basis != sellPrice && hold)
			{
				string profit = to_string(sellPrice - basis);
				font.Draw(profit, Point(MIN_X + PROFIT_X, y), color);
				showProfit = true;
			}
			int level = (basePrice - commodity.low);
			if(level < 0)
				level = 0;
			else if(level >= (commodity.high - commodity.low))
				level = 4;
			else
				level = (5 * level) / (commodity.high - commodity.low);
			font.Draw(TRADE_LEVEL[level], Point(MIN_X + LEVEL_X, y), color);

			font.Draw("[buy]", Point(MIN_X + BUY_X, y), color);
			font.Draw("[sell]", Point(MIN_X + SELL_X, y), color);
		}
		else
		{
			font.Draw("----", Point(MIN_X + PRICE_X, y), color);
			font.Draw("(not for sale)", Point(MIN_X + LEVEL_X, y), color);
		}

		if(hold)
		{
			sellOutfits = false;
			canSell |= (basePrice != 0);
			font.Draw(to_string(hold), Point(MIN_X + HOLD_X, y), selected);
		}
	}

	if(showProfit)
		font.Draw("Profit", Point(MIN_X + PROFIT_X, FIRST_Y), selected);

	Information info;
	if(sellOutfits)
		info.SetCondition("can sell outfits");
	else if(player.Cargo().HasOutfits() || canSell)
		info.SetCondition("can sell");
	if(player.Cargo().Free() > 0 && canBuy)
		info.SetCondition("can buy");
	tradeUi->Draw(info, this);
}



// Only override the ones you need; the default action is to return false.
bool TradingPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(command.Has(Command::HELP))
		DoHelp("trading", true);
	else if(key == SDLK_UP)
		player.SetMapColoring(max(0, player.MapColoring() - 1));
	else if(key == SDLK_DOWN)
		player.SetMapColoring(max(0, min(COMMODITY_COUNT - 1, player.MapColoring() + 1)));
	else if(key == SDLK_EQUALS || key == SDLK_KP_PLUS || key == SDLK_PLUS || key == SDLK_RETURN || key == SDLK_SPACE)
		Buy(1);
	else if(key == SDLK_MINUS || key == SDLK_KP_MINUS || key == SDLK_BACKSPACE || key == SDLK_DELETE)
		Buy(-1);
	else if(key == 'u' || key == 'B' || (key == 'b' && (mod & KMOD_SHIFT)))
		Buy(1000000000);
	else if(key == 'e' || key == 'S' || (key == 's' && (mod & KMOD_SHIFT)))
	{
		const SystemEconomy &economy = GameData::GetEconomicManager().GetSystemEconomy(&system);
		const Government *gov = system.GetGovernment();
		bool isBlackMarket = economy.IsBlackMarketOnly();
		double reputation = gov ? gov->Reputation() : 0.0;
		bool wasDetected = false;
		for(const auto &it : player.Cargo().Commodities())
		{
			const string &commodity = it.first;
			const int64_t &amount = it.second;
			int64_t basePrice = system.Trade(commodity);
			if(!basePrice || !amount)
				continue;

			double sellMod;
			if(isBlackMarket)
				sellMod = economy.GetBlackMarketModifier(false);
			else
			{
				double econSellMod = economy.GetPriceModifier(commodity, false);
				double repSellMod = economy.GetReputationModifier(reputation, false);
				sellMod = econSellMod * repSellMod;
			}
			int64_t price = static_cast<int64_t>(basePrice * sellMod);

			int64_t basis = player.GetBasis(commodity, -amount);
			profit += amount * price + basis;
			tonsSold += amount;

			GameData::AddPurchase(system, commodity, -amount);
			player.AdjustBasis(commodity, basis);
			player.Accounts().AddCredits(amount * price);
			player.Cargo().Remove(commodity, amount);

			int tons = static_cast<int>(amount);
			GameData::GetEconomicManager().RecordEvent(&system,
				EconomicEventType::TRADE_COMPLETED, tons, commodity, true);
			if(tons >= 500)
				GameData::GetEconomicManager().RecordEvent(&system,
					EconomicEventType::LARGE_SALE, tons, commodity, true);

			if(isBlackMarket && !wasDetected && Random::Real() < economy.GetBlackMarketDetectionChance())
			{
				GameData::GetEconomicManager().RecordEvent(&system,
					EconomicEventType::SMUGGLING_DETECTED, tons, commodity, true);
				wasDetected = true;
			}
		}
		if(wasDetected)
			Messages::Add({"Your black market dealings have been detected!",
				GameData::MessageCategories().Get("high")});
		int day = player.GetDate().DaysSinceEpoch();
		for(const auto &it : player.Cargo().Outfits())
		{
			const Outfit * const outfit = it.first;
			const int64_t &amount = it.second;
			if(outfit->Get("minable") <= 0. && !sellOutfits)
				continue;

			int64_t value = player.FleetDepreciation().Value(outfit, day, amount);
			profit += value;
			tonsSold += static_cast<int>(amount * outfit->Mass());

			player.AddStock(outfit, amount);
			player.Accounts().AddCredits(value);
			player.Cargo().Remove(outfit, amount);
		}
	}
	else if(command.Has(Command::MAP))
		GetUI()->Push(new MapDetailPanel(player));
	else
		return false;

	return true;
}



bool TradingPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT)
		return false;

	const Interface *tradeUi = GameData::Interfaces().Get(Screen::Width() < 1280 ? "trade (small screen)" : "trade");
	const Rectangle box = tradeUi->GetBox("content");
	const int MIN_X = box.Left();
	const int FIRST_Y = box.Top();
	const int MAX_X = box.Right();
	int maxY = FIRST_Y + 25 + 20 * COMMODITY_COUNT;
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		player.SetMapColoring((y - FIRST_Y - 25) / 20);
		if(x >= MIN_X + BUY_X && x < MIN_X + SELL_X)
			Buy(1);
		else if(x >= MIN_X + SELL_X && x < MIN_X + HOLD_X)
			Buy(-1);
	}
	else
		return false;

	return true;
}



void TradingPanel::Buy(int64_t amount)
{
	int selectedRow = player.MapColoring();
	if(selectedRow < 0 || selectedRow >= COMMODITY_COUNT)
		return;

	amount *= Modifier();
	const string &type = GameData::Commodities()[selectedRow].name;
	int64_t basePrice = system.Trade(type);
	if(!basePrice)
		return;

	const SystemEconomy &economy = GameData::GetEconomicManager().GetSystemEconomy(&system);
	const Government *gov = system.GetGovernment();
	bool buying = (amount > 0);
	bool isBlackMarket = economy.IsBlackMarketOnly();

	double priceMod;
	if(isBlackMarket)
		priceMod = economy.GetBlackMarketModifier(buying);
	else
	{
		double reputation = gov ? gov->Reputation() : 0.0;
		double econMod = economy.GetPriceModifier(type, buying);
		double repMod = economy.GetReputationModifier(reputation, buying);
		priceMod = econMod * repMod;
	}
	int64_t price = static_cast<int64_t>(basePrice * priceMod);

	if(amount > 0)
	{
		amount = min(amount, min<int64_t>(player.Cargo().Free(), player.Accounts().Credits() / price));
		player.AdjustBasis(type, amount * price);
	}
	else
	{
		amount = max<int64_t>(amount, -player.Cargo().Get(type));

		int64_t basis = player.GetBasis(type, amount);
		player.AdjustBasis(type, basis);
		profit += -amount * price + basis;
		tonsSold += -amount;
	}
	amount = player.Cargo().Add(type, amount);
	player.Accounts().AddCredits(-amount * price);
	GameData::AddPurchase(system, type, amount);

	if(amount != 0)
	{
		int tons = static_cast<int>(abs(amount));
		GameData::GetEconomicManager().RecordEvent(&system,
			EconomicEventType::TRADE_COMPLETED, tons, type, true);

		if(tons >= 500)
		{
			if(amount > 0)
				GameData::GetEconomicManager().RecordEvent(&system,
					EconomicEventType::LARGE_PURCHASE, tons, type, true);
			else
				GameData::GetEconomicManager().RecordEvent(&system,
					EconomicEventType::LARGE_SALE, tons, type, true);
		}

		if(isBlackMarket && Random::Real() < economy.GetBlackMarketDetectionChance())
		{
			GameData::GetEconomicManager().RecordEvent(&system,
				EconomicEventType::SMUGGLING_DETECTED, tons, type, true);
			Messages::Add({"Your black market dealings have been detected!",
				GameData::MessageCategories().Get("high")});
		}
	}
}
