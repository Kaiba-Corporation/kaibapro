// Headless duel harness for engine tests.
// Plays duels between random legal responders, one child process per duel, and reports
// results, script errors, invalid-response retries, crashes and hangs.
//
// Usage: harness CARDS_CDB DECK_FILE [first] [count] [jobs] [-v] [-t SECONDS] [-turns N]
// Run from a folder that holds ./script (the card scripts).

#include "ocgapi.h"
#include "card_data.h"
#include "common.h"

#include <sqlite3.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include <algorithm>
#include <cstdarg>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------- cards

static std::unordered_map<uint32_t, card_data> g_cards;
static std::vector<uint32_t> g_codes;

static uint32_t harness_card_reader(uint32_t code, card_data* data) {
	auto it = g_cards.find(code);
	if (it == g_cards.end()) {
		data->clear();
		return 0;
	}
	*data = it->second;
	return 0;
}

static bool load_cards(const char* path) {
	sqlite3* db = nullptr;
	if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
		return false;
	sqlite3_stmt* st = nullptr;
	const char* sql = "select id, alias, setcode, type, atk, def, level, race, attribute from datas";
	if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
		sqlite3_close(db);
		return false;
	}
	while (sqlite3_step(st) == SQLITE_ROW) {
		card_data cd;
		cd.code = sqlite3_column_int(st, 0);
		cd.alias = sqlite3_column_int(st, 1);
		if (sqlite3_column_type(st, 2) == SQLITE_BLOB) {
			auto blob = static_cast<const unsigned char*>(sqlite3_column_blob(st, 2));
			int bytes = sqlite3_column_bytes(st, 2);
			int count = 0;
			for (int i = 0; blob && i + 1 < bytes && count < SIZE_SETCODE; i += 2) {
				uint16_t c = (uint16_t)(blob[i] | (blob[i + 1] << 8));
				if (c)
					cd.setcode[count++] = c;
			}
		} else {
			uint64_t sc = (uint64_t)sqlite3_column_int64(st, 2);
			if (sc)
				cd.set_setcode(sc);
		}
		cd.type = (uint32_t)sqlite3_column_int64(st, 3);
		cd.attack = sqlite3_column_int(st, 4);
		cd.defense = sqlite3_column_int(st, 5);
		if (cd.type & TYPE_LINK) {
			cd.link_marker = cd.defense;
			cd.defense = 0;
		}
		uint32_t level = (uint32_t)sqlite3_column_int(st, 6);
		cd.level = level & 0xff;
		cd.lscale = (level >> 24) & 0xff;
		cd.rscale = (level >> 16) & 0xff;
		cd.race = (uint32_t)sqlite3_column_int64(st, 7);
		cd.attribute = (uint32_t)sqlite3_column_int64(st, 8);
		g_cards[cd.code] = cd;
		g_codes.push_back(cd.code);
	}
	sqlite3_finalize(st);
	sqlite3_close(db);
	std::sort(g_codes.begin(), g_codes.end());
	return true;
}

// Same test the engine uses for a declared card name (copied from playerop.cpp).
static bool is_declarable(const card_data& cd, const std::vector<uint32_t>& opcode) {
	std::stack<int32_t> stack;
	for (auto it : opcode) {
		auto pop2 = [&](int32_t& l, int32_t& r) {
			r = stack.top(); stack.pop();
			l = stack.top(); stack.pop();
		};
		int32_t l, r;
		switch (it) {
		case OPCODE_ADD: if (stack.size() >= 2) { pop2(l, r); stack.push(l + r); } break;
		case OPCODE_SUB: if (stack.size() >= 2) { pop2(l, r); stack.push(l - r); } break;
		case OPCODE_MUL: if (stack.size() >= 2) { pop2(l, r); stack.push(l * r); } break;
		case OPCODE_DIV: if (stack.size() >= 2) { pop2(l, r); stack.push(r ? l / r : 0); } break;
		case OPCODE_AND: if (stack.size() >= 2) { pop2(l, r); stack.push(l && r); } break;
		case OPCODE_OR: if (stack.size() >= 2) { pop2(l, r); stack.push(l || r); } break;
		case OPCODE_NEG: if (stack.size() >= 1) { l = stack.top(); stack.pop(); stack.push(-l); } break;
		case OPCODE_NOT: if (stack.size() >= 1) { l = stack.top(); stack.pop(); stack.push(!l); } break;
		case OPCODE_ISCODE: if (stack.size() >= 1) { l = stack.top(); stack.pop(); stack.push(cd.code == (uint32_t)l); } break;
		case OPCODE_ISSETCARD: if (stack.size() >= 1) { l = stack.top(); stack.pop(); stack.push(cd.is_setcode(l)); } break;
		case OPCODE_ISTYPE: if (stack.size() >= 1) { l = stack.top(); stack.pop(); stack.push(cd.type & l); } break;
		case OPCODE_ISRACE: if (stack.size() >= 1) { l = stack.top(); stack.pop(); stack.push(cd.race & l); } break;
		case OPCODE_ISATTRIBUTE: if (stack.size() >= 1) { l = stack.top(); stack.pop(); stack.push(cd.attribute & l); } break;
		default: stack.push(it); break;
		}
	}
	if (stack.size() != 1 || stack.top() == 0)
		return false;
	return cd.code == CARD_MARINE_DOLPHIN || cd.code == CARD_TWINKLE_MOSS
		|| (!cd.alias && (cd.type & (TYPE_MONSTER | TYPE_TOKEN)) != (TYPE_MONSTER | TYPE_TOKEN));
}

// ---------------------------------------------------------------- logging from the engine

static std::vector<std::string> g_engine_log;

static uint32_t harness_message_handler(intptr_t pduel, uint32_t type) {
	char buf[1024];
	get_log_message(pduel, buf);
	g_engine_log.push_back(std::string(type == 1 ? "E " : "D ") + buf);
	return 0;
}

// ---------------------------------------------------------------- options

struct Options {
	bool verbose = false;
	int timeout = 60;
	int max_turns = 60;
	int max_responses = 40000;
	int players = 2;
	uint32_t duel_flags = 0;
	int rule = 5;
	int start_lp = 8000;
	int start_hand = 5;
	int draw_count = 1;
};
static Options g_opt;

// ---------------------------------------------------------------- a duel

struct Deck {
	std::vector<uint32_t> main, extra;
};

struct Reader {
	const uint8_t* p;
	const uint8_t* end;
	bool bad = false;
	uint8_t u8() { if (p + 1 > end) { bad = true; return 0; } return *p++; }
	uint16_t u16() { if (p + 2 > end) { bad = true; return 0; } uint16_t v; std::memcpy(&v, p, 2); p += 2; return v; }
	uint32_t u32() { if (p + 4 > end) { bad = true; return 0; } uint32_t v; std::memcpy(&v, p, 4); p += 4; return v; }
	void skip(size_t n) { if (p + n > end) { bad = true; p = end; } else p += n; }
};

struct DuelResult {
	int outcome = 0;      // 0 finished, 1 turn cap, 2 response cap, 3 stuck on retries, 4 bad message
	int winner = -1;      // player, or 2 for a draw
	int win_reason = 0;
	int turns = 0;
	int responses = 0;
	int retries = 0;
	std::string note;
};

class Duel {
public:
	Duel(uint32_t seed, const std::vector<Deck>& decks) : rng(seed), seed(seed), decks(decks) {}
	DuelResult run();

private:
	std::mt19937 rng;
	uint32_t seed;
	std::vector<Deck> decks;
	intptr_t pd = 0;
	DuelResult res;

	int turn_player = 0;
	int actions_this_turn = 0;
	int attacks_this_turn = 0;
	int chains_this_turn = 0;
	std::vector<uint8_t> last_select;
	int retry_streak = 0;
	std::map<int, int> retries_by_msg;
	std::set<uint32_t> activated;

	int rnd(int n) { return n <= 0 ? 0 : (int)(rng() % (uint32_t)n); }
	bool chance(double p) { return std::uniform_real_distribution<double>(0, 1)(rng) < p; }
	void log(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

	// Returns 0 to keep processing, 1 when the duel ended, -1 on a bad message.
	int analyze(const uint8_t* buf, int len);
	bool respond(const uint8_t* msg, size_t len);
	void respond_i(int32_t v) { set_responsei(pd, v); log("  -> %d\n", v); }
	void respond_b(const std::vector<uint8_t>& b) {
		uint8_t buf[256]{};
		std::memcpy(buf, b.data(), std::min<size_t>(b.size(), sizeof buf));
		set_responseb(pd, buf);
		if (g_opt.verbose) {
			std::fprintf(stderr, "  -> [");
			for (auto x : b) std::fprintf(stderr, " %d", x);
			std::fprintf(stderr, " ]\n");
		}
	}
	std::vector<int> pick_indices(int n, int k) {
		std::vector<int> idx(n);
		for (int i = 0; i < n; ++i) idx[i] = i;
		std::shuffle(idx.begin(), idx.end(), rng);
		idx.resize(std::min(n, k));
		return idx;
	}
	bool respond_sum(Reader& r);
	int count_bits(uint32_t v) { int c = 0; while (v) { c += v & 1; v >>= 1; } return c; }
	uint32_t pick_bits(uint32_t avail, int count) {
		std::vector<uint32_t> bits;
		for (int i = 0; i < 32; ++i) if (avail & (1u << i)) bits.push_back(1u << i);
		std::shuffle(bits.begin(), bits.end(), rng);
		uint32_t v = 0;
		for (int i = 0; i < count && i < (int)bits.size(); ++i) v |= bits[i];
		return v;
	}
};

void Duel::log(const char* fmt, ...) {
	if (!g_opt.verbose)
		return;
	va_list ap;
	va_start(ap, fmt);
	std::vfprintf(stderr, fmt, ap);
	va_end(ap);
}

DuelResult Duel::run() {
	pd = create_duel(seed);
	for (int p = 0; p < g_opt.players; ++p)
		set_player_info(pd, p, g_opt.start_lp, g_opt.start_hand, g_opt.draw_count);
	for (int p = 0; p < (int)decks.size() && p < 2; ++p) {
		for (auto it = decks[p].main.rbegin(); it != decks[p].main.rend(); ++it)
			new_card(pd, *it, p, p, LOCATION_DECK, 0, POS_FACEDOWN_DEFENSE);
		for (auto it = decks[p].extra.rbegin(); it != decks[p].extra.rend(); ++it)
			new_card(pd, *it, p, p, LOCATION_EXTRA, 0, POS_FACEDOWN_DEFENSE);
	}
	start_duel(pd, ((uint32_t)g_opt.rule << 16) | g_opt.duel_flags);
	static uint8_t buf[0x20000];
	for (;;) {
		uint32_t result = process(pd);
		int len = result & PROCESSOR_BUFFER_LEN;
		uint32_t flag = result & PROCESSOR_FLAG;
		if (len > 0) {
			if (len > (int)sizeof buf) {
				res.outcome = 4;
				res.note = "message buffer too long";
				break;
			}
			get_message(pd, buf);
			int st = analyze(buf, len);
			if (st != 0)
				break;
		}
		if (flag == PROCESSOR_END) {
			res.note = "engine ended without a winner";
			res.outcome = 4;
			break;
		}
		if (res.turns > g_opt.max_turns) {
			res.outcome = 1;
			break;
		}
		if (res.responses > g_opt.max_responses) {
			res.outcome = 2;
			break;
		}
	}
	end_duel(pd);
	return res;
}

int Duel::analyze(const uint8_t* msgbuf, int len) {
	Reader r{msgbuf, msgbuf + len};
	while (r.p < r.end) {
		const uint8_t* start = r.p;
		uint8_t type = r.u8();
		int count;
		switch (type) {
		case MSG_RETRY: {
			++res.retries;
			++retries_by_msg[last_select.empty() ? 0 : last_select[0]];
			log("RETRY (msg %d)\n", last_select.empty() ? 0 : last_select[0]);
			if (++retry_streak > 40) {
				res.outcome = 3;
				char note[128];
				std::snprintf(note, sizeof note, "stuck retrying message %d", last_select.empty() ? 0 : last_select[0]);
				res.note = note;
				return 1;
			}
			if (last_select.empty() || !respond(last_select.data(), last_select.size())) {
				res.outcome = 4;
				res.note = "retry with nothing to answer";
				return -1;
			}
			return 0;
		}
		case MSG_HINT: r.skip(6); break;
		case MSG_WIN: {
			res.winner = r.u8();
			res.win_reason = r.u8();
			res.outcome = 0;
			log("WIN player %d reason %d\n", res.winner, res.win_reason);
			return 1;
		}
		case MSG_SELECT_BATTLECMD:
		case MSG_SELECT_IDLECMD:
		case MSG_SELECT_EFFECTYN:
		case MSG_SELECT_YESNO:
		case MSG_SELECT_OPTION:
		case MSG_SELECT_CARD:
		case MSG_SELECT_CHAIN:
		case MSG_SELECT_PLACE:
		case MSG_SELECT_DISFIELD:
		case MSG_SELECT_POSITION:
		case MSG_SELECT_TRIBUTE:
		case MSG_SELECT_COUNTER:
		case MSG_SELECT_SUM:
		case MSG_SORT_CARD:
		case MSG_SELECT_UNSELECT_CARD:
		case MSG_ROCK_PAPER_SCISSORS:
		case MSG_ANNOUNCE_RACE:
		case MSG_ANNOUNCE_ATTRIB:
		case MSG_ANNOUNCE_CARD:
		case MSG_ANNOUNCE_NUMBER: {
			// A selection is always the last message before the engine waits.
			last_select.assign(start, msgbuf + len);
			retry_streak = 0;
			++res.responses;
			if (!respond(last_select.data(), last_select.size())) {
				char note[96];
				std::snprintf(note, sizeof note, "could not read selection message %d", type);
				res.note = note;
				res.outcome = 4;
				return -1;
			}
			return 0;
		}
		case MSG_CONFIRM_DECKTOP:
		case MSG_CONFIRM_CARDS:
		case MSG_CONFIRM_EXTRATOP:
			r.u8(); count = r.u8(); r.skip(count * 7); break;
		case MSG_SHUFFLE_DECK: r.skip(1); break;
		case MSG_SHUFFLE_HAND:
		case MSG_SHUFFLE_EXTRA:
			r.u8(); count = r.u8(); r.skip(count * 4); break;
		case MSG_SWAP_GRAVE_DECK: r.skip(1); break;
		case MSG_SHUFFLE_SET_CARD: r.u8(); count = r.u8(); r.skip(count * 8); break;
		case MSG_REVERSE_DECK: break;
		case MSG_DECK_TOP: r.skip(6); break;
		case MSG_NEW_TURN: {
			turn_player = r.u8();
			++res.turns;
			actions_this_turn = attacks_this_turn = chains_this_turn = 0;
			log("=== TURN %d (player %d)\n", res.turns, turn_player);
			break;
		}
		case MSG_NEW_PHASE: { uint16_t ph = r.u16(); log("phase %x\n", ph); break; }
		case MSG_MOVE: r.skip(16); break;
		case MSG_POS_CHANGE: r.skip(9); break;
		case MSG_SET: r.skip(8); break;
		case MSG_SWAP: r.skip(16); break;
		case MSG_FIELD_DISABLED: r.skip(4); break;
		case MSG_SUMMONING: case MSG_SPSUMMONING: case MSG_FLIPSUMMONING: r.skip(8); break;
		case MSG_SUMMONED: case MSG_SPSUMMONED: case MSG_FLIPSUMMONED: break;
		case MSG_CHAINING: {
			uint32_t code = r.u32();
			r.skip(12);
			activated.insert(code);
			log("chaining %u\n", code);
			break;
		}
		case MSG_CHAINED: case MSG_CHAIN_SOLVING: case MSG_CHAIN_SOLVED:
		case MSG_CHAIN_NEGATED: case MSG_CHAIN_DISABLED:
			r.skip(1); break;
		case MSG_CHAIN_END: break;
		case MSG_RANDOM_SELECTED: r.u8(); count = r.u8(); r.skip(count * 4); break;
		case MSG_BECOME_TARGET: count = r.u8(); r.skip(count * 4); break;
		case MSG_DRAW: r.u8(); count = r.u8(); r.skip(count * 4); break;
		case MSG_DAMAGE: case MSG_RECOVER: case MSG_LPUPDATE: case MSG_PAY_LPCOST: r.skip(5); break;
		case MSG_EQUIP: case MSG_CARD_TARGET: case MSG_CANCEL_TARGET: r.skip(8); break;
		case MSG_ADD_COUNTER: case MSG_REMOVE_COUNTER: r.skip(7); break;
		case MSG_ATTACK: r.skip(8); break;
		case MSG_BATTLE: r.skip(26); break;
		case MSG_ATTACK_DISABLED: case MSG_DAMAGE_STEP_START: case MSG_DAMAGE_STEP_END: break;
		case MSG_MISSED_EFFECT: r.skip(8); break;
		case MSG_TOSS_COIN: case MSG_TOSS_DICE: r.u8(); count = r.u8(); r.skip(count); break;
		case MSG_HAND_RES: r.skip(1); break;
		case MSG_CARD_HINT: r.skip(9); break;
		case MSG_PLAYER_HINT: r.skip(6); break;
		case MSG_MATCH_KILL: r.skip(4); break;
		case MSG_TAG_SWAP: {
			r.u8();
			r.u8();
			int ecount = r.u8();
			r.u8();
			int hcount = r.u8();
			r.skip(4 + hcount * 4 + ecount * 4);
			break;
		}
		default: {
			char note[64];
			std::snprintf(note, sizeof note, "unknown message %d", type);
			res.note = note;
			res.outcome = 4;
			return -1;
		}
		}
		if (r.bad) {
			char note[64];
			std::snprintf(note, sizeof note, "message %d ran past the buffer", type);
			res.note = note;
			res.outcome = 4;
			return -1;
		}
	}
	return 0;
}

bool Duel::respond(const uint8_t* msg, size_t len) {
	Reader r{msg, msg + len};
	uint8_t type = r.u8();
	int player = r.u8();
	log("select %d (player %d)\n", type, player);
	switch (type) {
	case MSG_SELECT_BATTLECMD: {
		int nchain = r.u8();
		r.skip(nchain * 11);
		int natk = r.u8();
		r.skip(natk * 8);
		int can_m2 = r.u8();
		int can_ep = r.u8();
		if (r.bad) return false;
		if (nchain && chance(0.08) && chains_this_turn < 40) {
			++chains_this_turn;
			respond_i((rnd(nchain) << 16) | 0);
		} else if (natk && attacks_this_turn < 12 && chance(0.8)) {
			++attacks_this_turn;
			respond_i((rnd(natk) << 16) | 1);
		} else if (can_m2 && chance(0.6)) {
			respond_i(2);
		} else if (can_ep) {
			respond_i(3);
		} else if (can_m2) {
			respond_i(2);
		} else if (natk) {
			respond_i((rnd(natk) << 16) | 1);
		} else {
			respond_i((rnd(nchain) << 16) | 0);
		}
		return true;
	}
	case MSG_SELECT_IDLECMD: {
		int n[6];
		for (int i = 0; i < 5; ++i) {
			n[i] = r.u8();
			r.skip(n[i] * 7);
		}
		n[5] = r.u8();
		r.skip(n[5] * 11);
		int can_bp = r.u8();
		int can_ep = r.u8();
		r.u8();
		if (r.bad) return false;
		// type: 0 summon, 1 special summon, 2 change position, 3 set monster, 4 set spell/trap, 5 activate
		static const double weight[6] = {3, 3, 0.3, 1, 2, 3};
		double total = 0;
		if (actions_this_turn < 40)
			for (int i = 0; i < 6; ++i) total += weight[i] * n[i];
		double stop = (can_bp || can_ep) ? 2.0 + actions_this_turn * 0.5 : 0;
		double x = std::uniform_real_distribution<double>(0, total + stop)(rng);
		if (total > 0 && x < total) {
			for (int i = 0; i < 6; ++i) {
				double w = weight[i] * n[i];
				if (x < w || i == 5) {
					if (n[i] == 0) continue;
					++actions_this_turn;
					respond_i((rnd(n[i]) << 16) | i);
					return true;
				}
				x -= w;
			}
		}
		if (can_bp && chance(0.85))
			respond_i(6);
		else if (can_ep)
			respond_i(7);
		else if (can_bp)
			respond_i(6);
		else {
			for (int i = 0; i < 6; ++i)
				if (n[i]) {
					respond_i((rnd(n[i]) << 16) | i);
					return true;
				}
			return false;
		}
		return true;
	}
	case MSG_SELECT_EFFECTYN:
		respond_i(chance(0.75) ? 1 : 0);
		return true;
	case MSG_SELECT_YESNO:
		respond_i(chance(0.6) ? 1 : 0);
		return true;
	case MSG_SELECT_OPTION: {
		int c = r.u8();
		if (r.bad || c == 0) return false;
		respond_i(rnd(c));
		return true;
	}
	case MSG_SELECT_CARD: {
		int cancelable = r.u8();
		int mn = r.u8();
		int mx = r.u8();
		int c = r.u8();
		if (g_opt.verbose) {
			const uint8_t* q = r.p;
			for (int i = 0; i < c && q + 8 <= r.end; ++i, q += 8) {
				uint32_t code; std::memcpy(&code, q, 4);
				std::fprintf(stderr, "    card %u (p%d loc %d seq %d)\n", code, q[4], q[5], q[6]);
			}
		}
		r.skip(c * 8);
		if (r.bad) return false;
		if (cancelable && (chance(0.05) || c == 0)) {
			respond_i(-1);
			return true;
		}
		int k = (mx > mn && chance(0.5)) ? mn + rnd(mx - mn + 1) : mn;
		auto idx = pick_indices(c, k);
		std::vector<uint8_t> b{(uint8_t)idx.size()};
		for (int i : idx) b.push_back((uint8_t)i);
		respond_b(b);
		return true;
	}
	case MSG_SELECT_UNSELECT_CARD: {
		int finishable = r.u8();
		int cancelable = r.u8();
		r.u8();
		r.u8();
		int c1 = r.u8();
		r.skip(c1 * 8);
		int c2 = r.u8();
		r.skip(c2 * 8);
		if (r.bad) return false;
		if ((finishable || cancelable) && (c1 == 0 || (finishable && chance(0.3)) || (!finishable && chance(0.05)))) {
			respond_i(-1);
			return true;
		}
		int pick = c1 ? rnd(c1) : c1 + rnd(c2);
		if (c1 + c2 == 0) return false;
		respond_b({1, (uint8_t)pick});
		return true;
	}
	case MSG_SELECT_CHAIN: {
		int c = r.u8();
		r.u8();
		int forced = r.u8();
		r.skip(8 + c * 13);
		if (r.bad) return false;
		if (c && (forced || (chains_this_turn < 60 && chance(0.35)))) {
			++chains_this_turn;
			respond_i(rnd(c));
		} else
			respond_i(-1);
		return true;
	}
	case MSG_SELECT_PLACE:
	case MSG_SELECT_DISFIELD: {
		int c = r.u8();
		uint32_t flag = r.u32();
		if (r.bad) return false;
		uint32_t usable = ~flag & 0x7f7f7f7f;   // monster zones 0-6 and spell/trap zones 0-7, both sides
		std::vector<int> bits;
		for (int i = 0; i < 32; ++i) if (usable & (1u << i)) bits.push_back(i);
		std::shuffle(bits.begin(), bits.end(), rng);
		int want = std::max(1, c);
		std::vector<uint8_t> b;
		for (int i = 0; i < want; ++i) {
			int bit = bits.empty() ? 0 : bits[i % bits.size()];
			int p = (bit >= 16) ? 1 - player : player;
			int rel = bit % 16;
			int loc = rel >= 8 ? LOCATION_SZONE : LOCATION_MZONE;
			b.push_back((uint8_t)p);
			b.push_back((uint8_t)loc);
			b.push_back((uint8_t)(rel % 8));
		}
		respond_b(b);
		return true;
	}
	case MSG_SELECT_POSITION: {
		r.u32();
		int pos = r.u8();
		if (r.bad) return false;
		respond_i((int)pick_bits(pos & 0xf, 1));
		return true;
	}
	case MSG_SELECT_TRIBUTE: {
		int cancelable = r.u8();
		int mn = r.u8();
		int mx = r.u8();
		int c = r.u8();
		std::vector<int> param(c);
		for (int i = 0; i < c; ++i) {
			r.skip(7);
			param[i] = r.u8();
		}
		if (r.bad) return false;
		if (cancelable && chance(0.05)) {
			respond_i(-1);
			return true;
		}
		for (int attempt = 0; attempt < 20; ++attempt) {
			auto idx = pick_indices(c, c);
			std::vector<uint8_t> b{0};
			int sum = 0;
			for (int i : idx) {
				if (sum >= mn) break;
				b.push_back((uint8_t)i);
				sum += param[i];
			}
			b[0] = (uint8_t)(b.size() - 1);
			if (sum >= mn && b[0] <= mx) {
				respond_b(b);
				return true;
			}
		}
		if (cancelable) { respond_i(-1); return true; }
		respond_b({0});
		return true;
	}
	case MSG_SELECT_COUNTER: {
		r.u16();
		int need = r.u16();
		int c = r.u8();
		std::vector<int> have(c);
		for (int i = 0; i < c; ++i) {
			r.skip(7);
			have[i] = r.u16();
		}
		if (r.bad) return false;
		std::vector<int> take(c, 0);
		int left = need;
		for (int guard = 0; left > 0 && guard < 10000; ++guard) {
			int i = rnd(c);
			if (take[i] < have[i]) { ++take[i]; --left; }
		}
		std::vector<uint8_t> b;
		for (int i = 0; i < c; ++i) {
			b.push_back((uint8_t)(take[i] & 0xff));
			b.push_back((uint8_t)(take[i] >> 8));
		}
		respond_b(b);
		return true;
	}
	case MSG_SELECT_SUM:
		r.p = msg + 1;
		return respond_sum(r);
	case MSG_SORT_CARD:
		respond_i(-1);
		return true;
	case MSG_ROCK_PAPER_SCISSORS:
		respond_i(1 + rnd(3));
		return true;
	case MSG_ANNOUNCE_RACE:
	case MSG_ANNOUNCE_ATTRIB: {
		int c = r.u8();
		uint32_t avail = r.u32();
		if (r.bad) return false;
		avail &= type == MSG_ANNOUNCE_ATTRIB ? 0x7fu : ((1u << RACES_COUNT) - 1);
		respond_i((int)pick_bits(avail, c));
		return true;
	}
	case MSG_ANNOUNCE_CARD: {
		int c = r.u8();
		std::vector<uint32_t> ops(c);
		for (int i = 0; i < c; ++i) ops[i] = r.u32();
		if (r.bad) return false;
		std::vector<uint32_t> ok;
		for (auto code : g_codes)
			if (is_declarable(g_cards[code], ops)) ok.push_back(code);
		if (ok.empty()) {
			res.note = "no card can be declared";
			respond_i((int)g_codes[rnd((int)g_codes.size())]);
			return true;
		}
		respond_i((int)ok[rnd((int)ok.size())]);
		return true;
	}
	case MSG_ANNOUNCE_NUMBER: {
		int c = r.u8();
		if (r.bad || c == 0) return false;
		respond_i(rnd(c));
		return true;
	}
	}
	return false;
}

// Select cards whose levels (or other values) add up to a target, as the engine checks it.
bool Duel::respond_sum(Reader& r) {
	int mode = r.u8();   // 0: exactly the target, 1: at least the target (rituals)
	r.u8();
	int32_t acc = (int32_t)r.u32();
	int mn = r.u8();
	int mx = r.u8();
	int mc = r.u8();
	std::vector<uint32_t> must(mc);
	for (int i = 0; i < mc; ++i) { r.skip(7); must[i] = r.u32(); }
	int c = r.u8();
	std::vector<uint32_t> param(c);
	for (int i = 0; i < c; ++i) { r.skip(7); param[i] = r.u32(); }
	if (r.bad) return false;

	auto exact_ok = [&](const std::vector<uint32_t>& ops) {
		// Same as select_sum_check1 in the engine.
		std::function<bool(int, int32_t, int32_t)> chk = [&](int index, int32_t a, int32_t opmin) -> bool {
			int size = (int)ops.size();
			if (a == 0 || index == size) return false;
			int32_t o1 = ops[index] & 0xffff;
			int32_t o2 = ops[index] >> 16;
			if (index == size - 1)
				return (a == o1 && a + opmin > o1) || (o2 && a == o2 && a + opmin > o2);
			return (a > o1 && chk(index + 1, a - o1, std::min(o1, opmin)))
				|| (o2 > 0 && a > o2 && chk(index + 1, a - o2, std::min(o2, opmin)));
		};
		return chk(0, acc, 0xffff);
	};
	auto atleast_ok = [&](const std::vector<uint32_t>& ops) {
		int32_t sum = 0, most = 0, least = 0x7fffffff;
		for (auto op : ops) {
			int32_t o1 = op & 0xffff, o2 = op >> 16;
			int32_t ms = (o2 && o2 < o1) ? o2 : o1;
			sum += ms;
			most += std::max(o1, o2);
			least = std::min(least, ms);
		}
		return !ops.empty() && most >= acc && sum - least < acc;
	};
	int lo = mode == 0 ? mn : 0, hi = mode == 0 ? mx : c;
	for (int attempt = 0; attempt < 400; ++attempt) {
		auto order = pick_indices(c, c);
		std::vector<uint32_t> ops(must);
		std::vector<int> chosen;
		for (int i : order) {
			if ((int)chosen.size() >= hi) break;
			ops.push_back(param[i]);
			chosen.push_back(i);
			bool good = (int)chosen.size() >= lo && (mode == 0 ? exact_ok(ops) : atleast_ok(ops));
			if (good) {
				std::vector<uint8_t> b{(uint8_t)(mc + chosen.size())};
				for (int k = 0; k < mc; ++k) b.push_back(0);
				for (int k : chosen) b.push_back((uint8_t)k);
				respond_b(b);
				return true;
			}
		}
	}
	// Nothing found by random tries: send an empty answer and let the engine say retry.
	respond_b({(uint8_t)mc});
	return true;
}

// ---------------------------------------------------------------- driver

static bool parse_deck_line(const std::string& line, std::vector<Deck>& decks) {
	decks.clear();
	size_t start = 0;
	while (start <= line.size()) {
		size_t bar = line.find('|', start);
		std::string part = line.substr(start, bar == std::string::npos ? std::string::npos : bar - start);
		Deck d;
		size_t semi = part.find(';');
		auto read = [](const std::string& s, std::vector<uint32_t>& out) {
			char* p = const_cast<char*>(s.c_str());
			while (*p) {
				char* e;
				unsigned long v = std::strtoul(p, &e, 10);
				if (e == p) { ++p; continue; }
				out.push_back((uint32_t)v);
				p = e;
			}
		};
		read(part.substr(0, semi), d.main);
		if (semi != std::string::npos)
			read(part.substr(semi + 1), d.extra);
		decks.push_back(d);
		if (bar == std::string::npos) break;
		start = bar + 1;
	}
	return !decks.empty();
}

static void run_child(int id, uint32_t seed, const std::vector<Deck>& decks, const std::string& path) {
	alarm(g_opt.timeout);
	set_card_reader(harness_card_reader);
	set_message_handler(harness_message_handler);
	Duel duel(seed, decks);
	DuelResult res = duel.run();
	std::string out;
	char line[512];
	std::snprintf(line, sizeof line, "R %d %u %d %d %d %d %d %d %s\n", id, seed, res.outcome, res.winner, res.win_reason,
	              res.turns, res.responses, res.retries, res.note.c_str());
	out += line;
	for (auto& l : g_engine_log) {
		std::string one = l;
		for (auto& ch : one) if (ch == '\n') ch = ' ';
		out += one + "\n";
	}
	FILE* fp = std::fopen(path.c_str(), "w");
	if (fp) {
		std::fputs(out.c_str(), fp);
		std::fclose(fp);
	}
	_exit(0);
}

int main(int argc, char** argv) {
	if (argc < 3) {
		std::fprintf(stderr, "usage: harness CARDS_CDB DECK_FILE [first] [count] [jobs] [-v] [-t S] [-turns N]\n");
		return 1;
	}
	std::vector<std::string> pos;
	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		if (a == "-v") g_opt.verbose = true;
		else if (a == "-t" && i + 1 < argc) g_opt.timeout = std::atoi(argv[++i]);
		else if (a == "-turns" && i + 1 < argc) g_opt.max_turns = std::atoi(argv[++i]);
		else pos.push_back(a);
	}
	if (!load_cards(pos[0].c_str())) {
		std::fprintf(stderr, "can't read %s\n", pos[0].c_str());
		return 1;
	}
	std::vector<std::string> lines;
	{
		FILE* fp = std::fopen(pos[1].c_str(), "r");
		if (!fp) { std::fprintf(stderr, "can't read %s\n", pos[1].c_str()); return 1; }
		static char buf[1 << 16];
		while (std::fgets(buf, sizeof buf, fp)) lines.push_back(buf);
		std::fclose(fp);
	}
	int first = pos.size() > 2 ? std::atoi(pos[2].c_str()) : 0;
	int count = pos.size() > 3 ? std::atoi(pos[3].c_str()) : (int)lines.size() - first;
	int jobs = pos.size() > 4 ? std::atoi(pos[4].c_str()) : 1;
	if (g_opt.verbose) jobs = 1;

	struct Running { pid_t pid; int id; std::string path; };
	std::vector<Running> running;
	int next = first, last = std::min((int)lines.size(), first + count);
	while (next < last || !running.empty()) {
		while (next < last && (int)running.size() < jobs) {
			std::vector<Deck> decks;
			parse_deck_line(lines[next], decks);
			uint32_t seed = 1000003u * (uint32_t)next + 17u;
			std::string path = "res/d_" + std::to_string(getpid()) + "_" + std::to_string(next) + ".txt";
			std::fflush(stdout);
			pid_t pid = fork();
			if (pid == 0)
				run_child(next, seed, decks, path);
			running.push_back({pid, next, path});
			++next;
		}
		// Wait for any child, reading its output.
		int status = 0;
		pid_t done = waitpid(-1, &status, 0);
		for (size_t i = 0; i < running.size(); ++i) {
			if (running[i].pid != done) continue;
			if (WIFSIGNALED(status)) {
				int sig = WTERMSIG(status);
				std::printf("R %d - %s %d\n", running[i].id, sig == SIGALRM ? "HANG" : "CRASH", sig);
			} else if (FILE* fp = std::fopen(running[i].path.c_str(), "r")) {
				char buf[4096];
				size_t n;
				while ((n = std::fread(buf, 1, sizeof buf, fp)) > 0) std::fwrite(buf, 1, n, stdout);
				std::fclose(fp);
			}
			std::remove(running[i].path.c_str());
			std::fflush(stdout);
			running.erase(running.begin() + i);
			break;
		}
	}
	return 0;
}
