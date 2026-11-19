#include <algorithm>
#include <cmath>
#include <stack>
#include "client_field.h"
#include "client_card.h"
#include "duelclient.h"
#include "data_manager.h"
#include "image_manager.h"
#include "game.h"
#include "materials.h"
#include "bot_theater.h"
#include "../ocgcore/common.h"

namespace ygo {

ClientField::ClientField() {
	for(int p = 0; p < 4; ++p) {
		mzone[p].resize(7, 0);
		szone[p].resize(8, 0);
	}
	rnd.reset((uint_fast32_t)std::random_device()());
}
ClientField::~ClientField() {
	for (int i = 0; i < 4; ++i) {
		for (auto& card : deck[i]) {
			delete card;
		}
		deck[i].clear();
		for (auto& card : hand[i]) {
			delete card;
		}
		hand[i].clear();
		for (auto& card : mzone[i]) {
			if (card)
				delete card;
			card = nullptr;
		}
		for (auto& card : szone[i]) {
			if (card)
				delete card;
			card = nullptr;
		}
		for (auto& card : grave[i]) {
			delete card;
		}
		grave[i].clear();
		for (auto& card : remove[i]) {
			delete card;
		}
		remove[i].clear();

		for (auto& card : extra[i]) {
			delete card;
		}
		extra[i].clear();
	}
	for (auto& card : overlay_cards) {
		delete card;
	}
	overlay_cards.clear();
}
void ClientField::Clear() {
	for(int i = 0; i < 4; ++i) {
		for(auto cit = deck[i].begin(); cit != deck[i].end(); ++cit)
			delete *cit;
		deck[i].clear();
		for(auto cit = hand[i].begin(); cit != hand[i].end(); ++cit)
			delete *cit;
		hand[i].clear();
		for(auto cit = mzone[i].begin(); cit != mzone[i].end(); ++cit) {
			if(*cit)
				delete *cit;
			*cit = 0;
		}
		for(auto cit = szone[i].begin(); cit != szone[i].end(); ++cit) {
			if(*cit)
				delete *cit;
			*cit = 0;
		}
		for(auto cit = grave[i].begin(); cit != grave[i].end(); ++cit)
			delete *cit;
		grave[i].clear();
		for(auto cit = remove[i].begin(); cit != remove[i].end(); ++cit)
			delete *cit;
		remove[i].clear();
		for(auto cit = extra[i].begin(); cit != extra[i].end(); ++cit)
			delete *cit;
		extra[i].clear();
		deck_act[i] = false;
		grave_act[i] = false;
		remove_act[i] = false;
		extra_act[i] = false;
		pzone_act[i] = false;
	}
	for(auto sit = overlay_cards.begin(); sit != overlay_cards.end(); ++sit)
		delete *sit;
	overlay_cards.clear();
	extra_p_count[0] = 0;
	extra_p_count[1] = 0;
	player_desc_hints[0].clear();
	player_desc_hints[1].clear();
	chains.clear();
	activatable_cards.clear();
	summonable_cards.clear();
	spsummonable_cards.clear();
	msetable_cards.clear();
	ssetable_cards.clear();
	reposable_cards.clear();
	attackable_cards.clear();
	disabled_field = 0;
	panel = 0;
	hovered_card = 0;
	clicked_card = 0;
	highlighting_card = 0;
	menu_card = 0;
	hovered_controler = 0;
	hovered_location = 0;
	hovered_sequence = 0;
	conti_act = false;
	deck_reversed = false;
	cant_check_grave = false;
	tag_surrender = false;
	tag_teammate_surrender = false;
}
void ClientField::Initial(int player, int deckc, int extrac) {
	ClientCard* pcard;
	for(int i = 0; i < deckc; ++i) {
		pcard = new ClientCard;
		deck[player].push_back(pcard);
		pcard->owner = player;
		pcard->controler = player;
		pcard->location = LOCATION_DECK;
		pcard->sequence = i;
		pcard->position = POS_FACEDOWN_DEFENSE;
		GetCardLocation(pcard, &pcard->curPos, &pcard->curRot, true);
	}
	for(int i = 0; i < extrac; ++i) {
		pcard = new ClientCard;
		extra[player].push_back(pcard);
		pcard->owner = player;
		pcard->controler = player;
		pcard->location = LOCATION_EXTRA;
		pcard->sequence = i;
		pcard->position = POS_FACEDOWN_DEFENSE;
		GetCardLocation(pcard, &pcard->curPos, &pcard->curRot, true);
	}
}
void ClientField::ResetSequence(std::vector<ClientCard*>& list, bool reset_height) {
	unsigned char seq = 0;
	for (auto& pcard : list) {
		pcard->sequence = seq++;
		if (reset_height) {
			pcard->curPos.Z = 0.01f + 0.01f * pcard->sequence;
			pcard->mTransform.setTranslation(pcard->curPos);
		}
	}
}
ClientCard* ClientField::GetCard(int controler, int location, int sequence, int sub_seq) {
	std::vector<ClientCard*>* lst = 0;
	bool is_xyz = (location & LOCATION_OVERLAY) != 0;
	location &= 0x7f;
	switch(location) {
	case LOCATION_DECK:
		lst = &deck[controler];
		break;
	case LOCATION_HAND:
		lst = &hand[controler];
		break;
	case LOCATION_MZONE:
		lst = &mzone[controler];
		break;
	case LOCATION_SZONE:
		lst = &szone[controler];
		break;
	case LOCATION_GRAVE:
		lst = &grave[controler];
		break;
	case LOCATION_REMOVED:
		lst = &remove[controler];
		break;
	case LOCATION_EXTRA:
		lst = &extra[controler];
		break;
	}
	if(!lst)
		return 0;
	if(is_xyz) {
		if(sequence >= (int)lst->size())
			return 0;
		ClientCard* scard = (*lst)[sequence];
		if(scard && (int)scard->overlayed.size() > sub_seq)
			return scard->overlayed[sub_seq];
		else
			return 0;
	} else {
		if(sequence >= (int)lst->size())
			return 0;
		return (*lst)[sequence];
	}
}
void ClientField::AddCard(ClientCard* pcard, int controler, int location, int sequence) {
	pcard->controler = controler;
	pcard->location = location;
	pcard->sequence = sequence;
	switch(location) {
	case LOCATION_DECK: {
		if (sequence != 0 || deck[controler].size() == 0) {
			deck[controler].push_back(pcard);
		} else {
			deck[controler].insert(deck[controler].begin(), pcard);
		}
		ResetSequence(deck[controler], true);
		pcard->is_reversed = false;
		pcard->ClearData();
		pcard->ClearTarget();
		SetShowMark(pcard, false);
		break;
	}
	case LOCATION_HAND: {
		hand[controler].push_back(pcard);
		ResetSequence(hand[controler], false);
		break;
	}
	case LOCATION_MZONE: {
		mzone[controler][sequence] = pcard;
		break;
	}
	case LOCATION_SZONE: {
		szone[controler][sequence] = pcard;
		break;
	}
	case LOCATION_GRAVE: {
		grave[controler].push_back(pcard);
		ResetSequence(grave[controler], false);
		break;
	}
	case LOCATION_REMOVED: {
		remove[controler].push_back(pcard);
		ResetSequence(remove[controler], false);
		break;
	}
	case LOCATION_EXTRA: {
		if(extra_p_count[controler] == 0 || (pcard->position & POS_FACEUP)) {
			extra[controler].push_back(pcard);
		} else {
			size_t faceup_begin = extra[controler].size() - extra_p_count[controler];
			extra[controler].insert(extra[controler].begin() + faceup_begin, pcard);
		}
		ResetSequence(extra[controler], true);
		if (pcard->position & POS_FACEUP)
			extra_p_count[controler]++;
		break;
	}
	}
}
ClientCard* ClientField::RemoveCard(int controler, int location, int sequence) {
	ClientCard* pcard = nullptr;
	switch (location) {
	case LOCATION_DECK: {
		pcard = deck[controler][sequence];
		deck[controler].erase(deck[controler].begin() + sequence);
		ResetSequence(deck[controler], true);
		break;
	}
	case LOCATION_HAND: {
		pcard = hand[controler][sequence];
		hand[controler].erase(hand[controler].begin() + sequence);
		ResetSequence(hand[controler], false);
		break;
	}
	case LOCATION_MZONE: {
		pcard = mzone[controler][sequence];
		mzone[controler][sequence] = nullptr;
		break;
	}
	case LOCATION_SZONE: {
		pcard = szone[controler][sequence];
		szone[controler][sequence] = nullptr;
		break;
	}
	case LOCATION_GRAVE: {
		pcard = grave[controler][sequence];
		grave[controler].erase(grave[controler].begin() + sequence);
		ResetSequence(grave[controler], true);
		break;
	}
	case LOCATION_REMOVED: {
		pcard = remove[controler][sequence];
		remove[controler].erase(remove[controler].begin() + sequence);
		ResetSequence(remove[controler], true);
		break;
	}
	case LOCATION_EXTRA: {
		pcard = extra[controler][sequence];
		extra[controler].erase(extra[controler].begin() + sequence);
		ResetSequence(extra[controler], true);
		if (pcard->position & POS_FACEUP)
			extra_p_count[controler]--;
		break;
	}
	default:
		return nullptr;
	}
	pcard->location = 0;
	return pcard;
}
void ClientField::UpdateCard(int controler, int location, int sequence, unsigned char* data) {
	ClientCard* pcard = GetCard(controler, location, sequence);
	int len = BufferIO::ReadInt32(data);
	if (pcard && len > LEN_HEADER)
		pcard->UpdateInfo(data);
}
void ClientField::UpdateFieldCard(int controler, int location, unsigned char* data) {
	std::vector<ClientCard*>* lst = 0;
	switch(location) {
	case LOCATION_DECK:
		lst = &deck[controler];
		break;
	case LOCATION_HAND:
		lst = &hand[controler];
		break;
	case LOCATION_MZONE:
		lst = &mzone[controler];
		break;
	case LOCATION_SZONE:
		lst = &szone[controler];
		break;
	case LOCATION_GRAVE:
		lst = &grave[controler];
		break;
	case LOCATION_REMOVED:
		lst = &remove[controler];
		break;
	case LOCATION_EXTRA:
		lst = &extra[controler];
		break;
	}
	if(!lst)
		return;
	int len;
	for(auto cit = lst->begin(); cit != lst->end(); ++cit) {
		len = BufferIO::ReadInt32(data);
		if(len > LEN_HEADER)
			(*cit)->UpdateInfo(data);
		data += len - 4;
	}
}
void ClientField::ClearCommandFlag() {
	for(auto cit = activatable_cards.begin(); cit != activatable_cards.end(); ++cit)
		(*cit)->cmdFlag = 0;
	for(auto cit = summonable_cards.begin(); cit != summonable_cards.end(); ++cit)
		(*cit)->cmdFlag = 0;
	for(auto cit = spsummonable_cards.begin(); cit != spsummonable_cards.end(); ++cit)
		(*cit)->cmdFlag = 0;
	for(auto cit = msetable_cards.begin(); cit != msetable_cards.end(); ++cit)
		(*cit)->cmdFlag = 0;
	for(auto cit = ssetable_cards.begin(); cit != ssetable_cards.end(); ++cit)
		(*cit)->cmdFlag = 0;
	for(auto cit = reposable_cards.begin(); cit != reposable_cards.end(); ++cit)
		(*cit)->cmdFlag = 0;
	for(auto cit = attackable_cards.begin(); cit != attackable_cards.end(); ++cit)
		(*cit)->cmdFlag = 0;
	for(int i = 0; i < 4; ++i) {
		deck_act[i] = false;
		extra_act[i] = false;
		grave_act[i] = false;
		remove_act[i] = false;
		pzone_act[i] = false;
	}
	conti_cards.clear();
	conti_act = false;
}
void ClientField::ClearSelect() {
	for(auto cit = selectable_cards.begin(); cit != selectable_cards.end(); ++cit) {
		(*cit)->is_selectable = false;
		(*cit)->is_selected = false;
	}
	for(auto cit = selected_cards.begin(); cit != selected_cards.end(); ++cit) {
		(*cit)->is_selectable = false;
		(*cit)->is_selected = false;
	}
	for(auto cit = selectsum_all.begin(); cit != selectsum_all.end(); ++cit) {
		(*cit)->is_selectable = false;
		(*cit)->is_selected = false;
	}
	for(auto cit = selectsum_cards.begin(); cit != selectsum_cards.end(); ++cit) {
		(*cit)->is_selectable = false;
		(*cit)->is_selected = false;
	}
}
void ClientField::ClearChainSelect() {
	for(auto cit = activatable_cards.begin(); cit != activatable_cards.end(); ++cit) {
		(*cit)->cmdFlag = 0;
		(*cit)->chain_code = 0;
		(*cit)->is_selectable = false;
		(*cit)->is_selected = false;
	}
	for(int i = 0; i < 4; ++i) {
		deck_act[i] = false;
		extra_act[i] = false;
		grave_act[i] = false;
		remove_act[i] = false;
		pzone_act[i] = false;
	}
	conti_cards.clear();
	conti_act = false;
}
// needs to be synchronized with EGET_SCROLL_BAR_CHANGED
void ClientField::ShowSelectCard(bool buttonok, bool chain) {
	if(cant_check_grave) {
		bool has_card_in_grave = false;
		for (auto& pcard : selectable_cards) {
			if (pcard->location == LOCATION_GRAVE) {
				has_card_in_grave = true;
				break;
			}
		}
		if(has_card_in_grave) {
			rnd.shuffle_vector(selectable_cards);
		}
	}
	int startpos;
	int ct;
	if(selectable_cards.size() <= 5) {
		startpos = 30 + 125 * (5 - selectable_cards.size()) / 2;
		ct = selectable_cards.size();
	} else {
		startpos = 30;
		ct = 5;
	}
	for(int i = 0; i < ct; ++i) {
		mainGame->stCardPos[i]->enableOverrideColor(false);
		// image
		if(selectable_cards[i]->code)
			mainGame->imageLoading.insert(std::make_pair(mainGame->btnCardSelect[i], selectable_cards[i]->code));
		else if(conti_selecting)
			mainGame->imageLoading.insert(std::make_pair(mainGame->btnCardSelect[i], selectable_cards[i]->chain_code));
		else
			mainGame->btnCardSelect[i]->setImage(imageManager.tCover[selectable_cards[i]->controler + 2]);
		mainGame->btnCardSelect[i]->setRelativePosition(irr::core::rect<irr::s32>(startpos + i * 125, 55, startpos + 120 + i * 125, 225));
		mainGame->btnCardSelect[i]->setPressed(false);
		mainGame->btnCardSelect[i]->setVisible(true);
		if(mainGame->dInfo.curMsg != MSG_SORT_CARD) {
			// text
			wchar_t formatBuffer[2048];
			if(conti_selecting)
				myswprintf(formatBuffer, L"%ls", DataManager::unknown_string);
			else if(cant_check_grave && selectable_cards[i]->location == LOCATION_GRAVE)
				myswprintf(formatBuffer, L"%ls", dataManager.FormatLocation(selectable_cards[i]->location, 0));
			else if(selectable_cards[i]->location == LOCATION_OVERLAY)
				myswprintf(formatBuffer, L"%ls[%d](%d)", 
					dataManager.FormatLocation(selectable_cards[i]->overlayTarget->location, selectable_cards[i]->overlayTarget->sequence),
					selectable_cards[i]->overlayTarget->sequence + 1, selectable_cards[i]->sequence + 1);
			else
				myswprintf(formatBuffer, L"%ls[%d]", dataManager.FormatLocation(selectable_cards[i]->location, selectable_cards[i]->sequence),
					selectable_cards[i]->sequence + 1);
			mainGame->stCardPos[i]->setText(formatBuffer);
			// color
			if (selectable_cards[i]->is_selected)
				mainGame->stCardPos[i]->setBackgroundColor(0xffffff00);
			else {
				if(conti_selecting)
					mainGame->stCardPos[i]->setBackgroundColor(0xffffffff);
				else if(selectable_cards[i]->location == LOCATION_OVERLAY) {
					if(selectable_cards[i]->owner != selectable_cards[i]->overlayTarget->controler)
						mainGame->stCardPos[i]->setOverrideColor(0xff0000ff);
					if(selectable_cards[i]->overlayTarget->controler)
						mainGame->stCardPos[i]->setBackgroundColor(0xffd0d0d0);
					else
						mainGame->stCardPos[i]->setBackgroundColor(0xffffffff);
				} else if(selectable_cards[i]->location == LOCATION_DECK || selectable_cards[i]->location == LOCATION_EXTRA || selectable_cards[i]->location == LOCATION_REMOVED) {
					if(selectable_cards[i]->position & POS_FACEDOWN)
						mainGame->stCardPos[i]->setOverrideColor(0xff0000ff);
					if(selectable_cards[i]->controler)
						mainGame->stCardPos[i]->setBackgroundColor(0xffd0d0d0);
					else
						mainGame->stCardPos[i]->setBackgroundColor(0xffffffff);
				} else {
					if(selectable_cards[i]->controler)
						mainGame->stCardPos[i]->setBackgroundColor(0xffd0d0d0);
					else
						mainGame->stCardPos[i]->setBackgroundColor(0xffffffff);
				}
			}
		} else {
			if(sort_list[i]) {
				wchar_t formatBuffer[2048];
				myswprintf(formatBuffer, L"%d", sort_list[i]);
				mainGame->stCardPos[i]->setText(formatBuffer);
			} else
				mainGame->stCardPos[i]->setText(L"");
			mainGame->stCardPos[i]->setBackgroundColor(0xffffffff);
		}
		mainGame->stCardPos[i]->setVisible(true);
		mainGame->stCardPos[i]->setRelativePosition(irr::core::rect<irr::s32>(startpos + i * 125, 30, startpos + 120 + i * 125, 50));
	}
	if(selectable_cards.size() <= 5) {
		for(int i = selectable_cards.size(); i < 5; ++i) {
			mainGame->btnCardSelect[i]->setVisible(false);
			mainGame->stCardPos[i]->setVisible(false);
		}
		mainGame->scrCardList->setPos(0);
		mainGame->scrCardList->setVisible(false);
	} else {
		mainGame->scrCardList->setVisible(true);
		mainGame->scrCardList->setMin(0);
		mainGame->scrCardList->setMax((selectable_cards.size() - 5) * 10 + 9);
		mainGame->scrCardList->setPos(0);
	}
	mainGame->btnSelectOK->setVisible(buttonok);
	mainGame->PopupElement(mainGame->wCardSelect);
}
void ClientField::ShowChainCard() {
	int startpos;
	int ct;
	if(selectable_cards.size() <= 5) {
		startpos = 30 + 125 * (5 - selectable_cards.size()) / 2;
		ct = selectable_cards.size();
	} else {
		startpos = 30;
		ct = 5;
	}
	for(int i = 0; i < ct; ++i) {
		if(selectable_cards[i]->code)
			mainGame->imageLoading.insert(std::make_pair(mainGame->btnCardSelect[i], selectable_cards[i]->code));
		else
			mainGame->btnCardSelect[i]->setImage(imageManager.tCover[selectable_cards[i]->controler + 2]);
		mainGame->btnCardSelect[i]->setRelativePosition(irr::core::rect<irr::s32>(startpos + i * 125, 55, startpos + 120 + i * 125, 225));
		mainGame->btnCardSelect[i]->setPressed(false);
		mainGame->btnCardSelect[i]->setVisible(true);
		wchar_t formatBuffer[2048];
		myswprintf(formatBuffer, L"%ls[%d]", dataManager.FormatLocation(selectable_cards[i]->location, selectable_cards[i]->sequence),
			selectable_cards[i]->sequence + 1);
		mainGame->stCardPos[i]->setText(formatBuffer);
		if(selectable_cards[i]->location == LOCATION_OVERLAY) {
			if(selectable_cards[i]->owner != selectable_cards[i]->overlayTarget->controler)
				mainGame->stCardPos[i]->setOverrideColor(0xff0000ff);
			if(selectable_cards[i]->overlayTarget->controler)
				mainGame->stCardPos[i]->setBackgroundColor(0xffd0d0d0);
			else mainGame->stCardPos[i]->setBackgroundColor(0xffffffff);
		} else {
			if(selectable_cards[i]->controler)
				mainGame->stCardPos[i]->setBackgroundColor(0xffd0d0d0);
			else mainGame->stCardPos[i]->setBackgroundColor(0xffffffff);
		}
		mainGame->stCardPos[i]->setVisible(true);
		mainGame->stCardPos[i]->setRelativePosition(irr::core::rect<irr::s32>(startpos + i * 125, 30, startpos + 120 + i * 125, 50));
	} 
	if(selectable_cards.size() <= 5) {
		for(int i = selectable_cards.size(); i < 5; ++i) {
			mainGame->btnCardSelect[i]->setVisible(false);
			mainGame->stCardPos[i]->setVisible(false);
		}
		mainGame->scrCardList->setPos(0);
		mainGame->scrCardList->setVisible(false);
	} else {
		mainGame->scrCardList->setVisible(true);
		mainGame->scrCardList->setMin(0);
		mainGame->scrCardList->setMax((selectable_cards.size() - 5) * 10 + 9);
		mainGame->scrCardList->setPos(0);
	}
	if(!chain_forced)
		mainGame->btnSelectOK->setVisible(true);
	else mainGame->btnSelectOK->setVisible(false);
	mainGame->PopupElement(mainGame->wCardSelect);
}
void ClientField::ShowLocationCard() {
	int startpos;
	int ct;
	if(display_cards.size() <= 5) {
		startpos = 30 + 125 * (5 - display_cards.size()) / 2;
		ct = display_cards.size();
	} else {
		startpos = 30;
		ct = 5;
	}
	for(int i = 0; i < ct; ++i) {
		mainGame->stDisplayPos[i]->enableOverrideColor(false);
		if(display_cards[i]->code)
			mainGame->imageLoading.insert(std::make_pair(mainGame->btnCardDisplay[i], display_cards[i]->code));
		else
			mainGame->btnCardDisplay[i]->setImage(imageManager.tCover[display_cards[i]->controler + 2]);
		mainGame->btnCardDisplay[i]->setRelativePosition(irr::core::rect<irr::s32>(startpos + i * 125, 55, startpos + 120 + i * 125, 225));
		mainGame->btnCardDisplay[i]->setPressed(false);
		mainGame->btnCardDisplay[i]->setVisible(true);
		wchar_t formatBuffer[2048];
		if(display_cards[i]->location == LOCATION_OVERLAY)
			myswprintf(formatBuffer, L"%ls[%d](%d)", 
				dataManager.FormatLocation(display_cards[i]->overlayTarget->location, display_cards[i]->overlayTarget->sequence),
				display_cards[i]->overlayTarget->sequence + 1, display_cards[i]->sequence + 1);
		else
			myswprintf(formatBuffer, L"%ls[%d]", dataManager.FormatLocation(display_cards[i]->location, display_cards[i]->sequence),
				display_cards[i]->sequence + 1);
		mainGame->stDisplayPos[i]->setText(formatBuffer);
		if(display_cards[i]->location == LOCATION_OVERLAY) {
			if(display_cards[i]->owner != display_cards[i]->overlayTarget->controler)
				mainGame->stDisplayPos[i]->setOverrideColor(0xff0000ff);
			if(display_cards[i]->overlayTarget->controler)
				mainGame->stDisplayPos[i]->setBackgroundColor(0xffd0d0d0);
			else 
				mainGame->stDisplayPos[i]->setBackgroundColor(0xffffffff);
		} else if(display_cards[i]->location == LOCATION_EXTRA || display_cards[i]->location == LOCATION_REMOVED) {
			if(display_cards[i]->position & POS_FACEDOWN)
				mainGame->stDisplayPos[i]->setOverrideColor(0xff0000ff);
			if(display_cards[i]->controler)
				mainGame->stDisplayPos[i]->setBackgroundColor(0xffd0d0d0);
			else
				mainGame->stDisplayPos[i]->setBackgroundColor(0xffffffff);
		} else {
			if(display_cards[i]->controler)
				mainGame->stDisplayPos[i]->setBackgroundColor(0xffd0d0d0);
			else 
				mainGame->stDisplayPos[i]->setBackgroundColor(0xffffffff);
		}
		mainGame->stDisplayPos[i]->setVisible(true);
		mainGame->stDisplayPos[i]->setRelativePosition(irr::core::rect<irr::s32>(startpos + i * 125, 30, startpos + 120 + i * 125, 50));
	}
	if(display_cards.size() <= 5) {
		for(int i = display_cards.size(); i < 5; ++i) {
			mainGame->btnCardDisplay[i]->setVisible(false);
			mainGame->stDisplayPos[i]->setVisible(false);
		}
		mainGame->scrDisplayList->setPos(0);
		mainGame->scrDisplayList->setVisible(false);
	} else {
		mainGame->scrDisplayList->setVisible(true);
		mainGame->scrDisplayList->setMin(0);
		mainGame->scrDisplayList->setMax((display_cards.size() - 5) * 10 + 9);
		mainGame->scrDisplayList->setPos(0);
	}
	mainGame->btnDisplayOK->setVisible(true);
	mainGame->PopupElement(mainGame->wCardDisplay);
}
void ClientField::ShowSelectOption(int select_hint) {
	selected_option = 0;
	wchar_t textBuffer[256];
	int count = select_options.size();
	bool quickmode = true;
	mainGame->gMutex.lock();
	for(auto option : select_options) {
		if(mainGame->guiFont->getDimension(dataManager.GetDesc(option)).Width > 310) {
			quickmode = false;
			break;
		}
	}
	for(int i = 0; (i < count) && (i < 5) && quickmode; i++) {
		const wchar_t* option = dataManager.GetDesc(select_options[i]);
		mainGame->btnOption[i]->setText(option);
	}
	if(quickmode) {
		bool scrollbar = count > 5;
		mainGame->scrOption->setVisible(scrollbar);
		mainGame->scrOption->setPos(0);
		mainGame->scrOption->setMax(scrollbar ? (count - 5) : 1);
		mainGame->stOptions->setVisible(false);
		mainGame->btnOptionp->setVisible(false);
		mainGame->btnOptionn->setVisible(false);
		mainGame->btnOptionOK->setVisible(false);
		for(int i = 0; i < 5; i++)
			mainGame->btnOption[i]->setVisible(i < count);
		irr::core::recti pos = mainGame->wOptions->getRelativePosition();
		int newheight = 30 + 40 * (scrollbar ? 5 : count);
		int oldheight = pos.LowerRightCorner.Y - pos.UpperLeftCorner.Y;
		pos.UpperLeftCorner.Y = pos.UpperLeftCorner.Y + (oldheight - newheight) / 2;
		pos.LowerRightCorner.X = pos.UpperLeftCorner.X + (scrollbar ? 375 : 350);
		pos.LowerRightCorner.Y = pos.UpperLeftCorner.Y + newheight;
		mainGame->wOptions->setRelativePosition(pos);
	} else {
		mainGame->SetStaticText(mainGame->stOptions, 310, mainGame->guiFont, dataManager.GetDesc(select_options[0]));
		mainGame->stOptions->setVisible(true);
		mainGame->btnOptionp->setVisible(false);
		mainGame->btnOptionn->setVisible(count > 1);
		mainGame->btnOptionOK->setVisible(true);
		for(int i = 0; i < 5; i++)
			mainGame->btnOption[i]->setVisible(false);
		irr::core::recti pos = mainGame->wOptions->getRelativePosition();
		pos.LowerRightCorner.Y = pos.UpperLeftCorner.Y + 140;
		mainGame->wOptions->setRelativePosition(pos);
	}
	if(select_hint)
		myswprintf(textBuffer, L"%ls", dataManager.GetDesc(select_hint));
	else
		myswprintf(textBuffer, dataManager.GetSysString(555));
	mainGame->wOptions->setText(textBuffer);
	mainGame->PopupElement(mainGame->wOptions);
	mainGame->gMutex.unlock();
}
void ClientField::ReplaySwap() {
	if(mainGame->dInfo.isFourPlayer) {
		RotateFour();
		return;
	}
	std::swap(deck[0], deck[1]);
	std::swap(hand[0], hand[1]);
	std::swap(mzone[0], mzone[1]);
	std::swap(szone[0], szone[1]);
	std::swap(grave[0], grave[1]);
	std::swap(remove[0], remove[1]);
	std::swap(extra[0], extra[1]);
	std::swap(extra_p_count[0], extra_p_count[1]);
	for(int p = 0; p < 2; ++p) {
		for(auto cit = deck[p].begin(); cit != deck[p].end(); ++cit) {
			(*cit)->controler = 1 - (*cit)->controler;
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = hand[p].begin(); cit != hand[p].end(); ++cit) {
			(*cit)->controler = 1 - (*cit)->controler;
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = mzone[p].begin(); cit != mzone[p].end(); ++cit) {
			if(*cit) {
				(*cit)->controler = 1 - (*cit)->controler;
				GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
				(*cit)->is_moving = false;
			}
		}
		for(auto cit = szone[p].begin(); cit != szone[p].end(); ++cit) {
			if(*cit) {
				(*cit)->controler = 1 - (*cit)->controler;
				GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
				(*cit)->is_moving = false;
			}
		}
		for(auto cit = grave[p].begin(); cit != grave[p].end(); ++cit) {
			(*cit)->controler = 1 - (*cit)->controler;
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = remove[p].begin(); cit != remove[p].end(); ++cit) {
			(*cit)->controler = 1 - (*cit)->controler;
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = extra[p].begin(); cit != extra[p].end(); ++cit) {
			(*cit)->controler = 1 - (*cit)->controler;
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
	}
	for(auto cit = overlay_cards.begin(); cit != overlay_cards.end(); ++cit) {
		(*cit)->controler = 1 - (*cit)->controler;
		GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
		(*cit)->is_moving = false;
	}
	mainGame->dInfo.isFirst = !mainGame->dInfo.isFirst;
	mainGame->dInfo.isReplaySwapped = !mainGame->dInfo.isReplaySwapped;
	std::swap(mainGame->dInfo.lp[0], mainGame->dInfo.lp[1]);
	std::swap(mainGame->dInfo.strLP[0], mainGame->dInfo.strLP[1]);
	std::swap(mainGame->dInfo.hostname, mainGame->dInfo.clientname);
	std::swap(mainGame->dInfo.hostname_tag, mainGame->dInfo.clientname_tag);
	for(auto chit = chains.begin(); chit != chains.end(); ++chit) {
		chit->controler = 1 - chit->controler;
		GetChainLocation(chit->controler, chit->location, chit->sequence, &chit->chain_pos);
	}
	disabled_field = (disabled_field >> 16) | (disabled_field << 16);
}
// Four players: the swap button turns the table a quarter turn, so the next seat clockwise is
// shown at the bottom. Every side moves one place anticlockwise round the screen.
void ClientField::RotateFour() {
	auto& d = mainGame->dInfo;
	d.viewSeat = (d.viewSeat + 1) % 4;
	auto turn = [](std::vector<ClientCard*>* lists) { std::rotate(lists, lists + 1, lists + 4); };
	turn(deck);
	turn(hand);
	turn(mzone);
	turn(szone);
	turn(grave);
	turn(remove);
	turn(extra);
	std::rotate(extra_p_count, extra_p_count + 1, extra_p_count + 4);
	std::rotate(player_desc_hints, player_desc_hints + 1, player_desc_hints + 4);
	std::rotate(d.lp, d.lp + 1, d.lp + 4);
	for(int i = 0; i < 3; ++i)	// wchar_t[16] strings: swap along the row
		std::swap(d.strLP[i], d.strLP[i + 1]);
	std::rotate(d.playerOut, d.playerOut + 1, d.playerOut + 4);
	std::rotate(d.time_left, d.time_left + 1, d.time_left + 4);
	auto move = [this](ClientCard* pcard) {
		if(!pcard)
			return;
		pcard->controler = (pcard->controler + 3) % 4;
		GetCardLocation(pcard, &pcard->curPos, &pcard->curRot, true);
		pcard->is_moving = false;
	};
	for(int p = 0; p < 4; ++p) {
		for(auto lists : { &deck[p], &hand[p], &mzone[p], &szone[p], &grave[p], &remove[p], &extra[p] })
			for(auto pcard : *lists)
				move(pcard);
	}
	for(auto pcard : overlay_cards)
		move(pcard);
	for(auto& chain : chains) {
		chain.controler = (chain.controler + 3) % 4;
		GetChainLocation(chain.controler, chain.location, chain.sequence, &chain.chain_pos);
	}
	BufferIO::CopyWStr(d.names4[d.viewSeat], d.hostname, 20);
	BufferIO::CopyWStr(d.names4[(d.viewSeat + 2) % 4], d.clientname, 20);
}
void ClientField::RefreshAllCards() {
	for(int p = 0; p < mainGame->dInfo.players(); ++p) {
		for(auto cit = deck[p].begin(); cit != deck[p].end(); ++cit) {
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = hand[p].begin(); cit != hand[p].end(); ++cit) {
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = mzone[p].begin(); cit != mzone[p].end(); ++cit) {
			if(*cit) {
				GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
				(*cit)->is_moving = false;
			}
		}
		for(auto cit = szone[p].begin(); cit != szone[p].end(); ++cit) {
			if(*cit) {
				GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
				(*cit)->is_moving = false;
			}
		}
		for(auto cit = grave[p].begin(); cit != grave[p].end(); ++cit) {
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = remove[p].begin(); cit != remove[p].end(); ++cit) {
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
		for(auto cit = extra[p].begin(); cit != extra[p].end(); ++cit) {
			GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
			(*cit)->is_moving = false;
		}
	}
	for(auto cit = overlay_cards.begin(); cit != overlay_cards.end(); ++cit) {
		GetCardLocation(*cit, &(*cit)->curPos, &(*cit)->curRot, true);
		(*cit)->is_moving = false;
	}
}
void ClientField::GetChainLocation(int controler, int location, int sequence, irr::core::vector3df* t) {
	t->X = 0;
	t->Y = 0;
	t->Z = 0;
	int rule = (mainGame->dInfo.duel_rule >= 4) ? 1 : 0;
	switch((location & 0x7f)) {
	case LOCATION_DECK: {
		t->X = (matManager.vFieldDeck[controler][0].Pos.X + matManager.vFieldDeck[controler][1].Pos.X) / 2;
		t->Y = (matManager.vFieldDeck[controler][0].Pos.Y + matManager.vFieldDeck[controler][2].Pos.Y) / 2;
		t->Z = deck[controler].size() * 0.01f + 0.03f;
		break;
	}
	case LOCATION_HAND: {
		if (controler == 0) {
			t->X = 2.95f;
			t->Y = 3.15f;
			t->Z = 0.03f;
		} else {
			t->X = 2.95f;
			t->Y = -3.15f;
			t->Z = 0.03f;
		}
		break;
	}
	case LOCATION_MZONE: {
		t->X = (matManager.vFieldMzone[controler][sequence][0].Pos.X + matManager.vFieldMzone[controler][sequence][1].Pos.X) / 2;
		t->Y = (matManager.vFieldMzone[controler][sequence][0].Pos.Y + matManager.vFieldMzone[controler][sequence][2].Pos.Y) / 2;
		t->Z = 0.03f;
		break;
	}
	case LOCATION_SZONE: {
		t->X = (matManager.vFieldSzone[controler][sequence][rule][0].Pos.X + matManager.vFieldSzone[controler][sequence][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldSzone[controler][sequence][rule][0].Pos.Y + matManager.vFieldSzone[controler][sequence][rule][2].Pos.Y) / 2;
		t->Z = 0.03f;
		break;
	}
	case LOCATION_GRAVE: {
		t->X = (matManager.vFieldGrave[controler][rule][0].Pos.X + matManager.vFieldGrave[controler][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldGrave[controler][rule][0].Pos.Y + matManager.vFieldGrave[controler][rule][2].Pos.Y) / 2;
		t->Z = grave[controler].size() * 0.01f + 0.03f;
		break;
	}
	case LOCATION_REMOVED: {
		t->X = (matManager.vFieldRemove[controler][rule][0].Pos.X + matManager.vFieldRemove[controler][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldRemove[controler][rule][0].Pos.Y + matManager.vFieldRemove[controler][rule][2].Pos.Y) / 2;
		t->Z = remove[controler].size() * 0.01f + 0.03f;
		break;
	}
	case LOCATION_EXTRA: {
		t->X = (matManager.vFieldExtra[controler][0].Pos.X + matManager.vFieldExtra[controler][1].Pos.X) / 2;
		t->Y = (matManager.vFieldExtra[controler][0].Pos.Y + matManager.vFieldExtra[controler][2].Pos.Y) / 2;
		t->Z = extra[controler].size() * 0.01f + 0.03f;
		break;
	}
	}
}
// Four players: each side's zones lie on one square grid around the centre (see FOUR_PLAYER.md).
// In a side's own frame, a is the distance forward (towards the centre, negative = behind the
// centre) and b is to that player's right: Monster Zones a = -2, b = -2..2; Extra Monster Zones
// a = -1, b = -1/+1; Spell/Trap Zones a = -3; Field Zone a = -3, b = -3; Extra Deck and Deck
// a = -4, b = -3/-2; GY and banished a = -4, b = 2/3; hand a = -5.
static const float FOUR_PITCH = 1.2f;	// world units per grid square
float ClientField::four_view_left = -0.728f;
float ClientField::four_view_right = 0.397f;
float ClientField::four_view_half_height = 0.35f;
float ClientField::four_name_width = ClientField::FOUR_NAME_MAX_WIDTH;
// The cross fills the height of the window (or as much as fits in the width right of the card
// text panel, with the side players' hands and at least the narrowest name boxes beside it),
// centred in the space right of that panel; one world unit is the same size across and down.
// The name boxes beside the corner portraits get the width left over, up to FOUR_NAME_MAX_WIDTH.
void ClientField::UpdateFourView(int width, int height, int panel_right) {
	if(width <= 0 || height <= 0)
		return;
	float area = (float)(width - panel_right);
	float half_width = std::max(FOUR_HALF_WIDTH, FOUR_HALF_WIDTH_NAMES);
	float k = std::min(height / (2 * FOUR_HALF_HEIGHT), area / (2 * half_width));	// pixels per world unit
	if(k <= 0)
		k = height / (2 * FOUR_HALF_HEIGHT);
	float cx = panel_right + area / 2;
	four_view_left = -cx / k / FOUR_CAMERA_HEIGHT;
	four_view_right = (width - cx) / k / FOUR_CAMERA_HEIGHT;
	four_view_half_height = height / 2.0f / k / FOUR_CAMERA_HEIGHT;
	float room = area / 2 / k - FOUR_CORNER_EDGE - FOUR_NAME_GAP - 0.05f;
	four_name_width = std::max(FOUR_NAME_MIN_WIDTH, std::min(FOUR_NAME_MAX_WIDTH, room));
}
void ClientField::FourGridPoint(int side, float a, float b, float* x, float* y) {
	// forward and right of each side on the grid (grid y points to the top of the screen)
	static const int f[4][2] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
	int s = side & 3;
	float gx = a * f[s][0] + b * f[s][1];
	float gy = a * f[s][1] - b * f[s][0];
	*x = gx * FOUR_PITCH;
	*y = -gy * FOUR_PITCH;	// world Y grows towards the bottom of the screen
}
// Turn of a side's field cards about the Z axis so their tops point away from that player
// (a card's top lies towards -Y; a positive turn takes -Y towards +X, the right of the screen).
float ClientField::FourSideAngle(int side) {
	static const float angle[4] = { 0.0f, 3.1415926f / 2.0f, 3.1415926f, -3.1415926f / 2.0f };
	return angle[side & 3];
}
void ClientField::GetCardLocation4(ClientCard* pcard, irr::core::vector3df* t, irr::core::vector3df* r) {
	const float PI = 3.1415926f;
	int side = pcard->controler & 3;
	int sequence = pcard->sequence;
	float a = 0, b = 0, z = 0.01f, turn = 0;
	bool facedown = (pcard->position & POS_FACEDOWN) != 0;
	r->X = 0.0f;
	switch(pcard->location) {
	case LOCATION_DECK:
		a = -4; b = -2; z = 0.01f + 0.01f * sequence;
		facedown = (deck_reversed == pcard->is_reversed);
		break;
	case 0:
	case LOCATION_HAND: {
		int count = (int)hand[side].size();
		float width = 4.6f;	// grid squares the hand may spread over (over the Deck and GY if need be)
		float step = (count > 1) ? std::min(0.75f, width / (count - 1)) : 0;
		a = FourHandA(side);
		b = -step * (count - 1) / 2 + step * sequence;
		if(pcard->is_hovered)
			a += 0.25f;
		z = 0.5f + 0.001f * sequence;
		facedown = (pcard->code == 0);
		break;
	}
	case LOCATION_MZONE:
		if(sequence >= 5) {
			a = -1;
			b = (sequence == 5) ? -1.0f : 1.0f;
		} else {
			a = -2;
			b = (float)(sequence - 2);
			// Corner monsters stay square to their controller's side, so they show who controls them.
		}
		z = 0.020f;
		if(pcard->position & POS_DEFENSE)
			turn -= PI / 2;
		break;
	case LOCATION_SZONE:
		a = -3;
		if(sequence == 5)
			b = -3;
		else if(sequence == 6)
			b = -2;
		else if(sequence == 7)
			b = 2;
		else
			b = (float)(sequence - 2);
		break;
	case LOCATION_GRAVE:
		a = -4; b = 2; z = 0.01f + 0.01f * sequence;
		facedown = false;
		break;
	case LOCATION_REMOVED:
		a = -4; b = 3; z = 0.01f + 0.01f * sequence;
		break;
	case LOCATION_EXTRA:
		a = -4; b = -3; z = 0.01f + 0.01f * sequence;
		break;
	case LOCATION_OVERLAY: {
		ClientCard* target = pcard->overlayTarget;
		if(!target || target->location != LOCATION_MZONE)
			return;
		side = target->controler & 3;
		int oseq = target->sequence;
		int mseq = (sequence < MAX_LAYER_COUNT) ? sequence : (MAX_LAYER_COUNT - 1);
		if(oseq >= 5) {
			a = -1;
			b = (oseq == 5) ? -1.0f : 1.0f;
		} else {
			a = -2;
			b = (float)(oseq - 2);
		}
		a -= 0.04f;
		b += -0.1f + 0.05f * mseq;
		z = mseq * 0.003f;
		facedown = false;
		break;
	}
	}
	FourGridPoint(side, a, b, &t->X, &t->Y);
	t->Z = z;
	r->Y = facedown ? PI : 0.0f;
	// Hands are shown upright for everyone (as the opponent's hand is in two-player duels);
	// the side players' hands run down the edges of the screen.
	if(pcard->location == LOCATION_HAND || pcard->location == 0)
		r->Z = 0.0f;
	else
		r->Z = FourSideAngle(side) + turn;
}
// Four players: which zone or hand card is under the mouse (x, y in the 1024x640 layout). Sets
// hovered_controler, hovered_location and hovered_sequence as GetHoverField does.
void ClientField::GetHoverField4(int x, int y) {
	hovered_location = 0;
	// the point on the table under the mouse (camera straight above the centre, see MainLoop)
	float vx = four_view_left + (four_view_right - four_view_left) * x / 1024.0f;
	float vy = four_view_half_height - 2 * four_view_half_height * y / 640.0f;
	float gx = FOUR_CAMERA_HEIGHT * vx / FOUR_PITCH;
	float gy = FOUR_CAMERA_HEIGHT * vy / FOUR_PITCH;	// grid y points to the top of the screen
	static const int f[4][2] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };	// as in FourGridPoint
	for(int side = 0; side < 4; ++side) {
		// the point in this side's own frame: a forward, b to the right
		float a = gx * f[side][0] + gy * f[side][1];
		float b = gx * f[side][1] - gy * f[side][0];
		// hand: cards stand upright on the screen, so for the side players they are long along b
		bool across = (side & 1) == 0;
		float half_a = across ? 0.42f : 0.29f, half_b = across ? 0.29f : 0.42f;
		int count = (int)hand[side].size();
		if(count && std::fabs(a - FourHandA(side)) <= half_a) {
			float step = (count > 1) ? std::min(0.75f, 4.6f / (count - 1)) : 0;
			for(int seq = count - 1; seq >= 0; --seq) {	// the later cards lie on top
				float bs = -step * (count - 1) / 2 + step * seq;
				if(std::fabs(b - bs) <= half_b) {
					hovered_controler = side;
					hovered_location = LOCATION_HAND;
					hovered_sequence = seq;
					return;
				}
			}
		}
		int ra = (int)std::floor(a + 0.5f), rb = (int)std::floor(b + 0.5f);
		if(std::fabs(a - ra) > 0.5f || std::fabs(b - rb) > 0.5f)
			continue;
		int location = 0, sequence = 0;
		if(ra == -2 && rb >= -2 && rb <= 2) {
			location = LOCATION_MZONE; sequence = rb + 2;
		} else if(ra == -1 && (rb == -1 || rb == 1)) {
			location = LOCATION_MZONE; sequence = (rb == -1) ? 5 : 6;
		} else if(ra == -3 && rb >= -2 && rb <= 2) {
			location = LOCATION_SZONE; sequence = rb + 2;
			if(!szone[side][sequence] && (rb == -2 || rb == 2) && szone[side][rb == -2 ? 6 : 7])
				sequence = (rb == -2) ? 6 : 7;	// old-rule Pendulum Zones share these squares
		} else if(ra == -3 && rb == -3) {
			location = LOCATION_SZONE; sequence = 5;
		} else if(ra == -4 && rb == -3) {
			location = LOCATION_EXTRA;
		} else if(ra == -4 && rb == -2) {
			location = LOCATION_DECK;
		} else if(ra == -4 && rb == 2) {
			location = LOCATION_GRAVE;
		} else if(ra == -4 && rb == 3) {
			location = LOCATION_REMOVED;
		}
		if(!location)
			continue;
		// a shared corner or Extra Monster Zone belongs to whichever neighbour has a card there
		if(location == LOCATION_MZONE && !mzone[side][sequence] && (sequence == 0 || sequence == 4 || sequence >= 5))
			continue;
		hovered_controler = side;
		hovered_location = location;
		hovered_sequence = sequence;
		return;
	}
}
void ClientField::GetCardLocation(ClientCard* pcard, irr::core::vector3df* t, irr::core::vector3df* r, bool setTrans) {
	if(mainGame->dInfo.isFourPlayer) {
		GetCardLocation4(pcard, t, r);
		if(setTrans) {
			pcard->mTransform.setTranslation(*t);
			pcard->mTransform.setRotationRadians(*r);
		}
		return;
	}
	int controler = pcard->controler;
	int sequence = pcard->sequence;
	int location = pcard->location;
	int rule = (mainGame->dInfo.duel_rule >= 4) ? 1 : 0;
	const float overlay_buttom = 0.0f;
	const float material_height = 0.003f;
	const float mzone_buttom = 0.020f;
	switch (location) {
	case LOCATION_DECK: {
		t->X = (matManager.vFieldDeck[controler][0].Pos.X + matManager.vFieldDeck[controler][1].Pos.X) / 2;
		t->Y = (matManager.vFieldDeck[controler][0].Pos.Y + matManager.vFieldDeck[controler][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			if(deck_reversed == pcard->is_reversed) {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 0.0f;
			} else {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 0.0f;
			}
		} else {
			if(deck_reversed == pcard->is_reversed) {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 3.1415926f;
			} else {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 3.1415926f;
			}
		}
		break;
	}
	case 0:
	case LOCATION_HAND: {
		int count = hand[controler].size();
		if (controler == 0) {
			if (count <= 6)
				t->X = (5.5f - 0.8f * count) / 2 + 1.55f + sequence * 0.8f;
			else
				t->X = 1.9f + sequence * 4.0f / (count - 1);
			if (pcard->is_hovered) {
				t->Y = 3.84f;
				t->Z = 0.656f + 0.001f * sequence;
			} else {
				t->Y = 4.0f;
				t->Z = 0.5f + 0.001f * sequence;
			}
			if(pcard->code) {
				r->X = -0.798056f;
				r->Y = 0.0f;
				r->Z = 0.0f;
			} else {
				r->X = 0.798056f;
				r->Y = 3.1415926f;
				r->Z = 0;
			}
		} else {
			if (count <= 6)
				t->X = 6.25f - (5.5f - 0.8f * count) / 2 - sequence * 0.8f;
			else
				t->X = 5.9f - sequence * 4.0f / (count - 1);
			float raise = botTheater.TopHandShift();	// Bot Theater: may sit higher up, clear of holograms
			if (pcard->is_hovered) {
				t->Y = -3.56f - raise;
				t->Z = 0.656f - 0.001f * sequence;
			} else {
				t->Y = -3.4f - raise;
				t->Z = 0.5f - 0.001f * sequence;
			}
			if (pcard->code == 0) {
				r->X = 0.798056f;
				r->Y = 3.1415926f;
				r->Z = 0;
			} else {
				r->X = -0.798056f;
				r->Y = 0;
				r->Z = 0;
			}
		}
		break;
	}
	case LOCATION_MZONE: {
		t->X = (matManager.vFieldMzone[controler][sequence][0].Pos.X + matManager.vFieldMzone[controler][sequence][1].Pos.X) / 2;
		t->Y = (matManager.vFieldMzone[controler][sequence][0].Pos.Y + matManager.vFieldMzone[controler][sequence][2].Pos.Y) / 2;
		t->Z = mzone_buttom;
		if (controler == 0) {
			if (pcard->position & POS_DEFENSE) {
				r->X = 0.0f;
				r->Z = -3.1415926f / 2.0f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f + 0.001f;
				else r->Y = 0.0f;
			} else {
				r->X = 0.0f;
				r->Z = 0.0f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f;
				else r->Y = 0.0f;
			}
		} else {
			if (pcard->position & POS_DEFENSE) {
				r->X = 0.0f;
				r->Z = 3.1415926f / 2.0f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f + 0.001f;
				else r->Y = 0.0f;
			} else {
				r->X = 0.0f;
				r->Z = 3.1415926f;
				if (pcard->position & POS_FACEDOWN)
					r->Y = 3.1415926f;
				else r->Y = 0.0f;
			}
		}
		break;
	}
	case LOCATION_SZONE: {
		t->X = (matManager.vFieldSzone[controler][sequence][rule][0].Pos.X + matManager.vFieldSzone[controler][sequence][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldSzone[controler][sequence][rule][0].Pos.Y + matManager.vFieldSzone[controler][sequence][rule][2].Pos.Y) / 2;
		t->Z = 0.01f;
		if (controler == 0) {
			r->X = 0.0f;
			r->Z = 0.0f;
			if (pcard->position & POS_FACEDOWN)
				r->Y = 3.1415926f;
			else r->Y = 0.0f;
		} else {
			r->X = 0.0f;
			r->Z = 3.1415926f;
			if (pcard->position & POS_FACEDOWN)
				r->Y = 3.1415926f;
			else r->Y = 0.0f;
		}
		break;
	}
	case LOCATION_GRAVE: {
		t->X = (matManager.vFieldGrave[controler][rule][0].Pos.X + matManager.vFieldGrave[controler][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldGrave[controler][rule][0].Pos.Y + matManager.vFieldGrave[controler][rule][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 0.0f;
		} else {
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 3.1415926f;
		}
		break;
	}
	case LOCATION_REMOVED: {
		t->X = (matManager.vFieldRemove[controler][rule][0].Pos.X + matManager.vFieldRemove[controler][rule][1].Pos.X) / 2;
		t->Y = (matManager.vFieldRemove[controler][rule][0].Pos.Y + matManager.vFieldRemove[controler][rule][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			if(pcard->position & POS_FACEUP) {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 0.0f;
			} else {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 0.0f;
			}
		} else {
			if(pcard->position & POS_FACEUP) {
				r->X = 0.0f;
				r->Y = 0.0f;
				r->Z = 3.1415926f;
			} else {
				r->X = 0.0f;
				r->Y = 3.1415926f;
				r->Z = 3.1415926f;
			}
		}
		break;
	}
	case LOCATION_EXTRA: {
		t->X = (matManager.vFieldExtra[controler][0].Pos.X + matManager.vFieldExtra[controler][1].Pos.X) / 2;
		t->Y = (matManager.vFieldExtra[controler][0].Pos.Y + matManager.vFieldExtra[controler][2].Pos.Y) / 2;
		t->Z = 0.01f + 0.01f * sequence;
		if (controler == 0) {
			r->X = 0.0f;
			if(pcard->position & POS_FACEUP)
				r->Y = 0.0f;
			else r->Y = 3.1415926f;
			r->Z = 0.0f;
		} else {
			r->X = 0.0f;
			if(pcard->position & POS_FACEUP)
				r->Y = 0.0f;
			else r->Y = 3.1415926f;
			r->Z = 3.1415926f;
		}
		break;
	}
	case LOCATION_OVERLAY: {
		if (pcard->overlayTarget->location != LOCATION_MZONE) {
			return;
		}
		int oseq = pcard->overlayTarget->sequence;
		int mseq = (sequence < MAX_LAYER_COUNT) ? sequence : (MAX_LAYER_COUNT - 1);
		if (pcard->overlayTarget->controler == 0) {
			t->X = (matManager.vFieldMzone[0][oseq][0].Pos.X + matManager.vFieldMzone[0][oseq][1].Pos.X) / 2 - 0.12f + 0.06f * mseq;
			t->Y = (matManager.vFieldMzone[0][oseq][0].Pos.Y + matManager.vFieldMzone[0][oseq][2].Pos.Y) / 2 + 0.05f;
			t->Z = overlay_buttom + mseq * material_height;
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 0.0f;
		} else {
			t->X = (matManager.vFieldMzone[1][oseq][0].Pos.X + matManager.vFieldMzone[1][oseq][1].Pos.X) / 2 + 0.12f - 0.06f * mseq;
			t->Y = (matManager.vFieldMzone[1][oseq][0].Pos.Y + matManager.vFieldMzone[1][oseq][2].Pos.Y) / 2 - 0.05f;
			t->Z = overlay_buttom + mseq * material_height;
			r->X = 0.0f;
			r->Y = 0.0f;
			r->Z = 3.1415926f;
		}
		break;
	}
	}
	if(setTrans) {
		pcard->mTransform.setTranslation(*t);
		pcard->mTransform.setRotationRadians(*r);
	}
}
void ClientField::MoveCard(ClientCard * pcard, int frame) {
	irr::core::vector3df trans = pcard->curPos;
	irr::core::vector3df rot = pcard->curRot;
	GetCardLocation(pcard, &trans, &rot);
	pcard->dPos = (trans - pcard->curPos) / frame;
	float diff = rot.X - pcard->curRot.X;
	while (diff < 0) diff += 3.1415926f * 2;
	while (diff > 3.1415926f * 2)
		diff -= 3.1415926f * 2;
	if (diff < 3.1415926f)
		pcard->dRot.X = diff / frame;
	else
		pcard->dRot.X = -(3.1415926f * 2 - diff) / frame;
	diff = rot.Y - pcard->curRot.Y;
	while (diff < 0) diff += 3.1415926f * 2;
	while (diff > 3.1415926f * 2) diff -= 3.1415926f * 2;
	if (diff < 3.1415926f)
		pcard->dRot.Y = diff / frame;
	else
		pcard->dRot.Y = -(3.1415926f * 2 - diff) / frame;
	diff = rot.Z - pcard->curRot.Z;
	while (diff < 0) diff += 3.1415926f * 2;
	while (diff > 3.1415926f * 2) diff -= 3.1415926f * 2;
	if (diff < 3.1415926f)
		pcard->dRot.Z = diff / frame;
	else
		pcard->dRot.Z = -(3.1415926f * 2 - diff) / frame;
	pcard->is_moving = true;
	pcard->aniFrame = frame;
}
void ClientField::FadeCard(ClientCard * pcard, int alpha, int frame) {
	pcard->dAlpha = (alpha - pcard->curAlpha) / frame;
	pcard->is_fading = true;
	pcard->aniFrame = frame;
}
bool ClientField::ShowSelectSum(bool panelmode) {
	select_ready = CheckSelectSum();
	if(select_ready && (selectsum_cards.size() == 0 || selectable_cards.size() == 0)) {
		SetResponseSelectedCards();
		ShowCancelOrFinishButton(0);
		if(mainGame->wCardSelect->isVisible())
			mainGame->HideElement(mainGame->wCardSelect, true);
		else 
			DuelClient::SendResponse();
		return true;
	}

	auto display_hint = select_hint ? dataManager.GetDesc(select_hint) : dataManager.GetSysString(560);

	wchar_t cur_hint[20];
	if (select_curval_l == select_curval_h) {
		myswprintf(cur_hint, L"%d", select_curval_l);
	} else {
		myswprintf(cur_hint, L"%d-%d", select_curval_l, select_curval_h);
	}

	wchar_t target_hint[20];
	if (select_mode == 0) { // sum equal
		myswprintf(target_hint, L"%d", select_sumval);
	} else { // sum greater
		myswprintf(target_hint, L"%d+", select_sumval);
	}

	wchar_t textBuffer[256];
	myswprintf(textBuffer, L"%ls(%ls/%ls)", display_hint, cur_hint, target_hint);

	if(panelmode) {
		mainGame->wCardSelect->setText(textBuffer);
		mainGame->wCardSelect->setVisible(false);
		mainGame->dField.ShowSelectCard();
	} else {
		mainGame->stHintMsg->setText(textBuffer);
		mainGame->stHintMsg->setVisible(true);
	}
	if (select_ready) {
		ShowCancelOrFinishButton(2);
	} else {
		ShowCancelOrFinishButton(0);
	}
	return false;
}
bool ClientField::CheckSelectSum() {
	std::set<ClientCard*> selable;
	for(auto sc : selectsum_all) {
		sc->is_selectable = false;
		sc->is_selected = false;
		selable.insert(sc);
	}
	select_curval_l = 0;
	select_curval_h = 0;
	for(int i = 0; i < (int)selected_cards.size(); ++i) {
		if(i < must_select_count)
			selected_cards[i]->is_selectable = false;
		else
			selected_cards[i]->is_selectable = true;
		selected_cards[i]->is_selected = true;
		selable.erase(selected_cards[i]);

		int op1 = selected_cards[i]->opParam & 0xffff;
		int op2 = selected_cards[i]->opParam >> 16;
		int opmin = (op2 > 0 && op1 > op2) ? op2 : op1;
		int opmax = op2 > op1 ? op2 : op1;
		select_curval_l += opmin;
		select_curval_h += opmax;
	}
	selectsum_cards.clear();
	if (select_mode == 0) { // sum equal
		bool ret = check_sel_sum_s(selable, 0, select_sumval);
		selectable_cards.clear();
		for(auto sc : selectsum_cards) {
			sc->is_selectable = true;
			selectable_cards.push_back(sc);
		}
		for(auto sc : selected_cards) {
			selectable_cards.push_back(sc);
		}
		return ret;
	} else { // sum greater
		int mm = -1, mx = -1, max = 0, sumc = 0;
		bool ret = false;
		for (auto sc : selected_cards) {
			int op1 = sc->opParam & 0xffff;
			int op2 = sc->opParam >> 16;
			int opmin = (op2 > 0 && op1 > op2) ? op2 : op1;
			int opmax = op2 > op1 ? op2 : op1;
			if (mm == -1 || opmin < mm)
				mm = opmin;
			if (mx == -1 || opmax < mx)
				mx = opmax;
			sumc += opmin;
			max += opmax;
		}
		if (select_sumval <= sumc)
			return true;
		if (select_sumval <= max && select_sumval > max - mx)
			ret = true;
		for(auto sc : selable) {
			int op1 = sc->opParam & 0xffff;
			int op2 = sc->opParam >> 16;
			int m = op1;
			int sums = sumc;
			sums += m;
			int ms = mm;
			if (ms == -1 || m < ms)
				ms = m;
			if (sums >= select_sumval) {
				if (sums - ms < select_sumval)
					selectsum_cards.insert(sc);
			} else {
				std::set<ClientCard*> left(selable);
				left.erase(sc);
				if (check_min(left, left.begin(), select_sumval - sums, select_sumval - sums + ms - 1))
					selectsum_cards.insert(sc);
			}
			if (op2 == 0)
				continue;
			m = op2;
			sums = sumc;
			sums += m;
			ms = mm;
			if (ms == -1 || m < ms)
				ms = m;
			if (sums >= select_sumval) {
				if (sums - ms < select_sumval)
					selectsum_cards.insert(sc);
			} else {
				std::set<ClientCard*> left(selable);
				left.erase(sc);
				if (check_min(left, left.begin(), select_sumval - sums, select_sumval - sums + ms - 1))
					selectsum_cards.insert(sc);
			}
		}
		selectable_cards.clear();
		for(auto sc : selectsum_cards) {
			sc->is_selectable = true;
			selectable_cards.push_back(sc);
		}
		for(auto sc : selected_cards) {
			selectable_cards.push_back(sc);
		}
		return ret;
	}
}
bool ClientField::CheckSelectTribute() {
	std::set<ClientCard*> selable;
	for(auto sit = selectsum_all.begin(); sit != selectsum_all.end(); ++sit) {
		(*sit)->is_selectable = false;
		(*sit)->is_selected = false;
		selable.insert(*sit);
	}
	for(int i = 0; i < (int)selected_cards.size(); ++i) {
		selected_cards[i]->is_selectable = true;
		selected_cards[i]->is_selected = true;
		selable.erase(selected_cards[i]);
	}
	selectsum_cards.clear();
	bool ret = check_sel_sum_trib_s(selable, 0, 0);
	selectable_cards.clear();
	for(auto sit = selectsum_cards.begin(); sit != selectsum_cards.end(); ++sit) {
		(*sit)->is_selectable = true;
		selectable_cards.push_back(*sit);
	}
	return ret;
}
bool ClientField::check_min(const std::set<ClientCard*>& left, std::set<ClientCard*>::const_iterator index, int min, int max) {
	if (index == left.end())
		return false;
	int op1 = (*index)->opParam & 0xffff;
	int op2 = (*index)->opParam >> 16;
	int m = (op2 > 0 && op1 > op2) ? op2 : op1;
	if (m >= min && m <= max)
		return true;
	++index;
	return (min > m && check_min(left, index, min - m, max - m))
	        || check_min(left, index, min, max);
}
bool ClientField::check_sel_sum_s(const std::set<ClientCard*>& left, int index, int acc) {
	if (acc < 0)
		return false;
	if (index == (int)selected_cards.size()) {
		if (acc == 0) {
			int count = selected_cards.size() - must_select_count;
			return count >= select_min && count <= select_max;
		}
		check_sel_sum_t(left, acc);
		return false;
	}
	int l = selected_cards[index]->opParam;
	int l1 = l & 0xffff;
	int l2 = l >> 16;
	bool res1 = false, res2 = false;
	res1 = check_sel_sum_s(left, index + 1, acc - l1);
	if (l2 > 0)
		res2 = check_sel_sum_s(left, index + 1, acc - l2);
	return res1 || res2;
}
void ClientField::check_sel_sum_t(const std::set<ClientCard*>& left, int acc) {
	int count = selected_cards.size() + 1 - must_select_count;
	for (auto sit = left.begin(); sit != left.end(); ++sit) {
		if (selectsum_cards.find(*sit) != selectsum_cards.end())
			continue;
		std::set<ClientCard*> testlist(left);
		testlist.erase(*sit);
		int l = (*sit)->opParam;
		int l1 = l & 0xffff;
		int l2 = l >> 16;
		if (check_sum(testlist.begin(), testlist.end(), acc - l1, count)
		        || (l2 > 0 && check_sum(testlist.begin(), testlist.end(), acc - l2, count))) {
			selectsum_cards.insert(*sit);
		}
	}
}
bool ClientField::check_sum(std::set<ClientCard*>::const_iterator index, std::set<ClientCard*>::const_iterator end, int acc, int count) {
	if (acc == 0)
		return count >= select_min && count <= select_max;
	if (acc < 0 || index == end)
		return false;
	int l = (*index)->opParam;
	int l1 = l & 0xffff;
	int l2 = l >> 16;
	if ((l1 == acc || (l2 > 0 && l2 == acc)) && (count + 1 >= select_min) && (count + 1 <= select_max))
		return true;
	++index;
	return (acc > l1 && check_sum(index, end, acc - l1, count + 1))
	       || (l2 > 0 && acc > l2 && check_sum(index, end, acc - l2, count + 1))
	       || check_sum(index, end, acc, count);
}
bool ClientField::check_sel_sum_trib_s(const std::set<ClientCard*>& left, int index, int acc) {
	if(acc > select_max)
		return false;
	if(index == (int)selected_cards.size()) {
		check_sel_sum_trib_t(left, acc);
		return acc >= select_min && acc <= select_max;
	}
	int l = selected_cards[index]->opParam;
	int l1 = l & 0xffff;
	int l2 = l >> 16;
	bool res1 = false, res2 = false;
	res1 = check_sel_sum_trib_s(left, index + 1, acc + l1);
	if(l2 > 0)
		res2 = check_sel_sum_trib_s(left, index + 1, acc + l2);
	return res1 || res2;
}
void ClientField::check_sel_sum_trib_t(const std::set<ClientCard*>& left, int acc) {
	for(auto sit = left.begin(); sit != left.end(); ++sit) {
		if(selectsum_cards.find(*sit) != selectsum_cards.end())
			continue;
		std::set<ClientCard*> testlist(left);
		testlist.erase(*sit);
		int l = (*sit)->opParam;
		int l1 = l & 0xffff;
		int l2 = l >> 16;
		if(check_sum_trib(testlist.begin(), testlist.end(), acc + l1)
			|| (l2 > 0 && check_sum_trib(testlist.begin(), testlist.end(), acc + l2))) {
			selectsum_cards.insert(*sit);
		}
	}
}
bool ClientField::check_sum_trib(std::set<ClientCard*>::const_iterator index, std::set<ClientCard*>::const_iterator end, int acc) {
	if(acc >= select_min && acc <= select_max)
		return true;
	if(acc > select_max || index == end)
		return false;
	int l = (*index)->opParam;
	int l1 = l & 0xffff;
	int l2 = l >> 16;
	if((acc + l1 >= select_min && acc + l1 <= select_max) || (acc + l2 >= select_min && acc + l2 <= select_max))
		return true;
	++index;
	return check_sum_trib(index, end, acc + l1)
		|| check_sum_trib(index, end, acc + l2)
		|| check_sum_trib(index, end, acc);
}
static bool is_declarable(const CardData& cd, const std::vector<unsigned int>& opcode) {
	std::stack<int> stack;
	for(auto it = opcode.begin(); it != opcode.end(); ++it) {
		switch(*it) {
		case OPCODE_ADD: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs + rhs);
			}
			break;
		}
		case OPCODE_SUB: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs - rhs);
			}
			break;
		}
		case OPCODE_MUL: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs * rhs);
			}
			break;
		}
		case OPCODE_DIV: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(lhs / rhs);
			}
			break;
		}
		case OPCODE_AND: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(static_cast<int>(lhs && rhs));
			}
			break;
		}
		case OPCODE_OR: {
			if (stack.size() >= 2) {
				int rhs = stack.top();
				stack.pop();
				int lhs = stack.top();
				stack.pop();
				stack.push(static_cast<int>(lhs || rhs));
			}
			break;
		}
		case OPCODE_NEG: {
			if (stack.size() >= 1) {
				int val = stack.top();
				stack.pop();
				stack.push(-val);
			}
			break;
		}
		case OPCODE_NOT: {
			if (stack.size() >= 1) {
				int val = stack.top();
				stack.pop();
				stack.push(static_cast<int>(!val));
			}
			break;
		}
		case OPCODE_ISCODE: {
			if (stack.size() >= 1) {
				int code = stack.top();
				stack.pop();
				stack.push(cd.code == code);
			}
			break;
		}
		case OPCODE_ISSETCARD: {
			if (stack.size() >= 1) {
				int set_code = stack.top();
				stack.pop();
				bool res = cd.is_setcode(set_code);
				stack.push(res);
			}
			break;
		}
		case OPCODE_ISTYPE: {
			if (stack.size() >= 1) {
				int val = stack.top();
				stack.pop();
				stack.push(cd.type & val);
			}
			break;
		}
		case OPCODE_ISRACE: {
			if (stack.size() >= 1) {
				int race = stack.top();
				stack.pop();
				stack.push(cd.race & race);
			}
			break;
		}
		case OPCODE_ISATTRIBUTE: {
			if (stack.size() >= 1) {
				int attribute = stack.top();
				stack.pop();
				stack.push(cd.attribute & attribute);
			}
			break;
		}
		default: {
			stack.push(*it);
			break;
		}
		}
	}
	if(stack.size() != 1 || stack.top() == 0)
		return false;
	if (cd.type & TYPE_TOKEN)
		return false;
	return !cd.alias || second_code.find(cd.code) != second_code.end();
}
void ClientField::UpdateDeclarableList() {
	const wchar_t* pname = mainGame->ebANCard->getText();
	int trycode = BufferIO::GetVal(pname);
	CardString cstr;
	CardData cd;
	if(dataManager.GetString(trycode, &cstr) && dataManager.GetData(trycode, &cd) && is_declarable(cd, declare_opcodes)) {
		mainGame->lstANCard->clear();
		ancard.clear();
		mainGame->lstANCard->addItem(cstr.name.c_str());
		ancard.push_back(trycode);
		return;
	}
	if(pname[0] == 0) {
		int sel = mainGame->lstANCard->getSelected();
		trycode = (sel == -1) ? 0 : ancard[sel];
	}
	mainGame->lstANCard->clear();
	ancard.clear();
	for(auto cit = dataManager.strings_begin(); cit != dataManager.strings_end(); ++cit) {
		if(cit->second.name.find(pname) != std::wstring::npos) {
			auto cp = dataManager.GetCodePointer(cit->first);
			if (cp == dataManager.datas_end())
				continue;
			//datas.alias can be double card names or alias
			if(is_declarable(cp->second, declare_opcodes)) {
				if(pname == cit->second.name || trycode == cit->first) { //exact match or last used
					mainGame->lstANCard->insertItem(0, cit->second.name.c_str(), -1);
					ancard.insert(ancard.begin(), cit->first);
				} else {
					mainGame->lstANCard->addItem(cit->second.name.c_str());
					ancard.push_back(cit->first);
				}
			}
		}
	}
}
}
