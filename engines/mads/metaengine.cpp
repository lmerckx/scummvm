/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "mads/mads.h"

#include "base/plugins.h"
#include "engines/advancedDetector.h"

#include "common/savefile.h"
#include "common/str-array.h"
#include "common/memstream.h"
#include "common/system.h"
#include "common/translation.h"
#include "graphics/surface.h"

#include "mads/events.h"
#include "mads/game.h"
#include "mads/detection.h"

#define MAX_SAVES 99

namespace MADS {

uint32 MADSEngine::getGameID() const {
	return _gameDescription->gameID;
}

uint32 MADSEngine::getGameFeatures() const {
	return _gameDescription->features;
}

uint32 MADSEngine::getFeatures() const {
	return _gameDescription->desc.flags;
}

Common::Language MADSEngine::getLanguage() const {
	return _gameDescription->desc.language;
}

Common::Platform MADSEngine::getPlatform() const {
	return _gameDescription->desc.platform;
}

} // End of namespace MADS

class MADSMetaEngineConnect : public AdvancedMetaEngineConnect {
public:
	const char *getName() const override {
		return "mads";
	}

    bool hasFeature(MetaEngineFeature f) const override;
	bool createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override;

	SaveStateList listSaves(const char *target) const override;
	int getMaximumSaveSlot() const override;
	void removeSaveState(const char *target, int slot) const override;
	SaveStateDescriptor querySaveMetaInfos(const char *target, int slot) const override;
};

bool MADSMetaEngineConnect::hasFeature(MetaEngineFeature f) const {
	return
	    (f == kSupportsListSaves) ||
		(f == kSupportsLoadingDuringStartup) ||
		(f == kSupportsDeleteSave) ||
		(f == kSavesSupportMetaInfo) ||
		(f == kSavesSupportThumbnail) ||
		(f == kSimpleSavesNames);
}

bool MADS::MADSEngine::hasFeature(EngineFeature f) const {
	return
		(f == kSupportsReturnToLauncher) ||
		(f == kSupportsLoadingDuringRuntime) ||
		(f == kSupportsSavingDuringRuntime);
}

bool MADSMetaEngineConnect::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	const MADS::MADSGameDescription *gd = (const MADS::MADSGameDescription *)desc;
	if (gd) {
		*engine = new MADS::MADSEngine(syst, gd);
	}
	return gd != 0;
}

SaveStateList MADSMetaEngineConnect::listSaves(const char *target) const {
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::StringArray filenames;
	Common::String saveDesc;
	Common::String pattern = Common::String::format("%s.0##", target);
	MADS::MADSSavegameHeader header;

	filenames = saveFileMan->listSavefiles(pattern);

	SaveStateList saveList;
	for (Common::StringArray::const_iterator file = filenames.begin(); file != filenames.end(); ++file) {
		const char *ext = strrchr(file->c_str(), '.');
		int slot = ext ? atoi(ext + 1) : -1;

		if (slot >= 0 && slot < MAX_SAVES) {
			Common::InSaveFile *in = g_system->getSavefileManager()->openForLoading(*file);

			if (in) {
				if (MADS::Game::readSavegameHeader(in, header))
					saveList.push_back(SaveStateDescriptor(slot, header._saveName));
				delete in;
			}
		}
	}

	// Sort saves based on slot number.
	Common::sort(saveList.begin(), saveList.end(), SaveStateDescriptorSlotComparator());
	return saveList;
}

int MADSMetaEngineConnect::getMaximumSaveSlot() const {
	return MAX_SAVES;
}

void MADSMetaEngineConnect::removeSaveState(const char *target, int slot) const {
	Common::String filename = Common::String::format("%s.%03d", target, slot);
	g_system->getSavefileManager()->removeSavefile(filename);
}

SaveStateDescriptor MADSMetaEngineConnect::querySaveMetaInfos(const char *target, int slot) const {
	Common::String filename = Common::String::format("%s.%03d", target, slot);
	Common::InSaveFile *f = g_system->getSavefileManager()->openForLoading(filename);

	if (f) {
		MADS::MADSSavegameHeader header;
		if (!MADS::Game::readSavegameHeader(f, header, false)) {
			delete f;
			return SaveStateDescriptor();
		}
		delete f;

		// Create the return descriptor
		SaveStateDescriptor desc(slot, header._saveName);
		desc.setThumbnail(header._thumbnail);
		desc.setSaveDate(header._year, header._month, header._day);
		desc.setSaveTime(header._hour, header._minute);
		desc.setPlayTime(header._totalFrames * GAME_FRAME_TIME);

		return desc;
	}

	return SaveStateDescriptor();
}

#if PLUGIN_ENABLED_DYNAMIC(MADS)
	REGISTER_PLUGIN_DYNAMIC(MADS, PLUGIN_TYPE_ENGINE, MADSMetaEngineConnect);
#else
	REGISTER_PLUGIN_STATIC(MADS, PLUGIN_TYPE_ENGINE, MADSMetaEngineConnect);
#endif