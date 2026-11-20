#include "game.h"
#include "client_card.h"
#include "materials.h"
#include "image_manager.h"
#include "deck_manager.h"
#include "sound_manager.h"
#include "duelclient.h"
#include "../ocgcore/common.h"
#include "bot_theater.h"
#include <chrono>
#include <cmath>

namespace ygo {

void Game::DrawSelectionLine(irr::video::S3DVertex* vec, bool strip, int width, float* cv) {
	if(!gameConf.use_d3d) {
		float origin[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		glLineWidth(width);
		glLineStipple(1, linePatternGL);
		if(strip)
			glEnable(GL_LINE_STIPPLE);
		glDisable(GL_TEXTURE_2D);
		glMaterialfv(GL_FRONT, GL_AMBIENT, cv);
		glBegin(GL_LINE_LOOP);
		glVertex3fv(&vec[0].Pos.X);
		glVertex3fv(&vec[1].Pos.X);
		glVertex3fv(&vec[3].Pos.X);
		glVertex3fv(&vec[2].Pos.X);
		glEnd();
		glMaterialfv(GL_FRONT, GL_AMBIENT, origin);
		glDisable(GL_LINE_STIPPLE);
		glEnable(GL_TEXTURE_2D);
	} else {
		driver->setMaterial(matManager.mOutLine);
		if(strip) {
			if(linePatternD3D < 15) {
				driver->draw3DLine(vec[0].Pos, vec[0].Pos + (vec[1].Pos - vec[0].Pos) * (linePatternD3D + 1) / 15.0);
				driver->draw3DLine(vec[1].Pos, vec[1].Pos + (vec[3].Pos - vec[1].Pos) * (linePatternD3D + 1) / 15.0);
				driver->draw3DLine(vec[3].Pos, vec[3].Pos + (vec[2].Pos - vec[3].Pos) * (linePatternD3D + 1) / 15.0);
				driver->draw3DLine(vec[2].Pos, vec[2].Pos + (vec[0].Pos - vec[2].Pos) * (linePatternD3D + 1) / 15.0);
			} else {
				driver->draw3DLine(vec[0].Pos + (vec[1].Pos - vec[0].Pos) * (linePatternD3D - 14) / 15.0, vec[1].Pos);
				driver->draw3DLine(vec[1].Pos + (vec[3].Pos - vec[1].Pos) * (linePatternD3D - 14) / 15.0, vec[3].Pos);
				driver->draw3DLine(vec[3].Pos + (vec[2].Pos - vec[3].Pos) * (linePatternD3D - 14) / 15.0, vec[2].Pos);
				driver->draw3DLine(vec[2].Pos + (vec[0].Pos - vec[2].Pos) * (linePatternD3D - 14) / 15.0, vec[0].Pos);
			}
		} else {
			driver->draw3DLine(vec[0].Pos, vec[1].Pos);
			driver->draw3DLine(vec[1].Pos, vec[3].Pos);
			driver->draw3DLine(vec[3].Pos, vec[2].Pos);
			driver->draw3DLine(vec[2].Pos, vec[0].Pos);
		}
	}
}
void Game::DrawSelectionLine(irr::gui::IGUIElement* element, int width, irr::video::SColor color) {
	irr::core::recti pos = element->getAbsolutePosition();
	float x1 = pos.UpperLeftCorner.X;
	float x2 = pos.LowerRightCorner.X;
	float y1 = pos.UpperLeftCorner.Y;
	float y2 = pos.LowerRightCorner.Y;
	float w = pos.getWidth();
	float h = pos.getHeight();
	if(linePatternD3D < 15) {
		driver->draw2DRectangle(color, irr::core::recti(x1 - 1 - width, y1 - 1 - width, x1 + (w * (linePatternD3D + 1) / 15.0) + 1 + width, y1 - 1));
		driver->draw2DRectangle(color, irr::core::recti(x2 - (w * (linePatternD3D + 1) / 15.0) - 1 - width, y2 + 1, x2 + 1 + width, y2 + 1 + width));
		driver->draw2DRectangle(color, irr::core::recti(x1 - 1 - width, y1 - 1 - width, x1 - 1, y2 - (h * (linePatternD3D + 1) / 15.0) + 1 + width));
		driver->draw2DRectangle(color, irr::core::recti(x2 + 1, y1 + (h * (linePatternD3D + 1) / 15.0) - 1 - width, x2 + 1 + width, y2 + 1 + width));
	} else {
		driver->draw2DRectangle(color, irr::core::recti(x1 - 1 - width + (w * (linePatternD3D - 14) / 15.0), y1 - 1 - width, x2 + 1 + width, y1 - 1));
		driver->draw2DRectangle(color, irr::core::recti(x1 - 1 - width, y2 + 1, x2 - (w * (linePatternD3D - 14) / 15.0) + 1 + width, y2 + 1 + width));
		driver->draw2DRectangle(color, irr::core::recti(x1 - 1 - width, y2 - (h * (linePatternD3D - 14) / 15.0) - 1 - width, x1 - 1, y2 + 1 + width));
		driver->draw2DRectangle(color, irr::core::recti(x2 + 1, y1 - 1 - width, x2 + 1 + width, y1 + (h * (linePatternD3D - 14) / 15.0) + 1 + width));
	}
}
void Game::DrawBackGround() {
	static int selFieldAlpha = 255;
	static int selFieldDAlpha = -10;
//	matrix4 im = irr::core::IdentityMatrix;
//	im.setTranslation(irr::core::vector3df(0, 0, -0.01f));
//	driver->setTransform(irr::video::ETS_WORLD, im);
	//dark shade
//	matManager.mSelField.AmbientColor = 0xff000000;
//	matManager.mSelField.DiffuseColor = 0xa0000000;
//	driver->setMaterial(matManager.mSelField);
//	for(int i = 0; i < 120; i += 4)
//		driver->drawVertexPrimitiveList(&matManager.vFields[i], 4, matManager.iRectangle, 2);
//	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
//	driver->setMaterial(matManager.mBackLine);
//	driver->drawVertexPrimitiveList(matManager.vBackLine, 76, matManager.iBackLine, 58, irr::video::EVT_STANDARD, irr::scene::EPT_LINES);
	//draw field
	//draw field spell card
	// Bot Theater: a Field Spell with a backdrop picture (textures/fbg) fills its half of the
	// screen instead of the usual art on the field, over the background and under everything else.
	bool nearBackdrop = false, farBackdrop = false;
	{
		ClientCard* nearField = nullptr;
		ClientCard* farField = nullptr;
		if(gameConf.draw_field_spell) {
			if(dField.szone[0][5] && dField.szone[0][5]->position & POS_FACEUP)
				nearField = dField.szone[0][5];
			if(dField.szone[1][5] && dField.szone[1][5]->position & POS_FACEUP)
				farField = dField.szone[1][5];
		}
		botTheater.DrawFieldBackdrops(nearField, farField, nearBackdrop, farBackdrop);
	}
	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
	bool drawField = nearBackdrop || farBackdrop;	// see-through board, so the backdrops show
	int rule = (dInfo.duel_rule >= 4) ? 1 : 0;
	if(gameConf.draw_field_spell) {
		int fieldcode1 = -1;
		int fieldcode2 = -1;
		if(dField.szone[0][5] && dField.szone[0][5]->position & POS_FACEUP && !nearBackdrop)
			fieldcode1 = dField.szone[0][5]->code;
		if(dField.szone[1][5] && dField.szone[1][5]->position & POS_FACEUP && !farBackdrop)
			fieldcode2 = dField.szone[1][5]->code;
		int fieldcode = (fieldcode1 > 0) ? fieldcode1 : fieldcode2;
		// Bot Theater: the near side's Field Spell art is always turned round (180 degrees) and the
		// far side's is always the right way up. When both sides have one (even the same card),
		// each gets its own half of the field.
		auto drawTurned = [&](const irr::video::S3DVertex* quad) {
			// Same picture area, rotated about its centre: corners 0 and 3 are opposite.
			float u = quad[0].TCoords.X + quad[3].TCoords.X, v = quad[0].TCoords.Y + quad[3].TCoords.Y;
			irr::video::S3DVertex turned[4];
			for(int i = 0; i < 4; ++i) {
				turned[i] = quad[i];
				turned[i].TCoords.set(u - quad[i].TCoords.X, v - quad[i].TCoords.Y);
			}
			driver->drawVertexPrimitiveList(turned, 4, matManager.iRectangle, 2);
		};
		if(fieldcode1 > 0 && fieldcode2 > 0) {
			auto texture = imageManager.GetTextureField(fieldcode1);
			if(texture) {
				drawField = true;
				matManager.mTexture.setTexture(0, texture);
				driver->setMaterial(matManager.mTexture);
				drawTurned(matManager.vFieldSpell1);	// near half, turned round
			}
			texture = imageManager.GetTextureField(fieldcode2);
			if(texture) {
				drawField = true;
				matManager.mTexture.setTexture(0, texture);
				driver->setMaterial(matManager.mTexture);
				drawTurned(matManager.vFieldSpell2);	// far half: the game turns it, so this puts it the right way up
			}
		} else if(fieldcode > 0) {
			auto texture = imageManager.GetTextureField(fieldcode);
			if(texture) {
				drawField = true;
				matManager.mTexture.setTexture(0, texture);
				driver->setMaterial(matManager.mTexture);
				if(fieldcode1 > 0)	// only the near side has one (or the far side has a backdrop: near half only)
					drawTurned(farBackdrop ? matManager.vFieldSpell1 : matManager.vFieldSpell);
				else if(nearBackdrop)
					drawTurned(matManager.vFieldSpell2);	// far half only: the near side has a backdrop
				else
					driver->drawVertexPrimitiveList(matManager.vFieldSpell, 4, matManager.iRectangle, 2);
			}
		}
	}
	if(!botTheater.HideFieldOverlay()) {	// Bot Theater: custom backgrounds don't have TDOANE's built-in board
		matManager.mTexture.setTexture(0, drawField ? imageManager.tFieldTransparent[rule] : imageManager.tField[rule]);
		driver->setMaterial(matManager.mTexture);
		driver->drawVertexPrimitiveList(matManager.vField, 4, matManager.iRectangle, 2);
	}
	botTheater.DrawTurnGlow();	// Bot Theater: behind every card, hologram and piece of the interface
	if(dInfo.isFive) {
		// Bot Theater, five-a-side: a faint gold wash over the column of the player taking the
		// turn (their Monster Zone, Spell & Trap Zone, and Extra Monster Zone in columns 2 and 4).
		int local = botTheater.FiveTurnSide();
		if(local == 0 || local == 1) {
			int side = LocalPlayer(local);
			int col = 4 - dInfo.member5[side];
			irr::video::SMaterial m;
			m.Lighting = false;
			m.ZWriteEnable = false;
			m.MaterialType = irr::video::EMT_TRANSPARENT_VERTEX_ALPHA;
			driver->setMaterial(m);
			auto tint = [&](const irr::video::S3DVertex* quad) {
				irr::video::S3DVertex v[4];
				for(int i = 0; i < 4; ++i) {
					v[i] = quad[i];
					v[i].Color = 0x40ffd050;
				}
				driver->drawVertexPrimitiveList(v, 4, matManager.iRectangle, 2);
			};
			tint(matManager.vFieldMzone[local][col]);
			tint(matManager.vFieldSzone[local][col][rule]);
			if(rule && (col == 1 || col == 3))
				tint(matManager.vFieldMzone[local][col == 1 ? 5 : 6]);
		}
	}
	driver->setMaterial(matManager.mBackLine);
	//select field
	if(dInfo.curMsg == MSG_SELECT_PLACE || dInfo.curMsg == MSG_SELECT_DISFIELD || dInfo.curMsg == MSG_HINT) {
		float cv[4] = {0.0f, 0.0f, 1.0f, 1.0f};
		unsigned int filter = 0x1;
		for (int i = 0; i < 7; ++i, filter <<= 1) {
			if (dField.selectable_field & filter)
				DrawSelectionLine(matManager.vFieldMzone[0][i], !(dField.selected_field & filter), 2, cv);
		}
		filter = 0x100;
		for (int i = 0; i < 8; ++i, filter <<= 1) {
			if (dField.selectable_field & filter)
				DrawSelectionLine(matManager.vFieldSzone[0][i][rule], !(dField.selected_field & filter), 2, cv);
		}
		filter = 0x10000;
		for (int i = 0; i < 7; ++i, filter <<= 1) {
			if (dField.selectable_field & filter)
				DrawSelectionLine(matManager.vFieldMzone[1][i], !(dField.selected_field & filter), 2, cv);
		}
		filter = 0x1000000;
		for (int i = 0; i < 8; ++i, filter <<= 1) {
			if (dField.selectable_field & filter)
				DrawSelectionLine(matManager.vFieldSzone[1][i][rule], !(dField.selected_field & filter), 2, cv);
		}
	}
	//disabled field
	{
		/*float cv[4] = {0.0f, 0.0f, 1.0f, 1.0f};*/
		unsigned int filter = 0x1;
		for (int i = 0; i < 7; ++i, filter <<= 1) {
			if (dField.disabled_field & filter) {
				driver->draw3DLine(matManager.vFieldMzone[0][i][0].Pos, matManager.vFieldMzone[0][i][3].Pos, 0xffffffff);
				driver->draw3DLine(matManager.vFieldMzone[0][i][1].Pos, matManager.vFieldMzone[0][i][2].Pos, 0xffffffff);
			}
		}
		filter = 0x100;
		for (int i = 0; i < 8; ++i, filter <<= 1) {
			if (dField.disabled_field & filter) {
				driver->draw3DLine(matManager.vFieldSzone[0][i][rule][0].Pos, matManager.vFieldSzone[0][i][rule][3].Pos, 0xffffffff);
				driver->draw3DLine(matManager.vFieldSzone[0][i][rule][1].Pos, matManager.vFieldSzone[0][i][rule][2].Pos, 0xffffffff);
			}
		}
		filter = 0x10000;
		for (int i = 0; i < 7; ++i, filter <<= 1) {
			if (dField.disabled_field & filter) {
				driver->draw3DLine(matManager.vFieldMzone[1][i][0].Pos, matManager.vFieldMzone[1][i][3].Pos, 0xffffffff);
				driver->draw3DLine(matManager.vFieldMzone[1][i][1].Pos, matManager.vFieldMzone[1][i][2].Pos, 0xffffffff);
			}
		}
		filter = 0x1000000;
		for (int i = 0; i < 8; ++i, filter <<= 1) {
			if (dField.disabled_field & filter) {
				driver->draw3DLine(matManager.vFieldSzone[1][i][rule][0].Pos, matManager.vFieldSzone[1][i][rule][3].Pos, 0xffffffff);
				driver->draw3DLine(matManager.vFieldSzone[1][i][rule][1].Pos, matManager.vFieldSzone[1][i][rule][2].Pos, 0xffffffff);
			}
		}
	}
	//current sel
	if (dField.hovered_location != 0 && dField.hovered_location != 2 && dField.hovered_location != POSITION_HINT
		&& !(dInfo.duel_rule < 4 && dField.hovered_location == LOCATION_MZONE && dField.hovered_sequence > 4)
		&& !(dInfo.duel_rule >= 4 && dField.hovered_location == LOCATION_SZONE && dField.hovered_sequence > 5)) {
		irr::video::S3DVertex* vertex = 0;
		if (dField.hovered_location == LOCATION_DECK)
			vertex = matManager.vFieldDeck[dField.hovered_controler];
		else if (dField.hovered_location == LOCATION_MZONE) {
			vertex = matManager.vFieldMzone[dField.hovered_controler][dField.hovered_sequence];
			ClientCard* pcard = dField.mzone[dField.hovered_controler][dField.hovered_sequence];
			if(pcard && pcard->type & TYPE_LINK) {
				DrawLinkedZones(pcard);
			}
		} else if (dField.hovered_location == LOCATION_SZONE)
			vertex = matManager.vFieldSzone[dField.hovered_controler][dField.hovered_sequence][rule];
		else if (dField.hovered_location == LOCATION_GRAVE)
			vertex = matManager.vFieldGrave[dField.hovered_controler][rule];
		else if (dField.hovered_location == LOCATION_REMOVED)
			vertex = matManager.vFieldRemove[dField.hovered_controler][rule];
		else if (dField.hovered_location == LOCATION_EXTRA)
			vertex = matManager.vFieldExtra[dField.hovered_controler];
		selFieldAlpha += selFieldDAlpha;
		if (selFieldAlpha <= 5) {
			selFieldAlpha = 5;
			selFieldDAlpha = 10;
		}
		if (selFieldAlpha >= 205) {
			selFieldAlpha = 205;
			selFieldDAlpha = -10;
		}
		matManager.mSelField.AmbientColor = 0xffffffff;
		matManager.mSelField.DiffuseColor = selFieldAlpha << 24;
		driver->setMaterial(matManager.mSelField);
		driver->drawVertexPrimitiveList(vertex, 4, matManager.iRectangle, 2);
	}
}
void Game::DrawLinkedZones(ClientCard* pcard) {
	auto mark = pcard->link_marker;
	ClientCard* pcard2;
	if (dField.hovered_sequence < 5) {
		if (mark & LINK_MARKER_LEFT && dField.hovered_sequence > 0) {
			pcard2 = dField.mzone[dField.hovered_controler][dField.hovered_sequence - 1];
			CheckMutual(pcard2, LINK_MARKER_RIGHT);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[dField.hovered_controler][dField.hovered_sequence - 1], 4, matManager.iRectangle, 2);
		}
		if (mark & LINK_MARKER_RIGHT && dField.hovered_sequence < 4) {
			pcard2 = dField.mzone[dField.hovered_controler][dField.hovered_sequence + 1];
			CheckMutual(pcard2, LINK_MARKER_LEFT);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[dField.hovered_controler][dField.hovered_sequence + 1], 4, matManager.iRectangle, 2);
		}
		if (dInfo.duel_rule >= 4) {
			if ((mark & LINK_MARKER_TOP_LEFT && dField.hovered_sequence == 2)
				|| (mark & LINK_MARKER_TOP && dField.hovered_sequence == 1)
				|| (mark & LINK_MARKER_TOP_RIGHT && dField.hovered_sequence == 0)) {
				int arrow = (dField.hovered_sequence == 2) ? LINK_MARKER_BOTTOM_RIGHT : (dField.hovered_sequence == 1) ? LINK_MARKER_BOTTOM : LINK_MARKER_BOTTOM_LEFT;
				pcard2 = dField.mzone[dField.hovered_controler][5];
				if (!pcard2) {
					pcard2 = dField.mzone[1 - dField.hovered_controler][6];
					arrow = (dField.hovered_sequence == 2) ? LINK_MARKER_TOP_LEFT : (dField.hovered_sequence == 1) ? LINK_MARKER_TOP : LINK_MARKER_TOP_RIGHT;
				}
				CheckMutual(pcard2, arrow);
				driver->drawVertexPrimitiveList(&matManager.vFieldMzone[dField.hovered_controler][5], 4, matManager.iRectangle, 2);
			}
			if ((mark & LINK_MARKER_TOP_LEFT && dField.hovered_sequence == 4)
				|| (mark & LINK_MARKER_TOP && dField.hovered_sequence == 3)
				|| (mark & LINK_MARKER_TOP_RIGHT && dField.hovered_sequence == 2)) {
				int arrow = (dField.hovered_sequence == 4) ? LINK_MARKER_BOTTOM_RIGHT : (dField.hovered_sequence == 3) ? LINK_MARKER_BOTTOM : LINK_MARKER_BOTTOM_LEFT;
				pcard2 = dField.mzone[dField.hovered_controler][6];
				if (!pcard2) {
					pcard2 = dField.mzone[1 - dField.hovered_controler][5];
					arrow = (dField.hovered_sequence == 4) ? LINK_MARKER_TOP_LEFT : (dField.hovered_sequence == 3) ? LINK_MARKER_TOP : LINK_MARKER_TOP_RIGHT;
				}
				CheckMutual(pcard2, arrow);
				driver->drawVertexPrimitiveList(&matManager.vFieldMzone[dField.hovered_controler][6], 4, matManager.iRectangle, 2);
			}
		}
	} else {
		int swap = (dField.hovered_sequence == 5) ? 0 : 2;
		if (mark & LINK_MARKER_BOTTOM_LEFT) {
			pcard2 = dField.mzone[dField.hovered_controler][0 + swap];
			CheckMutual(pcard2, LINK_MARKER_TOP_RIGHT);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[dField.hovered_controler][0 + swap], 4, matManager.iRectangle, 2);
		}
		if (mark & LINK_MARKER_BOTTOM) {
			pcard2 = dField.mzone[dField.hovered_controler][1 + swap];
			CheckMutual(pcard2, LINK_MARKER_TOP);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[dField.hovered_controler][1 + swap], 4, matManager.iRectangle, 2);
		}
		if (mark & LINK_MARKER_BOTTOM_RIGHT) {
			pcard2 = dField.mzone[dField.hovered_controler][2 + swap];
			CheckMutual(pcard2, LINK_MARKER_TOP_LEFT);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[dField.hovered_controler][2 + swap], 4, matManager.iRectangle, 2);
		}
		if (mark & LINK_MARKER_TOP_LEFT) {
			pcard2 = dField.mzone[1 - dField.hovered_controler][4 - swap];
			CheckMutual(pcard2, LINK_MARKER_TOP_LEFT);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[1 - dField.hovered_controler][4 - swap], 4, matManager.iRectangle, 2);
		}
		if (mark & LINK_MARKER_TOP) {
			pcard2 = dField.mzone[1 - dField.hovered_controler][3 - swap];
			CheckMutual(pcard2, LINK_MARKER_TOP);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[1 - dField.hovered_controler][3 - swap], 4, matManager.iRectangle, 2);
		}
		if (mark & LINK_MARKER_TOP_RIGHT) {
			pcard2 = dField.mzone[1 - dField.hovered_controler][2 - swap];
			CheckMutual(pcard2, LINK_MARKER_TOP_RIGHT);
			driver->drawVertexPrimitiveList(&matManager.vFieldMzone[1 - dField.hovered_controler][2 - swap], 4, matManager.iRectangle, 2);
		}
	}
}
void Game::CheckMutual(ClientCard* pcard, int mark) {
	matManager.mSelField.AmbientColor = 0xff0261a2;
	driver->setMaterial(matManager.mSelField);
	if (pcard && pcard->type & TYPE_LINK && pcard->link_marker & mark) {
		matManager.mSelField.AmbientColor = 0xff009900;
		driver->setMaterial(matManager.mSelField);
	}
}
void Game::DrawCards() {
	for(int p = 0; p < dInfo.players(); ++p) {
		for(auto it = dField.mzone[p].begin(); it != dField.mzone[p].end(); ++it)
			if(*it)
				DrawCard(*it);
		for(auto it = dField.szone[p].begin(); it != dField.szone[p].end(); ++it)
			if(*it)
				DrawCard(*it);
		for(auto it = dField.deck[p].begin(); it != dField.deck[p].end(); ++it)
			DrawCard(*it);
		for(auto it = dField.hand[p].begin(); it != dField.hand[p].end(); ++it)
			DrawCard(*it);
		for(auto it = dField.grave[p].begin(); it != dField.grave[p].end(); ++it)
			DrawCard(*it);
		for(auto it = dField.remove[p].begin(); it != dField.remove[p].end(); ++it)
			DrawCard(*it);
		for(auto it = dField.extra[p].begin(); it != dField.extra[p].end(); ++it)
			DrawCard(*it);
	}
	for (auto cit = dField.overlay_cards.begin(); cit != dField.overlay_cards.end(); ++cit) {
		auto pcard = (*cit);
		auto olcard = pcard->overlayTarget;
		if (pcard->aniFrame) {
			DrawCard(pcard);
		}
		else if (olcard && olcard->location == LOCATION_MZONE) {
			if (pcard->sequence < MAX_LAYER_COUNT) {
				DrawCard(pcard);
			}
		}
		else {
			DrawCard(pcard);
		}
	}
}
void Game::DrawCard(ClientCard* pcard) {
	if(pcard->aniFrame) {
		if(pcard->is_moving) {
			pcard->curPos += pcard->dPos;
			pcard->curRot += pcard->dRot;
			pcard->mTransform.setTranslation(pcard->curPos);
			pcard->mTransform.setRotationRadians(pcard->curRot);
		}
		if(pcard->is_fading)
			pcard->curAlpha += pcard->dAlpha;
		pcard->aniFrame--;
		if(pcard->aniFrame == 0) {
			pcard->is_moving = false;
			pcard->is_fading = false;
		}
	}
	matManager.mCard.AmbientColor = 0xffffffff;
	matManager.mCard.DiffuseColor = (pcard->curAlpha << 24) | 0xffffff;
	driver->setTransform(irr::video::ETS_WORLD, pcard->mTransform);
	auto m22 = pcard->mTransform(2, 2);
	if(m22 > -0.99 || pcard->is_moving) {
		matManager.mCard.setTexture(0, imageManager.GetTexture(pcard->code));
		driver->setMaterial(matManager.mCard);
		driver->drawVertexPrimitiveList(matManager.vCardFront, 4, matManager.iRectangle, 2);
	}
	if(m22 < 0.99 || pcard->is_moving) {
		matManager.mCard.setTexture(0, imageManager.tCover[pcard->controler]);
		driver->setMaterial(matManager.mCard);
		driver->drawVertexPrimitiveList(matManager.vCardBack, 4, matManager.iRectangle, 2);
	}
	if(pcard->is_moving)
		return;
	if(pcard->is_selectable && (pcard->location & 0xe)) {
		float cv[4] = {1.0f, 1.0f, 0.0f, 1.0f};
		if((pcard->location == LOCATION_HAND && pcard->code) || ((pcard->location & 0xc) && (pcard->position & POS_FACEUP)))
			DrawSelectionLine(matManager.vCardOutline, !pcard->is_selected, 2, cv);
		else
			DrawSelectionLine(matManager.vCardOutliner, !pcard->is_selected, 2, cv);
	}
	if(pcard->is_highlighting) {
		float cv[4] = {0.0f, 1.0f, 1.0f, 1.0f};
		if((pcard->location == LOCATION_HAND && pcard->code) || ((pcard->location & 0xc) && (pcard->position & POS_FACEUP)))
			DrawSelectionLine(matManager.vCardOutline, true, 2, cv);
		else
			DrawSelectionLine(matManager.vCardOutliner, true, 2, cv);
	}
	irr::core::matrix4 im;
	im.setTranslation(pcard->curPos);
	driver->setTransform(irr::video::ETS_WORLD, im);
	if(pcard->is_showequip) {
		matManager.mTexture.setTexture(0, imageManager.tEquip);
		driver->setMaterial(matManager.mTexture);
		driver->drawVertexPrimitiveList(matManager.vSymbol, 4, matManager.iRectangle, 2);
	} else if(pcard->is_showtarget) {
		matManager.mTexture.setTexture(0, imageManager.tTarget);
		driver->setMaterial(matManager.mTexture);
		driver->drawVertexPrimitiveList(matManager.vSymbol, 4, matManager.iRectangle, 2);
	} else if(pcard->is_showchaintarget) {
		matManager.mTexture.setTexture(0, imageManager.tChainTarget);
		driver->setMaterial(matManager.mTexture);
		driver->drawVertexPrimitiveList(matManager.vSymbol, 4, matManager.iRectangle, 2);
	} else if((pcard->status & (STATUS_DISABLED | STATUS_FORBIDDEN))
		&& (pcard->location & LOCATION_ONFIELD) && (pcard->position & POS_FACEUP)) {
		matManager.mTexture.setTexture(0, imageManager.tNegated);
		driver->setMaterial(matManager.mTexture);
		driver->drawVertexPrimitiveList(matManager.vNegate, 4, matManager.iRectangle, 2);
	}
	if(pcard->cmdFlag & COMMAND_ATTACK) {
		matManager.mTexture.setTexture(0, imageManager.tAttack);
		driver->setMaterial(matManager.mTexture);
		irr::core::matrix4 atk;
		atk.setTranslation(pcard->curPos + irr::core::vector3df(0, (pcard->controler == 0 ? -1 : 1) * (atkdy / 4.0f + 0.35f), 0.05f));
		atk.setRotationRadians(irr::core::vector3df(0, 0, pcard->controler == 0 ? 0 : 3.1415926f));
		driver->setTransform(irr::video::ETS_WORLD, atk);
		driver->drawVertexPrimitiveList(matManager.vSymbol, 4, matManager.iRectangle, 2);
	}
}
template<typename T>
void Game::DrawShadowText(irr::gui::CGUITTFont* font, const T& text, const irr::core::rect<irr::s32>& position, const irr::core::rect<irr::s32>& padding,
			irr::video::SColor color, irr::video::SColor shadowcolor, bool hcenter, bool vcenter, const irr::core::rect<irr::s32>* clip) {
	irr::core::rect<irr::s32> shadowposition = irr::core::recti(position.UpperLeftCorner.X - padding.UpperLeftCorner.X, position.UpperLeftCorner.Y - padding.UpperLeftCorner.Y, 
										   position.LowerRightCorner.X - padding.LowerRightCorner.X, position.LowerRightCorner.Y - padding.LowerRightCorner.Y);
	font->drawUstring(text, shadowposition, shadowcolor, hcenter, vcenter, clip);
	font->drawUstring(text, position, color, hcenter, vcenter, clip);
}
void Game::DrawMisc() {
	static irr::core::vector3df act_rot(0, 0, 0);
	int rule = (dInfo.duel_rule >= 4) ? 1 : 0;
	irr::core::matrix4 im, ic, it, ig;
	act_rot.Z += 0.02f;
	im.setRotationRadians(act_rot);
	matManager.mTexture.setTexture(0, imageManager.tAct);
	driver->setMaterial(matManager.mTexture);
	for(int player = 0; player < 2; ++player) {
		if(dField.deck_act[player]) {
			im.setTranslation(irr::core::vector3df((matManager.vFieldDeck[player][0].Pos.X + matManager.vFieldDeck[player][1].Pos.X) / 2,
				(matManager.vFieldDeck[player][0].Pos.Y + matManager.vFieldDeck[player][2].Pos.Y) / 2, dField.deck[player].size() * 0.01f + 0.02f));
			driver->setTransform(irr::video::ETS_WORLD, im);
			driver->drawVertexPrimitiveList(matManager.vActivate, 4, matManager.iRectangle, 2);
		}
		if(dField.grave_act[player]) {
			im.setTranslation(irr::core::vector3df((matManager.vFieldGrave[player][rule][0].Pos.X + matManager.vFieldGrave[player][rule][1].Pos.X) / 2,
				(matManager.vFieldGrave[player][rule][0].Pos.Y + matManager.vFieldGrave[player][rule][2].Pos.Y) / 2, dField.grave[player].size() * 0.01f + 0.02f));
			driver->setTransform(irr::video::ETS_WORLD, im);
			driver->drawVertexPrimitiveList(matManager.vActivate, 4, matManager.iRectangle, 2);
		}
		if(dField.remove_act[player]) {
			im.setTranslation(irr::core::vector3df((matManager.vFieldRemove[player][rule][0].Pos.X + matManager.vFieldRemove[player][rule][1].Pos.X) / 2,
				(matManager.vFieldRemove[player][rule][0].Pos.Y + matManager.vFieldRemove[player][rule][2].Pos.Y) / 2, dField.remove[player].size() * 0.01f + 0.02f));
			driver->setTransform(irr::video::ETS_WORLD, im);
			driver->drawVertexPrimitiveList(matManager.vActivate, 4, matManager.iRectangle, 2);
		}
		if(dField.extra_act[player]) {
			im.setTranslation(irr::core::vector3df((matManager.vFieldExtra[player][0].Pos.X + matManager.vFieldExtra[player][1].Pos.X) / 2,
				(matManager.vFieldExtra[player][0].Pos.Y + matManager.vFieldExtra[player][2].Pos.Y) / 2, dField.extra[player].size() * 0.01f + 0.02f));
			driver->setTransform(irr::video::ETS_WORLD, im);
			driver->drawVertexPrimitiveList(matManager.vActivate, 4, matManager.iRectangle, 2);
		}
		if(dField.pzone_act[player]) {
			int seq = dInfo.duel_rule >= 4 ? 0 : 6;
			im.setTranslation(irr::core::vector3df((matManager.vFieldSzone[player][seq][rule][0].Pos.X + matManager.vFieldSzone[player][seq][rule][1].Pos.X) / 2,
				(matManager.vFieldSzone[player][seq][rule][0].Pos.Y + matManager.vFieldSzone[player][seq][rule][2].Pos.Y) / 2, 0.03f));
			driver->setTransform(irr::video::ETS_WORLD, im);
			driver->drawVertexPrimitiveList(matManager.vActivate, 4, matManager.iRectangle, 2);
		}
	}
	if(dField.conti_act) {
		irr::core::vector3df pos = irr::core::vector3df((matManager.vFieldContiAct[0].X + matManager.vFieldContiAct[1].X) / 2,
			(matManager.vFieldContiAct[0].Y + matManager.vFieldContiAct[2].Y) / 2, 0);
		im.setRotationRadians(irr::core::vector3df(0, 0, 0));
		for(auto cit = dField.conti_cards.begin(); cit != dField.conti_cards.end(); ++cit) {
			im.setTranslation(pos);
			driver->setTransform(irr::video::ETS_WORLD, im);
			matManager.mCard.setTexture(0, imageManager.GetTexture((*cit)->code));
			driver->setMaterial(matManager.mCard);
			driver->drawVertexPrimitiveList(matManager.vCardFront, 4, matManager.iRectangle, 2);
			pos.Z += 0.03f;
		}
		im.setTranslation(pos);
		im.setRotationRadians(act_rot);
		driver->setTransform(irr::video::ETS_WORLD, im);
		driver->setMaterial(matManager.mTexture);
		driver->drawVertexPrimitiveList(matManager.vActivate, 4, matManager.iRectangle, 2);
	}
	if(dField.chains.size() > 1 || mainGame->gameConf.draw_single_chain) {
		for(size_t i = 0; i < dField.chains.size(); ++i) {
			if(dField.chains[i].solved)
				break;
			matManager.mTRTexture.setTexture(0, imageManager.tChain);
			matManager.mTRTexture.AmbientColor = 0xffffff00;
			ic.setRotationRadians(act_rot);
			ic.setTranslation(dField.chains[i].chain_pos);
			driver->setMaterial(matManager.mTRTexture);
			driver->setTransform(irr::video::ETS_WORLD, ic);
			driver->drawVertexPrimitiveList(matManager.vSymbol, 4, matManager.iRectangle, 2);
			it.setScale(0.6f);
			it.setTranslation(dField.chains[i].chain_pos);
			matManager.mTRTexture.setTexture(0, imageManager.tNumber);
			matManager.vChainNum[0].TCoords = irr::core::vector2df(0.19375f * (i % 5), 0.2421875f * (i / 5));
			matManager.vChainNum[1].TCoords = irr::core::vector2df(0.19375f * (i % 5 + 1), 0.2421875f * (i / 5));
			matManager.vChainNum[2].TCoords = irr::core::vector2df(0.19375f * (i % 5), 0.2421875f * (i / 5 + 1));
			matManager.vChainNum[3].TCoords = irr::core::vector2df(0.19375f * (i % 5 + 1), 0.2421875f * (i / 5 + 1));
			driver->setMaterial(matManager.mTRTexture);
			driver->setTransform(irr::video::ETS_WORLD, it);
			driver->drawVertexPrimitiveList(matManager.vChainNum, 4, matManager.iRectangle, 2);
		}
	}
	if(dField.cant_check_grave) {
		matManager.mTexture.setTexture(0, imageManager.tNegated);
		driver->setMaterial(matManager.mTexture);
		ig.setTranslation(irr::core::vector3df((matManager.vFieldGrave[0][rule][0].Pos.X + matManager.vFieldGrave[0][rule][1].Pos.X) / 2,
			(matManager.vFieldGrave[0][rule][0].Pos.Y + matManager.vFieldGrave[0][rule][2].Pos.Y) / 2, dField.grave[0].size() * 0.01f + 0.02f));
		driver->setTransform(irr::video::ETS_WORLD, ig);
		driver->drawVertexPrimitiveList(matManager.vNegate, 4, matManager.iRectangle, 2);
		ig.setTranslation(irr::core::vector3df((matManager.vFieldGrave[1][rule][0].Pos.X + matManager.vFieldGrave[1][rule][1].Pos.X) / 2,
			(matManager.vFieldGrave[1][rule][0].Pos.Y + matManager.vFieldGrave[1][rule][2].Pos.Y) / 2, dField.grave[1].size() * 0.01f + 0.02f));
		driver->setTransform(irr::video::ETS_WORLD, ig);
		driver->drawVertexPrimitiveList(matManager.vNegate, 4, matManager.iRectangle, 2);
	}

	// Bot Theater: cards' art floating above the field. Drawn here so ATK/DEF, Levels,
	// names, Life Points and avatars all appear on top of the holograms.
	botTheater.DrawHolograms();

	DrawAvatars();

	//finish button
	if(btnCancelOrFinish->isVisible() && dField.select_ready)
		DrawSelectionLine(btnCancelOrFinish, 2, 0xffffff00);
	if(btnLeaveGame->isVisible() && dField.tag_teammate_surrender)
		DrawSelectionLine(btnLeaveGame, 2, 0xffffff00);
	//lp bar
	/*if((dInfo.turn % 2 && dInfo.isFirst) || (!(dInfo.turn % 2) && !dInfo.isFirst)) {
		driver->draw2DRectangle(0xa0000000, Resize(327, 8, 630, 51));
		driver->draw2DRectangleOutline(Resize(327, 8, 630, 51), 0xffff8080);
	} else {
		driver->draw2DRectangle(0xa0000000, Resize(689, 8, 991, 51));
		driver->draw2DRectangleOutline(Resize(689, 8, 991, 51), 0xffff8080);
	}*/
	driver->draw2DImage(imageManager.tLPFrame, Resize(330, 10, 629, 30), irr::core::recti(0, 0, 200, 20), 0, 0, true);
	driver->draw2DImage(imageManager.tLPFrame, Resize(691, 10, 990, 30), irr::core::recti(0, 0, 200, 20), 0, 0, true);
	if(dInfo.start_lp) {
		auto maxLP = dInfo.isTag ? dInfo.start_lp / 2 : dInfo.start_lp;
		if(dInfo.lp[0] >= maxLP) {
			auto layerCount = dInfo.lp[0] / maxLP;
			auto partialLP = dInfo.lp[0] % maxLP;
			auto bgColorPos = (layerCount - 1) % 5;
			auto fgColorPos = layerCount % 5; 
			driver->draw2DImage(imageManager.tLPBar, Resize(335 + 290 * partialLP / maxLP, 12, 625, 28), irr::core::recti(0, bgColorPos * 16, 16, (bgColorPos + 1) * 16), 0, 0, true);
			if(partialLP > 0) {
				driver->draw2DImage(imageManager.tLPBar, Resize(335, 12, 335 + 290 * partialLP / maxLP, 28), irr::core::recti(0, fgColorPos * 16, 16, (fgColorPos + 1) * 16), 0, 0, true);
			}
		}
		else driver->draw2DImage(imageManager.tLPBar, Resize(335, 12, 335 + 290 * dInfo.lp[0] / maxLP, 28), irr::core::recti(0, 0, 16, 16), 0, 0, true);
		if(dInfo.lp[1] >= maxLP) {
			auto layerCount = dInfo.lp[1] / maxLP;
			auto partialLP = dInfo.lp[1] % maxLP;
			auto bgColorPos = (layerCount - 1) % 5;
			auto fgColorPos = layerCount % 5;
			driver->draw2DImage(imageManager.tLPBar, Resize(696, 12, 986 - 290 * partialLP / maxLP, 28), irr::core::recti(0, bgColorPos * 16, 16, (bgColorPos + 1) * 16), 0, 0, true);
			if(partialLP > 0) {
				driver->draw2DImage(imageManager.tLPBar, Resize(986 - 290 * partialLP / maxLP, 12, 986, 28), irr::core::recti(0, fgColorPos * 16, 16, (fgColorPos + 1) * 16), 0, 0, true);
			}
		}
		else driver->draw2DImage(imageManager.tLPBar, Resize(986 - 290 * dInfo.lp[1] / maxLP, 12, 986, 28), irr::core::recti(0, 0, 16, 16), 0, 0, true);
	}
	if(lpframe) {
		dInfo.lp[lpplayer] -= lpd;
		myswprintf(dInfo.strLP[lpplayer], L"%d", dInfo.lp[lpplayer]);
		lpccolor -= 0x19000000;
		lpframe--;
	}
	if(lpcstring.size()) {
		if(lpplayer == 0) {
			DrawShadowText(lpcFont, lpcstring, ResizeF(400, 472, 922, 520), Resize(0, 2, 2, 0), lpccolor, lpccolor | 0x00ffffff, true, false, 0);
		} else {
			DrawShadowText(lpcFont, lpcstring, ResizeF(400, 162, 922, 210), Resize(0, 2, 2, 0), lpccolor, lpccolor | 0x00ffffff, true, false, 0);
		}
	}

	DrawBorders();

	if(!dInfo.isReplay && dInfo.player_type < 7 && dInfo.time_limit) {
		driver->draw2DRectangle(Resize(525, 34, 525 + dInfo.time_left[0] * 100 / dInfo.time_limit, 44), 0xa0e0e0e0, 0xa0e0e0e0, 0xa0c0c0c0, 0xa0c0c0c0);
		driver->draw2DRectangleOutline(Resize(525, 34, 625, 44), 0xffffffff);
		driver->draw2DRectangle(Resize(795 - dInfo.time_left[1] * 100 / dInfo.time_limit, 34, 795, 44), 0xa0e0e0e0, 0xa0e0e0e0, 0xa0c0c0c0, 0xa0c0c0c0);
		driver->draw2DRectangleOutline(Resize(695, 34, 795, 44), 0xffffffff);
	}

	DrawRanks();

	DrawShadowText(numFont, dInfo.strLP[0], Resize(330, 12, 631, 30), Resize(0, 1, 2, 0), mainGame->playerlpcolor, 0xff000000, true, false, 0);
	DrawShadowText(numFont, dInfo.strLP[1], Resize(691, 12, 992, 30), Resize(0, 1, 2, 0), mainGame->playerlpcolor, 0xff000000, true, false, 0);

	if(!gameConf.hide_player_name && !botTheater.TagPortraits()) {	// Bot Theater: tag portraits have their own names
		irr::core::recti p1size = Resize(416, 31, 815, 50);
		irr::core::recti p2size = Resize(905, 31, 906, 50);
		if(!dInfo.isTag || !dInfo.tag_player[0])
			textFont->drawUstring(dInfo.hostname, p1size, mainGame->usernamecolor, false, false, 0);
		else
			textFont->drawUstring(dInfo.hostname_tag, p1size, mainGame->usernamecolor, false, false, 0);
		if(!dInfo.isTag || !dInfo.tag_player[1]) {
			auto cld = textFont->getDimension(dInfo.clientname);
			p2size.UpperLeftCorner.X -= cld.Width;
			textFont->drawUstring(dInfo.clientname, p2size, mainGame->usernamecolor, false, false, 0);
		} else {
			auto cld = textFont->getDimension(dInfo.clientname_tag);
			p2size.UpperLeftCorner.X -= cld.Width;
			textFont->drawUstring(dInfo.clientname_tag, p2size, mainGame->usernamecolor, false, false, 0);
		}
	}
	driver->draw2DRectangle(Resize(632, 10, 688, 30), 0x00000000, 0x00000000, 0xffffffff, 0xffffffff);
	driver->draw2DRectangle(Resize(632, 30, 688, 50), 0xffffffff, 0xffffffff, 0x00000000, 0x00000000);
	DrawShadowText(lpcFont, dataManager.GetNumString(dInfo.turn), Resize(635, 5, 687, 40), Resize(0, 0, 2, 0), mainGame->turncolor, 0x80000000, true, false, 0);
	ClientCard* pcard;
	// Bot Theater: with holograms on, the near side's ATK/DEF sits just above the middle of
	// the card, in the gap between the monsters' holograms and the back row's, instead of
	// at the card's front edge where the back-row holograms cover it.
	bool raiseStats = botTheater.RaiseNearStats();
	for(int i = 0; i < 5; ++i) {
		pcard = dField.mzone[0][i];
		if(pcard && pcard->code != 0) {
			if(raiseStats)
				DrawStatus(pcard, 502 + i * 81, 373, 473 + i * 80, 356);
			else
				DrawStatus(pcard, 493 + i * 85, 416, 473 + i * 80, 356);
		}
	}
	pcard = dField.mzone[0][5];
	if(pcard && pcard->code != 0)
		DrawStatus(pcard, 589, 338, 563, 291);
	pcard = dField.mzone[0][6];
	if(pcard && pcard->code != 0)
		DrawStatus(pcard, 743, 338, 712, 291);
	for(int i = 0; i < 5; ++i) {
		pcard = dField.mzone[1][i];
		if(pcard && (pcard->position & POS_FACEUP))
			DrawStatus(pcard, 803 - i * 68, 235, 779 - i * 71, 272);
	}
	pcard = dField.mzone[1][5];
	if(pcard && (pcard->position & POS_FACEUP))
		DrawStatus(pcard, 739, 291, 710, 338);
	pcard = dField.mzone[1][6];
	if(pcard && (pcard->position & POS_FACEUP))
		DrawStatus(pcard, 593, 291, 555, 338);
	if(dInfo.duel_rule < 4) {
		pcard = dField.szone[0][6];
		if(pcard) {
			DrawShadowText(adFont, pcard->lscstring, ResizeF(427, 395, 439, 415), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		pcard = dField.szone[0][7];
		if(pcard) {
			DrawShadowText(adFont, pcard->rscstring, ResizeF(881, 395, 913, 415), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		pcard = dField.szone[1][6];
		if(pcard) {
			DrawShadowText(adFont, pcard->lscstring, ResizeF(840, 246, 872, 266), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		pcard = dField.szone[1][7];
		if(pcard) {
			DrawShadowText(adFont, pcard->rscstring, ResizeF(464, 246, 496, 266), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
	} else {
		pcard = dField.szone[0][0];
		if(pcard && (pcard->type & TYPE_PENDULUM) && !pcard->equipTarget) {
			DrawShadowText(adFont, pcard->lscstring, ResizeF(455, 431, 467, 451), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		pcard = dField.szone[0][4];
		if(pcard && (pcard->type & TYPE_PENDULUM) && !pcard->equipTarget) {
			DrawShadowText(adFont, pcard->rscstring, ResizeF(851, 431, 883, 451), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		pcard = dField.szone[1][0];
		if(pcard && (pcard->type & TYPE_PENDULUM) && !pcard->equipTarget) {
			DrawShadowText(adFont, pcard->lscstring, ResizeF(807, 223, 839, 243), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		pcard = dField.szone[1][4];
		if(pcard && (pcard->type & TYPE_PENDULUM) && !pcard->equipTarget) {
			DrawShadowText(adFont, pcard->rscstring, ResizeF(499, 223, 531, 243), Resize(1, 1, 1, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
	}
	if(dField.extra[0].size()) {
		int offset = (dField.extra[0].size() >= 10) ? 0 : numFont->getDimension(dataManager.GetNumString(1)).Width;
		DrawShadowText(numFont, dataManager.GetNumString(dField.extra[0].size()), ResizeF(320, 563, 373, 553, offset, 0, 0, 0), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		DrawShadowText(numFont, dataManager.GetNumString(dField.extra_p_count[0], true), ResizeF(340, 563, 393, 553), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
	}
	if(dField.deck[0].size()) {
		DrawShadowText(numFont, dataManager.GetNumString(dField.deck[0].size()), ResizeF(908, 563, 1023, 553), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
	}
	if (rule == 0) {
		if (dField.grave[0].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.grave[0].size()), ResizeF(837, 376, 986, 381), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		if (dField.remove[0].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.remove[0].size()), ResizeF(1015, 376, 959, 381), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
	} else {
		if (dField.grave[0].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.grave[0].size()), ResizeF(870, 457, 1004, 462), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		if (dField.remove[0].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.remove[0].size()), ResizeF(837, 376, 986, 381), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
	}
	if(dField.extra[1].size()) {
		int offset = (dField.extra[1].size() >= 10) ? 0 : numFont->getDimension(dataManager.GetNumString(1)).Width;
		DrawShadowText(numFont, dataManager.GetNumString(dField.extra[1].size()), ResizeF(808, 208, 900, 233, offset, 0, 0, 0), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		DrawShadowText(numFont, dataManager.GetNumString(dField.extra_p_count[1], true), ResizeF(828, 208, 920, 233), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
	}
	if(dField.deck[1].size()) {
		DrawShadowText(numFont, dataManager.GetNumString(dField.deck[1].size()), ResizeF(465, 208, 483, 233), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
	}
	if (rule == 0) {
		if (dField.grave[1].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.grave[1].size()), ResizeF(420, 311, 464, 282), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		if (dField.remove[1].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.remove[1].size()), ResizeF(300, 311, 445, 341), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
	} else {
		if (dField.grave[1].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.grave[1].size()), ResizeF(455, 250, 464, 300), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
		if (dField.remove[1].size()) {
			DrawShadowText(numFont, dataManager.GetNumString(dField.remove[1].size()), ResizeF(420, 311, 464, 282), Resize(0, 1, 2, 1), mainGame->extracolor, 0xff000000, true, false, 0);
		}
	}
}
void Game::DrawStatus(ClientCard* pcard, int x1, int y1, int x2, int y2) {
	DrawShadowText(adFont, L"/", ResizeF(x1 - 3, y1 + 1, x1 + 5, y1 + 21), Resize(1, 1, 1, 1), mainGame->statcolor, 0xff000000, true, false, 0);
	int w = adFont->getDimension(pcard->atkstring).Width;
	DrawShadowText(adFont, pcard->atkstring, ResizeF(x1 - 4, y1 + 1, x1 - 4, y1 + 21, -w, 0, 0, 0), Resize(1, 1, 1, 1),
		pcard->attack > pcard->base_attack ? mainGame->bonuscolor : pcard->attack < pcard->base_attack ? mainGame->negativecolor : mainGame->statcolor, 0xff000000);
	if(pcard->type & TYPE_LINK) {
		w = adFont->getDimension(pcard->linkstring).Width;
		DrawShadowText(adFont, pcard->linkstring, ResizeF(x1 + 5, y1 + 1, x1 + 5, y1 + 21, 0, 0, w, 0), Resize(1, 1, 1, 1), 0xff99ffff);
	} else {
		w = adFont->getDimension(pcard->defstring).Width;
		DrawShadowText(adFont, pcard->defstring, ResizeF(x1 + 5, y1 + 1, x1 + 5, y1 + 21, 0, 0, w, 0), Resize(1, 1, 1, 1),
			pcard->defense > pcard->base_defense ? mainGame->bonuscolor : pcard->defense < pcard->base_defense ? mainGame->negativecolor : mainGame->statcolor);
		DrawShadowText(adFont, pcard->lvstring, ResizeF(x2 + 1, y2, x2 + 3, y2 + 21), Resize(1, 1, 1, 1),
			(pcard->type & TYPE_XYZ) ? 0xffff80ff : (pcard->type & TYPE_TUNER) ? mainGame->bonuscolor : mainGame->statcolor);
	}
}
void Game::DrawGUI() {
	while (imageLoading.size()) {
		auto mit = imageLoading.cbegin();
		mit->first->setImage(imageManager.GetTexture(mit->second));
		imageLoading.erase(mit);
	}
	for(auto fit = fadingList.begin(); fit != fadingList.end();) {
		auto fthis = fit++;
		FadingUnit& fu = *fthis;
		if(fu.fadingFrame) {
			fu.guiFading->setVisible(true);
			if(fu.isFadein) {
				if(fu.fadingFrame > 5) {
					fu.fadingUL.X -= fu.fadingDiff.X;
					fu.fadingLR.X += fu.fadingDiff.X;
					fu.fadingFrame--;
					fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				} else {
					fu.fadingUL.Y -= fu.fadingDiff.Y;
					fu.fadingLR.Y += fu.fadingDiff.Y;
					fu.fadingFrame--;
					if(!fu.fadingFrame) {
						fu.guiFading->setRelativePosition(fu.fadingSize);
						if(fu.guiFading == wPosSelect) {
							btnPSAU->setDrawImage(true);
							btnPSAD->setDrawImage(true);
							btnPSDU->setDrawImage(true);
							btnPSDD->setDrawImage(true);
						}
						if(fu.guiFading == wCardSelect) {
							for(int i = 0; i < 5; ++i)
								btnCardSelect[i]->setDrawImage(true);
						}
						if(fu.guiFading == wCardDisplay) {
							for(int i = 0; i < 5; ++i)
								btnCardDisplay[i]->setDrawImage(true);
						}
						env->setFocus(fu.guiFading);
					} else
						fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				}
			} else {
				if(fu.fadingFrame > 5) {
					fu.fadingUL.Y += fu.fadingDiff.Y;
					fu.fadingLR.Y -= fu.fadingDiff.Y;
					fu.fadingFrame--;
					fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				} else {
					fu.fadingUL.X += fu.fadingDiff.X;
					fu.fadingLR.X -= fu.fadingDiff.X;
					fu.fadingFrame--;
					if(!fu.fadingFrame) {
						fu.guiFading->setVisible(false);
						fu.guiFading->setRelativePosition(fu.fadingSize);
						if(fu.guiFading == wPosSelect) {
							btnPSAU->setDrawImage(true);
							btnPSAD->setDrawImage(true);
							btnPSDU->setDrawImage(true);
							btnPSDD->setDrawImage(true);
						}
						if(fu.guiFading == wCardSelect) {
							for(int i = 0; i < 5; ++i)
								btnCardSelect[i]->setDrawImage(true);
						}
						if(fu.guiFading == wCardDisplay) {
							for(int i = 0; i < 5; ++i)
								btnCardDisplay[i]->setDrawImage(true);
						}
					} else
						fu.guiFading->setRelativePosition(irr::core::recti(fu.fadingUL, fu.fadingLR));
				}
				if(fu.signalAction && !fu.fadingFrame) {
					DuelClient::SendResponse();
					fu.signalAction = false;
				}
			}
		} else if(fu.autoFadeoutFrame) {
			fu.autoFadeoutFrame--;
			if(!fu.autoFadeoutFrame)
				HideElement(fu.guiFading);
		} else
			fadingList.erase(fthis);
	}
	env->drawAll();
	botTheater.DrawInfoBorder();
}
void Game::DrawSpec() {
	irr::s32 midx = 574 + (CARD_IMG_WIDTH * 0.5);
	irr::s32 midy = 150 + (CARD_IMG_HEIGHT * 0.5);
	if(showcard) {
		switch(showcard) {
		case 1: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tMask, ResizeCardMid(574, 150, 574 + (showcarddif > CARD_IMG_WIDTH ? CARD_IMG_WIDTH : showcarddif), 150 + CARD_IMG_HEIGHT, midx, midy),
								irr::core::recti(CARD_IMG_HEIGHT - showcarddif, 0, CARD_IMG_HEIGHT - (showcarddif > CARD_IMG_WIDTH ? showcarddif - CARD_IMG_WIDTH : 0), CARD_IMG_HEIGHT), 0, 0, true);
			showcarddif += 15;
			if(showcarddif >= CARD_IMG_HEIGHT) {
				showcard = 2;
				showcarddif = 0;
			}
			break;
		}
		case 2: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tMask, ResizeCardMid(574 + showcarddif, 150, 574 + CARD_IMG_WIDTH, 150 + CARD_IMG_HEIGHT, midx, midy),
								irr::core::recti(0, 0, CARD_IMG_WIDTH - showcarddif, CARD_IMG_HEIGHT), 0, 0, true);
			showcarddif += 15;
			if(showcarddif >= CARD_IMG_WIDTH) {
				showcard = 0;
			}
			break;
		}
		case 3: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tNegated, ResizeCardMid(536 + showcarddif, 141 + showcarddif, 792 - showcarddif, 397 - showcarddif, midx, midy), irr::core::recti(0, 0, 128, 128), 0, 0, true);
			if(showcarddif < 64)
				showcarddif += 4;
			break;
		}
		case 4: {
			matManager.c2d[0] = (showcarddif << 24) | 0xffffff;
			matManager.c2d[1] = (showcarddif << 24) | 0xffffff;
			matManager.c2d[2] = (showcarddif << 24) | 0xffffff;
			matManager.c2d[3] = (showcarddif << 24) | 0xffffff;
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150, 574 + CARD_IMG_WIDTH, 150 + CARD_IMG_HEIGHT),
								ResizeFit(0, 0, CARD_IMG_WIDTH, CARD_IMG_HEIGHT), 0, matManager.c2d, true);
			if(showcarddif < 255)
				showcarddif += 17;
			break;
		}
		case 5: {
			matManager.c2d[0] = (showcarddif << 25) | 0xffffff;
			matManager.c2d[1] = (showcarddif << 25) | 0xffffff;
			matManager.c2d[2] = (showcarddif << 25) | 0xffffff;
			matManager.c2d[3] = (showcarddif << 25) | 0xffffff;
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardMid(662 - showcarddif * 0.69685f, 277 - showcarddif, 662 + showcarddif * 0.69685f, 277 + showcarddif, midx, midy),
								ResizeFit(0, 0, CARD_IMG_WIDTH, CARD_IMG_HEIGHT), 0, matManager.c2d, true);
			if(showcarddif < 127)
				showcarddif += 9;
			break;
		}
		case 6: {
			driver->draw2DImage(imageManager.GetTexture(showcardcode, true), ResizeCardHint(574, 150));
			driver->draw2DImage(imageManager.tNumber, ResizeCardMid(536 + showcarddif, 141 + showcarddif, 792 - showcarddif, 397 - showcarddif, midx, midy),
			                    irr::core::recti((showcardp % 5) * 64, (showcardp / 5) * 64, (showcardp % 5 + 1) * 64, (showcardp / 5 + 1) * 64), 0, 0, true);
			if(showcarddif < 64)
				showcarddif += 4;
			break;
		}
		case 7: {
			float mul = xScale;
			if(xScale > yScale)
				mul = yScale;
			irr::core::vector2d<irr::s32> corner[4];
			float y = sin(showcarddif * 3.1415926f / 180.0f) * CARD_IMG_HEIGHT * mul;
			irr::s32 winx = midx * xScale + (574 - midx) * mul;
			irr::s32 winx2 = midx * xScale + (751 - midx) * mul;
			irr::s32 winy = midy * yScale + (404 - midy) * mul;
			corner[0] = irr::core::vector2d<irr::s32>(winx - (CARD_IMG_HEIGHT * mul - y) * 0.3f, winy - y);
			corner[1] = irr::core::vector2d<irr::s32>(winx2 + (CARD_IMG_HEIGHT * mul - y) * 0.3f, winy - y);
			corner[2] = irr::core::vector2d<irr::s32>(winx, winy);
			corner[3] = irr::core::vector2d<irr::s32>(winx2, winy);
			irr::gui::Draw2DImageQuad(driver, imageManager.GetTexture(showcardcode, true), ResizeFit(0, 0, CARD_IMG_WIDTH, CARD_IMG_HEIGHT), corner);
			showcardp++;
			showcarddif += 9;
			if(showcarddif >= 90)
				showcarddif = 90;
			if(showcardp == 60) {
				showcardp = 0;
				showcarddif = 0;
			}
			break;
		}
		case 100: {
			if(showcardp < 60) {
				driver->draw2DImage(imageManager.tHand[(showcardcode >> 16) & 0x3], irr::core::vector2di((615 + 44.5) * xScale - 44.5, (showcarddif + 64) * yScale - 64));
				driver->draw2DImage(imageManager.tHand[showcardcode & 0x3], irr::core::vector2di((615 + 44.5) * xScale - 44.5, (540 - showcarddif + 64) * yScale - 64));
				float dy = -0.333333f * showcardp + 10;
				showcardp++;
				if(showcardp < 30)
					showcarddif += (int)dy;
			} else
				showcard = 0;
			break;
		}
		case 101: {
			const wchar_t* lstr = L"";
			switch(showcardcode) {
			case 1:
				lstr = L"You Win!";
				break;
			case 2:
				lstr = L"You Lose!";
				break;
			case 3:
				lstr = L"Draw Game";
				break;
			case 4:
				lstr = L"Draw Phase";
				break;
			case 5:
				lstr = L"Standby Phase";
				break;
			case 6:
				lstr = L"Main Phase 1";
				break;
			case 7:
				lstr = L"Battle Phase";
				break;
			case 8:
				lstr = L"Main Phase 2";
				break;
			case 9:
				lstr = L"End Phase";
				break;
			case 10:
				lstr = botTheater.TurnText();	// Bot Theater: "<name>'s Turn"
				if(!lstr)
					lstr = L"Next Players Turn";
				break;
			case 11:
				lstr = L"Duel Start";
				break;
			case 12:
				lstr = L"Duel1 Start";
				break;
			case 13:
				lstr = L"Duel2 Start";
				break;
			case 14:
				lstr = L"Duel3 Start";
				break;
			}
			auto pos = lpcFont->getDimension(lstr);
			if(showcardp < 10) {
				int alpha = (showcardp * 25) << 24;
				DrawShadowText(lpcFont, lstr, ResizePhaseHint(660 - (9 - showcardp) * 40, 290, 960, 370, pos.Width), Resize(-1, -1, 0, 0), alpha | 0xffffff, alpha);
			} else if(showcardp < showcarddif) {
				DrawShadowText(lpcFont, lstr, ResizePhaseHint(660, 290, 960, 370, pos.Width), Resize(-1, -1, 0, 0), 0xffffffff);
				if(dInfo.vic_string.size() && (showcardcode == 1 || showcardcode == 2)) {
					int w = guiFont->getDimension(dInfo.vic_string).Width;
					if(w < 200)
						w = 200;
					driver->draw2DRectangle(0xa0000000, ResizeWin(640 - w / 2, 320, 690 + w / 2, 340));
					DrawShadowText(guiFont, dInfo.vic_string, ResizeWin(640 - w / 2, 320, 690 + w / 2, 340), Resize(-2, -1, 0, 0), 0xffffffff, 0xff000000, true, true, 0);
				}
			} else if(showcardp < showcarddif + 10) {
				int alpha = ((showcarddif + 10 - showcardp) * 25) << 24;
				DrawShadowText(lpcFont, lstr, ResizePhaseHint(660 + (showcardp - showcarddif) * 40, 290, 960, 370, pos.Width), Resize(-1, -1, 0, 0), alpha | 0xffffff, alpha);
			}
			showcardp++;
			break;
		}
		}
	}
	if(is_attacking) {
		irr::core::matrix4 matk;
		matk.setTranslation(atk_t);
		matk.setRotationRadians(atk_r);
		driver->setTransform(irr::video::ETS_WORLD, matk);
		driver->setMaterial(matManager.mATK);
		driver->drawVertexPrimitiveList(&matManager.vArrow[attack_sv], 12, matManager.iArrow, 10, irr::video::EVT_STANDARD, irr::scene::EPT_TRIANGLE_STRIP);
		attack_sv += 4;
		if (attack_sv > 28)
			attack_sv = 0;
	}
	bool showChat = true;
	if(hideChat) {
	    showChat = false;
	    hideChatTimer = 10;
	} else if(hideChatTimer > 0) {
	    showChat = false;
	    hideChatTimer--;
	}
	int chatRectY = 0;
	// Bot Theater, four players: no chat on the duel screen; the players' lines come up in
	// speech bubbles instead (DrawFourBubbles).
	bool fourNoChat = dInfo.isFourPlayer && dInfo.isStarted && !is_building;
	for(int i = 0; i < 8; ++i) {
		static unsigned int chatColor[] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xff8080ff, 0xffff4040, 0xffff4040,
		                                   0xffff4040, 0xff40ff40, 0xff4040ff, 0xff40ffff, 0xffff40ff, 0xffffff40, 0xffffffff, 0xff808080, 0xff404040};
		if(chatTiming[i]) {
			chatTiming[i]--;
			if(fourNoChat)
				continue;
			if(!is_building) {
				if(dInfo.isStarted && i >= 5)
					continue;
				if(!showChat && i > 2)
					continue;
			}

			int x = wChat->getRelativePosition().UpperLeftCorner.X;
			int y = window_size.Height - 25;
			int maxwidth = 705 * xScale;
			if(fivePanel)	// Bot Theater, five-a-side: keep clear of the right-hand portraits
				maxwidth = std::min(maxwidth, (int)((1023 - FIVE_COLUMN_WIDTH - 6) * xScale) - x);
			if(is_building) {
				x = 810 * xScale;
				maxwidth = 205 * xScale;
			}

			std::wstring msg = SetStaticText(nullptr, maxwidth, guiFont, chatMsg[i].c_str());
			int w = guiFont->getDimension(msg).Width;
			int h = guiFont->getDimension(msg).Height + 2;

			irr::core::recti rectloc(x, y - chatRectY - h, x + 2 + w, y - chatRectY);
			irr::core::recti msgloc(x, y - chatRectY - h, x - 4, y - chatRectY);
			irr::core::recti shadowloc = msgloc + irr::core::vector2di(1, 1);

			driver->draw2DRectangle(rectloc, 0xa0000000, 0xa0000000, 0xa0000000, 0xa0000000);
			guiFont->drawUstring(msg, msgloc, 0xff000000, false, false);
			guiFont->drawUstring(msg, shadowloc, chatColor[chatType[i]], false, false);

			chatRectY += h;
		}
	}
}
void Game::DrawBackImage(irr::video::ITexture* texture) {
	if(!texture)
		return;
	driver->draw2DImage(texture, Resize(0, 0, 1024, 640), irr::core::recti(0, 0, texture->getOriginalSize().Width, texture->getOriginalSize().Height));
}
void Game::ShowElement(irr::gui::IGUIElement * win, int autoframe) {
	FadingUnit fu;
	fu.fadingSize = win->getRelativePosition();
	for(auto fit = fadingList.begin(); fit != fadingList.end(); ++fit)
		if(win == fit->guiFading && win != wOptions && win != wANNumber) // the size of wOptions is always setted by ClientField::ShowSelectOption before showing it
			fu.fadingSize = fit->fadingSize;
	irr::core::vector2di center = fu.fadingSize.getCenter();
	fu.fadingDiff.X = fu.fadingSize.getWidth() / 10;
	fu.fadingDiff.Y = (fu.fadingSize.getHeight() - 4) / 10;
	fu.fadingUL = center;
	fu.fadingLR = center;
	fu.fadingUL.Y -= 2;
	fu.fadingLR.Y += 2;
	fu.guiFading = win;
	fu.isFadein = true;
	fu.fadingFrame = 10;
	fu.autoFadeoutFrame = autoframe;
	fu.signalAction = 0;
	if(win == wPosSelect) {
		btnPSAU->setDrawImage(false);
		btnPSAD->setDrawImage(false);
		btnPSDU->setDrawImage(false);
		btnPSDD->setDrawImage(false);
	}
	if(win == wCardSelect) {
		for(int i = 0; i < 5; ++i)
			btnCardSelect[i]->setDrawImage(false);
	}
	if(win == wCardDisplay) {
		for(int i = 0; i < 5; ++i)
			btnCardDisplay[i]->setDrawImage(false);
	}
	win->setRelativePosition(irr::core::recti(center.X, center.Y, 0, 0));
	win->setVisible(true);
	fadingList.push_back(fu);
}
void Game::HideElement(irr::gui::IGUIElement * win, bool set_action) {
	if(!win->isVisible() && !set_action)
		return;
	FadingUnit fu;
	fu.fadingSize = win->getRelativePosition();
	for(auto fit = fadingList.begin(); fit != fadingList.end(); ++fit)
		if(win == fit->guiFading)
			fu.fadingSize = fit->fadingSize;
	fu.fadingDiff.X = fu.fadingSize.getWidth() / 10;
	fu.fadingDiff.Y = (fu.fadingSize.getHeight() - 4) / 10;
	fu.fadingUL = fu.fadingSize.UpperLeftCorner;
	fu.fadingLR = fu.fadingSize.LowerRightCorner;
	fu.guiFading = win;
	fu.isFadein = false;
	fu.fadingFrame = 10;
	fu.autoFadeoutFrame = 0;
	fu.signalAction = set_action;
	if(win == wPosSelect) {
		btnPSAU->setDrawImage(false);
		btnPSAD->setDrawImage(false);
		btnPSDU->setDrawImage(false);
		btnPSDD->setDrawImage(false);
	}
	if(win == wCardSelect) {
		for(int i = 0; i < 5; ++i)
			btnCardSelect[i]->setDrawImage(false);
		dField.conti_selecting = false;
		stCardListTip->setVisible(false);
		for(auto& pcard : dField.selectable_cards)
			dField.SetShowMark(pcard, false);
	}
	if(win == wCardDisplay) {
		for(int i = 0; i < 5; ++i)
			btnCardDisplay[i]->setDrawImage(false);
		stCardListTip->setVisible(false);
		for(auto& pcard : dField.display_cards)
			dField.SetShowMark(pcard, false);
	}
	fadingList.push_back(fu);
}
void Game::PopupElement(irr::gui::IGUIElement * element, int hideframe) {
	soundManager.PlayDialogSound(element);
	element->getParent()->bringToFront(element);
	if(!is_building)
		dField.panel = element;
	env->setFocus(element);
	if(!hideframe)
		ShowElement(element);
	else ShowElement(element, hideframe);
}
void Game::WaitFrameSignal(int frame) {
	frameSignal.Reset();
	signalFrame = (gameConf.quick_animation && frame >= 12) ? 12 : frame;
	frameSignal.Wait();
}
void Game::DrawThumb(code_pointer cp, irr::core::vector2di pos, const LFList* lflist, bool drag) {
	int code = cp->first;
	auto lcode = cp->second.alias;
	if(lcode == 0)
		lcode = code;
	irr::video::ITexture* img = imageManager.GetTextureThumb(code);
	if(img == nullptr)
		return; //nullptr->getSize() will cause a crash
	irr::core::dimension2d<irr::u32> size = img->getOriginalSize();
	irr::core::recti dragloc = mainGame->Resize(pos.X, pos.Y, pos.X + CARD_THUMB_WIDTH, pos.Y + CARD_THUMB_HEIGHT);
	irr::core::recti limitloc = mainGame->Resize(pos.X, pos.Y, pos.X + 20, pos.Y + 20);
	irr::core::recti otloc = Resize(pos.X + 7, pos.Y + 50, pos.X + 37, pos.Y + 65);
	if(drag) {
		dragloc = irr::core::recti(pos.X, pos.Y, pos.X + CARD_THUMB_WIDTH * mainGame->xScale, pos.Y + CARD_THUMB_HEIGHT * mainGame->yScale);
		limitloc = irr::core::recti(pos.X, pos.Y, pos.X + 20 * mainGame->xScale, pos.Y + 20 * mainGame->yScale);
		otloc = irr::core::recti(pos.X + 7, pos.Y + 50 * mainGame->yScale, pos.X + 37 * mainGame->xScale, pos.Y + 65 * mainGame->yScale);
	}
	driver->draw2DImage(img, dragloc, irr::core::rect<irr::s32>(0, 0, size.Width, size.Height));
	auto lfit = lflist->content.find(lcode);
	if (lfit != lflist->content.end()) {
		switch(lfit->second) {
		case 0:
			driver->draw2DImage(imageManager.tLim, limitloc, irr::core::recti(0, 0, 64, 64), 0, 0, true);
			break;
		case 1:
			driver->draw2DImage(imageManager.tLim, limitloc, irr::core::recti(64, 0, 128, 64), 0, 0, true);
			break;
		case 2:
			driver->draw2DImage(imageManager.tLim, limitloc, irr::core::recti(0, 64, 64, 128), 0, 0, true);
			break;
		}
	}
	bool showAvail = false;
	bool showNotAvail = false;
	int filter_lm = cbLimit->getSelected();
	bool avail = !((filter_lm == 4 && !(cp->second.ot & AVAIL_OCG)
				|| (filter_lm == 5 && !(cp->second.ot & AVAIL_TCG))
				|| (filter_lm == 6 && !(cp->second.ot & AVAIL_SC))
				|| (filter_lm == 7 && !(cp->second.ot & AVAIL_CUSTOM))
				|| (filter_lm == 8 && (cp->second.ot & AVAIL_OCGTCG) != AVAIL_OCGTCG)));
	if(filter_lm >= 4) {
		showAvail = avail;
		showNotAvail = !avail;
	} else if(!(cp->second.ot & gameConf.defaultOT)) {
		showNotAvail = true;
	}
	if(showAvail) {
		if((cp->second.ot & AVAIL_OCG) && !(cp->second.ot & AVAIL_TCG))
			driver->draw2DImage(imageManager.tOT, otloc, irr::core::recti(0, 128, 128, 192), 0, 0, true);
		else if((cp->second.ot & AVAIL_TCG) && !(cp->second.ot & AVAIL_OCG))
			driver->draw2DImage(imageManager.tOT, otloc, irr::core::recti(0, 192, 128, 256), 0, 0, true);
	} else if(showNotAvail) {
		if(cp->second.ot & AVAIL_OCG)
			driver->draw2DImage(imageManager.tOT, otloc, irr::core::recti(0, 0, 128, 64), 0, 0, true);
		else if(cp->second.ot & AVAIL_TCG)
			driver->draw2DImage(imageManager.tOT, otloc, irr::core::recti(0, 64, 128, 128), 0, 0, true);
		else if(!avail)
			driver->draw2DImage(imageManager.tLim, otloc, irr::core::recti(0, 0, 64, 64), 0, 0, true);
	}
}
void Game::DrawDeckBd() {
	wchar_t textBuffer[64];
	//main deck
	int mainsize = deckManager.current_deck.main.size();
	driver->draw2DRectangle(Resize(310, 137, 410, 157), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
	driver->draw2DRectangleOutline(Resize(309, 136, 410, 157));
	DrawShadowText(textFont, dataManager.GetSysString(deckBuilder.showing_pack ? 1477 : 1330), Resize(315, 137, 410, 157), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
	DrawShadowText(numFont, dataManager.GetNumString(mainsize), Resize(380, 138, 440, 158), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
	driver->draw2DRectangle(Resize(310, 160, 797, deckBuilder.showing_pack ? 630 : 436), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
	driver->draw2DRectangleOutline(Resize(309, 159, 797, deckBuilder.showing_pack ? 630 : 436));
	int lx;
	int dy = 68;
	float dx;
	if(mainsize <= 40) {
		dx = 436.0f / 9;
		lx = 10;
	} else if(deckBuilder.showing_pack) {
		lx = 10;
		if(mainsize > 10 * 7)
			lx = 11;
		if(mainsize > 11 * 7)
			lx = 12;
		dx = (mainGame->scrPackCards->isVisible() ? 414.0f : 436.0f) / (lx - 1);
		if(mainsize > 60)
			dy = 66;
	} else {
		lx = (mainsize - 41) / 4 + 11;
		dx = 436.0f / (lx - 1);
	}
	int padding = scrPackCards->getPos() * lx;
	for(int i = 0; i < mainsize - padding && i < 7 * lx; ++i) {
		int j = i + padding;
		DrawThumb(deckManager.current_deck.main[j], irr::core::vector2di(314 + (i % lx) * dx, 164 + (i / lx) * dy), deckBuilder.filterList);
		if(deckBuilder.hovered_pos == 1 && deckBuilder.hovered_seq == j)
			driver->draw2DRectangleOutline(Resize(313 + (i % lx) * dx, 163 + (i / lx) * dy, 359 + (i % lx) * dx, 228 + (i / lx) * dy));
	}
	if(!deckBuilder.showing_pack) {
		//extra deck
		driver->draw2DRectangle(Resize(310, 440, 410, 460), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 439, 410, 460));
		DrawShadowText(textFont, dataManager.GetSysString(1331), Resize(315, 440, 410, 460), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		DrawShadowText(numFont, dataManager.GetNumString(deckManager.current_deck.extra.size()), Resize(380, 441, 440, 461), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		driver->draw2DRectangle(Resize(310, 463, 797, 533), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 462, 797, 533));
		if(deckManager.current_deck.extra.size() <= 10)
			dx = 436.0f / 9;
		else dx = 436.0f / (deckManager.current_deck.extra.size() - 1);
		for(size_t i = 0; i < deckManager.current_deck.extra.size(); ++i) {
			DrawThumb(deckManager.current_deck.extra[i], irr::core::vector2di(314 + i * dx, 466), deckBuilder.filterList);
			if(deckBuilder.hovered_pos == 2 && deckBuilder.hovered_seq == (int)i)
				driver->draw2DRectangleOutline(Resize(313 + i * dx, 465, 359 + i * dx, 531));
		}
		//side deck
		driver->draw2DRectangle(Resize(310, 537, 410, 557), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 536, 410, 557));
		DrawShadowText(textFont, dataManager.GetSysString(1332), Resize(315, 537, 410, 557), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		DrawShadowText(numFont, dataManager.GetNumString(deckManager.current_deck.side.size()), Resize(380, 538, 440, 558), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		driver->draw2DRectangle(Resize(310, 560, 797, 630), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(309, 559, 797, 630));
		if(deckManager.current_deck.side.size() <= 10)
			dx = 436.0f / 9;
		else dx = 436.0f / (deckManager.current_deck.side.size() - 1);
		for(size_t i = 0; i < deckManager.current_deck.side.size(); ++i) {
			DrawThumb(deckManager.current_deck.side[i], irr::core::vector2di(314 + i * dx, 564), deckBuilder.filterList);
			if(deckBuilder.hovered_pos == 3 && deckBuilder.hovered_seq == (int)i)
				driver->draw2DRectangleOutline(Resize(313 + i * dx, 563, 359 + i * dx, 629));
		}
	}
	if(is_siding) {
		// side chat background
		driver->draw2DRectangle(Resize(805, 10, 1020, 630), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(804, 9, 1020, 630));
	} else {
		//search result
		driver->draw2DRectangle(Resize(805, 137, 926, 157), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(804, 136, 926, 157));
		DrawShadowText(textFont, dataManager.GetSysString(1333), Resize(810, 137, 915, 157), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		DrawShadowText(numFont, deckBuilder.result_string, Resize(875, 137, 935, 157), Resize(1, 1, 1, 1), 0xffffffff, 0xff000000, false, true);
		driver->draw2DRectangle(Resize(805, 160, 1020, 630), 0x400000ff, 0x400000ff, 0x40000000, 0x40000000);
		driver->draw2DRectangleOutline(Resize(804, 159, 1020, 630));
	}
	for(int i = 0; i < 9 && i + scrFilter->getPos() < (int)deckBuilder.results.size(); ++i) {
		code_pointer ptr = deckBuilder.results[i + scrFilter->getPos()];
		if(i >= 7)
		{
			imageManager.GetTextureThumb(ptr->second.code);
			break;
		}
		if(deckBuilder.hovered_pos == 4 && deckBuilder.hovered_seq == (int)i)
			driver->draw2DRectangle(0x80000000, Resize(806, 164 + i * 66, 1019, 230 + i * 66));
		DrawThumb(ptr, irr::core::vector2di(810, 165 + i * 66), deckBuilder.filterList);
		const wchar_t* availBuffer = L"";
		if ((ptr->second.ot & AVAIL_OCGTCG) == AVAIL_OCG)
			availBuffer = L" [OCG]";
		else if ((ptr->second.ot & AVAIL_OCGTCG) == AVAIL_TCG)
			availBuffer = L" [TCG]";
		else if ((ptr->second.ot & AVAIL_CUSTOM) == AVAIL_CUSTOM)
			availBuffer = L" [Custom]";
		if(ptr->second.type & TYPE_MONSTER) {
			myswprintf(textBuffer, L"%ls", dataManager.GetName(ptr->first));
			DrawShadowText(textFont, textBuffer, Resize(860, 165 + i * 66, 955, 185 + i * 66), Resize(1, 1, 0, 0));
			const wchar_t* form = L"\u2605";
			wchar_t adBuffer[32]{};
			wchar_t scaleBuffer[16]{};
			if(!(ptr->second.type & TYPE_LINK)) {
				if(ptr->second.type & TYPE_XYZ)
					form = L"\u2606";
				if(ptr->second.attack < 0 && ptr->second.defense < 0)
					myswprintf(adBuffer, L"?/?");
				else if(ptr->second.attack < 0)
					myswprintf(adBuffer, L"?/%d", ptr->second.defense);
				else if(ptr->second.defense < 0)
					myswprintf(adBuffer, L"%d/?", ptr->second.attack);
				else
					myswprintf(adBuffer, L"%d/%d", ptr->second.attack, ptr->second.defense);
			} else {
				form = L"LINK-";
				if(ptr->second.attack < 0)
					myswprintf(adBuffer, L"?/-");
				else
					myswprintf(adBuffer, L"%d/-", ptr->second.attack);
			}
			myswprintf(textBuffer, L"%ls/%ls %ls%d", dataManager.FormatAttribute(ptr->second.attribute).c_str(), dataManager.FormatRace(ptr->second.race).c_str(),
				form, ptr->second.level);
			DrawShadowText(textFont, textBuffer, Resize(860, 187 + i * 66, 955, 207 + i * 66), Resize(1, 1, 0, 0));
			if(ptr->second.type & TYPE_PENDULUM) {
				myswprintf(scaleBuffer, L" %d/%d", ptr->second.lscale, ptr->second.rscale);
			}
			myswprintf(textBuffer, L"%ls%ls%ls", adBuffer, scaleBuffer, availBuffer);
			DrawShadowText(textFont, textBuffer, Resize(860, 209 + i * 66, 955, 229 + i * 66), Resize(1, 1, 0, 0));
		} else {
			myswprintf(textBuffer, L"%ls", dataManager.GetName(ptr->first));
			DrawShadowText(textFont, textBuffer, Resize(860, 165 + i * 66, 955, 185 + i * 66), Resize(1, 1, 0, 0));
			myswprintf(textBuffer, L"%ls", dataManager.FormatType(ptr->second.type).c_str());
			DrawShadowText(textFont, textBuffer, Resize(860, 187 + i * 66, 955, 207 + i * 66), Resize(1, 1, 0, 0));
			myswprintf(textBuffer, L"%ls", availBuffer);
			DrawShadowText(textFont, textBuffer, Resize(860, 209 + i * 66, 955, 229 + i * 66), Resize(1, 1, 0, 0));
		}
	}
	if(deckBuilder.is_draging) {
		DrawThumb(deckBuilder.draging_pointer, irr::core::vector2di(deckBuilder.dragx - CARD_THUMB_WIDTH / 2 * mainGame->xScale, deckBuilder.dragy - CARD_THUMB_HEIGHT / 2 * mainGame->yScale), deckBuilder.filterList, true);
	}
}
static void DrawPlayerAvatar(irr::video::IVideoDriver* driver, irr::core::vector2di pos, int player, bool right)
{
	irr::video::ITexture* avatar = imageManager.tAvatar[player];
	if (avatar)
	{
		irr::core::recti sourceRect(irr::core::dimension2di(0, 0), avatar->getSize());
		irr::core::position2di maxSize = mainGame->Resize(69, 69);
		irr::core::dimension2di textureSize(maxSize.X, maxSize.Y);
		if (right)
			pos.X -= textureSize.Width;
		irr::core::recti destRect(pos, pos + textureSize);
		driver->draw2DImage(avatar, destRect, sourceRect, NULL, NULL, true);
	}
}
static void DrawPlayerRank(irr::video::IVideoDriver* driver, irr::core::vector2di pos, int player, bool right)
{
	irr::video::ITexture* rank = imageManager.tRank[player];
	if (rank)
	{
		irr::core::recti sourceRect(irr::core::dimension2di(0, 0), rank->getSize());
		irr::core::position2di maxSize = mainGame->Resize(42, 42);
		irr::core::dimension2di textureSize(maxSize.X, maxSize.Y);
		if (right)
			pos.X -= textureSize.Width;
		irr::core::recti destRect(pos, pos + textureSize);
		driver->draw2DImage(rank, destRect, sourceRect, NULL, NULL, true);
	}
}
static void DrawPlayerBorder(irr::video::IVideoDriver* driver, irr::core::recti pos, int player)
{
	irr::video::ITexture* border = imageManager.tBorder[player];
	if (border)
	{
		irr::core::recti sourceRect(irr::core::dimension2di(0, 0), border->getSize());
		driver->draw2DImage(border, pos, sourceRect, NULL, NULL, true);
	}
}
// Bot Theater, five-a-side: the bottom team down the left of the field (just right of the card
// text panel) and the top team down the right-hand edge of the window. Each column of portraits
// runs, top to bottom, in the order of the field's columns from left to right as they look on
// screen. A team's member m has its own column 4 - m (from the team's own left; the top team
// faces us, so its columns are mirrored), so the bottom team reads members 4, 3, 2, 1, 0 from the
// top and the top team 0, 1, 2, 3, 4. The gold frame marks whoever's turn it is; the member
// answering for the other team (the last of them to play) has a white edge on their name.
void Game::DrawFivePortraits()
{
	int top = (int)(FIVE_COLUMN_TOP * yScale);
	int bottom = (int)window_size.Height - 2;
	int slot = (bottom - top) / 5;
	if (slot <= 0)
		return;
	int name_h = (int)textFont->getDimension(L"Ag").Height + 4;
	int turn_side = botTheater.FiveTurnSide();
	for (int local = 0; local < 2; ++local)
	{
		int side = LocalPlayer(local);	// the duel's numbering
		int x0 = (int)((local == 0 ? FIVE_COLUMN_LEFT : 1023 - FIVE_COLUMN_WIDTH) * xScale);
		int x1 = (int)((local == 0 ? FIVE_COLUMN_LEFT + FIVE_COLUMN_WIDTH : 1023) * xScale);
		int w = x1 - x0;
		for (int row = 0; row < 5; ++row)
		{
			int member = (local == 0) ? 4 - row : row;
			int place = side * 5 + member;
			int y0 = top + row * slot;
			int a = std::min(w - 6, slot - name_h - 8);	// the picture: square, as big as the slot allows
			if (a < 8)
				continue;
			int ax = x0 + (w - a) / 2, ay = y0 + 3;
			irr::video::ITexture* avatar = imageManager.tAvatar[place];
			if (avatar)
				driver->draw2DImage(avatar, irr::core::recti(ax, ay, ax + a, ay + a),
					irr::core::recti(irr::core::dimension2di(0, 0), avatar->getSize()), nullptr, nullptr, true);
			else	// no picture: a plain square, so the place still shows
			{
				driver->draw2DRectangle(0x90000000, irr::core::recti(ax, ay, ax + a, ay + a));
				driver->draw2DRectangleOutline(irr::core::recti(ax, ay, ax + a, ay + a), 0x60ffffff);
			}
			bool playing = dInfo.member5[side] == member;
			if (playing && turn_side == local)
				botTheater.DrawTurnHighlight(ax, ay, ax + a, ay + a);
			// the name in a dark box under the picture
			irr::core::recti box(x0, ay + a + 3, x1, ay + a + 3 + name_h);
			driver->draw2DRectangle(0xb0000000, box);
			if (playing)
				driver->draw2DRectangleOutline(box, turn_side == local ? 0xffffc828 : 0xa0ffffff);
			const wchar_t* name = gameConf.hide_player_name ? L"[****]" : dInfo.names5[place];
			textFont->drawUstring(name, box, mainGame->usernamecolor, true, true, &box);
		}
	}
}
void Game::DrawAvatars()
{
	if (dInfo.isFive)	// Bot Theater: five-a-side has its own portraits
	{
		DrawFivePortraits();
		return;
	}
	// Bot Theater: tag duels show all four duelists, each on the side of the screen where
	// their half of the field is: the far team in the top row, the near team below it.
	// The gold frame marks whoever's turn it is.
	if (botTheater.TagPortraits())
	{
		irr::core::position2di size = mainGame->Resize(69, 69);
		for (int row = 0; row < 2; ++row)
		{
			for (int column = 0; column < 2; ++column)
			{
				int seat = botTheater.PortraitSeat(row, column);
				irr::core::position2di pos = mainGame->Resize(column == 0 ? 332 : 987, row == 0 ? 30 : 103);
				pos.Y += 3;
				DrawPlayerAvatar(driver, irr::core::vector2di(pos.X, pos.Y), seat, column == 1);
				if (column == 1)
					pos.X -= size.X;
				if (botTheater.PortraitActive(row, column))
					botTheater.DrawTurnHighlight(pos.X, pos.Y, pos.X + size.X, pos.Y + size.Y);
				if (gameConf.hide_player_name)
					continue;
				// Names beside the portraits: the top row's under the Life Points bars as usual,
				// the bottom row's kept clear of the far player's hand.
				const wchar_t* name = botTheater.SeatName(seat);
				int top = (row == 0) ? 31 : 106;
				irr::core::recti box, clip;
				if (column == 0)
				{
					box = Resize(416, top, 815, top + 19);
					clip = Resize(416, top, row == 0 ? 815 : 535, top + 19);
				}
				else
				{
					box = Resize(905, top, 906, top + 19);
					box.UpperLeftCorner.X -= textFont->getDimension(name).Width;
					clip = Resize(row == 0 ? 500 : 795, top, 906, top + 19);
				}
				textFont->drawUstring(name, box, mainGame->usernamecolor, false, false, &clip);
			}
		}
		return;
	}
	int trueIds[4];
	bool isHostTeam = DuelClient::IsHostTeam();
	if (dInfo.isTag)
	{
		trueIds[0] = isHostTeam ? 0 : 2;
		trueIds[1] = isHostTeam ? 1 : 3;
		trueIds[2] = isHostTeam ? 2 : 0;
		trueIds[3] = isHostTeam ? 3 : 1;
	}
	else
	{
		trueIds[0] = isHostTeam ? 0 : 1;
		trueIds[2] = isHostTeam ? 1 : 0;
	}
	irr::core::position2di p1pos = mainGame->Resize(332, 30);
	irr::core::position2di p2pos = mainGame->Resize(987, 30);
	p1pos.Y += 3;
	p2pos.Y += 3;
	if (!dInfo.isTag || !dInfo.tag_player[0])
		DrawPlayerAvatar(driver, irr::core::vector2di(p1pos.X, p1pos.Y), trueIds[0], false);
	else
		DrawPlayerAvatar(driver, irr::core::vector2di(p1pos.X, p1pos.Y), trueIds[1], false);
	if (!dInfo.isTag || !dInfo.tag_player[1])
		DrawPlayerAvatar(driver, irr::core::vector2di(p2pos.X, p2pos.Y), trueIds[2], true);
	else
		DrawPlayerAvatar(driver, irr::core::vector2di(p2pos.X, p2pos.Y), trueIds[3], true);
	// Bot Theater: frame the avatar of the player whose turn it is
	int turnSide = botTheater.TurnSide();
	if (turnSide >= 0)
	{
		irr::core::position2di size = mainGame->Resize(69, 69);
		irr::core::position2di pos = (turnSide == 0) ? p1pos : p2pos;
		if (turnSide == 1)
			pos.X -= size.X;
		botTheater.DrawTurnHighlight(pos.X, pos.Y, pos.X + size.X, pos.Y + size.Y);
	}
}
void Game::DrawRanks()
{
	if (botTheater.TagPortraits() || dInfo.isFive)	// Bot Theater: no room for ranks beside tag or five-a-side portraits
		return;
	int trueIds[4];
	bool isHostTeam = DuelClient::IsHostTeam();
	if (dInfo.isTag)
	{
		trueIds[0] = isHostTeam ? 0 : 2;
		trueIds[1] = isHostTeam ? 1 : 3;
		trueIds[2] = isHostTeam ? 2 : 0;
		trueIds[3] = isHostTeam ? 3 : 1;
	}
	else
	{
		trueIds[0] = isHostTeam ? 0 : 1;
		trueIds[2] = isHostTeam ? 1 : 0;
	}
	irr::core::position2di p1pos = mainGame->Resize(412, 50);
	irr::core::position2di p2pos = mainGame->Resize(909, 50);
	if (!dInfo.isTag || !dInfo.tag_player[0])
		DrawPlayerRank(driver, irr::core::vector2di(p1pos.X - 1, p1pos.Y + 4), trueIds[0], false);
	else
		DrawPlayerRank(driver, irr::core::vector2di(p1pos.X - 1, p1pos.Y + 4), trueIds[1], false);
	if (!dInfo.isTag || !dInfo.tag_player[1])
		DrawPlayerRank(driver, irr::core::vector2di(p2pos.X - 3, p2pos.Y + 4), trueIds[2], true);
	else
		DrawPlayerRank(driver, irr::core::vector2di(p2pos.X - 3, p2pos.Y + 4), trueIds[3], true);
}
void Game::DrawBorders()
{
	if (dInfo.isFive)	// Bot Theater: five-a-side portraits have no borders
		return;
	int trueIds[4];
	bool isHostTeam = DuelClient::IsHostTeam();
	if (dInfo.isTag)
	{
		trueIds[0] = isHostTeam ? 0 : 2;
		trueIds[1] = isHostTeam ? 1 : 3;
		trueIds[2] = isHostTeam ? 2 : 0;
		trueIds[3] = isHostTeam ? 3 : 1;
	}
	else
	{
		trueIds[0] = isHostTeam ? 0 : 1;
		trueIds[2] = isHostTeam ? 1 : 0;
	}
	irr::core::recti p1pos = mainGame->Resize(328, 8, 629, 105);
	irr::core::recti p2pos = mainGame->Resize(689, 8, 990, 105);
	if (!dInfo.isTag || !dInfo.tag_player[0])
		DrawPlayerBorder(driver, p1pos, trueIds[0]);
	else
		DrawPlayerBorder(driver, p1pos, trueIds[1]);
	if (!dInfo.isTag || !dInfo.tag_player[1])
		DrawPlayerBorder(driver, p2pos, trueIds[2]);
	else
		DrawPlayerBorder(driver, p2pos, trueIds[3]);
}

// ---------------------------------------------------------------------------------------------
// Bot Theater: drawing a four-player free-for-all, seen from straight above the centre of the
// cross (see FOUR_PLAYER.md). Sides go clockwise from the bottom: 0 bottom, 1 left, 2 top,
// 3 right. Card positions come from ClientField::GetCardLocation4.
// The zones of one side, in that side's own frame (a forward, b to the right).
struct FourZone {
	float a, b;
	int kind;	// 0 Monster, 1 Extra Monster, 2 Spell/Trap, 3 Deck and Extra Deck, 4 Field, 5 GY, 6 banished
};
static const FourZone four_zones[] = {
	{ -2, -2, 0 }, { -2, -1, 0 }, { -2, 0, 0 }, { -2, 1, 0 }, { -2, 2, 0 },
	{ -1, -1, 1 }, { -1, 1, 1 },
	{ -3, -2, 2 }, { -3, -1, 2 }, { -3, 0, 2 }, { -3, 1, 2 }, { -3, 2, 2 },
	{ -3, -3, 4 }, { -4, -3, 3 }, { -4, -2, 3 }, { -4, 2, 5 }, { -4, 3, 6 },
};

static void FourQuad(irr::video::IVideoDriver* driver, float x, float y, float half, irr::video::SColor fill, irr::video::SColor line) {
	irr::video::S3DVertex v[4];
	v[0] = irr::video::S3DVertex(x - half, y - half, 0, 0, 0, 1, fill, 0, 0);
	v[1] = irr::video::S3DVertex(x + half, y - half, 0, 0, 0, 1, fill, 1, 0);
	v[2] = irr::video::S3DVertex(x - half, y + half, 0, 0, 0, 1, fill, 0, 1);
	v[3] = irr::video::S3DVertex(x + half, y + half, 0, 0, 0, 1, fill, 1, 1);
	driver->drawVertexPrimitiveList(v, 4, matManager.iRectangle, 2);
	irr::core::vector3df c[4] = { { x - half, y - half, 0.001f }, { x + half, y - half, 0.001f },
		{ x + half, y + half, 0.001f }, { x - half, y + half, 0.001f } };
	for(int i = 0; i < 4; ++i)
		driver->draw3DLine(c[i], c[(i + 1) % 4], line);
}

// The screen point (in window pixels) of a point on the table.
static irr::core::vector2di FourScreen(irr::scene::ISceneManager* smgr, irr::scene::ICameraSceneNode* camera, float x, float y, float z = 0) {
	return smgr->getSceneCollisionManager()->getScreenCoordinatesFrom3DPosition(irr::core::vector3df(x, y, z), camera);
}

// A picture lying on the table over the rectangle (x0, y0)-(x1, y1) (world Y grows down the
// screen), upright on the screen. Its transparency is the picture's own times the colour's.
// Draws textured triangles on the table, seen through the camera (in case 2D drawing came in
// between), blended with the texture's and the vertices' alpha, with no depth test.
static void DrawFourTextured(irr::video::IVideoDriver* driver, irr::scene::ICameraSceneNode* camera, irr::video::ITexture* texture,
		const irr::video::S3DVertex* v, int vertices, const irr::u16* idx, int triangles) {
	if(!texture)
		return;
	driver->setTransform(irr::video::ETS_PROJECTION, camera->getProjectionMatrix());
	driver->setTransform(irr::video::ETS_VIEW, camera->getViewMatrix());
	irr::video::SMaterial m;
	m.MaterialType = irr::video::EMT_ONETEXTURE_BLEND;
	m.MaterialTypeParam = irr::video::pack_textureBlendFunc(irr::video::EBF_SRC_ALPHA, irr::video::EBF_ONE_MINUS_SRC_ALPHA,
		irr::video::EMFN_MODULATE_1X, irr::video::EAS_VERTEX_COLOR | irr::video::EAS_TEXTURE);
	m.setFlag(irr::video::EMF_LIGHTING, false);
	m.setFlag(irr::video::EMF_ZBUFFER, false);
	m.setFlag(irr::video::EMF_ZWRITE_ENABLE, false);
	m.setFlag(irr::video::EMF_BACK_FACE_CULLING, false);
	m.setTexture(0, texture);
	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
	driver->setMaterial(m);
	driver->drawVertexPrimitiveList(v, vertices, idx, triangles);
}
// An upright picture on the table (x0, y0 its top left corner), with the part u0-u1, v0-v1 of the texture.
static void DrawFourPicture(irr::video::IVideoDriver* driver, irr::scene::ICameraSceneNode* camera, irr::video::ITexture* texture,
		float x0, float y0, float x1, float y1, irr::video::SColor color, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1,
		float z = 0.002f) {
	irr::video::S3DVertex v[4];
	v[0] = irr::video::S3DVertex(x0, y0, z, 0, 0, 1, color, u0, v0);
	v[1] = irr::video::S3DVertex(x1, y0, z, 0, 0, 1, color, u1, v0);
	v[2] = irr::video::S3DVertex(x0, y1, z, 0, 0, 1, color, u0, v1);
	v[3] = irr::video::S3DVertex(x1, y1, z, 0, 0, 1, color, u1, v1);
	DrawFourTextured(driver, camera, texture, v, 4, matManager.iRectangle, 2);
}

// Bot Theater, four players: holograms, seen from straight above. Each face-up card in a Monster
// Zone or a Spell & Trap / Pendulum Zone (not the Field Zone) has its art over it, upright, a
// whole grid square across, so holograms in neighbouring zones just meet. No glow or movement:
// a short fade-in once the card has landed, and it goes when the card leaves or turns face-down.
// bot_theater.conf: holograms (on/off), hologram_opacity, hologram_backrow (Spell & Trap and
// Pendulum Zones too); with hologram_stay = 0 each fades after hologram_seconds.
void Game::DrawHolograms4() {
	static std::unordered_map<ClientCard*, long long> since;	// when each card's hologram came up
	if(!botTheater.HologramsFour()) {
		since.clear();
		return;
	}
	const float HALF = 0.6f;	// half a grid square
	const long long FADE_IN_MS = 200, FADE_OUT_MS = 300;
	long long now = NowMs();
	float opacity = std::min(100, std::max(5, botTheater.hologram_opacity)) / 100.0f;
	long long shown_ms = (long long)(std::max(0.1, botTheater.hologram_seconds) * 1000);
	std::unordered_map<ClientCard*, long long> next;
	for(int side = 0; side < 4; ++side) {
		for(int row = 0; row < 2; ++row) {
			bool backrow = (row == 1);
			if(backrow && !botTheater.hologram_backrow)
				continue;
			auto& zones = backrow ? dField.szone[side] : dField.mzone[side];
			for(size_t s = 0; s < zones.size(); ++s) {
				if(backrow && s == 5)
					continue;	// Field Zone (Field Spells are left out for now)
				ClientCard* c = zones[s];
				if(!c || !c->code || !(c->position & POS_FACEUP) || c->is_moving)
					continue;
				auto it = since.find(c);
				long long start = (it != since.end()) ? it->second : now;
				next[c] = start;
				long long age = now - start;
				float alpha = std::min(1.0f, age / (float)FADE_IN_MS);
				if(!botTheater.hologram_stay && age > shown_ms)
					alpha *= std::max(0.0f, 1.0f - (age - shown_ms) / (float)FADE_OUT_MS);
				alpha *= opacity;
				if(alpha <= 0.002f)
					continue;
				irr::video::ITexture* art = imageManager.GetTexture(c->code);
				if(!art)
					continue;
				// the art box of a standard card image (square); a Pendulum's is wider, so its
				// middle is taken
				float u0 = 50 / 421.0f, u1 = 371 / 421.0f, v0 = 113 / 614.0f, v1 = 434 / 614.0f;
				auto cp = dataManager.GetCodePointer(c->code);
				if(cp != dataManager.datas_end() && (cp->second.type & TYPE_PENDULUM)) {
					v1 = 386 / 614.0f;
					u0 = 74 / 421.0f;
					u1 = 347 / 421.0f;
				}
				irr::core::vector3df t, r;
				dField.GetCardLocation4(c, &t, &r);
				// just above the cards on the field (0.02), well below the hands (0.5)
				DrawFourPicture(driver, camera, art, t.X - HALF, t.Y - HALF, t.X + HALF, t.Y + HALF,
					irr::video::SColor((irr::u32)(alpha * 255), 255, 255, 255), u0, v0, u1, v1, 0.05f);
			}
		}
	}
	since.swap(next);
}

// Bot Theater, four players: Field Spells. A Field Spell's art (its pics/field picture, or else
// the art box of its card picture) is laid square over the whole cross, centred on the middle of
// the field and upright, under the zones; the cross is cut into four triangles meeting in the
// middle, one per player, and each player's triangle shows only their own Field Spell's art.
void Game::DrawFieldSpells4() {
	if(!gameConf.draw_field_spell)
		return;
	const float edge = 4 * 1.2f + 0.55f;	// the cross's outer edge
	const float k = edge / 1.2f;	// the same, in grid squares
	for(int side = 0; side < 4; ++side) {
		ClientCard* pcard = (dField.szone[side].size() > 5) ? dField.szone[side][5] : nullptr;
		if(!pcard || !pcard->code || !(pcard->position & POS_FACEUP) || dInfo.playerOut[side])
			continue;
		float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
		irr::video::ITexture* art = imageManager.GetTextureField(pcard->code);
		if(!art && pcard->alias)
			art = imageManager.GetTextureField(pcard->alias);
		if(!art) {
			art = imageManager.GetTexture(pcard->code);	// the art box of the card picture
			u0 = 50 / 421.0f; u1 = 371 / 421.0f; v0 = 113 / 614.0f; v1 = 434 / 614.0f;
		}
		if(!art)
			continue;
		// the triangle: the middle, and the two corners of this player's edge of the cross
		float x[3] = { 0, 0, 0 }, y[3] = { 0, 0, 0 };
		ClientField::FourGridPoint(side, -k, -k, &x[1], &y[1]);
		ClientField::FourGridPoint(side, -k, k, &x[2], &y[2]);
		irr::video::S3DVertex v[3];
		for(int i = 0; i < 3; ++i) {
			float tu = u0 + (u1 - u0) * (x[i] + edge) / (2 * edge);
			float tv = v0 + (v1 - v0) * (y[i] + edge) / (2 * edge);
			v[i] = irr::video::S3DVertex(x[i], y[i], 0, 0, 0, 1, irr::video::SColor(255, 255, 255, 255), tu, tv);
		}
		static const irr::u16 idx[3] = { 0, 1, 2 };
		DrawFourTextured(driver, camera, art, v, 3, idx, 1);
	}
}

void Game::DrawBackGround4() {
	DrawFieldSpells4();
	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
	irr::video::SMaterial m;
	m.Lighting = false;
	m.ZWriteEnable = false;
	m.MaterialType = irr::video::EMT_TRANSPARENT_VERTEX_ALPHA;
	m.Thickness = 1;
	driver->setMaterial(m);
	// Field Zone green, GY violet, banished teal; the rest blue-grey
	static const irr::video::SColor fill[7] = { 0x60203048, 0x60284060, 0x60202838, 0x40101010,
		0x70204a28, 0x70402858, 0x70184c50 };
	static const irr::video::SColor edge[7] = { 0xa0a0c8ff, 0xc0c0a0ff, 0xa0a0c8ff, 0xa0a0c8ff,
		0xd080e090, 0xd0c090f0, 0xd070e0e0 };
	for(int side = 0; side < 4; ++side) {
		bool out = dInfo.playerOut[side];
		for(const auto& z : four_zones) {
			if(z.kind == 1 && dInfo.duel_rule < 4)
				continue;
			float x, y;
			ClientField::FourGridPoint(side, z.a, z.b, &x, &y);
			FourQuad(driver, x, y, 0.55f, fill[z.kind], out ? irr::video::SColor(0x60808080) : edge[z.kind]);
		}
	}
	// Bot Theater's glow round the hand of the player whose turn it is
	int turn_side = LocalPlayer(dInfo.turn_player4);
	if(dInfo.turn > 0 && !dInfo.playerOut[turn_side]) {
		// the area a hand of this size covers: cards stand upright on the screen, spaced along b
		size_t count = std::max<size_t>(dField.hand[turn_side].size(), 1);
		float step = (count > 1) ? std::min(0.75f, 4.6f / (count - 1)) : 0;
		bool across = (turn_side & 1) == 0;
		float half_a = across ? 0.42f : 0.29f, half_b = (across ? 0.29f : 0.42f) + step * (count - 1) / 2;
		irr::core::recti hand;
		for(int i = 0; i < 4; ++i) {
			float x, y;
			ClientField::FourGridPoint(turn_side, ClientField::FourHandA(turn_side) + ((i & 1) ? half_a : -half_a), (i & 2) ? half_b : -half_b, &x, &y);
			auto p = FourScreen(smgr, camera, x, y);
			if(i == 0)
				hand = irr::core::recti(p, p);
			else
				hand.addInternalPoint(p);
		}
		botTheater.DrawTurnGlowArea(hand);
	}
}

void Game::DrawMisc4() {
	static irr::core::vector3df act_rot(0, 0, 0);
	irr::core::matrix4 ic, it;
	act_rot.Z += 0.02f;
	DrawHolograms4();	// over the cards, under the chain markers and the numbers
	// chain markers, as in DrawMisc
	if(dField.chains.size() > 1 || gameConf.draw_single_chain) {
		for(size_t i = 0; i < dField.chains.size(); ++i) {
			if(dField.chains[i].solved)
				break;
			matManager.mTRTexture.setTexture(0, imageManager.tChain);
			matManager.mTRTexture.AmbientColor = 0xffffff00;
			ic.setRotationRadians(act_rot);
			ic.setTranslation(dField.chains[i].chain_pos);
			driver->setMaterial(matManager.mTRTexture);
			driver->setTransform(irr::video::ETS_WORLD, ic);
			driver->drawVertexPrimitiveList(matManager.vSymbol, 4, matManager.iRectangle, 2);
			it.setScale(0.6f);
			it.setTranslation(dField.chains[i].chain_pos);
			matManager.mTRTexture.setTexture(0, imageManager.tNumber);
			matManager.vChainNum[0].TCoords = irr::core::vector2df(0.19375f * (i % 5), 0.2421875f * (i / 5));
			matManager.vChainNum[1].TCoords = irr::core::vector2df(0.19375f * (i % 5 + 1), 0.2421875f * (i / 5));
			matManager.vChainNum[2].TCoords = irr::core::vector2df(0.19375f * (i % 5), 0.2421875f * (i / 5 + 1));
			matManager.vChainNum[3].TCoords = irr::core::vector2df(0.19375f * (i % 5 + 1), 0.2421875f * (i / 5 + 1));
			driver->setMaterial(matManager.mTRTexture);
			driver->setTransform(irr::video::ETS_WORLD, it);
			driver->drawVertexPrimitiveList(matManager.vChainNum, 4, matManager.iRectangle, 2);
		}
	}
	driver->setTransform(irr::video::ETS_WORLD, irr::core::IdentityMatrix);
	if(lpframe) {
		dInfo.lp[lpplayer] -= lpd;
		myswprintf(dInfo.strLP[lpplayer], L"%d", dInfo.lp[lpplayer]);
		lpccolor -= 0x0a000000;	// fades over FOUR_LP_FRAMES (24 x 10 of 255)
		lpframe--;
	}
	// Each side's portrait in its corner of the cross (just past its Extra Deck), and its name and
	// LP in a box beside that portrait, towards the nearer side of the window; and the pile counts.
	for(int side = 0; side < 4; ++side) {
		float x, y;
		bool out = dInfo.playerOut[side];
		bool turn = !out && dInfo.turn > 0 && LocalPlayer(dInfo.turn_player4) == side;
		int seat = (side + dInfo.viewSeat) % 4;
		float cx, cy;	// the corner square: (-4, -4) in the side's own frame
		ClientField::FourGridPoint(side, -4, -4, &cx, &cy);
		DrawFourPicture(driver, camera, imageManager.tAvatar[seat], cx - 0.55f, cy - 0.55f, cx + 0.55f, cy + 0.55f,
			out ? irr::video::SColor(255, 90, 90, 90) : irr::video::SColor(255, 255, 255, 255));
		auto p0 = FourScreen(smgr, camera, cx - 0.55f, cy - 0.55f), p1 = FourScreen(smgr, camera, cx + 0.55f, cy + 0.55f);
		irr::core::recti portrait(p0, p1);
		portrait.repair();
		driver->draw2DRectangleOutline(portrait, out ? 0xff606060 : 0xff8080a0);
		if(turn)
			botTheater.DrawTurnHighlight(portrait.UpperLeftCorner.X, portrait.UpperLeftCorner.Y,
				portrait.LowerRightCorner.X, portrait.LowerRightCorner.Y);
		// beside the portrait, level with it, on the outer side
		float name_w = ClientField::four_name_width;
		float near_x = cx + (cx < 0 ? -1 : 1) * (0.55f + ClientField::FOUR_NAME_GAP);
		float far_x = near_x + (cx < 0 ? -name_w : name_w);
		auto b0 = FourScreen(smgr, camera, near_x, cy - ClientField::FOUR_NAME_HEIGHT / 2);
		auto b1 = FourScreen(smgr, camera, far_x, cy + ClientField::FOUR_NAME_HEIGHT / 2);
		irr::core::recti box(b0, b1);
		box.repair();
		int h = box.getHeight();
		// Free-for-all: the box turns gold during that player's turn. Team match: the box
		// keeps its team's colour (red, blue or yellow), with a gold outline during the turn.
		irr::video::SColor fill = 0xa0000000, edge = 0xff8080a0;
		int team_colour = dInfo.hasTeams() ? dInfo.colour4[seat] : 0;
		if(team_colour >= 1 && team_colour <= 3) {
			static const irr::u32 fills[4] = { 0xa0000000, 0xc0961c1c, 0xc01c3c9c, 0xc08c6c08 };
			static const irr::u32 edges[4] = { 0xff8080a0, 0xffe86464, 0xff6c98ff, 0xffe8c84c };
			fill = fills[team_colour];
			edge = edges[team_colour];
			if(out)
				fill.setAlpha(0x60);
		} else if(turn) {
			fill = 0xc0604000;
			edge = 0xffffd040;
		}
		driver->draw2DRectangle(fill, box);
		driver->draw2DRectangleOutline(box, edge);
		if(team_colour && turn) {
			for(int i = 1; i <= 3; ++i) {
				irr::core::recti ring(box.UpperLeftCorner.X - i, box.UpperLeftCorner.Y - i, box.LowerRightCorner.X + i, box.LowerRightCorner.Y + i);
				driver->draw2DRectangleOutline(ring, 0xffffd040);
			}
		}
		int mid = box.UpperLeftCorner.Y + h / 2;
		irr::core::recti name_box(box.UpperLeftCorner.X + 4, box.UpperLeftCorner.Y + 1, box.LowerRightCorner.X - 4, mid);
		textFont->drawUstring(dInfo.names4[seat], name_box, out ? 0xff909090 : usernamecolor, true, true, &name_box);
		irr::core::recti lp_box(box.UpperLeftCorner.X, mid, box.LowerRightCorner.X, box.LowerRightCorner.Y);
		if(out)
			DrawShadowText(numFont, L"OUT", lp_box, irr::core::recti(0, 1, 2, 0), 0xffff6060, 0xff000000, true, false, 0);
		else
			DrawShadowText(numFont, dInfo.strLP[side], lp_box, irr::core::recti(0, 1, 2, 0), playerlpcolor, 0xff000000, true, false, 0);
		struct { float a, b; size_t count; } piles[] = {
			{ -4, -3, dField.extra[side].size() }, { -4, -2, dField.deck[side].size() },
			{ -4, 2, dField.grave[side].size() }, { -4, 3, dField.remove[side].size() } };
		for(const auto& pile : piles) {
			if(!pile.count)
				continue;
			ClientField::FourGridPoint(side, pile.a, pile.b, &x, &y);
			auto p = FourScreen(smgr, camera, x, y, 0.5f);
			DrawShadowText(numFont, dataManager.GetNumString((int)pile.count), irr::core::recti(p.X - 20, p.Y - 10, p.X + 20, p.Y + 10),
				irr::core::recti(0, 1, 2, 1), extracolor, 0xff000000, true, true, 0);
		}
		// ATK/DEF (or LINK) under each face-up monster
		for(auto pcard : dField.mzone[side]) {
			if(!pcard || !(pcard->position & POS_FACEUP))
				continue;
			// ATK over DEF (or LINK) in the bottom right corner of the monster's zone, lined up on
			// the right. The value the monster uses now (ATK in Attack Position, DEF in Defense
			// Position) is drawn bright; the other one faint.
			auto corner = FourScreen(smgr, camera, pcard->curPos.X + 0.55f, pcard->curPos.Y + 0.55f, pcard->curPos.Z);
			int right = corner.X - 4, bottom = corner.Y - 2;
			int line_h = (int)adFont->getDimension(L"0").Height + 1;
			auto at_right = [&](const wchar_t* text, int line_bottom) {
				int w = (int)adFont->getDimension(text).Width + 2;
				return irr::core::recti(right - w, line_bottom - line_h, right, line_bottom);
			};
			bool defending = (pcard->position & POS_DEFENSE) && !(pcard->type & TYPE_LINK);
			auto faint = [](irr::video::SColor c) { c.setAlpha(110); return c; };
			irr::video::SColor atk_color = pcard->attack > pcard->base_attack ? bonuscolor : pcard->attack < pcard->base_attack ? negativecolor : statcolor;
			DrawShadowText(adFont, pcard->atkstring, at_right(pcard->atkstring, bottom - line_h),
				irr::core::recti(1, 1, 1, 1), defending ? faint(atk_color) : atk_color, defending ? 0x50000000 : 0xff000000, false, false, 0);
			if(pcard->type & TYPE_LINK)
				DrawShadowText(adFont, pcard->linkstring, at_right(pcard->linkstring, bottom),
					irr::core::recti(1, 1, 1, 1), 0xff99ffff, 0xff000000, false, false, 0);
			else {
				irr::video::SColor def_color = pcard->defense > pcard->base_defense ? bonuscolor : pcard->defense < pcard->base_defense ? negativecolor : statcolor;
				DrawShadowText(adFont, pcard->defstring, at_right(pcard->defstring, bottom),
					irr::core::recti(1, 1, 1, 1), defending ? def_color : faint(def_color), defending ? 0xff000000 : 0x50000000, false, false, 0);
			}
		}
	}
	if(lpcstring.size()) {
		// The LP change shows over the player's side of the field; as it fades it flies to their
		// portrait (in the corner just past their Extra Deck).
		float x, y, px, py;
		ClientField::FourGridPoint(lpplayer, -1.0f, 0.0f, &x, &y);
		ClientField::FourGridPoint(lpplayer, -4.0f, -4.0f, &px, &py);
		auto from = FourScreen(smgr, camera, x, y), to = FourScreen(smgr, camera, px, py);
		float t = 1.0f - ((unsigned int)lpccolor >> 24) / 255.0f;	// 0 until it starts fading, then towards 1
		t = t * t * (3 - 2 * t);	// ease in and out
		irr::core::vector2di p(from.X + (int)((to.X - from.X) * t), from.Y + (int)((to.Y - from.Y) * t));
		DrawShadowText(lpcFont, lpcstring, irr::core::recti(p.X - 200, p.Y - 24, p.X + 200, p.Y + 24), irr::core::recti(0, 2, 2, 0),
			lpccolor, lpccolor | 0x00ffffff, true, true, 0);
	}
	// turn counter in the centre
	auto c = FourScreen(smgr, camera, 0, 0);
	DrawShadowText(lpcFont, dataManager.GetNumString(dInfo.turn), irr::core::recti(c.X - 30, c.Y - 20, c.X + 30, c.Y + 20),
		irr::core::recti(0, 0, 2, 0), turncolor, 0x80000000, true, true, 0);
}

// ---------------------------------------------------------------------------------------------
// Bot Theater, four players: speech bubbles. A player's line comes up in a bubble beside the
// field, just past their name box towards the middle of the screen, with its tail slanting to
// their portrait; it is just big enough for the line, grows down from the top corners and up from the bottom ones,
// stays for FOUR_BUBBLE_MS and fades. A player's next line waits until the one showing has been
// up for FOUR_BUBBLE_MIN_MS (a summon line and an attack line close together both get read).
// Drawn over everything else (after the side panel).

long long Game::NowMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void Game::ShowFourBubble(int seat, const wchar_t* text) {
	if(seat < 0 || seat > 3 || !text || !text[0])
		return;
	if(fourBubbleQueue[seat].size() >= FOUR_BUBBLE_QUEUE_MAX)
		fourBubbleQueue[seat].pop_front();	// falling far behind: skip the oldest waiting line
	fourBubbleQueue[seat].push_back(text);
	NextFourBubbles();
}
void Game::ClearFourBubbles() {
	for(int seat = 0; seat < 4; ++seat) {
		fourBubbleText[seat].clear();
		fourBubbleQueue[seat].clear();
	}
}
// Each seat: the next waiting line, once the one showing has been up long enough (or has gone).
void Game::NextFourBubbles() {
	long long now = NowMs();
	for(int seat = 0; seat < 4; ++seat) {
		if(fourBubbleQueue[seat].empty())
			continue;
		if(!fourBubbleText[seat].empty() && now - fourBubbleStart[seat] < FOUR_BUBBLE_MIN_MS)
			continue;
		fourBubbleText[seat] = fourBubbleQueue[seat].front();
		fourBubbleQueue[seat].pop_front();
		fourBubbleStart[seat] = now;
	}
}
// Called from the network thread (end of the duel), so it takes gMutex.
bool Game::FourBubblesShowing() {
	std::lock_guard<std::mutex> lock(gMutex);
	long long now = NowMs();
	for(int seat = 0; seat < 4; ++seat) {
		if(!fourBubbleQueue[seat].empty())
			return true;
		if(!fourBubbleText[seat].empty() && now - fourBubbleStart[seat] < FOUR_BUBBLE_MS + FOUR_BUBBLE_FADE_MS)
			return true;
	}
	return false;
}

static float Clamp01(float v) {
	return v < 0 ? 0 : (v > 1 ? 1 : v);
}
// Signed distance from p to a triangle (negative inside).
static float TriangleDistance(float px, float py, const float t[3][2]) {
	float d = 1e9f;
	float cross = (t[1][0] - t[0][0]) * (t[2][1] - t[0][1]) - (t[1][1] - t[0][1]) * (t[2][0] - t[0][0]);
	bool inside = true;
	for(int i = 0; i < 3; ++i) {
		const float* a = t[i];
		const float* b = t[(i + 1) % 3];
		float ex = b[0] - a[0], ey = b[1] - a[1], vx = px - a[0], vy = py - a[1];
		float h = Clamp01((vx * ex + vy * ey) / (ex * ex + ey * ey));
		float qx = vx - ex * h, qy = vy - ey * h;
		d = std::min(d, qx * qx + qy * qy);
		if((ex * vy - ey * vx) * cross < 0)
			inside = false;
	}
	return inside ? -std::sqrt(d) : std::sqrt(d);
}
static int PowerOfTwo(int n) {
	int p = 1;
	while(p < n)
		p <<= 1;
	return p;
}
// A speech bubble: a white body (body_w x body_h, round corners, dark edge) with a tail
// tail_len long on one edge (tail_side 0 left, 1 right, 2 top, 3 bottom), its point tail_at
// pixels along that edge from the body's start (left or top). The point may lie past either
// end of the body (a slanting tail); *lead is then how far the picture starts before the body.
// The picture is the body plus the tail, in the top left corner of a power-of-two texture (as
// some video cards need).
static irr::video::ITexture* MakeBubbleShape(irr::video::IVideoDriver* driver, int body_w, int body_h, int tail_len,
		int tail_side, int tail_at, int radius, const char* name, int* lead) {
	bool across = tail_side >= 2;	// the tail on the top or bottom edge
	int along = across ? body_w : body_h;
	int before = std::max(0, 2 - tail_at), after = std::max(0, tail_at + 3 - along);
	*lead = before;
	tail_at += before;
	int w = body_w + (across ? before + after : tail_len), h = body_h + (across ? tail_len : before + after);
	irr::video::IImage* img = driver->createImage(irr::video::ECF_A8R8G8B8, irr::core::dimension2du(PowerOfTwo(w), PowerOfTwo(h)));
	if(!img)
		return nullptr;
	img->fill(irr::video::SColor(0, 0, 0, 0));
	// worked out with u going away from the tail's edge and v along it
	int U = across ? h : w, V = across ? w : h;
	float u0 = tail_len + 0.5f, u1 = U - 1.5f, v0 = before + 1.5f, v1 = before + along - 1.5f;
	float r = std::min((float)radius, std::min(u1 - u0, v1 - v0) / 2);
	float cu = (u0 + u1) / 2, cv = (v0 + v1) / 2, hu = (u1 - u0) / 2 - r, hv = (v1 - v0) / 2 - r;
	float spread = std::min(along * 0.3f, (float)radius * 0.6f + 2);
	float tv = std::max(v0 + r * 0.5f + spread, std::min(v1 - r * 0.5f - spread, (float)tail_at));
	const float tri[3][2] = { { 1.5f, (float)tail_at }, { u0 + r * 0.7f, tv - spread }, { u0 + r * 0.7f, tv + spread } };
	const float edge = 2.0f;
	for(int y = 0; y < h; ++y) {
		for(int x = 0; x < w; ++x) {
			float pu, pv;
			switch(tail_side) {
			case 0: pu = (float)x; pv = (float)y; break;
			case 1: pu = (float)(w - 1 - x); pv = (float)y; break;
			case 2: pu = (float)y; pv = (float)x; break;
			default: pu = (float)(h - 1 - y); pv = (float)x; break;
			}
			pu += 0.5f;
			pv += 0.5f;
			float qu = std::fabs(pu - cu) - hu, qv = std::fabs(pv - cv) - hv;
			float ou = std::max(qu, 0.0f), ov = std::max(qv, 0.0f);
			float body = std::sqrt(ou * ou + ov * ov) + std::min(std::max(qu, qv), 0.0f) - r;
			float d = tail_len > 2 ? std::min(body, TriangleDistance(pu, pv, tri)) : body;
			float cover = Clamp01(0.5f - d);
			float ink = Clamp01(d + edge + 0.5f);	// 1 on the edge, 0 inside
			int red = (int)(250 + (40 - 250) * ink), green = (int)(250 + (40 - 250) * ink), blue = (int)(244 + (52 - 244) * ink);
			img->setPixel(x, y, irr::video::SColor((irr::u32)(cover * 255), red, green, blue));
		}
	}
	irr::video::ITexture* tex = driver->addTexture(name, img);
	img->drop();
	return tex;
}

// Splits text into lines no wider than width, breaking between words (a word too long for a
// line gets a line of its own).
static std::vector<std::wstring> WrapWords(irr::gui::CGUITTFont* font, const std::wstring& text, int width) {
	std::vector<std::wstring> lines;
	std::wstring line, word;
	auto flush_word = [&]() {
		if(word.empty())
			return;
		std::wstring longer = line.empty() ? word : line + L" " + word;
		if(!line.empty() && (int)font->getDimension(longer).Width > width) {
			lines.push_back(line);
			line = word;
		} else
			line = longer;
		word.clear();
	};
	for(wchar_t ch : text) {
		if(ch == L' ' || ch == L'\n')
			flush_word();
		else
			word += ch;
	}
	flush_word();
	if(!line.empty())
		lines.push_back(line);
	return lines;
}

void Game::DrawFourBubbles() {
	struct Bubble {
		irr::video::ITexture* shape = nullptr;
		std::wstring text;
		int body_w = 0, body_h = 0, tail_len = 0, tail_side = 0, tail_at = 0;
		int lead = 0;	// how far the picture starts left of the body (a slanting tail)
	};
	static Bubble made[4];
	static const char* names[4] = { "ffa_bubble_0", "ffa_bubble_1", "ffa_bubble_2", "ffa_bubble_3" };
	NextFourBubbles();
	long long now = NowMs();
	for(int seat = 0; seat < 4; ++seat) {
		if(fourBubbleText[seat].empty())
			continue;
		long long age = now - fourBubbleStart[seat];
		if(age < 0 || age >= FOUR_BUBBLE_MS + FOUR_BUBBLE_FADE_MS) {
			fourBubbleText[seat].clear();
			continue;
		}
		int alpha = (age < FOUR_BUBBLE_MS) ? 255 : (int)(255 * (FOUR_BUBBLE_MS + FOUR_BUBBLE_FADE_MS - age) / FOUR_BUBBLE_FADE_MS);
		int side = LocalPlayer(seat);
		// the portrait, in this side's corner of the cross
		float cx, cy;
		ClientField::FourGridPoint(side, -4, -4, &cx, &cy);
		auto p0 = FourScreen(smgr, camera, cx - 0.55f, cy - 0.55f), p1 = FourScreen(smgr, camera, cx + 0.55f, cy + 0.55f);
		irr::core::recti portrait(p0, p1);
		portrait.repair();
		bool left = cx < 0;	// which side of the field
		bool top = portrait.getCenter().Y < (int)window_size.Height / 2;	// a top corner
		float unit = std::abs(p1.X - p0.X) / 1.1f;	// pixels per world unit
		// the player's name box, beside the portrait (as DrawMisc4 places it)
		float near_x = cx + (left ? -1 : 1) * (0.55f + ClientField::FOUR_NAME_GAP);
		float far_x = near_x + (left ? -1 : 1) * ClientField::four_name_width;
		auto n0 = FourScreen(smgr, camera, near_x, cy - ClientField::FOUR_NAME_HEIGHT / 2);
		auto n1 = FourScreen(smgr, camera, far_x, cy + ClientField::FOUR_NAME_HEIGHT / 2);
		irr::core::recti namebox(n0, n1);
		namebox.repair();
		int line_h = textFont->getDimension(L"Ag").Height + 1;
		int pad_x = line_h * 3 / 5, pad_y = line_h / 2;
		int tail_len = std::max(8, (int)(0.3f * unit));
		// as wide as the name box (or a little wider, over the side panel, in a narrow window)
		int max_w = std::max(namebox.getWidth(), (int)(2.0f * unit));
		std::vector<std::wstring> lines = WrapWords(textFont, fourBubbleText[seat], std::max(20, max_w - 2 * pad_x));
		int text_w = 0;
		for(const auto& line : lines)
			text_w = std::max(text_w, (int)textFont->getDimension(line).Width);
		int body_w = text_w + 2 * pad_x, body_h = line_h * (int)lines.size() + 2 * pad_y;
		// just past the name box towards the middle of the screen (below a top corner's box,
		// growing down; above a bottom corner's, growing up), lined up with the box's field side
		int body_top = top ? namebox.LowerRightCorner.Y + tail_len : namebox.UpperLeftCorner.Y - tail_len - body_h;
		body_top = std::max(2, std::min(body_top, (int)window_size.Height - 2 - body_h));
		int body_left = left ? namebox.LowerRightCorner.X - body_w : namebox.UpperLeftCorner.X;
		body_left = std::max(4, std::min(body_left, (int)window_size.Width - 4 - body_w));
		// the tail slants from the bubble's field end to the middle of the portrait's edge
		int tail_side = top ? 2 : 3;
		int tail_at = portrait.getCenter().X - body_left;
		int image_top = top ? body_top - tail_len : body_top;
		Bubble& b = made[seat];
		if(!b.shape || b.text != fourBubbleText[seat] || b.body_w != body_w || b.body_h != body_h || b.tail_len != tail_len
				|| b.tail_side != tail_side || b.tail_at != tail_at) {
			if(b.shape)
				driver->removeTexture(b.shape);
			b.shape = MakeBubbleShape(driver, body_w, body_h, tail_len, tail_side, tail_at, line_h * 4 / 5, names[seat], &b.lead);
			b.text = fourBubbleText[seat];
			b.body_w = body_w;
			b.body_h = body_h;
			b.tail_len = tail_len;
			b.tail_side = tail_side;
			b.tail_at = tail_at;
		}
		if(b.shape)
			driver->draw2DImage(b.shape, irr::core::position2di(body_left - b.lead, image_top),
				irr::core::recti(0, 0, body_w + std::max(b.lead, 0) + std::max(0, tail_at + 3 - body_w), body_h + tail_len), 0,
				irr::video::SColor(alpha, 255, 255, 255), true);
		irr::video::SColor ink(alpha, 32, 32, 40);
		for(size_t i = 0; i < lines.size(); ++i) {
			int y = body_top + pad_y + line_h * (int)i;
			textFont->drawUstring(lines[i], irr::core::recti(body_left, y, body_left + body_w, y + line_h), ink, true, true, 0);
		}
	}
}

}
