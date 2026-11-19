#include "config.h"
#include "bot_theater.h"
#include "game.h"
#include "duelclient.h"
#include "network.h"
#include "image_manager.h"
#include "materials.h"
#include "client_card.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <cwctype>
#include <random>
#include "myfilesystem.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <vector>
#include <atomic>
#include <thread>
#ifdef _WIN32
#include <dbghelp.h>
#endif

namespace ygo {

BotTheater botTheater;

static const char* CONFIG_FILE = "bot_theater.conf";
static const wchar_t* LOG_FILE = L"bot_theater.log";
static const char* RESULTS_FILE = "bot_theater_results.txt";	// duel results, read by the script
static const char* MESSAGES_FILE = "bot_theater_messages.txt";	// announcements written by the script
static const int EXIT_TOURNAMENT_OVER = 10;	// the script's exit code once a tournament has its champion

static std::wstring FromUTF8(const char* s) {
	std::vector<wchar_t> buf(std::strlen(s) + 1);
	BufferIO::DecodeUTF8String(s, buf.data(), (int)buf.size());
	return std::wstring(buf.data());
}

static std::string ToUTF8(const std::wstring& s) {
	std::vector<char> buf(s.size() * 4 + 1);
	BufferIO::EncodeUTF8String(s.c_str(), buf.data(), (int)buf.size());
	return std::string(buf.data());
}

static std::string Trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	if(start == std::string::npos)
		return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

static int ToInt(const std::string& s, int fallback) {
	char* end = nullptr;
	long v = std::strtol(s.c_str(), &end, 10);
	return (end && end != s.c_str()) ? (int)v : fallback;
}

void BotTheater::Load() {
	FILE* fp = std::fopen(CONFIG_FILE, "r");
	if(!fp) {
		Save();	// first run: write a file with the defaults so it can be edited
		return;
	}
	char line[1024];
	bool first = true;
	while(std::fgets(line, sizeof line, fp)) {
		char* p = line;
		if(first && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
			p += 3;	// skip a UTF-8 marker added by Notepad
		first = false;
		std::string text = Trim(p);
		if(text.empty() || text[0] == '#')
			continue;
		size_t eq = text.find('=');
		if(eq == std::string::npos)
			continue;
		std::string key = Trim(text.substr(0, eq));
		std::string value = Trim(text.substr(eq + 1));
		if(key == "enabled") enabled = ToInt(value, 1) != 0;
		else if(key == "skip_host_window") skip_host_window = ToInt(value, 0) != 0;
		else if(key == "reveal_hands") reveal_hands = ToInt(value, 1) != 0;
		else if(key == "info_follows_actions") info_follows_actions = ToInt(value, 1) != 0;
		else if(key == "save_replays") save_replays = ToInt(value, 0) != 0;
		else if(key == "turn_highlight") turn_highlight = ToInt(value, 1) != 0;
		else if(key == "turn_glow") turn_glow = ToInt(value, 1) != 0;
		else if(key == "turn_glow_opacity") turn_glow_opacity = ToInt(value, turn_glow_opacity);
		else if(key == "holograms") holograms_on = ToInt(value, 1) != 0;
		else if(key == "hologram_size") hologram_size = ToInt(value, hologram_size);
		else if(key == "hologram_seconds") hologram_seconds = std::strtod(value.c_str(), nullptr);
		else if(key == "hologram_opacity") hologram_opacity = ToInt(value, hologram_opacity);
		else if(key == "hologram_stay") hologram_stay = ToInt(value, 1) != 0;
		else if(key == "hologram_rest_size") hologram_rest_size = ToInt(value, hologram_rest_size);
		else if(key == "hologram_rest_opacity") hologram_rest_opacity = ToInt(value, hologram_rest_opacity);
		else if(key == "hologram_backrow") hologram_backrow = ToInt(value, 1) != 0;
		else if(key == "hologram_backrow_size") hologram_backrow_size = ToInt(value, hologram_backrow_size);
		else if(key == "hologram_backrow_lift_bottom") hologram_backrow_lift_bottom = ToInt(value, hologram_backrow_lift_bottom);
		else if(key == "hologram_backrow_lift_top") hologram_backrow_lift_top = ToInt(value, hologram_backrow_lift_top);
		else if(key == "top_hand_raise") top_hand_raise = std::strtod(value.c_str(), nullptr);
		else if(key == "split_zones") split_zones = ToInt(value, 0) != 0;
		else if(key == "clockwise_tag") clockwise_tag = ToInt(value, 1) != 0;
		else if(key == "random_backgrounds") random_backgrounds = ToInt(value, 1) != 0;
		else if(key == "custom_background_field") custom_background_field = ToInt(value, 0) != 0;
		else if(key == "tournament") tournament = ToInt(value, 0) != 0;
		else if(key == "tournament_entrants") tournament_entrants = ToInt(value, tournament_entrants);
		else if(key == "tournament_format") tournament_format = (value.find("robin") != std::string::npos || value == "1") ? 1 : 0;
		else if(key == "tournament_pause") tournament_pause = ToInt(value, tournament_pause);
		else if(key == "python") python = FromUTF8(value.c_str());
		else if(key == "script") script = FromUTF8(value.c_str());
		else if(key == "script_args") script_args = FromUTF8(value.c_str());
		else if(key == "lflist") lflist_name = FromUTF8(value.c_str());
		else if(key == "allowed_cards") allowed_cards = ToInt(value, allowed_cards);
		else if(key == "mode") mode = ToInt(value, mode);
		else if(key == "time_limit") time_limit = ToInt(value, time_limit);
		else if(key == "duel_rule") duel_rule = ToInt(value, duel_rule);
		else if(key == "no_check_deck") no_check_deck = ToInt(value, 1) != 0;
		else if(key == "no_shuffle_deck") no_shuffle_deck = ToInt(value, 0) != 0;
		else if(key == "start_lp") start_lp = ToInt(value, start_lp);
		else if(key == "start_hand") start_hand = ToInt(value, start_hand);
		else if(key == "draw_count") draw_count = ToInt(value, draw_count);
		else if(key == "password") password = FromUTF8(value.c_str());
	}
	std::fclose(fp);
}

void BotTheater::Save() const {
	FILE* fp = std::fopen(CONFIG_FILE, "w");
	if(!fp)
		return;
	std::fprintf(fp, "# Bot Theater settings (used only by the modified game).\n");
	std::fprintf(fp, "# Lines starting with # are notes and are ignored.\n\n");
	std::fprintf(fp, "# 1 = open to the bot room's host window when the game opens, 0 = normal game\n");
	std::fprintf(fp, "enabled = %d\n\n", enabled ? 1 : 0);
	std::fprintf(fp, "# 1 = skip the host window and host straight away with the saved settings\n");
	std::fprintf(fp, "skip_host_window = %d\n\n", skip_host_window ? 1 : 0);
	std::fprintf(fp, "# 1 = show both players' hands to you while spectating a bot room\n");
	std::fprintf(fp, "reveal_hands = %d\n\n", reveal_hands ? 1 : 0);
	std::fprintf(fp, "# 1 = the card window at the top left shows the last card summoned or\n");
	std::fprintf(fp, "# activated (blue frame = bottom player, red = top), in bot rooms and replays;\n");
	std::fprintf(fp, "# pointing at a card shows that card until the mouse moves off it\n");
	std::fprintf(fp, "info_follows_actions = %d\n\n", info_follows_actions ? 1 : 0);
	std::fprintf(fp, "# Replays of bot duels are saved without asking. 0 = keep only the latest\n");
	std::fprintf(fp, "# (as _LastReplay), 1 = keep every one, named by date and time\n");
	std::fprintf(fp, "save_replays = %d\n\n", save_replays ? 1 : 0);
	std::fprintf(fp, "# 1 = gold frame around the avatar of the player whose turn it is\n");
	std::fprintf(fp, "# (in split-zone tag duels it's cut off on the side of the field that player can't use)\n");
	std::fprintf(fp, "turn_highlight = %d\n\n", turn_highlight ? 1 : 0);
	std::fprintf(fp, "# 1 = faint gold glow behind where the turn player's hand goes (in split-zone tag\n");
	std::fprintf(fp, "# duels, only on that player's side). turn_glow_opacity is a percentage.\n");
	std::fprintf(fp, "turn_glow = %d\n", turn_glow ? 1 : 0);
	std::fprintf(fp, "turn_glow_opacity = %d\n\n", turn_glow_opacity);
	std::fprintf(fp, "# Holograms of face-up cards' art. 1 = on, 0 = off\n");
	std::fprintf(fp, "# hologram_size and hologram_opacity are percentages (100 = normal).\n");
	std::fprintf(fp, "# hologram_seconds is how long it shows at full size after the summon.\n");
	std::fprintf(fp, "# hologram_stay: 1 = afterwards it stays, at the rest size/opacity (percent\n");
	std::fprintf(fp, "# of full), until the card leaves its zone; 0 = it fades out instead.\n");
	std::fprintf(fp, "# Either way it fades out early if the card leaves the field.\n");
	std::fprintf(fp, "# hologram_backrow: 1 = Spell/Trap and Pendulum zones get holograms too,\n");
	std::fprintf(fp, "# at hologram_backrow_size percent of the monster size.\n");
	std::fprintf(fp, "# hologram_backrow_lift_bottom / _top: how high back-row holograms float, as a\n");
	std::fprintf(fp, "# percent of the monster holograms' height, for the bottom and top player. The\n");
	std::fprintf(fp, "# bottom back row floats lower so it stays clear of the monsters behind it; the\n");
	std::fprintf(fp, "# top back row is behind its monsters, so it floats higher to peek over them.\n");
	std::fprintf(fp, "holograms = %d\n", holograms_on ? 1 : 0);
	std::fprintf(fp, "hologram_size = %d\n", hologram_size);
	std::fprintf(fp, "hologram_seconds = %.2f\n", hologram_seconds);
	std::fprintf(fp, "hologram_opacity = %d\n", hologram_opacity);
	std::fprintf(fp, "hologram_stay = %d\n", hologram_stay ? 1 : 0);
	std::fprintf(fp, "hologram_rest_size = %d\n", hologram_rest_size);
	std::fprintf(fp, "hologram_rest_opacity = %d\n", hologram_rest_opacity);
	std::fprintf(fp, "hologram_backrow = %d\n", hologram_backrow ? 1 : 0);
	std::fprintf(fp, "hologram_backrow_size = %d\n", hologram_backrow_size);
	std::fprintf(fp, "hologram_backrow_lift_bottom = %d\n", hologram_backrow_lift_bottom);
	std::fprintf(fp, "hologram_backrow_lift_top = %d\n\n", hologram_backrow_lift_top);
	std::fprintf(fp, "# 1 = each duel's background is picked at random from the images in\n");
	std::fprintf(fp, "# textures/backgrounds (.jpg, .jpeg, .png or .bmp). If that folder is empty or\n");
	std::fprintf(fp, "# missing, the usual textures/bg.jpg is used.\n");
	std::fprintf(fp, "random_backgrounds = %d\n", random_backgrounds ? 1 : 0);
	std::fprintf(fp, "# TDOANE's normal background has the board built in, and field2/field3.png only\n");
	std::fprintf(fp, "# add the Extra Monster Zones on top. 0 = leave those out over custom backgrounds,\n");
	std::fprintf(fp, "# 1 = draw them anyway\n");
	std::fprintf(fp, "custom_background_field = %d\n\n", custom_background_field ? 1 : 0);
	std::fprintf(fp, "# How far to raise the top player's hand, in card heights, so holograms\n");
	std::fprintf(fp, "# don't cover it while spectating. 0 = normal position\n");
	std::fprintf(fp, "top_hand_raise = %.2f\n\n", top_hand_raise);
	std::fprintf(fp, "# Avatars: put images in textures/avatars named after the bots, e.g.\n");
	std::fprintf(fp, "# textures/avatars/Lady Luck.png (.png, .jpg or .jpeg). Characters that\n");
	std::fprintf(fp, "# Windows doesn't allow in file names (\\ / : * ? \" < > |) become _ instead.\n\n");
	std::fprintf(fp, "# How the bot script is started, and any extra options for it\n");
	std::fprintf(fp, "# (for example: script_args = --bot-deck Test)\n");
	std::fprintf(fp, "python = %s\n", ToUTF8(python).c_str());
	std::fprintf(fp, "script = %s\n", ToUTF8(script).c_str());
	std::fprintf(fp, "script_args = %s\n\n", ToUTF8(script_args).c_str());
	std::fprintf(fp, "# Room settings. These are saved automatically whenever you host,\n");
	std::fprintf(fp, "# so the easiest way to change them is in the game's host window.\n");
	std::fprintf(fp, "# mode: 0 = single duel, 1 = match, 2 = tag duel\n");
	std::fprintf(fp, "# allowed_cards: 0 = OCG, 1 = TCG, 2 = TCG/OCG, 3 = Custom, 4 = No Exclusive, 5 = All Cards\n");
	std::fprintf(fp, "# duel_rule: 1-3 = Master Rules 1-3, 4 = New Master Rules, 5 = Master Rules 2020\n");
	std::fprintf(fp, "# time_limit: seconds per player, 0 = no limit\n");
	std::fprintf(fp, "lflist = %s\n", ToUTF8(lflist_name).c_str());
	std::fprintf(fp, "allowed_cards = %d\n", allowed_cards);
	std::fprintf(fp, "mode = %d\n", mode);
	std::fprintf(fp, "time_limit = %d\n", time_limit);
	std::fprintf(fp, "duel_rule = %d\n", duel_rule);
	std::fprintf(fp, "no_check_deck = %d\n", no_check_deck ? 1 : 0);
	std::fprintf(fp, "no_shuffle_deck = %d\n", no_shuffle_deck ? 1 : 0);
	std::fprintf(fp, "start_lp = %d\n", start_lp);
	std::fprintf(fp, "start_hand = %d\n", start_hand);
	std::fprintf(fp, "draw_count = %d\n", draw_count);
	std::fprintf(fp, "password = %s\n", ToUTF8(password).c_str());
	std::fprintf(fp, "# Tournaments (also set in the host window). With tournament = 1, each Host plays\n");
	std::fprintf(fp, "# the next match of the tournament kept in tournament.json, and when it ends the\n");
	std::fprintf(fp, "# next one starts by itself after tournament_pause seconds. A new tournament\n");
	std::fprintf(fp, "# starts when there isn't one going; tournament_entrants (2-64) and\n");
	std::fprintf(fp, "# tournament_format (single = single elimination, roundrobin = everyone plays\n");
	std::fprintf(fp, "# everyone) are only used then. Tag rooms make teams of two.\n");
	std::fprintf(fp, "tournament = %d\n", tournament ? 1 : 0);
	std::fprintf(fp, "tournament_entrants = %d\n", tournament_entrants);
	std::fprintf(fp, "tournament_format = %s\n", tournament_format == 1 ? "roundrobin" : "single");
	std::fprintf(fp, "tournament_pause = %d\n", tournament_pause);
	std::fprintf(fp, "# Tag duels only: 1 = each team's first player may only place cards in the left\n");
	std::fprintf(fp, "# section of the field and its second player in the right (middle zones shared)\n");
	std::fprintf(fp, "split_zones = %d\n", split_zones ? 1 : 0);
	std::fprintf(fp, "# Tag duels only: 1 = turns go clockwise round the table (bottom left, top left,\n");
	std::fprintf(fp, "# top right, bottom right), so after the first turn each team plays twice in a row\n");
	std::fprintf(fp, "clockwise_tag = %d\n", clockwise_tag ? 1 : 0);
	std::fclose(fp);
}

void BotTheater::ApplyToHostWindow() {
	if(!chkSplitZones) {
		// Our own tag-duel options, in the free row between "Cards per Draw" and "Host Name".
		chkSplitZones = mainGame->env->addCheckBox(false, irr::core::rect<irr::s32>(20, 327, 185, 347),
			mainGame->wCreateHost, -1, L"Tag: split zones");
		chkSplitZones->setToolTipText(L"Tag duels: each teammate may only place cards in their own half of the field "
			L"(the middle column is shared).");
		chkClockwise = mainGame->env->addCheckBox(false, irr::core::rect<irr::s32>(190, 327, 370, 347),
			mainGame->wCreateHost, -1, L"Tag: clockwise turns");
		chkClockwise->setToolTipText(L"Tag duels: turns go clockwise round the table (bottom left, top left, top right, "
			L"bottom right), so after the first turn each team plays twice in a row.");
	}
	chkSplitZones->setChecked(split_zones);
	chkClockwise->setChecked(clockwise_tag);
	if(!chkTournament) {
		// Tournament options, in the free space right of Start LP / Start Hand / Draw Count.
		auto env = mainGame->env;
		chkTournament = env->addCheckBox(false, irr::core::rect<irr::s32>(240, 235, 370, 260), mainGame->wCreateHost, -1, L"Tournament");
		chkTournament->setToolTipText(L"Each Host plays the next match of the tournament, and the next match starts by itself. "
			L"Entrants and format are used when a new tournament starts.");
		env->addStaticText(L"Entrants", irr::core::rect<irr::s32>(240, 270, 310, 290), false, false, mainGame->wCreateHost);
		ebEntrants = env->addEditBox(L"8", irr::core::rect<irr::s32>(310, 265, 370, 290), true, mainGame->wCreateHost);
		ebEntrants->setTextAlignment(irr::gui::EGUIA_CENTER, irr::gui::EGUIA_CENTER);
		cbFormat = env->addComboBox(irr::core::rect<irr::s32>(240, 295, 370, 320), mainGame->wCreateHost);
		cbFormat->addItem(L"Single elimination");
		cbFormat->addItem(L"Round robin");
	}
	chkTournament->setChecked(tournament);
	{
		wchar_t nbuf[16];
		myswprintf(nbuf, L"%d", tournament_entrants);
		ebEntrants->setText(nbuf);
	}
	cbFormat->setSelected(tournament_format == 1 ? 1 : 0);
	wchar_t buf[32];
	auto* lf = mainGame->cbHostLFlist;
	for(irr::u32 i = 0; i < lf->getItemCount(); ++i) {
		if(lflist_name == lf->getItem(i)) {
			lf->setSelected(i);
			break;
		}
	}
	if(allowed_cards >= 0 && allowed_cards < (int)mainGame->cbRule->getItemCount())
		mainGame->cbRule->setSelected(allowed_cards);
	if(mode >= 0 && mode < (int)mainGame->cbMatchMode->getItemCount())
		mainGame->cbMatchMode->setSelected(mode);
	if(duel_rule >= 1 && duel_rule <= (int)mainGame->cbDuelRule->getItemCount())
		mainGame->cbDuelRule->setSelected(duel_rule - 1);
	myswprintf(buf, L"%d", time_limit);
	mainGame->ebTimeLimit->setText(buf);
	myswprintf(buf, L"%d", start_lp);
	mainGame->ebStartLP->setText(buf);
	myswprintf(buf, L"%d", start_hand);
	mainGame->ebStartHand->setText(buf);
	myswprintf(buf, L"%d", draw_count);
	mainGame->ebDrawCount->setText(buf);
	mainGame->chkNoCheckDeck->setChecked(no_check_deck);
	mainGame->chkNoShuffleDeck->setChecked(no_shuffle_deck);
	mainGame->ebServerPass->setText(password.c_str());
}

void BotTheater::ReadFromHostWindow() {
	auto* lf = mainGame->cbHostLFlist;
	if(lf->getSelected() >= 0)
		lflist_name = lf->getItem(lf->getSelected());
	allowed_cards = mainGame->cbRule->getSelected();
	mode = mainGame->cbMatchMode->getSelected();
	duel_rule = mainGame->cbDuelRule->getSelected() + 1;
	time_limit = std::wcstol(mainGame->ebTimeLimit->getText(), nullptr, 10);
	start_lp = std::wcstol(mainGame->ebStartLP->getText(), nullptr, 10);
	start_hand = std::wcstol(mainGame->ebStartHand->getText(), nullptr, 10);
	draw_count = std::wcstol(mainGame->ebDrawCount->getText(), nullptr, 10);
	no_check_deck = mainGame->chkNoCheckDeck->isChecked();
	no_shuffle_deck = mainGame->chkNoShuffleDeck->isChecked();
	password = mainGame->ebServerPass->getText();
	if(chkSplitZones)
		split_zones = chkSplitZones->isChecked();
	if(chkClockwise)
		clockwise_tag = chkClockwise->isChecked();
	if(chkTournament) {
		tournament = chkTournament->isChecked();
		int n = (int)std::wcstol(ebEntrants->getText(), nullptr, 10);
		tournament_entrants = std::min(64, std::max(2, n > 0 ? n : 8));
		tournament_format = (cbFormat->getSelected() == 1) ? 1 : 0;
	}
}

void BotTheater::OnHostConfirm() {
	active = enabled;
	observer_requested = false;
	bots_launched = false;
	start_sent = false;
	room_is_tournament = false;
	tour_state = TOUR_IDLE;
	panel_lines.clear();
	if(!enabled)
		return;
	ReadFromHostWindow();
	Save();
	room_port = mainGame->gameConf.serverport;
	room_is_tag = (mode == 2);
	room_password = password;
	room_is_tournament = tournament;
}

void BotTheater::OnTypeChange(unsigned char selftype) {
	if(!active)
		return;
	if(selftype != NETPLAYER_TYPE_OBSERVER) {
		// We start in a duelist seat; move to spectator to leave the seats for the bots.
		if(!observer_requested) {
			observer_requested = true;
			DuelClient::SendPacketToServer(CTOS_HS_TOOBSERVER);
		}
		return;
	}
	if(bots_launched)
		return;
	bots_launched = true;
	std::wstring error;
	mainGame->gMutex.lock();	// the main loop watches the script's process
	bool launched = LaunchBots(error);
	mainGame->gMutex.unlock();
	if(launched) {
		SystemMessage(room_is_tag ? L"Bot Theater: sending in four bots..." : L"Bot Theater: sending in two bots...");
	} else {
		std::wstring msg = L"Bot Theater: couldn't start the bot script (" + error + L"). Check python and script in bot_theater.conf.";
		SystemMessage(msg.c_str());
	}
}

void BotTheater::OnReadyChanged(bool all_ready, bool is_host) {
	if(!active || !is_host)
		return;
	if(!all_ready) {
		start_sent = false;
		return;
	}
	if(start_sent)
		return;
	start_sent = true;
	DuelClient::SendPacketToServer(CTOS_HS_START);
}

bool BotTheater::SkipPhaseBanner() const {
	return enabled && mainGame->dInfo.player_type == NETPLAYER_TYPE_OBSERVER && !mainGame->dInfo.isReplay;
}

bool BotTheater::SkipEndPrompts() const {
	return active && mainGame->dInfo.player_type == NETPLAYER_TYPE_OBSERVER && !mainGame->dInfo.isReplay;
}

bool BotTheater::SpectatingBotRoom() const {
	return active && mainGame->dInfo.player_type == NETPLAYER_TYPE_OBSERVER && !mainGame->dInfo.isReplay;
}

bool BotTheater::InfoFollowsActions() const {
	// live bot rooms, and replays (which only a spectator watches)
	return info_follows_actions && (SpectatingBotRoom() || (enabled && mainGame->dInfo.isReplay));
}

void BotTheater::PeekCard(unsigned int code) {
	peeking = true;
	mainGame->ShowCardInfo(code);
	info_owner = -1;	// no player's border round a card the viewer picked
}

void BotTheater::EndPeek() {
	if(!peeking)
		return;
	peeking = false;
	if(last_info_code) {
		mainGame->ShowCardInfo(last_info_code);
		info_owner = last_info_owner;
	}
}

void BotTheater::OnCardAction(unsigned int code, int local_player) {
	if(!code || !InfoFollowsActions())
		return;
	// A replay that is skipping ahead holds the GUI lock while it runs (and shows nothing).
	if(mainGame->dInfo.isReplay && mainGame->dInfo.isReplaySkiping)
		return;
	// Called from the network thread. Showing the card can load its picture, which uses the
	// graphics device, and Direct3D 9 must only be used from the drawing thread (doing it here
	// could hang the game inside the graphics driver). So the next frame shows it (Tick).
	mainGame->gMutex.lock();
	pending_info_code = code;
	pending_info_owner = local_player;
	mainGame->gMutex.unlock();
}

void BotTheater::SetTurnPlayer(int local_player) {
	turn_player = mainGame->LocalPlayer(local_player);	// back to the duel's own numbering
	const auto& d = mainGame->dInfo;
	// Which teammate is playing, kept the same way as the duel engine keeps it: the first
	// turn starts with each team's starting player, then each new turn hands the turn
	// player's team over to the other teammate (with the clockwise order, from turn 3 on).
	if(d.turn <= 1) {
		tag_member[0] = 0;
		tag_member[1] = 1;
	} else if(d.turn >= FirstTagSwapTurn())
		tag_member[turn_player] ^= 1;
	const wchar_t* name;
	if(d.isTag)
		name = SeatName(SeatOf(local_player, tag_member[turn_player]));
	else
		name = (local_player == 0) ? d.hostname : d.clientname;
	if(name && name[0])
		myswprintf(turn_text, L"%ls's Turn", name);
	else
		turn_text[0] = 0;
}

int BotTheater::CurrentTurnSide() const {
	if(!active || turn_player < 0 || !mainGame->dInfo.isStarted)
		return -1;
	return mainGame->LocalPlayer(turn_player);	// follows the spectator swap button
}

int BotTheater::TurnSide() const {
	return turn_highlight ? CurrentTurnSide() : -1;
}

// Split-zone tag duels: the half of the field the player taking this turn can use, as it
// looks on screen: 0 = left, 1 = right, -1 = the whole field (any other duel).
int BotTheater::TurnSection() const {
	if(!SplitZones() || !mainGame->dInfo.isTag)
		return -1;
	int side = CurrentTurnSide();
	if(side < 0)
		return -1;
	// A team's first player has its left three columns, the second its right three. The
	// top player faces us, so their left is the right-hand side of the screen.
	bool first = (tag_member[turn_player] == 0);
	return (first == (side == 0)) ? 0 : 1;
}

bool BotTheater::ClockwiseTurns() const {
	const auto& d = mainGame->dInfo;
	if(!d.isTag)
		return false;
	if(d.isReplay)
		return replay_clockwise;
	return ClockwiseTag();
}

// ---------------------------------------------------------------------------
// Tag duels: four portraits. Seats 0 and 1 are the host's team and 2 and 3 the other
// team; the lower seat of each team is its first player, who has the team's left half
// of the field. The near team is at the bottom of the screen unless the sides have been
// swapped, and the far team faces us, so its first player's half is on our right.

bool BotTheater::TagPortraits() const {
	return SpectatingBotRoom() && mainGame->dInfo.isTag && mainGame->dInfo.isStarted;
}

int BotTheater::SeatOf(int local_side, int member) const {
	bool host_at_bottom = DuelClient::IsHostTeam() != mainGame->dInfo.isReplaySwapped;
	bool host_team = (local_side == 0) == host_at_bottom;
	return (host_team ? 0 : 2) + (member ? 1 : 0);
}

const wchar_t* BotTheater::SeatName(int seat) const {
	const auto& d = mainGame->dInfo;
	bool swapped = d.isReplaySwapped;	// swapping sides also swaps the stored names
	switch(seat) {
	case 0: return swapped ? d.clientname : d.hostname;
	case 1: return swapped ? d.clientname_tag : d.hostname_tag;
	case 2: return swapped ? d.hostname : d.clientname;
	default: return swapped ? d.hostname_tag : d.clientname_tag;
	}
}

// Row 0 shows the top side, row 1 the bottom side. A team's first player (member 0) is on
// the left for the bottom team and on the right for the top team.
static int PortraitSide(int row) { return row == 0 ? 1 : 0; }
static int PortraitMember(int side, int column) { return side == 0 ? column : 1 - column; }

int BotTheater::PortraitSeat(int row, int column) const {
	int side = PortraitSide(row);
	return SeatOf(side, PortraitMember(side, column));
}

bool BotTheater::PortraitActive(int row, int column) const {
	int side = TurnSide();
	if(side < 0 || side != PortraitSide(row))
		return false;
	return tag_member[turn_player] == PortraitMember(side, column);
}

void BotTheater::DrawTurnHighlight(int left, int top, int right, int bottom) const {
	irr::video::SColor gold(255, 255, 200, 40);
	int t = std::max(2, (int)(3 * mainGame->xScale));
	// The frame sits just outside the picture, so it isn't covered up. In split-zone tag duels
	// it's cut off on the side of the field this player can't use, and only wraps the 3/5 of
	// the portrait that matches their three columns (the middle column is shared).
	int section = TagPortraits() ? -1 : TurnSection();	// four portraits: each one is a single player
	int part = (right - left) * 3 / 5;
	int from = left - t, to = right + t;
	if(section == 0)
		to = left + part;
	else if(section == 1)
		from = right - part;
	irr::video::IVideoDriver* driver = mainGame->driver;
	driver->draw2DRectangle(gold, irr::core::recti(from, top - t, to, top));			// top edge
	driver->draw2DRectangle(gold, irr::core::recti(from, bottom, to, bottom + t));	// bottom edge
	if(section != 1)
		driver->draw2DRectangle(gold, irr::core::recti(left - t, top - t, left, bottom + t));	// left edge
	if(section != 0)
		driver->draw2DRectangle(gold, irr::core::recti(right, top - t, right + t, bottom + t));	// right edge
}

const wchar_t* BotTheater::TurnText() const {
	if(!active || !turn_text[0] || mainGame->gameConf.hide_player_name)
		return nullptr;
	return turn_text;
}

void BotTheater::DrawInfoBorder() const {
	if(info_owner < 0 || !InfoFollowsActions() || !mainGame->dInfo.isStarted || !mainGame->wCardImg->isVisible())
		return;
	irr::video::SColor color = (info_owner == 0) ? irr::video::SColor(255, 40, 120, 255)
		: irr::video::SColor(255, 230, 40, 40);
	irr::core::recti r = mainGame->wCardImg->getAbsolutePosition();
	int thickness = std::max(2, (int)(3 * mainGame->xScale));
	for(int i = 0; i < thickness; ++i) {
		irr::core::recti frame(r.UpperLeftCorner.X + i, r.UpperLeftCorner.Y + i,
			r.LowerRightCorner.X - i, r.LowerRightCorner.Y - i);
		mainGame->driver->draw2DRectangleOutline(frame, color);
	}
}

static bool FileExists(const std::wstring& path) {
#ifdef _WIN32
	FILE* f = _wfopen(path.c_str(), L"rb");
#else
	FILE* f = std::fopen(ToUTF8(path).c_str(), "rb");
#endif
	if(!f)
		return false;
	std::fclose(f);
	return true;
}

static std::wstring FindAvatar(const wchar_t* name) {
	std::wstring safe;
	for(const wchar_t* p = name; *p; ++p)
		safe += std::wcschr(L"\\/:*?\"<>|", *p) ? L'_' : *p;
	if(safe.empty())
		return L"";
	static const wchar_t* exts[] = { L".png", L".jpg", L".jpeg" };
	for(auto ext : exts) {
		std::wstring path = L"textures/avatars/" + safe + ext;
		if(FileExists(path))
			return path;
	}
	return L"";
}

void BotTheater::OnDuelStart() {
	// Called when a duel starts, with the game's GUI lock already held.
	info_owner = -1;
	pending_info_code = 0;
	last_info_code = 0;
	peeking = false;
	for(int side = 0; side < 2; ++side) {	// no Field Spell backdrops carried over from the last duel
		backdrop_now[side] = Backdrop();
		backdrop_old[side] = Backdrop();
	}
	turn_text[0] = 0;
	turn_player = -1;
	holograms.clear();
	if(!active)
		return;
	if(random_backgrounds)
		PickBackground();
	int seats = mainGame->dInfo.isTag ? 4 : 2;
	for(int i = 0; i < seats; ++i) {
		std::wstring path = FindAvatar(mainGame->stHostPrepDuelist[i]->getToolTipText().c_str());
		imageManager.LoadLocalAvatar(i, path.empty() ? L"textures/avatar.png" : path.c_str());
	}
}

// ---------------------------------------------------------------------------
// Holograms: when a card turns up face-up in a Monster, Spell & Trap or Pendulum
// zone, its art rises out of the zone with a cyan glow. It then either rests
// (smaller and dimmer) or fades, and it fades out once the card leaves.
// Spell/Trap/Pendulum ("back row") holograms are smaller and float lower.

void BotTheater::MakeHologramTextures() {
	auto driver = mainGame->driver;
	if(!tWhite) {
		irr::video::IImage* img = driver->createImage(irr::video::ECF_A8R8G8B8, irr::core::dimension2du(4, 4));
		img->fill(irr::video::SColor(255, 255, 255, 255));
		tWhite = driver->addTexture("bot_theater_white", img);
		img->drop();
	}
	if(!tGlow) {
		// Soft-edged rectangle: solid in the middle, fading to nothing at the edges.
		const int size = 64;
		irr::video::IImage* img = driver->createImage(irr::video::ECF_A8R8G8B8, irr::core::dimension2du(size, size));
		for(int y = 0; y < size; ++y) {
			for(int x = 0; x < size; ++x) {
				float dx = std::fabs((x + 0.5f) / size * 2 - 1), dy = std::fabs((y + 0.5f) / size * 2 - 1);
				float edge = std::max(dx, dy);	// 0 in the middle, 1 at the border
				float a = edge < 0.6f ? 1.0f : 1.0f - (edge - 0.6f) / 0.4f;
				a = a * a;
				img->setPixel(x, y, irr::video::SColor((irr::u32)(a * 255), 255, 255, 255));
			}
		}
		tGlow = driver->addTexture("bot_theater_glow", img);
		img->drop();
	}
}

static void DrawQuad(irr::video::IVideoDriver* driver, const irr::core::vector3df& tl, const irr::core::vector3df& tr,
	const irr::core::vector3df& bl, const irr::core::vector3df& br, irr::video::SColor top, irr::video::SColor bottom,
	float u1 = 0, float v1 = 0, float u2 = 1, float v2 = 1) {
	irr::core::vector3df n(0, 0, 1);
	irr::video::S3DVertex v[4] = {
		irr::video::S3DVertex(tl, n, top, irr::core::vector2df(u1, v1)),
		irr::video::S3DVertex(tr, n, top, irr::core::vector2df(u2, v1)),
		irr::video::S3DVertex(bl, n, bottom, irr::core::vector2df(u1, v2)),
		irr::video::S3DVertex(br, n, bottom, irr::core::vector2df(u2, v2)),
	};
	irr::u16 idx[6] = { 0, 1, 2, 2, 1, 3 };
	driver->drawVertexPrimitiveList(v, 4, idx, 2);
}

void BotTheater::ScanForNewHolograms() {
	// Give every face-up card in a monster or back-row zone a hologram when it first
	// appears there. This covers summons, activations, Pendulum Scales, cards placed
	// face-up by effects, and cards flipped face-up.
	const unsigned char FACE_UP = 0x05;
	auto has_hologram = [this](ClientCard* c) {
		for(auto& h : holograms)
			if(h.card == c && h.leave_frame < 0)
				return true;
		return false;
	};
	for(int side = 0; side < 2; ++side) {
		for(int row = 0; row < 2; ++row) {
			bool backrow = (row == 1);
			if(backrow && !hologram_backrow)
				continue;
			auto& zones = backrow ? mainGame->dField.szone[side] : mainGame->dField.mzone[side];
			int count = std::min((int)zones.size(), backrow ? 8 : 7);
			for(int s = 0; s < count; ++s) {
				if(backrow && s == 5)
					continue;	// Field Zone: the field spell already shows as the background
				ClientCard* c = zones[s];
				if(c && c->code && (c->position & FACE_UP) && !has_hologram(c) && holograms.size() < 32)
					holograms.push_back(Hologram{ c->code, side, s, backrow, 0, c, -1 });
			}
		}
	}
}

void BotTheater::DrawHolograms() {
	// Called every frame from the field drawing, with the game's GUI lock held.
	if(!holograms_on || !SpectatingBotRoom() || !mainGame->dInfo.isStarted) {
		holograms.clear();
		return;
	}
	ScanForNewHolograms();
	if(holograms.empty())
		return;
	MakeHologramTextures();
	auto driver = mainGame->driver;
	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
	irr::video::SMaterial mat;
	// Same blending the game uses for cards, but taking transparency from both the
	// image (for the soft glow) and the vertex colours (for fading in and out).
	mat.MaterialType = irr::video::EMT_ONETEXTURE_BLEND;
	mat.MaterialTypeParam = irr::video::pack_textureBlendFunc(irr::video::EBF_SRC_ALPHA, irr::video::EBF_ONE_MINUS_SRC_ALPHA,
		irr::video::EMFN_MODULATE_1X, irr::video::EAS_VERTEX_COLOR | irr::video::EAS_TEXTURE);
	mat.setFlag(irr::video::EMF_LIGHTING, false);
	mat.setFlag(irr::video::EMF_ZBUFFER, false);	// always drawn on top of the field
	mat.setFlag(irr::video::EMF_ZWRITE_ENABLE, false);
	mat.setFlag(irr::video::EMF_BACK_FACE_CULLING, false);

	// Face the game's fixed camera (at (4.2, 8, 7.8), looking at (4.2, 0, 0)).
	irr::core::vector3df view(0, -8.0f, -7.8f);
	view.normalize();
	irr::core::vector3df up = irr::core::vector3df(0, 0, 1) - view * view.Z;
	up.normalize();
	irr::core::vector3df right(1, 0, 0);

	const int rise = 18, fade = 18, settle = 20;	// frames (the game runs at 60 a second)
	const unsigned char FACE_UP = 0x05;
	int full = std::max(rise, (int)(hologram_seconds * 60));
	float size_scale = std::max(10, hologram_size) / 100.0f;
	float backrow_scale = std::max(10, hologram_backrow_size) / 100.0f;
	float opacity = std::min(100, std::max(5, hologram_opacity)) / 100.0f;
	float rest_scale = std::max(10, hologram_rest_size) / 100.0f;
	float rest_alpha = std::min(100, std::max(0, hologram_rest_opacity)) / 100.0f;
	float lift_bottom = std::min(300, std::max(0, hologram_backrow_lift_bottom)) / 100.0f;
	float lift_top = std::min(300, std::max(0, hologram_backrow_lift_top)) / 100.0f;
	int rule = (mainGame->dInfo.duel_rule >= 4) ? 1 : 0;

	// First pass: update every hologram and work out where it goes this frame.
	struct HoloDraw {
		unsigned int code;
		bool backrow;
		float alpha, scale;
		irr::core::vector3df base;	// bottom centre of the art
		irr::core::vector3df floor;	// spot on the card the beam comes from
		float depth;				// distance from the camera
	};
	std::vector<HoloDraw> draws;
	draws.reserve(holograms.size());
	for(size_t i = 0; i < holograms.size();) {
		Hologram& h = holograms[i];
		auto& field = mainGame->dField;

		// Follow the card: start the fade-out once it leaves its zone or turns face-down.
		if(h.leave_frame < 0) {
			auto& zones = h.backrow ? field.szone[h.side] : field.mzone[h.side];
			auto& other_side = h.backrow ? field.szone[1 - h.side] : field.mzone[1 - h.side];
			ClientCard* here = (h.sequence < (int)zones.size()) ? zones[h.sequence] : nullptr;
			if(here != h.card) {
				ClientCard* there = (h.sequence < (int)other_side.size()) ? other_side[h.sequence] : nullptr;
				if(there == h.card)
					h.side = 1 - h.side;	// the spectator swapped sides
				else
					h.leave_frame = 0;	// destroyed, banished, bounced, sent to the Graveyard, moved...
			}
			if(h.leave_frame < 0 && (h.card->code != h.code || !(h.card->position & FACE_UP)))
				h.leave_frame = 0;	// flipped face-down, or became a different card (which gets its own hologram)
		}
		if(h.leave_frame >= fade) {
			holograms.erase(holograms.begin() + i);
			continue;
		}

		// Animation: rise and grow, hover at full size, then either settle down to the
		// resting size/opacity or fade away; fade out early if the card leaves.
		float alpha, scale, lift;
		if(h.frame < rise) {
			float p = (h.frame + 1) / (float)rise;
			p = 1 - (1 - p) * (1 - p);
			alpha = p;
			scale = 0.4f + 0.6f * p;
			lift = 0.15f + 0.35f * p;
		} else if(h.frame < full) {
			alpha = 1;
			scale = 1;
			lift = 0.5f;
		} else if(hologram_stay) {
			float k = std::min(1.0f, (h.frame - full) / (float)settle);
			alpha = 1 + (rest_alpha - 1) * k;
			scale = 1 + (rest_scale - 1) * k;
			lift = 0.5f - 0.1f * k;
		} else {
			float q = std::max(0.0f, 1 - (h.frame - full) / (float)fade);
			alpha = q;
			scale = 1.0f + 0.08f * (1 - q);
			lift = 0.5f + 0.15f * (1 - q);
		}
		if(h.leave_frame >= 0) {
			float q = 1 - h.leave_frame / (float)fade;
			alpha *= q;
			scale *= 1.0f + 0.08f * (1 - q);
			lift += 0.15f * (1 - q);
		}
		lift += 0.04f * std::sin(h.frame * 0.1f);
		alpha *= opacity;
		if(h.backrow) {
			scale *= backrow_scale;
			// The bottom back row is in front of its monsters, so it floats lower to stay clear
			// of them; the top back row is behind its monsters, so it floats higher instead.
			lift *= (h.side == 0) ? lift_bottom : lift_top;
		}
		if(h.frame < 1000000)
			++h.frame;
		if(h.leave_frame >= 0)
			++h.leave_frame;
		++i;
		if(alpha <= 0.002f)
			continue;	// faded away, but still following the card so it doesn't reappear

		const irr::video::S3DVertex* zone = h.backrow ? matManager.vFieldSzone[h.side][h.sequence][rule]
			: matManager.vFieldMzone[h.side][h.sequence];
		irr::core::vector3df center = (zone[0].Pos + zone[1].Pos + zone[2].Pos + zone[3].Pos) / 4.0f;
		HoloDraw d;
		d.code = h.code;
		d.backrow = h.backrow;
		d.alpha = alpha;
		d.scale = scale;
		d.base = center + irr::core::vector3df(0, 0, lift);
		d.floor = center + irr::core::vector3df(0, 0, 0.02f);
		d.depth = (d.base - irr::core::vector3df(4.2f, 8.0f, 7.8f)).dotProduct(view);
		draws.push_back(d);
	}

	// Second pass: draw the furthest first, so nearer holograms cover the ones behind them
	// (instead of whichever appeared last ending up in front).
	std::stable_sort(draws.begin(), draws.end(), [](const HoloDraw& x, const HoloDraw& y) { return x.depth > y.depth; });
	for(const HoloDraw& d : draws) {
		float alpha = d.alpha, scale = d.scale;
		const irr::core::vector3df& base = d.base;
		const irr::core::vector3df& floor = d.floor;

		// Crop to the art box (measured from standard card images); Pendulums stop above their text box.
		bool pendulum = false;
		auto cp = dataManager.GetCodePointer(d.code);
		if(cp != dataManager.datas_end() && (cp->second.type & 0x1000000))
			pendulum = true;
		float u1 = 50 / 421.0f, u2 = 371 / 421.0f, v1 = 113 / 614.0f, v2 = (pendulum ? 386 : 434) / 614.0f;
		float width = 1.2f * size_scale * scale;
		float height = width * ((v2 - v1) * 614.0f) / ((u2 - u1) * 421.0f);

		irr::core::vector3df half = right * (width / 2);
		irr::core::vector3df tall = up * height;
		irr::core::vector3df tl = base - half + tall, tr = base + half + tall, bl = base - half, br = base + half;

		auto a = [alpha](float f) { return (irr::u32)std::min(255.0f, std::max(0.0f, 255 * alpha * f)); };
		irr::video::SColor cyanTop(a(0.15f), 80, 220, 255), cyanBottom(a(0.45f), 80, 220, 255);

		// Projector beam from the card to the bottom of the art.
		mat.setTexture(0, tWhite);
		driver->setMaterial(mat);
		irr::core::vector3df floorHalf = right * (d.backrow ? 0.22f : 0.3f);
		DrawQuad(driver, bl, br, floor - floorHalf, floor + floorHalf, cyanTop, cyanBottom);

		// Soft glow behind the art.
		irr::core::vector3df gHalf = half * 1.25f, gExtra = up * (height * 0.125f);
		mat.setTexture(0, tGlow);
		driver->setMaterial(mat);
		irr::video::SColor glow(a(0.55f), 80, 220, 255);
		DrawQuad(driver, base - gHalf + tall + gExtra, base + gHalf + tall + gExtra, base - gHalf - gExtra, base + gHalf - gExtra, glow, glow);

		// The art itself, slightly cyan-tinted.
		irr::video::ITexture* art = imageManager.GetTexture(d.code);
		if(art) {
			mat.setTexture(0, art);
			driver->setMaterial(mat);
			irr::video::SColor tint(a(1.0f), 215, 245, 255);
			DrawQuad(driver, tl, tr, bl, br, tint, tint, u1, v1, u2, v2);
		}

		// Thin bright frame.
		mat.setTexture(0, tWhite);
		driver->setMaterial(mat);
		irr::video::SColor edge(a(0.9f), 140, 235, 255);
		float t = width * 0.015f;
		irr::core::vector3df tx = right * t, ty = up * t;
		DrawQuad(driver, tl, tr, tl - ty, tr - ty, edge, edge);			// top
		DrawQuad(driver, bl + ty, br + ty, bl, br, edge, edge);			// bottom
		DrawQuad(driver, tl, tl + tx, bl, bl + tx, edge, edge);			// left
		DrawQuad(driver, tr - tx, tr, br - tx, br, edge, edge);			// right
	}
}

// ---------------------------------------------------------------------------
// Raising the top player's hand. The game places hand cards on the board, seen
// through its fixed camera at (4.2, 8, 7.8) looking at (4.2, 0, 0). Moving the hand
// further back by about 1.955 board units raises it on screen by one card height.

static const float TOP_HAND_Y = -3.4f, HAND_Z = 0.5f;
static const float SHIFT_PER_CARD_HEIGHT = 1.955f;

// Distance from the camera, and how far above the camera's centre line, a point appears.
static void CameraDepth(float y, float z, float& depth, float& height) {
	const float fy = -0.71601f, fz = -0.69811f;	// viewing direction
	const float uy = -0.69811f, uz = 0.71601f;	// camera's "up"
	depth = (y - 8.0f) * fy + (z - 7.8f) * fz;
	height = (y - 8.0f) * uy + (z - 7.8f) * uz;
}

float BotTheater::TopHandShift() const {
	if(top_hand_raise <= 0 || !SpectatingBotRoom())
		return 0;
	return (float)std::min(5.0, top_hand_raise) * SHIFT_PER_CARD_HEIGHT;
}

void BotTheater::MapTopHandPoint(int& x, int& y) const {
	float shift = TopHandShift();
	if(shift <= 0)
		return;
	// Screen positions here are in the game's base 1024x640 layout.
	float d0, h0, d1, h1;
	CameraDepth(TOP_HAND_Y, HAND_Z, d0, h0);
	CameraDepth(TOP_HAND_Y - shift, HAND_Z, d1, h1);
	auto screen_y = [](float d, float h) { return (0.42f - h / d) / 0.84f * 640.0f; };
	const float axis_x = 0.9f / 1.35f * 1024.0f;	// where the camera's centre line meets the screen
	float shrink = d1 / d0;	// the raised hand is further away, so it looks smaller
	x = (int)(axis_x + (x - axis_x) * shrink);
	y = (int)(screen_y(d0, h0) + (y - screen_y(d1, h1)) * shrink);
}

// ---------------------------------------------------------------------------
// Turn glow: a faint gold glow where the turn player's hand goes. It's drawn straight
// after the field, so cards, holograms and everything else sit on top of it. In
// split-zone tag duels only the three-fifths on the turn player's side glows.

static const int TURN_GLOW_TEX = 256;	// texture size
static const int TURN_GLOW_EDGE = 120;	// fade on each side of the texture; the middle is solid
static const float BOTTOM_HAND_Y = 4.0f;
static const float HAND_LEFT = 1.55f, HAND_RIGHT = 6.25f;	// edges of a full hand (card centres 1.9 to 5.9)
static const float HAND_LEAN = 0.798056f;	// hand cards lean back by this much to face the camera

// Where a point on the board appears in the game's base 1024x640 layout.
static void BaseScreenPoint(float x, float y, float z, float& sx, float& sy) {
	float d, h;
	CameraDepth(y, z, d, h);
	const float axis_x = 0.9f / 1.35f * 1024.0f;	// where the camera's centre line meets the screen
	sx = axis_x + (x - 4.2f) / d * (1024.0f / 1.35f);
	sy = (0.42f - h / d) / 0.84f * 640.0f;
}

void BotTheater::MakeTurnGlowTexture() {
	if(tTurnGlow)
		return;
	auto driver = mainGame->driver;
	irr::video::IImage* img = driver->createImage(irr::video::ECF_A8R8G8B8,
		irr::core::dimension2du(TURN_GLOW_TEX, TURN_GLOW_TEX));
	auto ramp = [](int i) {
		float d = std::min(i + 0.5f, TURN_GLOW_TEX - i - 0.5f) / TURN_GLOW_EDGE;
		d = std::min(1.0f, d);
		return d * d * (3 - 2 * d);	// eases in and out, so the glow has no hard edge
	};
	for(int y = 0; y < TURN_GLOW_TEX; ++y) {
		for(int x = 0; x < TURN_GLOW_TEX; ++x) {
			float a = ramp(x) * ramp(y);
			img->setPixel(x, y, irr::video::SColor((irr::u32)(a * 255 + 0.5f), 255, 255, 255));
		}
	}
	tTurnGlow = driver->addTexture("bot_theater_turn_glow", img);
	img->drop();
}

void BotTheater::DrawTurnGlow() {
	if(!turn_glow || turn_glow_opacity <= 0)
		return;
	int side = CurrentTurnSide();
	if(side < 0)
		return;
	MakeTurnGlowTexture();
	if(!tTurnGlow)
		return;
	// The screen area a full hand covers (base layout). Hand cards lean back to face the
	// camera, so take each end of a card: the far end is higher up off the board.
	float cy = (side == 0) ? BOTTOM_HAND_Y : TOP_HAND_Y - TopHandShift();
	float left = 1e9f, top = 1e9f, right = -1e9f, bottom = -1e9f;
	const float xs[2] = { HAND_LEFT, HAND_RIGHT };
	for(float x : xs) {
		for(int end = -1; end <= 1; end += 2) {
			float y = cy + end * 0.5f * std::cos(HAND_LEAN);
			float z = HAND_Z - end * 0.5f * std::sin(HAND_LEAN);
			float sx, sy;
			BaseScreenPoint(x, y, z, sx, sy);
			left = std::min(left, sx);
			right = std::max(right, sx);
			top = std::min(top, sy);
			bottom = std::max(bottom, sy);
		}
	}
	// Split-zone tag duels: only the three-fifths on the turn player's side.
	int section = TurnSection();
	float part = (right - left) * 3 / 5;
	if(section == 0)
		right = left + part;
	else if(section == 1)
		left = right - part;
	// The glow reaches a little past the hand all round and fades out over its edge.
	float pad = 0.4f * (bottom - top);
	irr::core::recti area = mainGame->Resize((int)(left - pad), (int)(top - pad), (int)(right + pad), (int)(bottom + pad));
	int w = area.getWidth(), h = area.getHeight();
	if(w <= 0 || h <= 0)
		return;
	int ex = std::min(w / 2, std::max(1, (int)(1.5f * pad * mainGame->xScale)));
	int ey = std::min(h / 2, std::max(1, (int)(1.5f * pad * mainGame->yScale)));
	irr::u32 alpha = (irr::u32)(std::min(100, turn_glow_opacity) * 255 / 100);
	irr::video::SColor gold(alpha, 255, 200, 40);
	const irr::video::SColor colors[4] = { gold, gold, gold, gold };
	// Drawn in nine pieces (corners, edges, middle) so the fade keeps its width.
	const int src[4] = { 0, TURN_GLOW_EDGE, TURN_GLOW_TEX - TURN_GLOW_EDGE, TURN_GLOW_TEX };
	const int dx[4] = { area.UpperLeftCorner.X, area.UpperLeftCorner.X + ex, area.LowerRightCorner.X - ex, area.LowerRightCorner.X };
	const int dy[4] = { area.UpperLeftCorner.Y, area.UpperLeftCorner.Y + ey, area.LowerRightCorner.Y - ey, area.LowerRightCorner.Y };
	for(int j = 0; j < 3; ++j) {
		for(int i = 0; i < 3; ++i) {
			irr::core::recti dest(dx[i], dy[j], dx[i + 1], dy[j + 1]);
			if(dest.getWidth() <= 0 || dest.getHeight() <= 0)
				continue;
			irr::core::recti from(src[i], src[j], src[i + 1], src[j + 1]);
			mainGame->driver->draw2DImage(tTurnGlow, dest, from, nullptr, colors, true);
		}
	}
}

// ---------------------------------------------------------------------------
// Random duel backgrounds from textures/backgrounds.

void BotTheater::PickBackground() {
	// Called at duel start (network thread): just choose a file; the drawing code loads it.
	static const wchar_t* folder = L"./textures/backgrounds";
	static std::mt19937 rng((unsigned int)std::time(nullptr));
	std::vector<std::wstring> files;
	FileSystem::TraversalDir(folder, [&files](const wchar_t* name, bool isdir) {
		if(isdir)
			return;
		std::wstring lower(name);
		for(auto& ch : lower)
			ch = (wchar_t)std::towlower(ch);
		static const wchar_t* exts[] = { L".jpg", L".jpeg", L".png", L".bmp" };
		for(auto ext : exts) {
			size_t n = std::wcslen(ext);
			if(lower.size() > n && lower.compare(lower.size() - n, n, ext) == 0) {
				files.push_back(std::wstring(folder) + L"/" + name);
				break;
			}
		}
	});
	if(files.empty()) {
		pending_background.clear();	// use the normal background
	} else {
		size_t pick = rng() % files.size();
		if(files.size() > 1 && files[pick] == last_background)	// avoid the same one twice in a row
			pick = (pick + 1 + rng() % (files.size() - 1)) % files.size();
		pending_background = files[pick];
		last_background = files[pick];
	}
	background_pending = true;
}

// ---------------------------------------------------------------------------------------------
// Field Spell backdrops (textures/fbg/<passcode>_a and _b, .png or .jpg)

static const int BACKDROP_FADE_BAND = 12;		// percent of the screen height, centred on the middle,
												// over which a backdrop fades out
static const int BACKDROP_FADE_STEP = 16;		// alpha per frame when one appears or goes (about 1/4 s)
static const size_t BACKDROP_CACHE_CARDS = 6;	// pictures kept loaded for cards not on the field

irr::video::ITexture* BotTheater::BackdropTexture(unsigned int code, int side) {
	if(code == 0)
		return nullptr;
	BackdropImages& images = backdrop_cache[code];
	images.last_used = ++backdrop_clock;
	if(!images.tried[side]) {
		images.tried[side] = true;
		const wchar_t* suffix = (side == 1) ? L"_a" : L"_b";
		const wchar_t* extensions[] = { L".png", L".jpg" };
		for(const wchar_t* ext : extensions) {
			wchar_t path[256];
			myswprintf(path, L"textures/fbg/%u%ls%ls", code, suffix, ext);
			if(!FileSystem::IsFileExists(path))
				continue;	// checked first, so a missing picture doesn't fill the log every frame
			images.tex[side] = mainGame->driver->getTexture(path);
			if(images.tex[side])
				break;
		}
		if(images.tex[side])
			FreeOldBackdrops();
	}
	return images.tex[side];
}

void BotTheater::FreeOldBackdrops() {
	// Each picture takes several MB of video memory, so only keep a few for cards that have left.
	for(;;) {
		size_t loaded = 0;
		auto oldest = backdrop_cache.end();
		for(auto it = backdrop_cache.begin(); it != backdrop_cache.end(); ++it) {
			if(!it->second.tex[0] && !it->second.tex[1])
				continue;
			++loaded;
			bool showing = false;
			for(int side = 0; side < 2; ++side)
				if(backdrop_now[side].code == it->first || backdrop_old[side].code == it->first)
					showing = true;
			if(!showing && (oldest == backdrop_cache.end() || it->second.last_used < oldest->second.last_used))
				oldest = it;
		}
		if(loaded <= BACKDROP_CACHE_CARDS || oldest == backdrop_cache.end())
			return;
		for(int side = 0; side < 2; ++side)
			if(oldest->second.tex[side])
				mainGame->driver->removeTexture(oldest->second.tex[side]);
		backdrop_cache.erase(oldest);
	}
}

// One backdrop: side 1 (far) fills the top half of the screen, side 0 (near) the bottom half.
// It reaches a little past the middle and fades out across a band centred on it, so it blends
// into the background or the other side's backdrop. The picture keeps its proportions: when it
// is wider than the area, the extra is cut off its right-hand side (taller: top and bottom equally).
void BotTheater::DrawBackdrop(irr::video::ITexture* tex, int side, int alpha) {
	if(!tex || alpha <= 0)
		return;
	irr::core::recti screen = mainGame->Resize(0, 0, 1024, 640);
	int height = screen.getHeight();
	int mid = screen.UpperLeftCorner.Y + height / 2;
	int band = std::max(1, height * BACKDROP_FADE_BAND / 200);	// half the fade band
	int top = (side == 1) ? screen.UpperLeftCorner.Y : mid - band;
	int bottom = (side == 1) ? mid + band : screen.LowerRightCorner.Y;
	if(bottom <= top)
		return;
	irr::core::dimension2du size = tex->getOriginalSize();
	double area_aspect = (double)screen.getWidth() / (bottom - top);
	double use_w = size.Width, use_h = size.Height;
	if(use_w / use_h > area_aspect)
		use_w = use_h * area_aspect;
	else
		use_h = use_w / area_aspect;
	int sx0 = 0, sx1 = (int)use_w;	// keep the left-hand edge
	int sy0 = (int)((size.Height - use_h) / 2), sy1 = sy0 + (int)use_h;
	auto source_y = [&](int y) { return sy0 + (int)((long long)(y - top) * (sy1 - sy0) / (bottom - top)); };
	irr::video::SColor solid(alpha, 255, 255, 255), clear(0, 255, 255, 255);
	auto draw = [&](int y0, int y1, irr::video::SColor upper, irr::video::SColor lower) {
		if(y1 <= y0)
			return;
		irr::core::recti dest(screen.UpperLeftCorner.X, y0, screen.LowerRightCorner.X, y1);
		irr::core::recti source(sx0, source_y(y0), sx1, source_y(y1));
		irr::video::SColor colors[4] = { upper, lower, lower, upper };	// upper left, lower left, lower right, upper right
		mainGame->driver->draw2DImage(tex, dest, source, nullptr, colors, true);
	};
	if(side == 1) {
		draw(top, mid - band, solid, solid);
		draw(mid - band, mid + band, solid, clear);
	} else {
		draw(mid - band, mid + band, clear, solid);
		draw(mid + band, bottom, solid, solid);
	}
}

void BotTheater::DrawFieldBackdrops(ClientCard* near_card, ClientCard* far_card, bool& near_drawn, bool& far_drawn) {
	ClientCard* cards[2] = { near_card, far_card };
	bool* drawn[2] = { &near_drawn, &far_drawn };
	for(int side = 0; side < 2; ++side) {
		// The picture for this side's Field Spell, if there is one (alternative artworks use the original's).
		unsigned int code = 0;
		ClientCard* pcard = cards[side];
		if(pcard) {
			if(BackdropTexture(pcard->code, side))
				code = pcard->code;
			else if(pcard->alias && BackdropTexture(pcard->alias, side))
				code = pcard->alias;
		}
		*drawn[side] = (code != 0);
		Backdrop& now = backdrop_now[side];
		Backdrop& old = backdrop_old[side];
		if(code != now.code) {
			old = now;	// the previous one fades out
			now.code = code;
			now.alpha = 0;
		}
		if(now.code)
			now.alpha = std::min(255, now.alpha + BACKDROP_FADE_STEP);
		if(old.code) {
			old.alpha -= BACKDROP_FADE_STEP;
			if(old.alpha <= 0)
				old = Backdrop();
		}
		if(old.code)
			DrawBackdrop(BackdropTexture(old.code, side), side, old.alpha);
		if(now.code)
			DrawBackdrop(BackdropTexture(now.code, side), side, now.alpha);
	}
}

irr::video::ITexture* BotTheater::DuelBackground() {
	if(!random_backgrounds || !SpectatingBotRoom())
		return imageManager.tBackGround;
	if(background_pending) {
		background_pending = false;
		irr::video::ITexture* chosen = nullptr;
		if(!pending_background.empty())
			chosen = mainGame->driver->getTexture(pending_background.c_str());
		if(tDuelBackground && tDuelBackground != chosen)
			mainGame->driver->removeTexture(tDuelBackground);	// free the previous one
		tDuelBackground = chosen;
	}
	return tDuelBackground ? tDuelBackground : imageManager.tBackGround;
}

bool BotTheater::HideFieldOverlay() const {
	// tDuelBackground is only set while one of the custom backgrounds is in use.
	return !custom_background_field && random_backgrounds && tDuelBackground && SpectatingBotRoom();
}

void BotTheater::ShowWindowAfterRoom() const {
	// Go back to the host window (settings already filled in) so another
	// bot duel is one click away.
	mainGame->btnHostConfirm->setEnabled(true);
	mainGame->btnHostCancel->setEnabled(true);
	mainGame->ShowElement(mainGame->wCreateHost);
}

void BotTheater::SystemMessage(const wchar_t* msg) const {
	mainGame->gMutex.lock();
	mainGame->AddChatMsg(msg, 8);	// shown as [System], not as a script error
	mainGame->gMutex.unlock();
}

bool BotTheater::LaunchBots(std::wstring& error) {
	wchar_t num[16];
	myswprintf(num, L"%d", (int)room_port);
	std::wstring args = std::wstring(L"--launch --port ") + num;
	if(room_is_tag)
		args += L" --tag";
	if(!room_password.empty())
		args += L" --password \"" + room_password + L"\"";
	if(room_is_tournament) {
		wchar_t tbuf[64];
		myswprintf(tbuf, L" --tournament --entrants %d --format %ls", tournament_entrants,
			tournament_format == 1 ? L"roundrobin" : L"single");
		args += tbuf;
	}
	return RunScript(args, SCRIPT_LAUNCH, error);
}

// Starts the bot script with the given options (plus script_args from the settings).
// Its output goes to bot_theater.log; the game checks on it every frame (PollScript).
bool BotTheater::RunScript(const std::wstring& args, int kind, std::wstring& error) {
#ifdef _WIN32
	if(script_process) {	// an earlier run that's still going: stop watching it
		CloseHandle((HANDLE)script_process);
		script_process = nullptr;
	}
	std::wstring cmd = L"\"" + python + L"\" \"" + script + L"\" " + args;
	if(!script_args.empty())
		cmd += L" " + script_args;

	// The launch run starts a fresh log; the result run adds to it.
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof sa;
	sa.lpSecurityDescriptor = nullptr;
	sa.bInheritHandle = TRUE;
	HANDLE log;
	if(kind == SCRIPT_RECORD)
		log = CreateFileW(LOG_FILE, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	else
		log = CreateFileW(LOG_FILE, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	STARTUPINFOEXW si;
	std::memset(&si, 0, sizeof si);
	si.StartupInfo.cb = sizeof si;
	DWORD flags = CREATE_NO_WINDOW;
	BOOL inherit = FALSE;
	std::vector<char> attr_buf;
	if(log != INVALID_HANDLE_VALUE) {
		// Pass on only the log file, not the game's own network connections.
		SIZE_T attr_size = 0;
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attr_size);
		attr_buf.resize(attr_size);
		auto attrs = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attr_buf.data());
		if(InitializeProcThreadAttributeList(attrs, 1, 0, &attr_size)
			&& UpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &log, sizeof log, nullptr, nullptr)) {
			si.lpAttributeList = attrs;
			si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
			si.StartupInfo.hStdOutput = log;
			si.StartupInfo.hStdError = log;
			si.StartupInfo.hStdInput = nullptr;
			flags |= EXTENDED_STARTUPINFO_PRESENT;
			inherit = TRUE;
		}
	}
	std::vector<wchar_t> cmdline(cmd.begin(), cmd.end());
	cmdline.push_back(0);
	PROCESS_INFORMATION pi;
	std::memset(&pi, 0, sizeof pi);
	BOOL ok = CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, inherit, flags, nullptr, nullptr,
		&si.StartupInfo, &pi);
	DWORD err = GetLastError();
	if(si.lpAttributeList)
		DeleteProcThreadAttributeList(si.lpAttributeList);
	if(log != INVALID_HANDLE_VALUE)
		CloseHandle(log);
	if(!ok) {
		wchar_t ebuf[32];
		myswprintf(ebuf, L"Windows error %u", (unsigned)err);
		error = ebuf;
		return false;
	}
	CloseHandle(pi.hThread);
	script_process = pi.hProcess;
	script_kind = kind;
	return true;
#else
	(void)args;
	(void)kind;
	error = L"only supported on Windows";
	return false;
#endif
}

// True once the script has finished, with its exit code.
bool BotTheater::PollScript(int& exit_code) {
#ifdef _WIN32
	if(!script_process)
		return false;
	if(WaitForSingleObject((HANDLE)script_process, 0) != WAIT_OBJECT_0)
		return false;
	DWORD code = 1;
	GetExitCodeProcess((HANDLE)script_process, &code);
	CloseHandle((HANDLE)script_process);
	script_process = nullptr;
	exit_code = (int)code;
	return true;
#else
	(void)exit_code;
	return false;
#endif
}

// The script leaves announcements in bot_theater_messages.txt; show them in the chat
// (and on the between-matches panel), then remove the file.
int BotTheater::ShowScriptMessages(bool to_panel) {
	if(to_panel)
		panel_lines.clear();
	FILE* fp = std::fopen(MESSAGES_FILE, "r");
	if(!fp)
		return 0;
	int count = 0;
	char line[1024];
	bool first = true;
	while(std::fgets(line, sizeof line, fp)) {
		char* p = line;
		if(first && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF)
			p += 3;
		first = false;
		std::string text = Trim(p);
		if(text.empty())
			continue;
		std::wstring msg = FromUTF8(text.c_str());
		mainGame->AddChatMsg(msg.c_str(), 8);
		++count;
		if(to_panel && panel_lines.size() < 8)
			panel_lines.push_back(msg);
	}
	std::fclose(fp);
	std::remove(MESSAGES_FILE);
	return count;
}

// ---------------------------------------------------------------------------
// Tournaments. The script (random_duel.py) keeps the bracket in tournament.json.
// The game writes down who wins each duel, runs the script to record the match
// once the room closes, then hosts the next match after a short pause.

void BotTheater::OnDuelWin(int side) {
	// Network thread. One line per duel: WIN or DRAW, then the two sides' names
	// (winners first), two names per side (the second is empty outside tag duels).
	if(!room_is_tournament || !SpectatingBotRoom())
		return;
	const auto& d = mainGame->dInfo;
	auto names = [&d](int s) {
		std::wstring first = (s == 0) ? d.hostname : d.clientname;
		std::wstring second = d.isTag ? ((s == 0) ? d.hostname_tag : d.clientname_tag) : L"";
		return first + L"\t" + second;
	};
	std::wstring line = (side < 0) ? L"DRAW\t" + names(0) + L"\t" + names(1)
		: L"WIN\t" + names(side) + L"\t" + names(1 - side);
	FILE* fp = std::fopen(RESULTS_FILE, "a");
	if(!fp)
		return;
	std::fprintf(fp, "%s\n", ToUTF8(line).c_str());
	std::fclose(fp);
}

void BotTheater::OnRoomEnd() {
	// Network thread, GUI lock held: the room has closed after its duel or match.
	if(!room_is_tournament) {
		ShowWindowAfterRoom();
		return;
	}
	room_is_tournament = false;
	pause_requested = false;
	tour_state = TOUR_RECORD;	// Tick() runs the script to record the result
	panel_lines.clear();
	panel_lines.push_back(L"Saving the result...");
}

static unsigned long long NowMs() {
	return (unsigned long long)std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

// While spectating a bot duel, the phase indicator sits just under the turn counter
// (above the far player's hand) instead of between the two monster rows. Its usual
// spot is restored for everything else. Checked every frame, so it also follows window
// resizes (the game resets the position when the window changes size).
void BotTheater::PlacePhaseBar() {
	if(!mainGame->wPhase)
		return;
	// Same 375x20 bar as the game's own (480, 310, 855, 330); the current-phase box is its
	// first 50 pixels, so it lands centred under the turn counter (632-688).
	irr::core::recti wanted = SpectatingBotRoom() ? mainGame->Resize(635, 54, 1010, 74)
		: mainGame->Resize(480, 310, 855, 330);
	if(mainGame->wPhase->getRelativePosition() != wanted)
		mainGame->wPhase->setRelativePosition(wanted);
}

// ---------------------------------------------------------------------------
// Freeze reports. A background thread watches the frame count; if a duel is on
// screen and no frame has been drawn for FREEZE_SECONDS, it writes
// bot_theater_freeze.log (what the duel was doing) and bot_theater_freeze.dmp
// (a snapshot of every part of the game, to read with the build's YGOPro_BotTheater.pdb).

static const int FREEZE_SECONDS = 15;
static const int TRACE_SIZE = 40;
static std::atomic<unsigned> frames_drawn{ 0 };
struct MessageTrace {
	std::atomic<unsigned> next{ 0 };
	std::atomic<int> msg[TRACE_SIZE];
	std::atomic<unsigned long long> when[TRACE_SIZE];
};
static MessageTrace client_trace, server_trace;

void BotTheater::NoteMessage(int msg, bool server) {
	MessageTrace& t = server ? server_trace : client_trace;
	unsigned i = t.next.fetch_add(1) % TRACE_SIZE;
	t.msg[i].store(msg);
	t.when[i].store(NowMs());
}

#ifdef _WIN32
static void WriteTrace(FILE* fp, const char* who, MessageTrace& t, unsigned long long now) {
	unsigned count = t.next.load();
	std::fprintf(fp, "  Last duel messages %s (oldest first, seconds before the report):\n   ", who);
	unsigned first = count > (unsigned)TRACE_SIZE ? count - TRACE_SIZE : 0;
	for(unsigned k = first; k < count; ++k) {
		unsigned i = k % TRACE_SIZE;
		std::fprintf(fp, " %d(%.1f)", t.msg[i].load(), ((long long)now - (long long)t.when[i].load()) / 1000.0);
	}
	std::fprintf(fp, count ? "\n" : " none\n");
}

static void WriteFreezeReport() {
	unsigned long long now = NowMs();
	FILE* fp = std::fopen("bot_theater_freeze.log", "a");
	if(fp) {
		char stamp[40];
		std::time_t t = std::time(nullptr);
		std::strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", std::localtime(&t));
		const auto& d = mainGame->dInfo;
		std::fprintf(fp, "[%s] The game stopped drawing for %d seconds.\n", stamp, FREEZE_SECONDS);
		std::fprintf(fp, "  Turn %d, LP %d / %d, tag %d, message being handled: %d\n",
			d.turn, d.lp[0], d.lp[1], d.isTag ? 1 : 0, (int)d.curMsg);
		// If the drawing lock is free, the frame loop is stuck somewhere other than waiting for it.
		if(mainGame->gMutex.try_lock()) {
			mainGame->gMutex.unlock();
			std::fprintf(fp, "  The game's drawing lock was free.\n");
		} else
			std::fprintf(fp, "  The game's drawing lock was taken.\n");
		WriteTrace(fp, "the screen received", client_trace, now);
		WriteTrace(fp, "the duel engine sent", server_trace, now);
	}
	bool dumped = false;
	HMODULE dbghelp = LoadLibraryW(L"dbghelp.dll");
	if(dbghelp) {
		typedef BOOL(WINAPI* WriteDumpFn)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
			PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
		WriteDumpFn write_dump = (WriteDumpFn)GetProcAddress(dbghelp, "MiniDumpWriteDump");
		HANDLE file = CreateFileW(L"bot_theater_freeze.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(write_dump && file != INVALID_HANDLE_VALUE) {
			MINIDUMP_TYPE type = (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory
				| MiniDumpWithThreadInfo | MiniDumpWithHandleData | MiniDumpWithUnloadedModules);
			dumped = write_dump(GetCurrentProcess(), GetCurrentProcessId(), file, type, nullptr, nullptr, nullptr) != FALSE;
		}
		if(file != INVALID_HANDLE_VALUE)
			CloseHandle(file);
	}
	if(fp) {
		std::fprintf(fp, dumped ? "  Snapshot saved to bot_theater_freeze.dmp\n\n" : "  Couldn't save a snapshot.\n\n");
		std::fclose(fp);
	}
}

static void FreezeWatch() {
	unsigned last = frames_drawn.load();
	int still = 0;
	for(;;) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		unsigned now = frames_drawn.load();
		if(now != last) {
			last = now;
			still = 0;
			continue;
		}
		// Only while a duel is on screen; one report per freeze.
		if(++still == FREEZE_SECONDS && mainGame->dInfo.isStarted)
			WriteFreezeReport();
	}
}
#endif

void BotTheater::Tick() {
	frames_drawn.fetch_add(1);
#ifdef _WIN32
	static bool watching = false;
	if(!watching) {
		watching = true;
		std::thread(FreezeWatch).detach();
	}
#endif
	PlacePhaseBar();
	if(!mainGame->dInfo.isStarted) {
		last_info_code = 0;
		peeking = false;
	}
	if(pending_info_code) {		// the info window follows the duel (see OnCardAction)
		last_info_code = pending_info_code;
		last_info_owner = pending_info_owner;
		if(!peeking) {	// a card the viewer is pointing at stays until the mouse moves off it
			mainGame->ShowCardInfo(pending_info_code);
			info_owner = pending_info_owner;
		}
		pending_info_code = 0;
	}
	int code = 0;
	if(script_process && PollScript(code)) {
		int kind = script_kind;
		script_kind = SCRIPT_NONE;
		if(kind == SCRIPT_RECORD) {
			int shown = ShowScriptMessages(true);
			if(code == 0 || code == EXIT_TOURNAMENT_OVER) {
				if(code == EXIT_TOURNAMENT_OVER) {
					tour_state = TOUR_FINISHED;
				} else if(pause_requested) {
					StopTournamentLoop(L"Bot Theater: tournament paused. Press Enter to play the next match.");
				} else {
					tour_state = TOUR_COUNTDOWN;
					countdown_end = NowMs() + (unsigned long long)std::max(0, tournament_pause) * 1000;
				}
			} else if(shown) {
				StopTournamentLoop(L"Bot Theater: tournament paused (see the message above).");
			} else if(code == 2) {
				StopTournamentLoop(L"Bot Theater: the bot script couldn't run its tournament step (details in bot_theater.log). "
					L"Make sure random_duel.py is the new version.");
			} else {
				StopTournamentLoop(L"Bot Theater: couldn't record the tournament result (details in bot_theater.log). Press Enter to replay the match.");
			}
		} else {
			int shown = ShowScriptMessages(false);
			if(code == 2 && !shown)
				mainGame->AddChatMsg(L"Bot Theater: the bot script couldn't start (details in bot_theater.log). Is random_duel.py up to date?", 8);
			else if(code != 0 && !shown)
				mainGame->AddChatMsg(L"Bot Theater: the bot script stopped with a problem (details in bot_theater.log).", 8);
		}
	}
	switch(tour_state) {
	case TOUR_RECORD: {
		if(script_process)
			break;	// the launch run is still going; wait for it
		std::wstring error;
		if(RunScript(L"--tournament-record", SCRIPT_RECORD, error)) {
			tour_state = TOUR_RECORDING;
		} else {
			std::wstring msg = L"Bot Theater: couldn't start the bot script (" + error + L"). Check python and script in bot_theater.conf.";
			StopTournamentLoop(msg.c_str());
		}
		break;
	}
	case TOUR_COUNTDOWN:
		if(NowMs() >= countdown_end)
			StartNextMatch();
		break;
	default:
		break;
	}
	if(tour_state != TOUR_IDLE)
		DrawTournamentPanel();
}

void BotTheater::AfterFrame() {
	// Hosting runs network code, so it's done here, outside the GUI lock.
	if(!host_click_pending)
		return;
	host_click_pending = false;
	irr::SEvent click;
	click.EventType = irr::EET_GUI_EVENT;
	click.GUIEvent.Caller = mainGame->btnHostConfirm;
	click.GUIEvent.Element = 0;
	click.GUIEvent.EventType = irr::gui::EGET_BUTTON_CLICKED;
	mainGame->device->postEventFromUser(click);
}

void BotTheater::StartNextMatch() {
	// Host again with the same settings; the host window stays hidden.
	tour_state = TOUR_IDLE;
	panel_lines.clear();
	mainGame->btnHostConfirm->setEnabled(true);
	mainGame->btnHostCancel->setEnabled(true);
	host_click_pending = true;
}

void BotTheater::StopTournamentLoop(const wchar_t* msg) {
	tour_state = TOUR_IDLE;
	panel_lines.clear();
	if(msg)
		mainGame->AddChatMsg(msg, 8);
	ShowWindowAfterRoom();
}

bool BotTheater::OnKey(int key, bool pressed) {
	// Enter: start the next match now (or close the final panel). Esc: pause the
	// tournament and go back to the host window.
	if(!pressed) {
		if(key == swallow_key) {
			swallow_key = -1;
			return true;
		}
		return false;
	}
	if(tour_state == TOUR_IDLE)
		return false;
	if(key != irr::KEY_RETURN && key != irr::KEY_ESCAPE)
		return false;
	swallow_key = key;
	switch(tour_state) {
	case TOUR_COUNTDOWN:
		if(key == irr::KEY_RETURN)
			countdown_end = 0;
		else
			StopTournamentLoop(L"Bot Theater: tournament paused. Press Enter to play the next match.");
		break;
	case TOUR_RECORD:
	case TOUR_RECORDING:
		if(key == irr::KEY_ESCAPE) {
			pause_requested = true;
			panel_lines.push_back(L"Pausing once the result is saved...");
		}
		break;
	case TOUR_FINISHED:
		StopTournamentLoop(nullptr);
		break;
	default:
		break;
	}
	return true;
}

void BotTheater::DrawTournamentPanel() {
	auto driver = mainGame->driver;
	auto font = mainGame->textFont;
	auto big = mainGame->lpcFont;
	if(!font)
		return;
	std::vector<std::wstring> lines = panel_lines;
	std::wstring countdown, hint;
	if(tour_state == TOUR_COUNTDOWN) {
		unsigned long long now = NowMs();
		unsigned long long left = countdown_end > now ? (countdown_end - now + 999) / 1000 : 0;
		countdown = std::to_wstring(left);
		hint = L"Enter: start now      Esc: pause the tournament";
	} else if(tour_state == TOUR_FINISHED) {
		hint = L"Enter or Esc: back to the host window";
	} else {
		hint = L"Esc: pause after this";
	}
	const std::wstring title = L"BOT THEATER TOURNAMENT";
	int line_h = font->getDimension(L"Ay").Height + (int)(8 * mainGame->yScale);
	int big_h = (!countdown.empty() && big) ? big->getDimension(L"0").Height + (int)(10 * mainGame->yScale) : 0;
	int width = (int)(520 * mainGame->xScale);
	for(auto& l : lines)
		width = std::max(width, (int)font->getDimension(l.c_str()).Width + (int)(60 * mainGame->xScale));
	width = std::min(width, (int)mainGame->window_size.Width - 20);
	int pad = (int)(18 * mainGame->yScale);
	int height = pad * 2 + line_h * (int)(lines.size() + 3) + big_h;
	int cx = mainGame->window_size.Width / 2, cy = mainGame->window_size.Height / 2;
	irr::core::recti box(cx - width / 2, cy - height / 2, cx + width / 2, cy + height / 2);
	driver->draw2DRectangle(irr::video::SColor(215, 10, 14, 30), box);
	irr::video::SColor gold(255, 255, 200, 40);
	for(int i = 0; i < 2; ++i)
		driver->draw2DRectangleOutline(irr::core::recti(box.UpperLeftCorner.X + i, box.UpperLeftCorner.Y + i,
			box.LowerRightCorner.X - i, box.LowerRightCorner.Y - i), gold);
	int y = box.UpperLeftCorner.Y + pad;
	auto row = [&](const std::wstring& text, irr::gui::CGUITTFont* f, int h, irr::video::SColor color) {
		irr::core::recti r(box.UpperLeftCorner.X, y, box.LowerRightCorner.X, y + h);
		f->drawUstring(text, r + irr::core::vector2di(1, 1), irr::video::SColor(255, 0, 0, 0), true, true);	// shadow
		f->drawUstring(text, r, color, true, true);
		y += h;
	};
	row(title, font, line_h, gold);
	y += line_h / 2;
	for(auto& l : lines)
		row(l, font, line_h, irr::video::SColor(255, 255, 255, 255));
	if(!countdown.empty() && big)
		row(countdown, big, big_h, irr::video::SColor(255, 120, 220, 255));
	y += line_h / 2 - (countdown.empty() ? 0 : line_h / 2);
	row(hint, font, line_h, irr::video::SColor(255, 170, 170, 170));
}

}
