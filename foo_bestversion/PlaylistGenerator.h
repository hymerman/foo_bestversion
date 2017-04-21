#pragma once

#include "FoobarSDKWrapper.h"

namespace bestversion {

void generatePlaylistFromTracks(const pfc::list_t<metadb_handle_ptr>& tracks);
void generatePlaylistFromTracks(const pfc::list_t<metadb_handle_ptr>& tracks, const std::string& playlistName);

// Note that no undo backup is performed; make sure to set an undo point before calling this function.
void replaceTrackInActivePlaylist(const metadb_handle_ptr& trackToReplace, const metadb_handle_ptr& replacement);
void replaceTrackInAllPlaylists(const metadb_handle_ptr& trackToReplace, const metadb_handle_ptr& replacement);

} // namespace bestversion
