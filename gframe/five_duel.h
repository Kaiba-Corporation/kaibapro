#ifndef FIVE_DUEL_H
#define FIVE_DUEL_H

#include <set>
#include "network.h"
#include "deck_manager.h"
#include "replay.h"

namespace ygo {

// Bot Theater: a five-a-side room (MODE_FIVE). Two teams of five share the two sides of a
// tag duel; each member has one column of their side's field (the engine's DUEL_FIVE_A_SIDE).
// The room has ten seats. When the duel starts the ten players are put into two teams in a
// random order (no rock-paper-scissors): team 0 plays first, its members taking turns 1-5 in
// order, then team 1's members turns 6-10, and so on. Member m of a team has the column
// 4 - m counted from that team's own left, so the turns go clockwise round the table.
//
// Seat numbers: a player's lobby position (dp->type, and the position in the lobby messages
// and in chat) is the seat number, except that it skips 7, which means a spectator: seats
// 0-9 are positions 0-6 and 8-10 (PosOf, SeatOf).
class FiveDuel: public DuelMode {
public:
	static constexpr int MEMBERS = 5;
	static constexpr int SEATS = 10;
	static int PosOf(int seat) { return seat < 7 ? seat : seat + 1; }
	static int SeatOf(int pos) { return pos < 7 ? pos : (pos > 7 && pos <= SEATS) ? pos - 1 : -1; }

	FiveDuel();
	~FiveDuel() override;
	void Chat(DuelPlayer* dp, unsigned char* pdata, int len) override;
	void JoinGame(DuelPlayer* dp, unsigned char* pdata, bool is_creater) override;
	void LeaveGame(DuelPlayer* dp) override;
	void ToDuelist(DuelPlayer* dp) override;
	void ToObserver(DuelPlayer* dp) override;
	void PlayerReady(DuelPlayer* dp, bool ready) override;
	void PlayerKick(DuelPlayer* dp, unsigned char pos) override;
	void UpdateDeck(DuelPlayer* dp, unsigned char* pdata, int len) override;
	void StartDuel(DuelPlayer* dp) override;
	void HandResult(DuelPlayer* dp, unsigned char res) override {}
	void TPResult(DuelPlayer* dp, unsigned char tp) override {}
	void Process() override;
	void Surrender(DuelPlayer* dp) override {}	// not supported (yet) in five-a-side
	int Analyze(unsigned char* msgbuffer, unsigned int len) override;
	void GetResponse(DuelPlayer* dp, unsigned char* pdata, unsigned int len) override;
	void TimeConfirm(DuelPlayer* dp) override {}
	void EndDuel() override;

	void DuelEndProc();
	// Once the duel is over (MSG_WIN and the replay sent), the room waits this long before ending
	// (DelayEnd), so the bots' last lines, sent when they see MSG_WIN, arrive first.
	static constexpr int END_DELAY_MS = 1500;
	void DelayEnd();
	void WaitforResponse(int playerid);
	void RefreshMzone(int player, int flag = 0x881fff, int use_cache = 1);
	void RefreshSzone(int player, int flag = 0x681fff, int use_cache = 1);
	void RefreshHand(int player, int flag = 0x681fff, int use_cache = 1);
	void RefreshGrave(int player, int flag = 0x81fff, int use_cache = 1);
	void RefreshExtra(int player, int flag = 0xe81fff, int use_cache = 1);
	void RefreshSingle(int player, int location, int sequence, int flag = 0xf81fff);

	static uint32_t MessageHandler(intptr_t fduel, uint32_t type);
	static void FiveTimer(evutil_socket_t fd, short events, void* arg);

	// The MSG_START sent at the beginning of a five-a-side duel: a tag duel's MSG_START
	//   MSG_START, type (the receiver's side 0 or 1, or 0x10 for a spectator), duel rule,
	//   each side's LP (int32), each side's Deck and Extra Deck sizes (int16 each),
	// then the receiver's place in their team (member 0-4, 0xff for a spectator), and for each
	// side and member (side 0's members 0-4, then side 1's) the seat (0-9) of that player.
	static constexpr int START_MSG_SIZE = 3 + 2 * 4 + 2 * 2 * 2 + 1 + SEATS;

private:
	// Sends the message to every player in the room but `skip` (and the spectators if asked).
	void SendToAll(const unsigned char* buf, int len, bool spectators = true, DuelPlayer* skip = nullptr);
	// Sends the message to the five members of a side.
	void SendToSide(int side, const unsigned char* buf, int len);
	void SendPacketToAll(unsigned char proto);
	void SendBufferToSpectators();	// the send buffer as it is, to every spectator
	void UpdateWatchCount();
	bool OnSide(DuelPlayer* dp, int side) const;
	int WriteUpdateData(int player, int location, int flag, unsigned char* qbuf, int use_cache);

protected:
	DuelPlayer* players[SEATS];	// by seat
	std::set<DuelPlayer*> observers;
	bool ready[SEATS];
	Deck pdeck[SEATS];
	unsigned int deck_error[SEATS];
	// Once the duel has started: the seat of each side's members (side * 5 + member), the
	// member each side has playing, and that player.
	unsigned char team_seat[SEATS];
	int member[2];
	DuelPlayer* cur_player[2];
	unsigned char last_response;
	Replay last_replay;
	bool end_pending{ false };	// waiting to end the duel (DelayEnd)
};

}

#endif //FIVE_DUEL_H
