#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include "config.h"
#include "data_manager.h"
#include <unordered_map>
#include <queue>
#include <mutex>

namespace ygo {

	enum TextureType
	{
		SLEEVE = 0,
		AVATAR = 1,
		RANK = 2,
		BORDER = 3
	};
	struct TextureData
	{
		TextureType type;
		int player;
		int textureId;
		char hostname[256];
		char filename[256];
		char fakename[32];
	};

class ImageManager {
public:
	bool Initial();
	void SetDevice(irr::IrrlichtDevice* dev);
	void ClearTexture();
	void RemoveTexture(int code);
	void ResizeTexture();

	void LoadTexture(TextureType type, int textureId, int player, wchar_t* site, wchar_t* dir);
	void LoadPendingTextures();

	irr::video::ITexture* GetTextureFromFile(const char* file, irr::s32 width, irr::s32 height);
	irr::video::ITexture* GetTexture(int code, bool fit = false);
	irr::video::ITexture* GetBigPicture(int code, float zoom);
	irr::video::ITexture* GetTextureThumb(int code);
	irr::video::ITexture* GetTextureField(int code);
	static int LoadThumbThread();

	std::unordered_map<int, irr::video::ITexture*> tMap[2];
	std::unordered_map<int, irr::video::ITexture*> tThumb;
	std::unordered_map<int, irr::video::ITexture*> tFields;
	std::unordered_map<int, irr::video::IImage*> tThumbLoading;
	std::queue<int> tThumbLoadingCodes;
	std::mutex tThumbLoadingMutex;
	bool tThumbLoadingThreadRunning;
	irr::IrrlichtDevice* device;
	irr::video::IVideoDriver* driver;
	irr::video::ITexture* tCover[4];
	irr::video::ITexture* tUnknown;
	irr::video::ITexture* tUnknownFit;
	irr::video::ITexture* tUnknownThumb;
	irr::video::ITexture* tBigPicture;
	irr::video::ITexture* tLoading;
	irr::video::ITexture* tAct;
	irr::video::ITexture* tAttack;
	irr::video::ITexture* tNegated;
	irr::video::ITexture* tChain;
	irr::video::ITexture* tNumber;
	irr::video::ITexture* tLPFrame;
	irr::video::ITexture* tLPBar;
	irr::video::ITexture* tMask;
	irr::video::ITexture* tEquip;
	irr::video::ITexture* tTarget;
	irr::video::ITexture* tChainTarget;
	irr::video::ITexture* tLim;
	irr::video::ITexture* tOT;
	irr::video::ITexture* tHand[3];
	irr::video::ITexture* tBackGround;
	irr::video::ITexture* tBackGround_menu;
	irr::video::ITexture* tBackGround_deck;
	irr::video::ITexture* tField[2];
	irr::video::ITexture* tFieldTransparent[2];

	irr::video::ITexture* tAvatar[4];
	irr::video::ITexture* tRank[4];
	irr::video::ITexture* tBorder[4];
private:
	irr::video::ITexture* ReadTexture(TextureData* textureData);
	irr::video::ITexture* DownloadTexture(TextureData* textureData);
	irr::video::ITexture* GetRankTexture(int rank);
	irr::video::ITexture* GetBorderTexture(TextureData* textureData);
	void ApplyTexture(TextureData* textureData, irr::video::ITexture* texture);
	std::vector<TextureData*> pendingTextures;
};

extern ImageManager imageManager;

}

#endif // IMAGEMANAGER_H
