#include "PlaylistGenerator.h"

#include "FoobarSDKWrapper.h"

namespace bestversion {

//------------------------------------------------------------------------------

void generatePlaylistFromTracks(const pfc::list_t<metadb_handle_ptr>& tracks)
{
	generatePlaylistFromTracks(tracks, "New Playlist");
}

//------------------------------------------------------------------------------

void generatePlaylistFromTracks(const pfc::list_t<metadb_handle_ptr>& tracks, const std::string& playlistName)
{
	static_api_ptr_t<playlist_manager> pm;
	t_size playlist = pm->find_playlist(playlistName.c_str(), pfc_infinite);

	if(playlist != pfc_infinite)
	{
		pm->playlist_undo_backup(playlist);
		pm->playlist_clear(playlist);
	}
	else
	{
		playlist = pm->create_playlist(
			playlistName.c_str(),
			pfc_infinite,
			pfc_infinite
		);
	}

	pm->playlist_add_items(playlist, tracks, bit_array_true());
	pm->set_active_playlist(playlist);
	pm->set_playing_playlist(playlist);
}

//------------------------------------------------------------------------------

void replaceTrackInActivePlaylist(const metadb_handle_ptr& trackToReplace, const metadb_handle_ptr& replacement)
{
	t_size index = 0;
	static_api_ptr_t<playlist_manager> pm;

	if(!pm->activeplaylist_find_item(trackToReplace, index))
	{
		console::printf("Couldn't find track in active playlist to replace it: %s", trackToReplace->get_path());
		return;
	}

	if(!pm->activeplaylist_replace_item(index, replacement))
	{
		console::printf("Couldn't replace track in active playlist: %s", trackToReplace->get_path());
		return;
	}
}

//------------------------------------------------------------------------------

} // namespace bestversion
