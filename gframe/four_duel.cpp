#include "config.h"
#include "four_duel.h"
#include "netserver.h"
#include "game.h"
#include "data_manager.h"
#include "bot_theater.h"
#include "../ocgcore/mtrandom.h"
#include <algorithm>
#include <random>

namespace ygo {

FourDuel::FourDuel() {
	for(int i = 0; i < 4; ++i) {
		players[i] = nullptr;
		ready[i] = false;
		deck_error[i] = 0;
	}
	last_response = 0;
}
FourDuel::~FourDuel() {
}

// ---------------------------------------------------------------- sending

void FourDuel::Send(unsigned seats, bool spectators, const unsigned char* buf, int len) {
	NetServer::SendBufferToPlayer(nullptr, STOC_GAME_MSG, (void*)buf, len);	// fills the send buffer only
	for(int i = 0; i < 4; ++i)
		if(seats & (1u << i))
			NetServer::ReSendToPlayer(players[i]);
	if(spectators)
		for(auto dp : observers)
			NetServer::ReSendToPlayer(dp);
}
void FourDuel::SendPacketToAll(unsigned char proto) {
	NetServer::SendPacketToPlayer(nullptr, proto);
	for(int i = 0; i < 4; ++i)
		NetServer::ReSendToPlayer(players[i]);
	for(auto dp : observers)
		NetServer::ReSendToPlayer(dp);
}
void FourDuel::UpdateWatchCount() {
	STOC_HS_WatchChange scwc;
	scwc.watch_count = (unsigned short)observers.size();
	NetServer::SendPacketToPlayer(nullptr, STOC_HS_WATCH_CHANGE, scwc);
	for(int i = 0; i < 4; ++i)
		NetServer::ReSendToPlayer(players[i]);
	for(auto dp : observers)
		NetServer::ReSendToPlayer(dp);
}

// ---------------------------------------------------------------- the room

void FourDuel::Chat(DuelPlayer* dp, unsigned char* pdata, int len) {
	unsigned char scc[SIZE_STOC_CHAT];
	const auto scc_size = NetServer::CreateChatPacket(pdata, len, scc, dp->type);
	if (!scc_size)
		return;
	NetServer::SendBufferToPlayer(nullptr, STOC_CHAT, scc, scc_size);
	for(int i = 0; i < 4; ++i)
		NetServer::ReSendToPlayer(players[i]);
	for(auto pit = observers.begin(); pit != observers.end(); ++pit)
		NetServer::ReSendToPlayer(*pit);
}
void FourDuel::JoinGame(DuelPlayer* dp, unsigned char* pdata, bool is_creater) {
	if(!is_creater) {
		if(dp->game && dp->type != 0xff) {
			STOC_ErrorMsg scem;
			scem.msg = ERRMSG_JOINERROR;
			scem.code = 0;
			NetServer::SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
			NetServer::DisconnectPlayer(dp);
			return;
		}
		CTOS_JoinGame packet;
		std::memcpy(&packet, pdata, sizeof packet);
		auto pkt = &packet;
		if(pkt->version != PRO_VERSION) {
			STOC_ErrorMsg scem;
			scem.msg = ERRMSG_VERERROR;
			scem.code = PRO_VERSION;
			NetServer::SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
			NetServer::DisconnectPlayer(dp);
			return;
		}
		wchar_t jpass[20];
		BufferIO::NullTerminate(pkt->pass);
		BufferIO::CopyCharArray(pkt->pass, jpass);
		if(std::wcscmp(jpass, pass)) {
			STOC_ErrorMsg scem;
			scem.msg = ERRMSG_JOINERROR;
			scem.code = 1;
			NetServer::SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
			return;
		}
	}
	dp->game = this;
	if(!players[0] && !players[1] && !players[2] && !players[3] && observers.size() == 0)
		host_player = dp;
	STOC_JoinGame scjg;
	scjg.info = host_info;
	STOC_TypeChange sctc;
	sctc.type = (host_player == dp) ? 0x10 : 0;
	int pos = 0;
	while(pos < 4 && players[pos])
		++pos;
	if(pos < 4) {
		STOC_HS_PlayerEnter scpe;
		BufferIO::CopyCharArray(dp->name, scpe.name);
		scpe.pos = pos;
		for(int i = 0; i < 4; ++i)
			if(players[i])
				NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_ENTER, scpe);
		for(auto pit = observers.begin(); pit != observers.end(); ++pit)
			NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_ENTER, scpe);
		players[pos] = dp;
		dp->type = pos;
		sctc.type |= pos;
	} else {
		observers.insert(dp);
		dp->type = NETPLAYER_TYPE_OBSERVER;
		sctc.type |= NETPLAYER_TYPE_OBSERVER;
		UpdateWatchCount();
	}
	NetServer::SendPacketToPlayer(dp, STOC_JOIN_GAME, scjg);
	NetServer::SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
	for(int i = 0; i < 4; ++i)
		if(players[i]) {
			STOC_HS_PlayerEnter scpe;
			BufferIO::CopyCharArray(players[i]->name, scpe.name);
			scpe.pos = i;
			NetServer::SendPacketToPlayer(dp, STOC_HS_PLAYER_ENTER, scpe);
			if(ready[i]) {
				STOC_HS_PlayerChange scpc;
				scpc.status = (i << 4) | PLAYERCHANGE_READY;
				NetServer::SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
			}
		}
	if(observers.size()) {
		STOC_HS_WatchChange scwc;
		scwc.watch_count = (unsigned short)observers.size();
		NetServer::SendPacketToPlayer(dp, STOC_HS_WATCH_CHANGE, scwc);
	}
}
void FourDuel::LeaveGame(DuelPlayer* dp) {
	if(dp == host_player) {
		EndDuel();
		NetServer::StopServer();
	} else if(dp->type == NETPLAYER_TYPE_OBSERVER) {
		observers.erase(dp);
		if(duel_stage == DUEL_STAGE_BEGIN)
			UpdateWatchCount();
		NetServer::DisconnectPlayer(dp);
	} else {
		if(duel_stage == DUEL_STAGE_BEGIN) {
			STOC_HS_PlayerChange scpc;
			players[dp->type] = nullptr;
			ready[dp->type] = false;
			scpc.status = (dp->type << 4) | PLAYERCHANGE_LEAVE;
			for(int i = 0; i < 4; ++i)
				if(players[i])
					NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_CHANGE, scpc);
			for(auto pit = observers.begin(); pit != observers.end(); ++pit)
				NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_CHANGE, scpc);
		} else if(duel_stage != DUEL_STAGE_END) {
			// A player who leaves during the duel ends it for everyone (as in tag duels).
			EndDuel();
			DuelEndProc();
		}
		if(dp->type < 4 && players[dp->type] == dp)
			players[dp->type] = nullptr;
		NetServer::DisconnectPlayer(dp);
	}
}
void FourDuel::ToDuelist(DuelPlayer* dp) {
	if(players[0] && players[1] && players[2] && players[3])
		return;
	if(dp->type == NETPLAYER_TYPE_OBSERVER) {
		observers.erase(dp);
		STOC_HS_PlayerEnter scpe;
		BufferIO::CopyCharArray(dp->name, scpe.name);
		int pos = 0;
		while(players[pos])
			++pos;
		dp->type = pos;
		players[pos] = dp;
		scpe.pos = pos;
		STOC_HS_WatchChange scwc;
		scwc.watch_count = (unsigned short)observers.size();
		for(int i = 0; i < 4; ++i)
			if(players[i]) {
				NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_ENTER, scpe);
				NetServer::SendPacketToPlayer(players[i], STOC_HS_WATCH_CHANGE, scwc);
			}
		for(auto pit = observers.begin(); pit != observers.end(); ++pit) {
			NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_ENTER, scpe);
			NetServer::SendPacketToPlayer(*pit, STOC_HS_WATCH_CHANGE, scwc);
		}
		STOC_TypeChange sctc;
		sctc.type = (dp == host_player ? 0x10 : 0) | dp->type;
		NetServer::SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
	} else {
		if(ready[dp->type])
			return;
		unsigned char dptype = (dp->type + 1) % 4;
		while(players[dptype])
			dptype = (dptype + 1) % 4;
		STOC_HS_PlayerChange scpc;
		scpc.status = (dp->type << 4) | dptype;
		for(int i = 0; i < 4; ++i)
			if(players[i])
				NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_CHANGE, scpc);
		for(auto pit = observers.begin(); pit != observers.end(); ++pit)
			NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_CHANGE, scpc);
		STOC_TypeChange sctc;
		sctc.type = (dp == host_player ? 0x10 : 0) | dptype;
		NetServer::SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
		players[dptype] = dp;
		players[dp->type] = nullptr;
		std::swap(pdeck[dptype], pdeck[dp->type]);
		std::swap(deck_error[dptype], deck_error[dp->type]);
		dp->type = dptype;
	}
}
void FourDuel::ToObserver(DuelPlayer* dp) {
	if(dp->type > 3)
		return;
	STOC_HS_PlayerChange scpc;
	scpc.status = (dp->type << 4) | PLAYERCHANGE_OBSERVE;
	for(int i = 0; i < 4; ++i)
		if(players[i])
			NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_CHANGE, scpc);
	for(auto pit = observers.begin(); pit != observers.end(); ++pit)
		NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_CHANGE, scpc);
	players[dp->type] = nullptr;
	ready[dp->type] = false;
	dp->type = NETPLAYER_TYPE_OBSERVER;
	observers.insert(dp);
	STOC_TypeChange sctc;
	sctc.type = (dp == host_player ? 0x10 : 0) | dp->type;
	NetServer::SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
}
void FourDuel::PlayerReady(DuelPlayer* dp, bool is_ready) {
	if(dp->type > 3 || ready[dp->type] == is_ready)
		return;
	if(is_ready) {
		unsigned int deckerror = 0;
		if(!host_info.no_check_deck) {
			if(deck_error[dp->type]) {
				deckerror = (DECKERROR_UNKNOWNCARD << 28) + deck_error[dp->type];
			} else {
				deckerror = deckManager.CheckDeck(pdeck[dp->type], host_info.lflist, host_info.rule);
			}
		}
		if(deckerror) {
			STOC_HS_PlayerChange scpc;
			scpc.status = (dp->type << 4) | PLAYERCHANGE_NOTREADY;
			NetServer::SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
			STOC_ErrorMsg scem;
			scem.msg = ERRMSG_DECKERROR;
			scem.code = deckerror;
			NetServer::SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
			return;
		}
	}
	ready[dp->type] = is_ready;
	STOC_HS_PlayerChange scpc;
	scpc.status = (dp->type << 4) | (is_ready ? PLAYERCHANGE_READY : PLAYERCHANGE_NOTREADY);
	for(int i = 0; i < 4; ++i)
		if(players[i])
			NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_CHANGE, scpc);
	for(auto pit = observers.begin(); pit != observers.end(); ++pit)
		NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_CHANGE, scpc);
}
void FourDuel::PlayerKick(DuelPlayer* dp, unsigned char pos) {
	if(pos > 3 || dp != host_player || dp == players[pos] || !players[pos])
		return;
	LeaveGame(players[pos]);
}
void FourDuel::UpdateDeck(DuelPlayer* dp, unsigned char* pdata, int len) {
	if(dp->type > 3 || ready[dp->type])
		return;
	if (len < 8 || len > (int)sizeof(CTOS_DeckData))
		return;
	bool valid = true;
	CTOS_DeckData deckbuf;
	std::memcpy(&deckbuf, pdata, len);
	if (deckbuf.mainc < 0 || deckbuf.mainc > MAINC_MAX)
		valid = false;
	else if (deckbuf.sidec < 0 || deckbuf.sidec > SIDEC_MAX)
		valid = false;
	else if (len < (2 + deckbuf.mainc + deckbuf.sidec) * (int)sizeof(int32_t))
		valid = false;
	if (!valid) {
		STOC_ErrorMsg scem;
		scem.msg = ERRMSG_DECKERROR;
		scem.code = 0;
		NetServer::SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
		return;
	}
	deck_error[dp->type] = DeckManager::LoadDeck(pdeck[dp->type], deckbuf.list, deckbuf.mainc, deckbuf.sidec);
}

// ---------------------------------------------------------------- the duel

void FourDuel::StartDuel(DuelPlayer* dp) {
	if(dp != host_player)
		return;
	for(int i = 0; i < 4; ++i)
		if(!players[i] || !ready[i])
			return;
	NetServer::StopListen();
	for(int i = 0; i < 4; ++i)
		NetServer::SendPacketToPlayer(players[i], STOC_DUEL_START);
	for(auto oit = observers.begin(); oit != observers.end(); ++oit) {
		(*oit)->state = CTOS_LEAVE_GAME;
		NetServer::ReSendToPlayer(*oit);
	}
	duel_stage = DUEL_STAGE_DUELING;

	// Seat the four players in a random order round the table: seat 0 goes first.
	std::random_device rd;
	unsigned int seed = rd();
	mt19937 rnd((uint_fast32_t)seed);
	auto duel_seed = rnd.rand();	// first, as the replay viewer starts the engine with it
	std::mt19937 seat_rng(rd());
	unsigned char lobby_pos[4] = { 0, 1, 2, 3 };
	std::shuffle(lobby_pos, lobby_pos + 4, seat_rng);
	DuelPlayer* lobby_players[4];
	Deck lobby_decks[4];
	for(int i = 0; i < 4; ++i) {
		lobby_players[i] = players[i];
		lobby_decks[i] = pdeck[i];
	}
	for(int seat = 0; seat < 4; ++seat) {
		players[seat] = lobby_players[lobby_pos[seat]];
		pdeck[seat] = lobby_decks[lobby_pos[seat]];
		players[seat]->type = seat;
		players[seat]->state = 0xff;
	}

	ReplayHeader rh;
	rh.id = 0x31707279;
	rh.version = PRO_VERSION;
	rh.flag = REPLAY_UNIFORM | REPLAY_FOUR_PLAYER;
	rh.seed = seed;
	rh.start_time = (unsigned int)std::time(nullptr);
	last_replay.BeginRecord();
	last_replay.WriteHeader(rh);
	for(int seat = 0; seat < 4; ++seat)
		last_replay.WriteData(players[seat]->name, 40, false);
	if(!host_info.no_shuffle_deck)
		for(int seat = 0; seat < 4; ++seat)
			rnd.shuffle_vector(pdeck[seat].main);
	set_script_reader(DataManager::ScriptReaderEx);
	set_card_reader(DataManager::CardReader);
	set_message_handler(FourDuel::MessageHandler);
	pduel = create_duel(duel_seed);
	set_player_count(pduel, 4);
	for(int seat = 0; seat < 4; ++seat)
		set_player_info(pduel, seat, host_info.start_lp, host_info.start_hand, host_info.draw_count);
	unsigned int opt = (unsigned int)host_info.duel_rule << 16;
	if(host_info.no_shuffle_deck)
		opt |= DUEL_PSEUDO_SHUFFLE;
	last_replay.WriteInt32(host_info.start_lp, false);
	last_replay.WriteInt32(host_info.start_hand, false);
	last_replay.WriteInt32(host_info.draw_count, false);
	last_replay.WriteInt32(opt, false);
	last_replay.Flush();
	auto load = [&](const std::vector<code_pointer>& deck_container, uint8_t p, uint8_t location) {
		last_replay.WriteInt32(deck_container.size(), false);
		for (auto cit = deck_container.rbegin(); cit != deck_container.rend(); ++cit) {
			new_card(pduel, (*cit)->first, p, p, location, 0, POS_FACEDOWN_DEFENSE);
			last_replay.WriteInt32((*cit)->first, false);
		}
	};
	for(int seat = 0; seat < 4; ++seat) {
		load(pdeck[seat].main, seat, LOCATION_DECK);
		load(pdeck[seat].extra, seat, LOCATION_EXTRA);
	}
	last_replay.Flush();

	unsigned char startbuf[START_MSG_SIZE]{};
	auto pbuf = startbuf;
	BufferIO::WriteInt8(pbuf, MSG_START);
	BufferIO::WriteInt8(pbuf, 0);
	BufferIO::WriteInt8(pbuf, host_info.duel_rule);
	for(int seat = 0; seat < 4; ++seat)
		BufferIO::WriteInt8(pbuf, lobby_pos[seat]);
	for(int seat = 0; seat < 4; ++seat)
		BufferIO::WriteInt32(pbuf, host_info.start_lp);
	for(int seat = 0; seat < 4; ++seat) {
		BufferIO::WriteInt16(pbuf, query_field_count(pduel, seat, LOCATION_DECK));
		BufferIO::WriteInt16(pbuf, query_field_count(pduel, seat, LOCATION_EXTRA));
	}
	for(int seat = 0; seat < 4; ++seat) {
		startbuf[1] = seat;
		NetServer::SendBufferToPlayer(players[seat], STOC_GAME_MSG, startbuf, START_MSG_SIZE);
	}
	startbuf[1] = 0x10;
	for(auto oit = observers.begin(); oit != observers.end(); ++oit)
		NetServer::SendBufferToPlayer(*oit, STOC_GAME_MSG, startbuf, START_MSG_SIZE);
	for(int seat = 0; seat < 4; ++seat)
		RefreshExtra(seat);
	start_duel(pduel, opt);
	Process();
}
void FourDuel::Process() {
	std::vector<unsigned char> engineBuffer;
	engineBuffer.reserve(SIZE_MESSAGE_BUFFER);
	unsigned int engFlag = 0;
	int engLen = 0;
	int stop = 0;
	while (!stop) {
		if (engFlag == PROCESSOR_END)
			break;
		unsigned int result = process(pduel);
		engLen = result & PROCESSOR_BUFFER_LEN;
		engFlag = result & PROCESSOR_FLAG;
		if (engLen > 0) {
			if (engLen > (int)engineBuffer.size())
				engineBuffer.resize(engLen);
			get_message(pduel, engineBuffer.data());
			stop = Analyze(engineBuffer.data(), engLen);
		}
	}
	if(stop == 2)
		DuelEndProc();
}
void FourDuel::DuelEndProc() {
	SendPacketToAll(STOC_DUEL_END);
	duel_stage = DUEL_STAGE_END;
}
void FourDuel::Surrender(DuelPlayer* dp) {
	// Not supported yet in a free-for-all: the engine has no way to knock a player out on request.
}
int FourDuel::Analyze(unsigned char* msgbuffer, unsigned int len) {
	unsigned char* offset, *pbufw, *pbuf = msgbuffer;
	int player, count, type;
	while (pbuf - msgbuffer < (int)len) {
		offset = pbuf;
		unsigned char engType = BufferIO::ReadUInt8(pbuf);
		botTheater.NoteMessage(engType, true);	// Bot Theater: for freeze reports
		// Messages the player who must answer gets on its own, then the engine waits.
		auto ask = [&](int p) {
			WaitforResponse(p);
			NetServer::SendBufferToPlayer(players[p], STOC_GAME_MSG, offset, pbuf - offset);
		};
		switch (engType) {
		case MSG_RETRY: {
			WaitforResponse(last_response);
			NetServer::SendBufferToPlayer(players[last_response], STOC_GAME_MSG, offset, pbuf - offset);
			return 1;
		}
		case MSG_HINT: {
			type = BufferIO::ReadUInt8(pbuf);
			player = BufferIO::ReadUInt8(pbuf);
			BufferIO::ReadInt32(pbuf);
			switch (type) {
			case 1:
			case 2:
			case 3:
			case 5:
				Send(Seat(player), false, offset, pbuf - offset);
				break;
			case 4:
			case 6:
			case 7:
			case 8:
			case 9:
			case 11:
				Send(OtherSeats(player), true, offset, pbuf - offset);
				break;
			case 10:
				SendToAll(offset, pbuf - offset);
				break;
			}
			break;
		}
		case MSG_WIN: {
			player = BufferIO::ReadUInt8(pbuf);
			type = BufferIO::ReadUInt8(pbuf);
			SendToAll(offset, pbuf - offset);
			EndDuel();
			return 2;
		}
		case MSG_PLAYER_OUT: {	// a player is knocked out (player, reason)
			pbuf += 2;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_SELECT_BATTLECMD: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 11;
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 8 + 2;
			RefreshField();
			ask(player);
			return 1;
		}
		case MSG_SELECT_IDLECMD: {
			player = BufferIO::ReadUInt8(pbuf);
			for(int i = 0; i < 5; ++i) {
				count = BufferIO::ReadUInt8(pbuf);
				pbuf += count * 7;
			}
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 11 + 3;
			RefreshField();
			ask(player);
			return 1;
		}
		case MSG_SELECT_EFFECTYN: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 12;
			ask(player);
			return 1;
		}
		case MSG_SELECT_YESNO: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 4;
			ask(player);
			return 1;
		}
		case MSG_SELECT_OPTION: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 4;
			ask(player);
			return 1;
		}
		case MSG_SELECT_CARD:
		case MSG_SELECT_TRIBUTE: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 3;
			count = BufferIO::ReadUInt8(pbuf);
			for (int i = 0; i < count; ++i) {
				pbufw = pbuf;
				BufferIO::ReadInt32(pbuf);
				int c = BufferIO::ReadUInt8(pbuf);
				pbuf += 3;
				if (c != player) BufferIO::WriteInt32(pbufw, 0);
			}
			ask(player);
			return 1;
		}
		case MSG_SELECT_UNSELECT_CARD: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 4;
			for(int list = 0; list < 2; ++list) {
				count = BufferIO::ReadUInt8(pbuf);
				for (int i = 0; i < count; ++i) {
					pbufw = pbuf;
					BufferIO::ReadInt32(pbuf);
					int c = BufferIO::ReadUInt8(pbuf);
					pbuf += 3;
					if (c != player) BufferIO::WriteInt32(pbufw, 0);
				}
			}
			ask(player);
			return 1;
		}
		case MSG_SELECT_CHAIN: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += 10 + count * 13;
			ask(player);
			return 1;
		}
		case MSG_SELECT_PLACE:
		case MSG_SELECT_DISFIELD: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 5;
			ask(player);
			return 1;
		}
		case MSG_SELECT_POSITION: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 5;
			ask(player);
			return 1;
		}
		case MSG_SELECT_COUNTER: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 4;
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 9;
			ask(player);
			return 1;
		}
		case MSG_SELECT_SUM: {
			pbuf++;
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 6;
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 11;
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 11;
			ask(player);
			return 1;
		}
		case MSG_SORT_CARD: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 7;
			ask(player);
			return 1;
		}
		case MSG_CONFIRM_DECKTOP:
		case MSG_CONFIRM_EXTRATOP: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 7;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_CONFIRM_CARDS: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			bool from_deck = count > 0 && pbuf[5] == LOCATION_DECK;
			pbuf += count * 7;
			if(!from_deck)
				SendToAll(offset, pbuf - offset);
			else	// cards looked at in the Deck: only the player who looks
				Send(Seat(player), false, offset, pbuf - offset);
			break;
		}
		case MSG_SHUFFLE_DECK: {
			player = BufferIO::ReadUInt8(pbuf);
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_SHUFFLE_HAND: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			bool reveal = botTheater.RevealHands();	// Bot Theater: spectators see hands like their owners
			Send(Seat(player), reveal, offset, (pbuf - offset) + count * 4);
			for(int i = 0; i < count; ++i)
				BufferIO::WriteInt32(pbuf, 0);
			Send(OtherSeats(player), !reveal, offset, pbuf - offset);
			RefreshHand(player, 0x781fff, 0);
			break;
		}
		case MSG_SHUFFLE_EXTRA: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			Send(Seat(player), false, offset, (pbuf - offset) + count * 4);
			for(int i = 0; i < count; ++i)
				BufferIO::WriteInt32(pbuf, 0);
			Send(OtherSeats(player), true, offset, pbuf - offset);
			RefreshExtra(player);
			break;
		}
		case MSG_REFRESH_DECK: {
			pbuf++;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_SWAP_GRAVE_DECK: {
			player = BufferIO::ReadUInt8(pbuf);
			SendToAll(offset, pbuf - offset);
			RefreshGrave(player);
			break;
		}
		case MSG_REVERSE_DECK: {
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_DECK_TOP: {
			pbuf += 6;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_SHUFFLE_SET_CARD: {
			unsigned int loc = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 8;
			SendToAll(offset, pbuf - offset);
			for(int p = 0; p < 4; ++p) {
				if(loc == LOCATION_MZONE)
					RefreshMzone(p, 0x181fff, 0);
				else
					RefreshSzone(p, 0x181fff, 0);
			}
			break;
		}
		case MSG_NEW_TURN: {
			pbuf++;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_NEW_PHASE: {
			pbuf += 2;
			SendToAll(offset, pbuf - offset);
			RefreshField();
			break;
		}
		case MSG_MOVE: {
			pbufw = pbuf;
			int pc = pbuf[4];
			int pl = pbuf[5];
			int cc = pbuf[8];
			int cl = pbuf[9];
			int cs = pbuf[10];
			int cp = pbuf[11];
			pbuf += 16;
			bool hidden = !(cl & (LOCATION_GRAVE + LOCATION_OVERLAY)) && ((cl & (LOCATION_DECK + LOCATION_HAND)) || (cp & POS_FACEDOWN));
			bool reveal = hidden && (cl & LOCATION_HAND) && botTheater.RevealHands();	// Bot Theater: spectators see cards going into a hand
			Send(Seat(cc), reveal, offset, pbuf - offset);
			if (hidden)
				BufferIO::WriteInt32(pbufw, 0);
			Send(OtherSeats(cc), !reveal, offset, pbuf - offset);
			if (cl != 0 && (cl & LOCATION_OVERLAY) == 0 && (cl != pl || pc != cc))
				RefreshSingle(cc, cl, cs);
			break;
		}
		case MSG_POS_CHANGE: {
			int cc = pbuf[4];
			int cl = pbuf[5];
			int cs = pbuf[6];
			int pp = pbuf[7];
			int cp = pbuf[8];
			pbuf += 9;
			SendToAll(offset, pbuf - offset);
			if((pp & POS_FACEDOWN) && (cp & POS_FACEUP))
				RefreshSingle(cc, cl, cs);
			break;
		}
		case MSG_SET: {
			BufferIO::WriteInt32(pbuf, 0);
			pbuf += 4;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_SWAP: {
			int c1 = pbuf[4];
			int l1 = pbuf[5];
			int s1 = pbuf[6];
			int c2 = pbuf[12];
			int l2 = pbuf[13];
			int s2 = pbuf[14];
			pbuf += 16;
			SendToAll(offset, pbuf - offset);
			RefreshSingle(c1, l1, s1);
			RefreshSingle(c2, l2, s2);
			break;
		}
		case MSG_FIELD_DISABLED: {
			pbuf += 4;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_SUMMONING:
		case MSG_SPSUMMONING: {
			pbuf += 8;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_SUMMONED:
		case MSG_SPSUMMONED:
		case MSG_FLIPSUMMONED: {
			SendToAll(offset, pbuf - offset);
			for(int p = 0; p < 4; ++p) {
				RefreshMzone(p);
				RefreshSzone(p);
			}
			break;
		}
		case MSG_FLIPSUMMONING: {
			RefreshSingle(pbuf[4], pbuf[5], pbuf[6]);
			pbuf += 8;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_CHAINING: {
			pbuf += 16;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_CHAINED: {
			pbuf++;
			SendToAll(offset, pbuf - offset);
			RefreshField();
			break;
		}
		case MSG_CHAIN_SOLVING:
		case MSG_CHAIN_NEGATED:
		case MSG_CHAIN_DISABLED: {
			pbuf++;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_CHAIN_SOLVED: {
			pbuf++;
			SendToAll(offset, pbuf - offset);
			RefreshField();
			break;
		}
		case MSG_CHAIN_END: {
			SendToAll(offset, pbuf - offset);
			RefreshField();
			break;
		}
		case MSG_CARD_SELECTED: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 4;
			break;
		}
		case MSG_RANDOM_SELECTED: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 4;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_BECOME_TARGET: {
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 4;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_DRAW: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbufw = pbuf;
			pbuf += count * 4;
			bool reveal = botTheater.RevealHands();	// Bot Theater: spectators see drawn cards
			Send(Seat(player), reveal, offset, pbuf - offset);
			for (int i = 0; i < count; ++i) {
				if(!(pbufw[3] & 0x80))
					BufferIO::WriteInt32(pbufw, 0);
				else
					pbufw += 4;
			}
			Send(OtherSeats(player), !reveal, offset, pbuf - offset);
			break;
		}
		case MSG_DAMAGE:
		case MSG_RECOVER:
		case MSG_LPUPDATE:
		case MSG_PAY_LPCOST: {
			pbuf += 5;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_EQUIP:
		case MSG_CARD_TARGET:
		case MSG_CANCEL_TARGET: {
			pbuf += 8;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_UNEQUIP: {
			pbuf += 4;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_ADD_COUNTER:
		case MSG_REMOVE_COUNTER: {
			pbuf += 7;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_ATTACK: {
			pbuf += 8;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_BATTLE: {
			pbuf += 26;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_ATTACK_DISABLED: {
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_DAMAGE_STEP_START:
		case MSG_DAMAGE_STEP_END: {
			SendToAll(offset, pbuf - offset);
			for(int p = 0; p < 4; ++p)
				RefreshMzone(p);
			break;
		}
		case MSG_MISSED_EFFECT: {
			player = pbuf[0];
			pbuf += 8;
			Send(Seat(player), false, offset, pbuf - offset);
			break;
		}
		case MSG_TOSS_COIN:
		case MSG_TOSS_DICE: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_ROCK_PAPER_SCISSORS: {
			player = BufferIO::ReadUInt8(pbuf);
			ask(player);
			return 1;
		}
		case MSG_HAND_RES: {
			pbuf += 1;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_ANNOUNCE_RACE:
		case MSG_ANNOUNCE_ATTRIB: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 5;
			ask(player);
			return 1;
		}
		case MSG_ANNOUNCE_CARD:
		case MSG_ANNOUNCE_NUMBER: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += 4 * count;
			ask(player);
			return 1;
		}
		case MSG_CARD_HINT: {
			pbuf += 9;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_PLAYER_HINT: {
			pbuf += 6;
			SendToAll(offset, pbuf - offset);
			break;
		}
		case MSG_MATCH_KILL: {
			pbuf += 4;
			break;
		}
		default: {
			// A message this server doesn't know: its length is unknown, so the rest of the
			// buffer can't be read. Stop the duel rather than send the players garbage.
			char msg[64];
			snprintf(msg, sizeof msg, "Free-for-all host: unknown duel message %d", engType);
			mainGame->AddDebugMsg(msg);
			EndDuel();
			return 2;
		}
		}
	}
	return 0;
}
void FourDuel::GetResponse(DuelPlayer* dp, unsigned char* pdata, unsigned int len) {
	if(dp->type > 3 || dp->type != last_response)
		return;
	unsigned char resb[SIZE_RETURN_VALUE]{};
	if (len > SIZE_RETURN_VALUE)
		len = SIZE_RETURN_VALUE;
	std::memcpy(resb, pdata, len);
	last_replay.Write<uint8_t>(len);
	last_replay.WriteData(resb, len);
	set_responseb(pduel, resb);
	players[dp->type]->state = 0xff;
	Process();
}
void FourDuel::EndDuel() {
	if(!pduel)
		return;
	last_replay.EndRecord();
	std::vector<char> replaybuf(sizeof(ReplayHeader) + last_replay.comp_size);
	std::memcpy(replaybuf.data(), &last_replay.pheader, sizeof(ReplayHeader));
	std::memcpy(replaybuf.data() + sizeof(ReplayHeader), last_replay.comp_data, last_replay.comp_size);
	if(replaybuf.size() <= MAX_DATA_SIZE) {	// too big for one packet: no replay
		NetServer::SendBufferToPlayer(nullptr, STOC_REPLAY, replaybuf.data(), replaybuf.size());
		for(int i = 0; i < 4; ++i)
			NetServer::ReSendToPlayer(players[i]);
		for(auto oit = observers.begin(); oit != observers.end(); ++oit)
			NetServer::ReSendToPlayer(*oit);
	}
	end_duel(pduel);
	event_del(etimer);
	pduel = 0;
}
void FourDuel::WaitforResponse(int seat) {
	last_response = seat;
	unsigned char msg = MSG_WAITING;
	NetServer::SendPacketToPlayer(nullptr, STOC_GAME_MSG, msg);
	for(int i = 0; i < 4; ++i)
		if(i != seat)
			NetServer::ReSendToPlayer(players[i]);
	if(players[seat])
		players[seat]->state = CTOS_RESPONSE;
}

// ---------------------------------------------------------------- what each player may see

int FourDuel::WriteUpdateData(int player, int location, int flag, unsigned char* qbuf, int use_cache) {
	flag |= (QUERY_CODE | QUERY_POSITION);
	BufferIO::WriteInt8(qbuf, MSG_UPDATE_DATA);
	BufferIO::WriteInt8(qbuf, player);
	BufferIO::WriteInt8(qbuf, location);
	return query_field_card(pduel, player, location, flag, qbuf, use_cache);
}
// Field zones: the controller sees everything, everyone else sees face-down cards blank.
static void HideFaceDown(unsigned char* qbuf, int len, bool hand) {
	int qlen = 0;
	while(qlen < len) {
		int clen = BufferIO::ReadInt32(qbuf);
		qlen += clen;
		if (clen <= LEN_HEADER)
			continue;
		auto position = GetPosition(qbuf, 8);
		if (hand ? !(position & POS_FACEUP) : (position & POS_FACEDOWN))
			std::memset(qbuf, 0, clen - 4);
		qbuf += clen - 4;
	}
}
void FourDuel::RefreshMzone(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_MZONE, flag, query_buffer.data(), use_cache);
	Send(Seat(player), false, query_buffer.data(), len + 3);
	HideFaceDown(query_buffer.data() + 3, len, false);
	Send(OtherSeats(player), true, query_buffer.data(), len + 3);
}
void FourDuel::RefreshSzone(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_SZONE, flag, query_buffer.data(), use_cache);
	Send(Seat(player), false, query_buffer.data(), len + 3);
	HideFaceDown(query_buffer.data() + 3, len, false);
	Send(OtherSeats(player), true, query_buffer.data(), len + 3);
}
void FourDuel::RefreshHand(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_HAND, flag, query_buffer.data(), use_cache);
	bool reveal = botTheater.RevealHands();	// Bot Theater: spectators see the hand like its owner does
	Send(Seat(player), reveal, query_buffer.data(), len + 3);
	HideFaceDown(query_buffer.data() + 3, len, true);
	Send(OtherSeats(player), !reveal, query_buffer.data(), len + 3);
}
void FourDuel::RefreshGrave(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_GRAVE, flag, query_buffer.data(), use_cache);
	SendToAll(query_buffer.data(), len + 3);
}
void FourDuel::RefreshExtra(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_EXTRA, flag, query_buffer.data(), use_cache);
	Send(Seat(player), false, query_buffer.data(), len + 3);
}
void FourDuel::RefreshField() {
	for(int p = 0; p < 4; ++p) {
		RefreshMzone(p);
		RefreshSzone(p);
		RefreshHand(p);
	}
}
void FourDuel::RefreshSingle(int player, int location, int sequence, int flag) {
	if(player < 0 || player > 3)
		return;
	flag |= (QUERY_CODE | QUERY_POSITION);
	unsigned char query_buffer[0x1000];
	auto qbuf = query_buffer;
	BufferIO::WriteInt8(qbuf, MSG_UPDATE_CARD);
	BufferIO::WriteInt8(qbuf, player);
	BufferIO::WriteInt8(qbuf, location);
	BufferIO::WriteInt8(qbuf, sequence);
	int len = query_card(pduel, player, location, sequence, flag, qbuf, 0);
	auto position = GetPosition(qbuf, 12);
	if(location & LOCATION_ONFIELD) {
		Send(Seat(player), false, query_buffer, len + 4);
		if(position & POS_FACEUP)
			Send(OtherSeats(player), true, query_buffer, len + 4);
	} else {
		bool reveal = (location == LOCATION_HAND) && botTheater.RevealHands();	// Bot Theater: spectators see hand cards
		Send(Seat(player), reveal, query_buffer, len + 4);
		if(location == LOCATION_REMOVED && (position & POS_FACEDOWN))
			return;
		if (location & 0x90)	// GY or Xyz material
			Send(OtherSeats(player), !reveal, query_buffer, len + 4);
	}
}
uint32_t FourDuel::MessageHandler(intptr_t fduel, uint32_t type) {
	if(!enable_log)
		return 0;
	char msgbuf[1024];
	get_log_message(fduel, msgbuf);
	mainGame->AddDebugMsg(msgbuf);
	return 0;
}
void FourDuel::FourTimer(evutil_socket_t fd, short events, void* arg) {
	// Free-for-all rooms have no time limit (the host turns it off), so there's nothing to time.
}

}
