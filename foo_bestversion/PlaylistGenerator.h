#pragma once

#include "FoobarSDKWrapper.h"

namespace lastfmgrabber {

void generatePlaylistFromTracks(const pfc::list_t<metadb_handle_ptr>& tracks);
void generatePlaylistFromTracks(const pfc::list_t<metadb_handle_ptr>& tracks, const std::string& playlistName);
void replaceTrackInActivePlaylist(const metadb_handle_ptr& trackToReplace, const metadb_handle_ptr& replacement);

} // namespace lastfmgrabber
