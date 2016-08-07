#include "ContextMenu.h"

#include "FoobarSDKWrapper.h"

#include "BestVersion.h"
#include "LastFm.h"
#include "PlaylistGenerator.h"

#include <map>

using namespace bestversion;

namespace {

// {BC1DCA96-02B5-4E33-A543-5ADA223B16E4}
const GUID lastFmContextMenuGroupGUID = { 0xbc1dca96, 0x2b5, 0x4e33, { 0xa5, 0x43, 0x5a, 0xda, 0x22, 0x3b, 0x16, 0xe4 } };
const contextmenu_group_popup_factory lastFmContextMenuGroupFactory(lastFmContextMenuGroupGUID, contextmenu_groups::root, "Last.fm", 0.0f);

// {B04B2F1C-9BC0-409E-ADBE-C35285F9D310}
static const GUID bestVersionContextMenuGroupGUID = { 0xb04b2f1c, 0x9bc0, 0x409e, { 0xad, 0xbe, 0xc3, 0x52, 0x85, 0xf9, 0xd3, 0x10 } };
const contextmenu_group_popup_factory bestVersionContextMenuGroupFactory(bestVersionContextMenuGroupGUID, contextmenu_groups::root, "Best Version", 0.0f);

//------------------------------------------------------------------------------

void generateArtistPlaylist(const pfc::list_base_const_t<metadb_handle_ptr>& tracks);
void replaceWithBestVersion(const pfc::list_base_const_t<metadb_handle_ptr>& tracks);
void replaceWithLibraryVersion(const pfc::list_base_const_t<metadb_handle_ptr>& tracks, bool acrossAllPlaylists=false);

//------------------------------------------------------------------------------

} // anonymous namespace

namespace bestversion {

class LastFmGrabberContextMenu : public contextmenu_item_simple
{
public:
	struct Items
	{
		enum
		{
			GetArtistTopTracks = 0,
			MAX
		};
	};

	virtual GUID
	get_parent()
	{
		return lastFmContextMenuGroupGUID;
	}

	virtual unsigned int
	get_num_items()
	{
		return Items::MAX;
	}

	virtual void
	get_item_name(
		unsigned int		index,
		pfc::string_base&	out
	)
	{
		switch(index)
		{
			case Items::GetArtistTopTracks:
			{
				out = "Get artist top tracks";
				break;
			}

			default:
			{
				uBugCheck();
			}
		}
	}

	virtual void
	context_command(
		unsigned int			index,
		metadb_handle_list_cref	tracks,
		const GUID&				/*caller*/
	)
	{
		switch(index)
		{
			case Items::GetArtistTopTracks:
			{
				generateArtistPlaylist(tracks);
				break;
			}

			default:
			{
				uBugCheck();
			}
		}
	}

	virtual bool
	context_get_display(
		unsigned int			index,
		metadb_handle_list_cref	tracks,
		pfc::string_base&		out,
		unsigned int&			/*displayflags*/,
		const GUID&				/*caller*/
	)
	{
		switch(index)
		{
			case Items::GetArtistTopTracks:
			{
				const auto& mainArtist = getMainArtist(tracks);

				const t_size stringLength = mainArtist.length();

				if(stringLength > 0)
				{
					// We have found a main artist; set the display of the item to be of the form:
					// "Artist's top tracks".
					out = mainArtist.c_str();

					// Grammar alert! Artists ending with "s" get just a "'" rather than "'s".
					if(out[stringLength - 1] == 's')
					{
						out.add_string("' top tracks");
					}
					else
					{
						out.add_string("'s top tracks");
					}
				}
				else
				{
					// Failed to find a main artist; return the normal, non-custom name for the item.
					get_item_name(index, out);
				}

				return true;
			}

			default:
			{
				// Nothing wants to customise the display of the item; let the regular name be displayed.
				get_item_name(index, out);
				return true;
			}
		}
	}

	virtual GUID
	get_item_guid(
		unsigned int index
	)
	{
		// {6D1AF128-2C33-4214-BAB2-6119D43D968F}
		static const GUID GetArtistTopTracksGUID = { 0x6d1af128, 0x2c33, 0x4214, { 0xba, 0xb2, 0x61, 0x19, 0xd4, 0x3d, 0x96, 0x8f } };

		switch(index)
		{
			case Items::GetArtistTopTracks:
			{
				return GetArtistTopTracksGUID;
			}

			default:
			{
				uBugCheck();
			}
		}
	}

	virtual bool
	get_item_description(
		unsigned int		index,
		pfc::string_base&	out
	)
	{
		switch(index)
		{
			case Items::GetArtistTopTracks:
			{
				out = "Generate a playlist containing last.fm's top tracks for the selected artist.";
				return true;
			}

			default:
			{
				uBugCheck();
			}
		}
	}
};

//------------------------------------------------------------------------------

static contextmenu_item_factory_t<LastFmGrabberContextMenu> lastFmGrabberContextMenuFactory;

//------------------------------------------------------------------------------

class BestVersionContextMenu : public contextmenu_item_simple
{
public:
	struct Items
	{
		enum
		{
			ReplaceWithBestVersion = 0,
			ReplaceWithLibraryVersion,
			ReplaceWIthLibraryVersionAcrossAllPlaylists,
			MAX
		};
	};

	virtual GUID
	get_parent()
	{
		return bestVersionContextMenuGroupGUID;
	}

	virtual unsigned int
	get_num_items()
	{
		return Items::MAX;
	}

	virtual void
	get_item_name(
		unsigned int		index,
		pfc::string_base&	out
	)
	{
		switch(index)
		{
			case Items::ReplaceWithBestVersion:
			{
				out = "Replace with best version of track";
				break;
			}

			case Items::ReplaceWithLibraryVersion:
			{
				out = "Replace with same track in the library";
				break;
			}

			case Items::ReplaceWIthLibraryVersionAcrossAllPlaylists:
			{
				out = "Replace with same track in the library in all playlists";
				break;
			}

			default:
			{
				uBugCheck();
			}
		}
	}

	virtual void
	context_command(
		unsigned int			index,
		metadb_handle_list_cref	tracks,
		const GUID&				/*caller*/
	)
	{
		switch(index)
		{
			case Items::ReplaceWithBestVersion:
			{
				replaceWithBestVersion(tracks);
				break;
			}

			case Items::ReplaceWithLibraryVersion:
			{
				replaceWithLibraryVersion(tracks);
				break;
			}

			case Items::ReplaceWIthLibraryVersionAcrossAllPlaylists:
			{
				replaceWithLibraryVersion(tracks, true);
				break;
			}

			default:
			{
				uBugCheck();
			}
		}
	}

	virtual bool
	context_get_display(
		unsigned int			index,
		metadb_handle_list_cref	tracks,
		pfc::string_base&		out,
		unsigned int&			/*displayflags*/,
		const GUID&				/*caller*/
	)
	{
		switch(index)
		{
			case Items::ReplaceWithBestVersion:
			{
				out = "Replace with best version of track";

				if(tracks.get_count() > 1)
				{
					out.add_string("s");
				}

				return true;
			}

			default:
			{
				// Nothing wants to customise the display of the item; let the regular name be displayed.
				get_item_name(index, out);
				return true;
			}
		}
	}

	virtual GUID
	get_item_guid(
		unsigned int index
	)
	{
		// {4CAA2F50-2818-4DE5-B682-FB36483C9A5E}
		static const GUID ReplaceWithBestVersionGUID = { 0x4caa2f50, 0x2818, 0x4de5, { 0xb6, 0x82, 0xfb, 0x36, 0x48, 0x3c, 0x9a, 0x5e } };

		// {B9144A8F-8839-49D7-B2DA-C9CE9ED7517E}
		static const GUID ReplaceWithLibraryVersionGUID = { 0xb9144a8f, 0x8839, 0x49d7,{ 0xb2, 0xda, 0xc9, 0xce, 0x9e, 0xd7, 0x51, 0x7e } };

		// {1B059698-B4EC-4E1F-962A-6C749AC14985}
		static const GUID ReplaceWithLibraryVersionAcrossAllPlaylistsGUID =	{ 0x1b059698, 0xb4ec, 0x4e1f,{ 0x96, 0x2a, 0x6c, 0x74, 0x9a, 0xc1, 0x49, 0x85 } };



		switch(index)
		{
			case Items::ReplaceWithBestVersion:
			{
				return ReplaceWithBestVersionGUID;
			}

			case Items::ReplaceWithLibraryVersion:
			{
				return ReplaceWithLibraryVersionGUID;
			}

			case Items::ReplaceWIthLibraryVersionAcrossAllPlaylists:
			{
				return ReplaceWithLibraryVersionAcrossAllPlaylistsGUID;
			}

			default:
			{
				uBugCheck();
			}
		}
	}

	virtual bool
	get_item_description(
		unsigned int		index,
		pfc::string_base&	out
	)
	{
		switch(index)
		{
			case Items::ReplaceWithBestVersion:
			{
				out = "Replace a track in a playlist with a better version of the track from the library.";
				return true;
			}

			case Items::ReplaceWithLibraryVersion:
			{
				out = "Replace a track in a playlist with the same file from the library, if it exists.";
				return true;
			}

			case Items::ReplaceWIthLibraryVersionAcrossAllPlaylists:
			{
				out = "Replace a track in a playlist with the same file from the library, if it exists, across all playlists.";
				return true;
			}

			default:
			{
				uBugCheck();
			}
		}
	}
};

//------------------------------------------------------------------------------

static contextmenu_item_factory_t<BestVersionContextMenu> bestVersionContextMenuFactory;

//------------------------------------------------------------------------------

} // namespace bestversion

//------------------------------------------------------------------------------

namespace {

//------------------------------------------------------------------------------

class ArtistPlaylistGenerator : public threaded_process_callback
{
private:
	std::string artist;
	pfc::list_t<metadb_handle_ptr> library;
	pfc::list_t<metadb_handle_ptr> tracks;
	bool success;

public:
	ArtistPlaylistGenerator(const std::string& artist_)
		: artist(artist_)
		, success(false)
	{
	}

	virtual void on_init(HWND /*p_wnd*/)
	{
		static_api_ptr_t<library_manager> lm;
		lm->get_all_items(library);
	}

	virtual void run(threaded_process_status& p_status, abort_callback& p_abort)
	{
		try
		{
			console::printf("Downloading chart listing from Last.Fm... for %s", artist.c_str());

			p_status.set_item("Downloading chart listing from Last.Fm...");
			p_status.set_progress_float(0.0f);
			p_status.force_update();

			const bestversion::ArtistChart trackList = bestversion::getArtistChart(
				artist,
				[](const std::string& message){ console::print(message.c_str()); },
				p_abort
			);

			p_abort.check();
			p_status.set_item("Generating Playlist...");
			p_status.set_progress_float(0.5f);
			p_status.force_update();

			// Just look at one artist.
			filterTracksByArtist(artist, library);

			for(auto iter = trackList.begin(); iter != trackList.end(); iter++ )
			{
				// Copy the library then filter it to tracks with this title.
				auto subsetOfLibrary = library;
				filterTracksByCloseTitle(iter->second, subsetOfLibrary);

				// Pick the best version of all these tracks and add it to the list if found.
				metadb_handle_ptr track = getBestTrackByTitle(iter->second, subsetOfLibrary);

				if(track != 0)
				{
					tracks.add_item(track);
				}
			}

			if(tracks.get_count() == 0)
			{
				throw pfc::exception("Did not find enough tracks to make a playlist");
			}

			success = true;
			p_abort.check();
		}
		catch(exception_aborted&)
		{
			success = false;
		}
	}

	virtual void on_done(HWND /*p_wnd*/, bool /*p_was_aborted*/)
	{
		if (!success)
		{
			return;
		}

		generatePlaylistFromTracks(tracks, artist + "'s top tracks");
	}
};

//------------------------------------------------------------------------------

void generateArtistPlaylist(const pfc::list_base_const_t<metadb_handle_ptr>& tracks)
{
	const std::string mainArtist = getMainArtist(tracks);

	if(!mainArtist.empty())
	{
		std::string title("Generating top tracks playlist for ");
		title += mainArtist;

		console::print(title.c_str());

		// New this up since it's going to live on another thread, which will delete it when it's ready.
		auto generator = new service_impl_t<ArtistPlaylistGenerator>(mainArtist);

		static_api_ptr_t<threaded_process> tp;

		tp->run_modeless(
			generator,
			tp->flag_show_abort | tp->flag_show_item,
			core_api::get_main_window(),
			title.c_str(),
			pfc_infinite
		);
	}
	else
	{
		console::error("no Artist Information found");
	}
}

//------------------------------------------------------------------------------

void replaceWithBestVersion(const metadb_handle_ptr& track)
{
	service_ptr_t<metadb_info_container> outInfo;
	if(!track->get_async_info_ref(outInfo))
	{
		console::printf("Couldn't get file info for file %s", track->get_path());
		return;
	}

	const file_info& fileInfo = outInfo->info();

	if(!fileInfo.meta_exists("title"))
	{
		console::printf("File is missing track tag: %s", track->get_path());
		return;
	}

	const bool has_artist_tag = fileInfo.meta_exists("artist");
	const bool has_album_artist_tag = fileInfo.meta_exists("album artist");
	if(!has_artist_tag && !has_album_artist_tag)
	{
		console::printf("File is missing artist and album artist tags: %s", track->get_path());
		return;
	}

	const std::string artist = has_artist_tag ? fileInfo.meta_get("artist", 0) : fileInfo.meta_get("album artist", 0);
	const std::string title = fileInfo.meta_get("title", 0);

	if(artist == "" || title == "")
	{
		console::printf("File has empty artist or track tag: %s", track->get_path());
		return;
	}

	pfc::list_t<metadb_handle_ptr> library;

	static_api_ptr_t<library_manager> lm;
	lm->get_all_items(library);

	filterTracksByArtist(artist, library);
	filterTracksByCloseTitle(title, library);

	const metadb_handle_ptr bestVersionOfTrack = getBestTrackByTitle(title, library);

	if(bestVersionOfTrack == 0)
	{
		console::printf("Couldn't find a better version of %s", title.c_str());
	}
	else
	{
		replaceTrackInActivePlaylist(track, bestVersionOfTrack);
	}
}

void replaceWithBestVersion(const pfc::list_base_const_t<metadb_handle_ptr>& tracks)
{
	for(t_size index = 0; index < tracks.get_count(); ++index)
	{
		replaceWithBestVersion(tracks[index]);
	}
}

//------------------------------------------------------------------------------

void replaceWithLibraryVersion(const metadb_handle_ptr& track, bool acrossAllPlaylists)
{
	service_ptr_t<metadb_info_container> outInfo;
	if (!track->get_async_info_ref(outInfo))
	{
		console::printf("Couldn't get file info for file %s", track->get_path());
		return;
	}

	const file_info& fileInfo = outInfo->info();

	if (!fileInfo.meta_exists("title"))
	{
		console::printf("File is missing title tag: %s", track->get_path());
		return;
	}

	if (!fileInfo.meta_exists("tracknumber"))
	{
		console::printf("File is missing track number tag: %s", track->get_path());
		return;
	}

	if (!fileInfo.meta_exists("album"))
	{
		console::printf("File is missing album tag: %s", track->get_path());
		return;
	}

	const bool has_artist_tag = fileInfo.meta_exists("artist");
	const bool has_album_artist_tag = fileInfo.meta_exists("album artist");
	if (!has_artist_tag && !has_album_artist_tag)
	{
		console::printf("File is missing artist and album artist tags: %s", track->get_path());
		return;
	}

	const std::string artist = has_artist_tag ? fileInfo.meta_get("artist", 0) : fileInfo.meta_get("album artist", 0);
	const std::string title = fileInfo.meta_get("title", 0);
	const std::string album = fileInfo.meta_get("album", 0);
	const std::string track_n = fileInfo.meta_get("tracknumber", 0);


	if (artist == "" || title == "" || album == "" || track_n == "")
	{
		console::printf("File has empty artist or title or track or album tag: %s", track->get_path());
		return;
	}

	pfc::list_t<metadb_handle_ptr> library;

	static_api_ptr_t<library_manager> lm;
	lm->get_all_items(library);

	filterTracksByArtist(artist, library);
	filterTracksByCloseTitle(title, library, true);
	filterTracksByTagField("album", album, library);
	filterTracksByTagField("tracknumber", track_n, library);


	const metadb_handle_ptr bestVersionOfTrack = getBestTrackByTitle(title, library);

	if (bestVersionOfTrack == 0)
	{
		console::printf("Couldn't find a library version of %s", title.c_str());
	}
	else if (!acrossAllPlaylists)
	{
		replaceTrackInActivePlaylist(track, bestVersionOfTrack);
	}
	else
	{
		replaceTrackInAllPlaylists(track, bestVersionOfTrack);
	}
}

void replaceWithLibraryVersion(const pfc::list_base_const_t<metadb_handle_ptr>& tracks, bool acrossAllPlaylists)
{
	for (t_size index = 0; index < tracks.get_count(); ++index)
	{
		replaceWithLibraryVersion(tracks[index], acrossAllPlaylists);
	}
}

}
