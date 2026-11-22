#include "config.h"
#include "five_duel.h"
#include "netserver.h"
#include "game.h"
#include "data_manager.h"
#include "bot_theater.h"
#include "../ocgcore/mtrandom.h"
#include <algorithm>
#include <random>

namespace ygo {

FiveDuel::FiveDuel() {
	for(int i = 0; i < SEATS; ++i) {
		players[i] = nullptr;
		ready[i] = false;
		deck_error[i] = 0;
		team_seat[i] = (unsigned char)i;
	}
	member[0] = member[1] = 0;
	cur_player[0] = cur_player[1] = nullptr;
	last_response = 0;
}
FiveDuel::~FiveDuel() {
}

// ---------------------------------------------------------------- sending

void FiveDuel::SendToAll(const unsigned char* buf, int len, bool spectators, DuelPlayer* skip) {
	NetServer::SendBufferToPlayer(nullptr, STOC_GAME_MSG, (void*)buf, len);	// fills the send buffer only
	for(int i = 0; i < SEATS; ++i)
		if(players[i] && players[i] != skip)
			NetServer::ReSendToPlayer(players[i]);
	if(spectators)
		SendBufferToSpectators();
}
void FiveDuel::SendToSide(int side, const unsigned char* buf, int len) {
	NetServer::SendBufferToPlayer(nullptr, STOC_GAME_MSG, (void*)buf, len);
	for(int m = 0; m < MEMBERS; ++m)
		NetServer::ReSendToPlayer(players[team_seat[(side & 1) * MEMBERS + m]]);
}
void FiveDuel::SendBufferToSpectators() {
	for(auto dp : observers)
		NetServer::ReSendToPlayer(dp);
}
void FiveDuel::SendPacketToAll(unsigned char proto) {
	NetServer::SendPacketToPlayer(nullptr, proto);
	for(int i = 0; i < SEATS; ++i)
		NetServer::ReSendToPlayer(players[i]);
	SendBufferToSpectators();
}
void FiveDuel::UpdateWatchCount() {
	STOC_HS_WatchChange scwc;
	scwc.watch_count = (unsigned short)observers.size();
	NetServer::SendPacketToPlayer(nullptr, STOC_HS_WATCH_CHANGE, scwc);
	for(int i = 0; i < SEATS; ++i)
		NetServer::ReSendToPlayer(players[i]);
	SendBufferToSpectators();
}
bool FiveDuel::OnSide(DuelPlayer* dp, int side) const {
	for(int m = 0; m < MEMBERS; ++m)
		if(players[team_seat[(side & 1) * MEMBERS + m]] == dp)
			return true;
	return false;
}

// ---------------------------------------------------------------- the room

void FiveDuel::Chat(DuelPlayer* dp, unsigned char* pdata, int len) {
	unsigned char scc[SIZE_STOC_CHAT];
	const auto scc_size = NetServer::CreateChatPacket(pdata, len, scc, dp->type);
	if (!scc_size)
		return;
	NetServer::SendBufferToPlayer(nullptr, STOC_CHAT, scc, scc_size);
	for(int i = 0; i < SEATS; ++i)
		NetServer::ReSendToPlayer(players[i]);
	SendBufferToSpectators();
}
void FiveDuel::JoinGame(DuelPlayer* dp, unsigned char* pdata, bool is_creater) {
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
	bool empty = observers.empty();
	for(int i = 0; i < SEATS; ++i)
		if(players[i])
			empty = false;
	if(empty)
		host_player = dp;
	STOC_JoinGame scjg;
	scjg.info = host_info;
	STOC_TypeChange sctc;
	sctc.type = (host_player == dp) ? 0x10 : 0;
	int seat = 0;
	while(seat < SEATS && players[seat])
		++seat;
	if(seat < SEATS) {
		STOC_HS_PlayerEnter scpe;
		BufferIO::CopyCharArray(dp->name, scpe.name);
		scpe.pos = (unsigned char)PosOf(seat);
		for(int i = 0; i < SEATS; ++i)
			if(players[i])
				NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_ENTER, scpe);
		for(auto pit = observers.begin(); pit != observers.end(); ++pit)
			NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_ENTER, scpe);
		players[seat] = dp;
		dp->type = (unsigned char)PosOf(seat);
		sctc.type |= dp->type;
	} else {
		observers.insert(dp);
		dp->type = NETPLAYER_TYPE_OBSERVER;
		sctc.type |= NETPLAYER_TYPE_OBSERVER;
		UpdateWatchCount();
	}
	NetServer::SendPacketToPlayer(dp, STOC_JOIN_GAME, scjg);
	NetServer::SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
	for(int i = 0; i < SEATS; ++i)
		if(players[i]) {
			STOC_HS_PlayerEnter scpe;
			BufferIO::CopyCharArray(players[i]->name, scpe.name);
			scpe.pos = (unsigned char)PosOf(i);
			NetServer::SendPacketToPlayer(dp, STOC_HS_PLAYER_ENTER, scpe);
			if(ready[i]) {
				STOC_HS_PlayerChange scpc;
				scpc.status = (unsigned char)((PosOf(i) << 4) | PLAYERCHANGE_READY);
				NetServer::SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
			}
		}
	if(observers.size()) {
		STOC_HS_WatchChange scwc;
		scwc.watch_count = (unsigned short)observers.size();
		NetServer::SendPacketToPlayer(dp, STOC_HS_WATCH_CHANGE, scwc);
	}
}
void FiveDuel::LeaveGame(DuelPlayer* dp) {
	if(dp == host_player) {
		EndDuel();
		NetServer::StopServer();
	} else if(dp->type == NETPLAYER_TYPE_OBSERVER) {
		observers.erase(dp);
		if(duel_stage == DUEL_STAGE_BEGIN)
			UpdateWatchCount();
		NetServer::DisconnectPlayer(dp);
	} else {
		int seat = SeatOf(dp->type);
		if(duel_stage == DUEL_STAGE_BEGIN) {
			if(seat >= 0) {
				players[seat] = nullptr;
				ready[seat] = false;
			}
			STOC_HS_PlayerChange scpc;
			scpc.status = (unsigned char)((dp->type << 4) | PLAYERCHANGE_LEAVE);
			for(int i = 0; i < SEATS; ++i)
				if(players[i])
					NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_CHANGE, scpc);
			for(auto pit = observers.begin(); pit != observers.end(); ++pit)
				NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_CHANGE, scpc);
		} else if(duel_stage != DUEL_STAGE_END) {
			// A player who leaves during the duel ends it for everyone (as in tag duels).
			EndDuel();
			DuelEndProc();
		}
		if(seat >= 0 && players[seat] == dp)
			players[seat] = nullptr;
		NetServer::DisconnectPlayer(dp);
	}
}
void FiveDuel::ToDuelist(DuelPlayer* dp) {
	// Only from the spectators to a free seat: moving between seats isn't supported (the
	// lobby message for it can't name seats past 7).
	if(dp->type != NETPLAYER_TYPE_OBSERVER)
		return;
	int seat = 0;
	while(seat < SEATS && players[seat])
		++seat;
	if(seat >= SEATS)
		return;
	observers.erase(dp);
	players[seat] = dp;
	dp->type = (unsigned char)PosOf(seat);
	STOC_HS_PlayerEnter scpe;
	BufferIO::CopyCharArray(dp->name, scpe.name);
	scpe.pos = dp->type;
	STOC_HS_WatchChange scwc;
	scwc.watch_count = (unsigned short)observers.size();
	for(int i = 0; i < SEATS; ++i)
		if(players[i]) {
			NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_ENTER, scpe);
			NetServer::SendPacketToPlayer(players[i], STOC_HS_WATCH_CHANGE, scwc);
		}
	for(auto pit = observers.begin(); pit != observers.end(); ++pit) {
		NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_ENTER, scpe);
		NetServer::SendPacketToPlayer(*pit, STOC_HS_WATCH_CHANGE, scwc);
	}
	STOC_TypeChange sctc;
	sctc.type = (unsigned char)((dp == host_player ? 0x10 : 0) | dp->type);
	NetServer::SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
}
void FiveDuel::ToObserver(DuelPlayer* dp) {
	int seat = SeatOf(dp->type);
	if(seat < 0)
		return;
	STOC_HS_PlayerChange scpc;
	scpc.status = (unsigned char)((dp->type << 4) | PLAYERCHANGE_OBSERVE);
	for(int i = 0; i < SEATS; ++i)
		if(players[i])
			NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_CHANGE, scpc);
	for(auto pit = observers.begin(); pit != observers.end(); ++pit)
		NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_CHANGE, scpc);
	players[seat] = nullptr;
	ready[seat] = false;
	dp->type = NETPLAYER_TYPE_OBSERVER;
	observers.insert(dp);
	STOC_TypeChange sctc;
	sctc.type = (unsigned char)((dp == host_player ? 0x10 : 0) | dp->type);
	NetServer::SendPacketToPlayer(dp, STOC_TYPE_CHANGE, sctc);
}
void FiveDuel::PlayerReady(DuelPlayer* dp, bool is_ready) {
	int seat = SeatOf(dp->type);
	if(seat < 0 || ready[seat] == is_ready)
		return;
	if(is_ready) {
		unsigned int deckerror = 0;
		if(!host_info.no_check_deck) {
			if(deck_error[seat]) {
				deckerror = (DECKERROR_UNKNOWNCARD << 28) + deck_error[seat];
			} else {
				deckerror = deckManager.CheckDeck(pdeck[seat], host_info.lflist, host_info.rule);
			}
		}
		if(deckerror) {
			STOC_HS_PlayerChange scpc;
			scpc.status = (unsigned char)((dp->type << 4) | PLAYERCHANGE_NOTREADY);
			NetServer::SendPacketToPlayer(dp, STOC_HS_PLAYER_CHANGE, scpc);
			STOC_ErrorMsg scem;
			scem.msg = ERRMSG_DECKERROR;
			scem.code = deckerror;
			NetServer::SendPacketToPlayer(dp, STOC_ERROR_MSG, scem);
			return;
		}
	}
	ready[seat] = is_ready;
	STOC_HS_PlayerChange scpc;
	scpc.status = (unsigned char)((dp->type << 4) | (is_ready ? PLAYERCHANGE_READY : PLAYERCHANGE_NOTREADY));
	for(int i = 0; i < SEATS; ++i)
		if(players[i])
			NetServer::SendPacketToPlayer(players[i], STOC_HS_PLAYER_CHANGE, scpc);
	for(auto pit = observers.begin(); pit != observers.end(); ++pit)
		NetServer::SendPacketToPlayer(*pit, STOC_HS_PLAYER_CHANGE, scpc);
}
void FiveDuel::PlayerKick(DuelPlayer* dp, unsigned char pos) {
	int seat = SeatOf(pos);
	if(seat < 0 || dp != host_player || dp == players[seat] || !players[seat])
		return;
	LeaveGame(players[seat]);
}
void FiveDuel::UpdateDeck(DuelPlayer* dp, unsigned char* pdata, int len) {
	int seat = SeatOf(dp->type);
	if(seat < 0 || ready[seat])
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
	deck_error[seat] = DeckManager::LoadDeck(pdeck[seat], deckbuf.list, deckbuf.mainc, deckbuf.sidec);
}

// ---------------------------------------------------------------- the duel

void FiveDuel::StartDuel(DuelPlayer* dp) {
	if(dp != host_player)
		return;
	for(int i = 0; i < SEATS; ++i)
		if(!players[i] || !ready[i])
			return;
	NetServer::StopListen();
	for(int i = 0; i < SEATS; ++i)
		NetServer::SendPacketToPlayer(players[i], STOC_DUEL_START);
	for(auto oit = observers.begin(); oit != observers.end(); ++oit) {
		(*oit)->state = CTOS_LEAVE_GAME;
		NetServer::ReSendToPlayer(*oit);
	}
	duel_stage = DUEL_STAGE_DUELING;

	// Two teams in a random order: the first five seats in team_seat are side 0 (which plays
	// first), members 0-4 in turn order, and the next five side 1.
	std::random_device rd;
	unsigned int seed = rd();
	mt19937 rnd((uint_fast32_t)seed);
	auto duel_seed = rnd.rand();	// first, as the replay viewer starts the engine with it
	std::mt19937 seat_rng(rd());
	for(int i = 0; i < SEATS; ++i)
		team_seat[i] = (unsigned char)i;
	std::shuffle(team_seat, team_seat + SEATS, seat_rng);
	for(int i = 0; i < SEATS; ++i)
		players[i]->state = 0xff;
	member[0] = member[1] = 0;
	cur_player[0] = players[team_seat[0]];
	cur_player[1] = players[team_seat[MEMBERS]];

	ReplayHeader rh;
	rh.id = 0x31707279;
	rh.version = PRO_VERSION;
	rh.flag = REPLAY_UNIFORM | REPLAY_FIVE;
	rh.seed = seed;
	rh.start_time = (unsigned int)std::time(nullptr);
	last_replay.BeginRecord();
	last_replay.WriteHeader(rh);
	for(int place = 0; place < SEATS; ++place)
		last_replay.WriteData(players[team_seat[place]]->name, 40, false);
	if(!host_info.no_shuffle_deck)
		for(int place = 0; place < SEATS; ++place)
			rnd.shuffle_vector(pdeck[team_seat[place]].main);
	set_script_reader(DataManager::ScriptReaderEx);
	set_card_reader(DataManager::CardReader);
	set_message_handler(FiveDuel::MessageHandler);
	pduel = create_duel(duel_seed);
	set_player_info(pduel, 0, host_info.start_lp, host_info.start_hand, host_info.draw_count);
	set_player_info(pduel, 1, host_info.start_lp, host_info.start_hand, host_info.draw_count);
	unsigned int opt = (unsigned int)host_info.duel_rule << 16;
	if(host_info.no_shuffle_deck)
		opt |= DUEL_PSEUDO_SHUFFLE;
	opt |= DUEL_TAG_MODE | DUEL_SPLIT_ZONES | DUEL_FIVE_A_SIDE;
	last_replay.WriteInt32(host_info.start_lp, false);
	last_replay.WriteInt32(host_info.start_hand, false);
	last_replay.WriteInt32(host_info.draw_count, false);
	last_replay.WriteInt32(opt, false);
	last_replay.Flush();
	// Member 0 of each side plays first (new_card); the others wait (new_squad_card).
	auto load = [&](const std::vector<code_pointer>& deck_container, uint8_t side, uint8_t m, uint8_t location) {
		last_replay.WriteInt32(deck_container.size(), false);
		for (auto cit = deck_container.rbegin(); cit != deck_container.rend(); ++cit) {
			if(m == 0)
				new_card(pduel, (*cit)->first, side, side, location, 0, POS_FACEDOWN_DEFENSE);
			else
				new_squad_card(pduel, (*cit)->first, side, m, location);
			last_replay.WriteInt32((*cit)->first, false);
		}
	};
	for(int place = 0; place < SEATS; ++place) {
		uint8_t side = (uint8_t)(place / MEMBERS), m = (uint8_t)(place % MEMBERS);
		load(pdeck[team_seat[place]].main, side, m, LOCATION_DECK);
		load(pdeck[team_seat[place]].extra, side, m, LOCATION_EXTRA);
	}
	last_replay.Flush();

	unsigned char startbuf[START_MSG_SIZE]{};
	auto pbuf = startbuf;
	BufferIO::WriteInt8(pbuf, MSG_START);
	BufferIO::WriteInt8(pbuf, 0);
	BufferIO::WriteInt8(pbuf, host_info.duel_rule);
	BufferIO::WriteInt32(pbuf, host_info.start_lp);
	BufferIO::WriteInt32(pbuf, host_info.start_lp);
	BufferIO::WriteInt16(pbuf, query_field_count(pduel, 0, LOCATION_DECK));
	BufferIO::WriteInt16(pbuf, query_field_count(pduel, 0, LOCATION_EXTRA));
	BufferIO::WriteInt16(pbuf, query_field_count(pduel, 1, LOCATION_DECK));
	BufferIO::WriteInt16(pbuf, query_field_count(pduel, 1, LOCATION_EXTRA));
	auto pmember = pbuf;
	BufferIO::WriteInt8(pbuf, 0xff);
	for(int place = 0; place < SEATS; ++place)
		BufferIO::WriteInt8(pbuf, team_seat[place]);
	for(int place = 0; place < SEATS; ++place) {
		startbuf[1] = (unsigned char)(place / MEMBERS);
		*pmember = (unsigned char)(place % MEMBERS);
		NetServer::SendBufferToPlayer(players[team_seat[place]], STOC_GAME_MSG, startbuf, START_MSG_SIZE);
	}
	startbuf[1] = 0x10;
	*pmember = 0xff;
	for(auto oit = observers.begin(); oit != observers.end(); ++oit)
		NetServer::SendBufferToPlayer(*oit, STOC_GAME_MSG, startbuf, START_MSG_SIZE);
	RefreshExtra(0);
	RefreshExtra(1);
	start_duel(pduel, opt);
	Process();
}
void FiveDuel::Process() {
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
		DelayEnd();
}
// The duel is over; end the room after END_DELAY_MS (FiveTimer), so the bots' last lines get
// here and go out first.
void FiveDuel::DelayEnd() {
	end_pending = true;
	timeval tv = { END_DELAY_MS / 1000, (END_DELAY_MS % 1000) * 1000 };
	event_add(etimer, &tv);
}
void FiveDuel::DuelEndProc() {
	if(end_pending) {
		end_pending = false;
		event_del(etimer);
	}
	SendPacketToAll(STOC_DUEL_END);
	duel_stage = DUEL_STAGE_END;
}
int FiveDuel::Analyze(unsigned char* msgbuffer, unsigned int len) {
	unsigned char* offset, *pbufw, *pbuf = msgbuffer;
	int player, count, type;
	// the message so far to everyone, to the playing member of a side, to everyone but them
	auto all = [&]() { SendToAll(offset, (int)(pbuf - offset)); };
	auto to_cur = [&](int p) { NetServer::SendBufferToPlayer(cur_player[p & 1], STOC_GAME_MSG, offset, pbuf - offset); };
	auto refresh_field = [&](bool hands) {
		RefreshMzone(0);
		RefreshMzone(1);
		RefreshSzone(0);
		RefreshSzone(1);
		if(hands) {
			RefreshHand(0);
			RefreshHand(1);
		}
	};
	while (pbuf - msgbuffer < (int)len) {
		offset = pbuf;
		unsigned char engType = BufferIO::ReadUInt8(pbuf);
		botTheater.NoteMessage(engType, true);	// Bot Theater: for freeze reports
		switch (engType) {
		case MSG_RETRY: {
			WaitforResponse(last_response);
			to_cur(last_response);
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
				to_cur(player);
				break;
			case 4:
			case 6:
			case 7:
			case 8:
			case 9:
			case 11:
				SendToAll(offset, (int)(pbuf - offset), true, cur_player[player & 1]);
				break;
			case 10:
				all();
				break;
			}
			break;
		}
		case MSG_WIN: {
			player = BufferIO::ReadUInt8(pbuf);
			type = BufferIO::ReadUInt8(pbuf);
			all();
			EndDuel();
			return 2;
		}
		case MSG_SELECT_BATTLECMD: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 11;
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 8 + 2;
			refresh_field(true);
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_IDLECMD: {
			player = BufferIO::ReadUInt8(pbuf);
			for(int k = 0; k < 5; ++k) {
				count = BufferIO::ReadUInt8(pbuf);
				pbuf += count * 7;
			}
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 11 + 3;
			refresh_field(true);
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_EFFECTYN: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 12;
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_YESNO: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 4;
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_OPTION: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 4;
			WaitforResponse(player);
			to_cur(player);
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
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_UNSELECT_CARD: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 4;
			for(int k = 0; k < 2; ++k) {
				count = BufferIO::ReadUInt8(pbuf);
				for (int i = 0; i < count; ++i) {
					pbufw = pbuf;
					BufferIO::ReadInt32(pbuf);
					int c = BufferIO::ReadUInt8(pbuf);
					pbuf += 3;
					if (c != player) BufferIO::WriteInt32(pbufw, 0);
				}
			}
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_CHAIN: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += 10 + count * 13;
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_PLACE:
		case MSG_SELECT_DISFIELD:
		case MSG_SELECT_POSITION: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 5;
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SELECT_COUNTER: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 4;
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 9;
			WaitforResponse(player);
			to_cur(player);
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
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_SORT_CARD: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 7;
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_CONFIRM_DECKTOP:
		case MSG_CONFIRM_EXTRATOP: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 7;
			all();
			break;
		}
		case MSG_CONFIRM_CARDS: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			bool deck = (pbuf[5] == LOCATION_DECK);
			pbuf += count * 7;
			if(!deck)
				all();
			else
				to_cur(player);
			break;
		}
		case MSG_SHUFFLE_HAND:
		case MSG_SHUFFLE_EXTRA: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			NetServer::SendBufferToPlayer(cur_player[player & 1], STOC_GAME_MSG, offset, (pbuf - offset) + count * 4);
			bool reveal = (engType == MSG_SHUFFLE_HAND) && botTheater.RevealHands();
			if(reveal)	// Bot Theater: spectators see the hand like its owner does
				SendBufferToSpectators();
			for(int i = 0; i < count; ++i)
				BufferIO::WriteInt32(pbuf, 0);
			SendToAll(offset, (int)(pbuf - offset), !reveal, cur_player[player & 1]);
			if(engType == MSG_SHUFFLE_HAND)
				RefreshHand(player, 0x781fff, 0);
			else
				RefreshExtra(player);
			break;
		}
		case MSG_SHUFFLE_DECK: {
			player = BufferIO::ReadUInt8(pbuf);
			all();
			break;
		}
		case MSG_REFRESH_DECK: {
			pbuf++;
			all();
			break;
		}
		case MSG_SWAP_GRAVE_DECK: {
			player = BufferIO::ReadUInt8(pbuf);
			all();
			RefreshGrave(player);
			break;
		}
		case MSG_REVERSE_DECK: {
			all();
			break;
		}
		case MSG_DECK_TOP: {
			pbuf += 6;
			all();
			break;
		}
		case MSG_SHUFFLE_SET_CARD: {
			unsigned int loc = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 8;
			all();
			if(loc == LOCATION_MZONE) {
				RefreshMzone(0, 0x181fff, 0);
				RefreshMzone(1, 0x181fff, 0);
			} else {
				RefreshSzone(0, 0x181fff, 0);
				RefreshSzone(1, 0x181fff, 0);
			}
			break;
		}
		case MSG_NEW_TURN: {
			pbuf++;
			all();
			break;
		}
		case MSG_NEW_PHASE: {
			pbuf += 2;
			all();
			refresh_field(true);
			break;
		}
		case MSG_MOVE: {
			pbufw = pbuf;
			int pc = pbuf[4];
			int pl = pbuf[5];
			int cc = pbuf[8] & 1;
			int cl = pbuf[9];
			int cs = pbuf[10];
			int cp = pbuf[11];
			pbuf += 16;
			to_cur(cc);
			bool hidden = !(cl & (LOCATION_GRAVE + LOCATION_OVERLAY)) && ((cl & (LOCATION_DECK + LOCATION_HAND)) || (cp & POS_FACEDOWN));
			bool reveal = hidden && (cl & LOCATION_HAND) && botTheater.RevealHands();
			if(reveal)	// Bot Theater: spectators see cards going into a hand
				SendBufferToSpectators();
			if (hidden)
				BufferIO::WriteInt32(pbufw, 0);
			SendToAll(offset, (int)(pbuf - offset), !reveal, cur_player[cc]);
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
			all();
			if((pp & POS_FACEDOWN) && (cp & POS_FACEUP))
				RefreshSingle(cc, cl, cs);
			break;
		}
		case MSG_SET: {
			BufferIO::WriteInt32(pbuf, 0);
			pbuf += 4;
			all();
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
			all();
			RefreshSingle(c1, l1, s1);
			RefreshSingle(c2, l2, s2);
			break;
		}
		case MSG_FIELD_DISABLED: {
			pbuf += 4;
			all();
			break;
		}
		case MSG_SUMMONING:
		case MSG_SPSUMMONING: {
			pbuf += 8;
			all();
			break;
		}
		case MSG_SUMMONED:
		case MSG_SPSUMMONED:
		case MSG_FLIPSUMMONED: {
			all();
			refresh_field(false);
			break;
		}
		case MSG_FLIPSUMMONING: {
			RefreshSingle(pbuf[4], pbuf[5], pbuf[6]);
			pbuf += 8;
			all();
			break;
		}
		case MSG_CHAINING: {
			pbuf += 16;
			all();
			break;
		}
		case MSG_CHAINED:
		case MSG_CHAIN_SOLVED: {
			pbuf++;
			all();
			refresh_field(true);
			break;
		}
		case MSG_CHAIN_END: {
			all();
			refresh_field(true);
			break;
		}
		case MSG_CHAIN_SOLVING:
		case MSG_CHAIN_NEGATED:
		case MSG_CHAIN_DISABLED: {
			pbuf++;
			all();
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
			all();
			break;
		}
		case MSG_BECOME_TARGET: {
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count * 4;
			all();
			break;
		}
		case MSG_DRAW: {
			player = BufferIO::ReadUInt8(pbuf) & 1;
			count = BufferIO::ReadUInt8(pbuf);
			pbufw = pbuf;
			pbuf += count * 4;
			to_cur(player);
			bool reveal = botTheater.RevealHands();
			if(reveal)	// Bot Theater: spectators see drawn cards
				SendBufferToSpectators();
			for (int i = 0; i < count; ++i) {
				if(!(pbufw[3] & 0x80))
					BufferIO::WriteInt32(pbufw, 0);
				else
					pbufw += 4;
			}
			SendToAll(offset, (int)(pbuf - offset), !reveal, cur_player[player]);
			break;
		}
		case MSG_DAMAGE:
		case MSG_RECOVER:
		case MSG_LPUPDATE:
		case MSG_PAY_LPCOST: {
			pbuf += 5;
			all();
			break;
		}
		case MSG_EQUIP:
		case MSG_CARD_TARGET:
		case MSG_CANCEL_TARGET:
		case MSG_ATTACK: {
			pbuf += 8;
			all();
			break;
		}
		case MSG_UNEQUIP: {
			pbuf += 4;
			all();
			break;
		}
		case MSG_ADD_COUNTER:
		case MSG_REMOVE_COUNTER: {
			pbuf += 7;
			all();
			break;
		}
		case MSG_BATTLE: {
			pbuf += 26;
			all();
			break;
		}
		case MSG_ATTACK_DISABLED: {
			all();
			break;
		}
		case MSG_DAMAGE_STEP_START:
		case MSG_DAMAGE_STEP_END: {
			all();
			RefreshMzone(0);
			RefreshMzone(1);
			break;
		}
		case MSG_MISSED_EFFECT: {
			player = pbuf[0];
			pbuf += 8;
			to_cur(player);
			break;
		}
		case MSG_TOSS_COIN:
		case MSG_TOSS_DICE: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += count;
			all();
			break;
		}
		case MSG_ROCK_PAPER_SCISSORS: {
			player = BufferIO::ReadUInt8(pbuf);
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_HAND_RES: {
			pbuf += 1;
			all();
			break;
		}
		case MSG_ANNOUNCE_RACE:
		case MSG_ANNOUNCE_ATTRIB: {
			player = BufferIO::ReadUInt8(pbuf);
			pbuf += 5;
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_ANNOUNCE_CARD:
		case MSG_ANNOUNCE_NUMBER: {
			player = BufferIO::ReadUInt8(pbuf);
			count = BufferIO::ReadUInt8(pbuf);
			pbuf += 4 * count;
			WaitforResponse(player);
			to_cur(player);
			return 1;
		}
		case MSG_CARD_HINT: {
			pbuf += 9;
			all();
			break;
		}
		case MSG_PLAYER_HINT: {
			pbuf += 6;
			all();
			break;
		}
		case MSG_TAG_SWAP: {
			player = BufferIO::ReadUInt8(pbuf) & 1;
			// The engine has handed this side over to its next member (it follows straight
			// after MSG_NEW_TURN), so they get the full message below.
			member[player] = (member[player] + 1) % MEMBERS;
			cur_player[player] = players[team_seat[player * MEMBERS + member[player]]];
			/*int mcount = */BufferIO::ReadUInt8(pbuf);
			int ecount = BufferIO::ReadUInt8(pbuf);
			/*int pcount = */BufferIO::ReadUInt8(pbuf);
			int hcount = BufferIO::ReadUInt8(pbuf);
			pbufw = pbuf + 4;
			pbuf += hcount * 4 + ecount * 4 + 4;
			to_cur(player);
			bool reveal = botTheater.RevealHands();
			if(reveal)	// Bot Theater: spectators see the incoming player's hand
				SendBufferToSpectators();
			for (int i = 0; i < hcount + ecount; ++i) {
				if(!(pbufw[3] & 0x80))
					BufferIO::WriteInt32(pbufw, 0);
				else
					pbufw += 4;
			}
			SendToAll(offset, (int)(pbuf - offset), !reveal, cur_player[player]);
			RefreshExtra(player);
			RefreshMzone(0, 0x81fff, 0);
			RefreshMzone(1, 0x81fff, 0);
			RefreshSzone(0, 0x681fff, 0);
			RefreshSzone(1, 0x681fff, 0);
			RefreshHand(0, 0x781fff, 0);
			RefreshHand(1, 0x781fff, 0);
			break;
		}
		case MSG_MATCH_KILL: {
			pbuf += 4;
			break;
		}
		default: {
			// Not a message this room knows: stop here rather than misread the rest.
			char note[64];
			snprintf(note, sizeof note, "five-a-side room: unknown message %d", (int)engType);
			mainGame->AddDebugMsg(note);
			return 0;
		}
		}
	}
	return 0;
}
void FiveDuel::GetResponse(DuelPlayer* dp, unsigned char* pdata, unsigned int len) {
	if(dp != cur_player[last_response] || dp->state != CTOS_RESPONSE)
		return;	// only the player who was asked
	unsigned char resb[SIZE_RETURN_VALUE]{};
	if (len > SIZE_RETURN_VALUE)
		len = SIZE_RETURN_VALUE;
	std::memcpy(resb, pdata, len);
	last_replay.Write<uint8_t>(len);
	last_replay.WriteData(resb, len);
	set_responseb(pduel, resb);
	dp->state = 0xff;
	Process();
}
void FiveDuel::EndDuel() {
	if(!pduel)
		return;
	last_replay.EndRecord();
	std::vector<char> replaybuf(sizeof(ReplayHeader) + last_replay.comp_size);
	std::memcpy(replaybuf.data(), &last_replay.pheader, sizeof(ReplayHeader));
	std::memcpy(replaybuf.data() + sizeof(ReplayHeader), last_replay.comp_data, last_replay.comp_size);
	if(replaybuf.size() <= MAX_DATA_SIZE) {	// too big for one packet: no replay
		NetServer::SendBufferToPlayer(nullptr, STOC_REPLAY, replaybuf.data(), replaybuf.size());
		for(int i = 0; i < SEATS; ++i)
			NetServer::ReSendToPlayer(players[i]);
		SendBufferToSpectators();
	}
	end_duel(pduel);
	event_del(etimer);
	pduel = 0;
}
void FiveDuel::WaitforResponse(int playerid) {
	playerid &= 1;
	last_response = (unsigned char)playerid;
	unsigned char msg = MSG_WAITING;
	NetServer::SendPacketToPlayer(nullptr, STOC_GAME_MSG, msg);
	for(int i = 0; i < SEATS; ++i)
		if(players[i] && players[i] != cur_player[playerid])
			NetServer::ReSendToPlayer(players[i]);
	if(cur_player[playerid])
		cur_player[playerid]->state = CTOS_RESPONSE;
}

// ---------------------------------------------------------------- what each player may see

int FiveDuel::WriteUpdateData(int player, int location, int flag, unsigned char* qbuf, int use_cache) {
	flag |= (QUERY_CODE | QUERY_POSITION);
	BufferIO::WriteInt8(qbuf, MSG_UPDATE_DATA);
	BufferIO::WriteInt8(qbuf, player);
	BufferIO::WriteInt8(qbuf, location);
	return query_field_card(pduel, player, location, flag, qbuf, use_cache);
}
// Blanks the cards the other side may not see: face-down ones (field), or all but face-up
// ones (hand).
static void HideCards(unsigned char* qbuf, int len, bool hand) {
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
// Monster and Spell & Trap Zones: the side's whole team sees everything, the other side and
// spectators see face-down cards blank.
void FiveDuel::RefreshMzone(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_MZONE, flag, query_buffer.data(), use_cache);
	SendToSide(player, query_buffer.data(), len + 3);
	HideCards(query_buffer.data() + 3, len, false);
	SendToSide(1 - player, query_buffer.data(), len + 3);
	SendBufferToSpectators();
}
void FiveDuel::RefreshSzone(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_SZONE, flag, query_buffer.data(), use_cache);
	SendToSide(player, query_buffer.data(), len + 3);
	HideCards(query_buffer.data() + 3, len, false);
	SendToSide(1 - player, query_buffer.data(), len + 3);
	SendBufferToSpectators();
}
// The hand: its player sees it, everyone else only its face-up cards (spectators see it all
// when Bot Theater's reveal_hands is on).
void FiveDuel::RefreshHand(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_HAND, flag, query_buffer.data(), use_cache);
	NetServer::SendBufferToPlayer(cur_player[player & 1], STOC_GAME_MSG, query_buffer.data(), len + 3);
	bool reveal = botTheater.RevealHands();
	if(reveal)
		SendBufferToSpectators();
	HideCards(query_buffer.data() + 3, len, true);
	SendToAll(query_buffer.data(), len + 3, !reveal, cur_player[player & 1]);
}
void FiveDuel::RefreshGrave(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_GRAVE, flag, query_buffer.data(), use_cache);
	SendToAll(query_buffer.data(), len + 3);
}
void FiveDuel::RefreshExtra(int player, int flag, int use_cache) {
	std::vector<unsigned char> query_buffer(SIZE_QUERY_BUFFER);
	int len = WriteUpdateData(player, LOCATION_EXTRA, flag, query_buffer.data(), use_cache);
	NetServer::SendBufferToPlayer(cur_player[player & 1], STOC_GAME_MSG, query_buffer.data(), len + 3);
}
void FiveDuel::RefreshSingle(int player, int location, int sequence, int flag) {
	flag |= (QUERY_CODE | QUERY_POSITION);
	unsigned char query_buffer[0x1000];
	auto qbuf = query_buffer;
	BufferIO::WriteInt8(qbuf, MSG_UPDATE_CARD);
	BufferIO::WriteInt8(qbuf, player);
	BufferIO::WriteInt8(qbuf, location);
	BufferIO::WriteInt8(qbuf, sequence);
	int len = query_card(pduel, player, location, sequence, flag, qbuf, 0);
	auto position = GetPosition(qbuf, 12);
	player &= 1;
	SendToSide(player, query_buffer, len + 4);
	if(location & LOCATION_ONFIELD) {
		if(position & POS_FACEUP) {
			SendToSide(1 - player, query_buffer, len + 4);
			SendBufferToSpectators();
		}
	} else {
		if(location == LOCATION_HAND && botTheater.RevealHands())	// Bot Theater: spectators see hand cards
			SendBufferToSpectators();
		if(location == LOCATION_REMOVED && (position & POS_FACEDOWN))
			return;
		if (location & 0x90) {
			SendToSide(1 - player, query_buffer, len + 4);
			SendBufferToSpectators();
		}
	}
}
uint32_t FiveDuel::MessageHandler(intptr_t fduel, uint32_t type) {
	if(!enable_log)
		return 0;
	char msgbuf[1024];
	get_log_message(fduel, msgbuf);
	mainGame->AddDebugMsg(msgbuf);
	return 0;
}
void FiveDuel::FiveTimer(evutil_socket_t fd, short events, void* arg) {
	// No time limit in five-a-side rooms: the timer only ends the wait after the duel (DelayEnd).
	FiveDuel* sd = static_cast<FiveDuel*>(arg);
	event_del(sd->etimer);
	if(sd->end_pending) {
		sd->end_pending = false;
		if(sd->duel_stage == DUEL_STAGE_DUELING)
			sd->DuelEndProc();
	}
}

}
