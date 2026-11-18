// Checks the four-player field grid (shared zones, Link Arrows and columns across fields).
// Build: make OUT=... geomtest; run: geomtest cards.cdb (prints PASS or the failed checks).
// (The card reader below is the same as in harness.cpp.)

#include "ocgapi.h"
#include "card_data.h"
#include "common.h"

#include <sqlite3.h>
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
#include "duel.h"
#include "field.h"
#include "card.h"
static byte* no_script(const char*, int* len) { *len = 0; return nullptr; }
static uint32_t no_message(intptr_t, uint32_t) { return 0; }
static int failures = 0;
static void check(bool ok, const char* what) {
	if (!ok) {
		printf("FAIL %s\n", what);
		++failures;
	}
}
int main(int argc, char** argv) {
	if (argc < 2 || !load_cards(argv[1])) {
		printf("usage: geomtest cards.cdb\n");
		return 2;
	}
	set_card_reader(harness_card_reader);
	set_script_reader(no_script);
	set_message_handler(no_message);
	intptr_t pd = create_duel(1);
	set_player_count(pd, 4);
	for (int p = 0; p < 4; ++p)
		set_player_info(pd, p, 8000, 0, 1);
	field* f = ((duel*)pd)->game_field;
	f->core.duel_rule = 5;
	// Link Arrow TOP from player 0's left corner (zone 0) points at player 1's zone 3.
	new_card(pd, 1482001, 0, 0, LOCATION_MZONE, 0, POS_FACEUP_ATTACK);	// Clara & Rushka (TOP)
	new_card(pd, 32864, 1, 1, LOCATION_MZONE, 3, POS_FACEUP_ATTACK);
	// LEFT and RIGHT from player 2's middle zone: player 2's zones 1 and 3.
	new_card(pd, 5329790, 2, 2, LOCATION_MZONE, 2, POS_FACEUP_ATTACK);	// Protocol Gardna (LEFT, RIGHT)
	new_card(pd, 32864, 2, 2, LOCATION_MZONE, 1, POS_FACEUP_ATTACK);
	// BOTTOM from player 3's right Extra Monster Zone (6): player 3's zone 3.
	new_card(pd, 1948619, 3, 3, LOCATION_MZONE, 6, POS_FACEUP_ATTACK);	// Xtra HERO Wonder Driver (BOTTOM, TOP)
	new_card(pd, 549481, 3, 3, LOCATION_MZONE, 3, POS_FACEUP_ATTACK);
	card* corner = f->player[0].list_mzone[0];
	card_set s;
	corner->get_linked_cards(&s);
	check(s.size() == 1 && s.count(f->player[1].list_mzone[3]), "corner TOP arrow reaches the left neighbour's zone 3");
	check(f->get_linked_zone(1) == 0x8, "player 1's zone 3 counts as linked");
	s.clear();
	f->player[2].list_mzone[2]->get_linked_cards(&s);
	check(s.size() == 1 && f->player[2].list_mzone[2]->get_linked_zone() == 0xa, "LEFT/RIGHT arrows stay in the row");
	s.clear();
	f->player[3].list_mzone[6]->get_linked_cards(&s);
	check(s.size() == 1 && s.count(f->player[3].list_mzone[3]), "Extra Monster Zone BOTTOM arrow");
	s.clear();
	corner->get_column_cards(&s);
	check(s.size() == 1 && s.count(f->player[1].list_mzone[3]) && !corner->is_all_column(), "a corner's column runs along the neighbour's row");
	check(!f->is_location_useable(1, LOCATION_MZONE, 4), "a used corner is used for both neighbours");
	check(f->is_location_useable(1, LOCATION_MZONE, 0), "the other corner is free");
	check(!f->is_location_useable(2, LOCATION_MZONE, 5), "a used Extra Monster Zone is used for both neighbours");
	end_duel(pd);
	if (!failures)
		printf("PASS\n");
	return failures ? 1 : 0;
}
