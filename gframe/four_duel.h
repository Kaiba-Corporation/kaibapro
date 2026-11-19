#ifndef FOUR_DUEL_H
#define FOUR_DUEL_H

#include <set>
#include "network.h"
#include "deck_manager.h"
#include "replay.h"

namespace ygo {

// Bot Theater: a four-player free-for-all room (MODE_FOUR). Four seats as in a tag room;
// when the duel starts the four players are seated in a random order round the table
// (seat 0 goes first, then clockwise), so there is no rock-paper-scissors. Each player
// is sent the duel as they may see it; spectators see no hidden cards (except hands, when
// Bot Theater's reveal_hands is on).
class FourDuel: public DuelMode {
public:
	FourDuel();
	~FourDuel() override;
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
	void Surrender(DuelPlayer* dp) override;
	int Analyze(unsigned char* msgbuffer, unsigned int len) override;
	void GetResponse(DuelPlayer* dp, unsigned char* pdata, unsigned int len) override;
	void TimeConfirm(DuelPlayer* dp) override {}
	void EndDuel() override;

	void DuelEndProc();
	void AfterAnalyze(int stop);
	void ResumeHeld();
	void WaitforResponse(int seat);
	void RefreshMzone(int player, int flag = 0x881fff, int use_cache = 1);
	void RefreshSzone(int player, int flag = 0x681fff, int use_cache = 1);
	void RefreshHand(int player, int flag = 0x681fff, int use_cache = 1);
	void RefreshGrave(int player, int flag = 0x81fff, int use_cache = 1);
	void RefreshExtra(int player, int flag = 0xe81fff, int use_cache = 1);
	void RefreshSingle(int player, int location, int sequence, int flag = 0xf81fff);
	void RefreshField();	// every player's Monster Zones, Spell & Trap Zones and hand

	static uint32_t MessageHandler(intptr_t fduel, uint32_t type);
	static void FourTimer(evutil_socket_t fd, short events, void* arg);

	// The MSG_START sent at the beginning of a four-player duel:
	//   MSG_START, type (the receiver's seat 0-3, or 0x10 for a spectator), duel rule,
	//   for each seat the lobby position (0-3) of the player sitting there,
	//   for each seat its LP (int32), then for each seat its Deck and Extra Deck sizes (int16 each).
	static constexpr int START_MSG_SIZE = 3 + 4 + 4 * 4 + 4 * 2 * 2;

private:
	static constexpr unsigned ALL_SEATS = 0xf;
	static unsigned Seat(int player) { return (player >= 0 && player < 4) ? (1u << player) : 0u; }
	static unsigned OtherSeats(int player) { return ALL_SEATS & ~Seat(player); }
	// Sends a game message to the seats in the mask, and to the spectators if asked.
	void Send(unsigned seats, bool spectators, const unsigned char* buf, int len);
	void SendToAll(const unsigned char* buf, int len) { Send(ALL_SEATS, true, buf, len); }
	void SendPacketToAll(unsigned char proto);
	int WriteUpdateData(int player, int location, int flag, unsigned char* qbuf, int use_cache);
	void UpdateWatchCount();

protected:
	DuelPlayer* players[4];	// by lobby position until the duel starts, then by seat
	std::set<DuelPlayer*> observers;
	bool ready[4];
	Deck pdeck[4];
	unsigned int deck_error[4];
	unsigned char last_response;
	Replay last_replay;
	// The duel pauses briefly after a player is knocked out, so the bots' lines about it reach
	// everyone before the duel goes on, and the end of the duel waits for the winner's line.
	static constexpr int PAUSE_AFTER_OUT_MS = 1200, END_DELAY_MS = 1500;
	std::vector<unsigned char> held;	// messages not yet sent when the duel paused
	bool pause_pending;
	bool end_pending;
};

}

#endif //FOUR_DUEL_H
