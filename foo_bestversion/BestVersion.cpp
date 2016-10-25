#include "BestVersion.h"

#include "Maths.h"
#include "ToString.h"

#include <limits>
#include <map>

namespace bestversion {

//------------------------------------------------------------------------------

std::string getMainArtist(metadb_handle_list_cref tracks)
{
	// Don't look at too many tracks - this method should be quick but won't be if we don't limit ourselves.
	static const t_size maxNumberOfTracksToConsider = 100;

	// Keep track of the number of each artist name we encounter.
	std::map<const char*, t_size> artists;

	// For each track, increment the number of occurrences of its artist.
	for(t_size i = 0; i < tracks.get_count() && i < maxNumberOfTracksToConsider; i++)
	{
		service_ptr_t<metadb_info_container> outInfo;
		if(tracks[i]->get_async_info_ref(outInfo))
		{
			const file_info& fileInfo = outInfo->info();
			const bool has_artist_tag = fileInfo.meta_exists("artist");
			const bool has_album_artist_tag = fileInfo.meta_exists("album artist");
			if(has_artist_tag || has_album_artist_tag)
			{
				const char * artist = has_artist_tag ? fileInfo.meta_get("artist", 0) : fileInfo.meta_get("album artist", 0);
				++artists[artist];
			}
		}
	}

	// Find the artist that occurred the most.
	t_size maxCount = 0;
	const char* maxArtist = "";
	for(auto iter = artists.begin(); iter != artists.end(); iter++)
	{
		if((iter->second) > maxCount)
		{
			maxCount = iter->second;
			maxArtist = iter->first;
		}
	}

	// We've kept the database locked this whole time because we're just storing char* in the map, not copying the strings.
	return maxArtist;
}

//------------------------------------------------------------------------------

std::string getArtist(metadb_handle_ptr track)
{
	service_ptr_t<metadb_info_container> outInfo;
	if(!track->get_async_info_ref(outInfo))
	{
		return "";
	}

	const file_info& fileInfo = outInfo->info();
	const bool has_artist_tag = fileInfo.meta_exists("artist");
	const bool has_album_artist_tag = fileInfo.meta_exists("album artist");

	if(!(has_artist_tag || has_album_artist_tag))
	{
		return "";
	}

	const char* artistName = has_artist_tag ? fileInfo.meta_get("artist", 0) : fileInfo.meta_get("album artist", 0);

	return artistName;
}

//------------------------------------------------------------------------------

std::string getTitle(metadb_handle_ptr track)
{
	service_ptr_t<metadb_info_container> outInfo;
	if(!track->get_async_info_ref(outInfo))
	{
		return "";
	}

	const file_info& fileInfo = outInfo->info();
	const bool has_title_tag = fileInfo.meta_exists("title");

	if(!has_title_tag)
	{
		return "";
	}

	const char* title = fileInfo.meta_get("title", 0);

	return title;
}

//------------------------------------------------------------------------------

inline bool isTrackByArtist(const std::string& artist, const metadb_handle_ptr& track)
{
	// todo: ignore slight differences, e.g. in punctuation

	service_ptr_t<metadb_info_container> outInfo;
	if(track->get_async_info_ref(outInfo))
	{
		const file_info& fileInfo = outInfo->info();
		for(t_size j = 0; j < fileInfo.meta_get_count_by_name("artist"); j++)
		{
			if(stricmp_utf8(fileInfo.meta_get("artist", j), artist.c_str()) == 0)
			{
				return true;
			}
		}

		for(t_size j = 0; j < fileInfo.meta_get_count_by_name("album artist"); j++)
		{
			if(stricmp_utf8(fileInfo.meta_get("album artist", j), artist.c_str()) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------

bool doesTrackHaveSimilarTitle(const std::string& title, const metadb_handle_ptr& track)
{
	// todo: ignore slight differences, e.g. in punctuation
	service_ptr_t<metadb_info_container> outInfo;
	if (!track->get_async_info_ref(outInfo))
	{
		return false;
	}

	const file_info& fileInfo = outInfo->info();
	
	if(!fileInfo.meta_exists("title"))
	{
		return false;
	}


	const std::string fileTitle = fileInfo.meta_get("title", 0);

	if(stricmp_utf8(fileTitle.c_str(), title.c_str()) == 0)
	{
		return true;
	}
	else if(fileTitlesMatchExcludingBracketsOnLhs(fileTitle, title))
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------

void filterTracksByArtist(const std::string& artist, pfc::list_base_t<metadb_handle_ptr>& tracks)
{
	const t_size n = tracks.get_count();
	bit_array_bittable deleteMask(n);

	for(t_size i = 0; i < n; i++)
	{
		deleteMask.set(i, !isTrackByArtist(artist, tracks[i]));
	}

	tracks.remove_mask(deleteMask);
}

//------------------------------------------------------------------------------

void filterTracksByCloseTitle(const std::string& title, pfc::list_base_t<metadb_handle_ptr>& tracks)
{
	const t_size n = tracks.get_count();
	bit_array_bittable deleteMask(n);

	for(t_size i = 0; i < n; i++)
	{
		deleteMask.set(i, !doesTrackHaveSimilarTitle(title, tracks[i]));
	}

	tracks.remove_mask(deleteMask);
}

//------------------------------------------------------------------------------

bool fileTitlesMatchExcludingBracketsOnLhs(const std::string& lhs, const std::string& rhs)
{
	auto lhsBracketPos = strcspn(lhs.c_str(),"([");
	if(lhsBracketPos != pfc_infinite)
	{
		if((stricmp_utf8_ex(lhs.c_str(), lhsBracketPos - 1u, rhs.c_str(), pfc_infinite) == 0)
			|| (stricmp_utf8_ex(lhs.c_str(), lhsBracketPos, rhs.c_str(), pfc_infinite) == 0))
		{
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------

float calculateTrackRating(const std::string& title, const metadb_handle_ptr& track)
{
	service_ptr_t<metadb_info_container> outInfo;
	if(!track->get_async_info_ref(outInfo))
	{
		// Don't pick it we can't get any info.
		return -1.0f;
	}

	const file_info& fileInfo = outInfo->info();

	if (!fileInfo.meta_exists("title"))
	{
		// Don't pick it if it doesn't have a title.
		return -1.0f;
	}

	float totalRating = 0.0f;

	const std::string fileTitle = fileInfo.meta_get("title", 0);

	// Assume title is already roughly correct.
	if(stricmp_utf8(fileTitle.c_str(), title.c_str()) == 0)
	{
		static const float ratingForExactTitleMatch = 2.0f;

		totalRating += ratingForExactTitleMatch;
	}
	else
	{
		static const float ratingForTitleMatchWithBrackets = 1.0f;

		totalRating += ratingForTitleMatchWithBrackets;
	}

	if(fileInfo.meta_exists("PLAY_COUNTER"))
	{
		const int playCount = atoi(fileInfo.meta_get("PLAY_COUNTER",0));

		static const float lowPlayCount = 0.0f;
		static const float highPlayCount = 10.0f;

		static const float lowPlayCountRating = 0.0f;
		static const float highPlayCountRating = 0.5f;

		const float playCountRating = maths::map(static_cast<float>(playCount), lowPlayCount, highPlayCount, lowPlayCountRating, highPlayCountRating);

		totalRating += playCountRating;
	}

	const auto bitrate = fileInfo.info_get_bitrate();

	static const float lowBitrate = 0.0f;
	static const float highBitrate = 1000.0f;

	static const float lowBitrateRating = 0.0f;
	static const float highBitrateRating = 2.0f;

	const float bitrateRating = maths::map(static_cast<float>(bitrate), lowBitrate, highBitrate, lowBitrateRating, highBitrateRating);

	totalRating += bitrateRating;

	static const float releaseTypeWeighting = 3.0f;
	float releaseTypeRating = 0.55f;	// Default for if nothing is set; assume it's somewhere between a live album and a soundtrack.

	if(fileInfo.meta_exists("musicbrainz album type") || fileInfo.meta_exists("releasetype"))
	{
		const std::string albumType = fileInfo.meta_exists("musicbrainz album type") ? fileInfo.meta_get("musicbrainz album type", 0) : fileInfo.meta_get("releasetype", 0);

		if(albumType == "album")
		{
			releaseTypeRating = 1.0f;
		}
		else if(albumType == "single")
		{
			releaseTypeRating = 0.9f;
		}
		else if(albumType == "compilation")
		{
			releaseTypeRating = 0.8f;
		}
		else if(albumType == "ep")
		{
			releaseTypeRating = 0.7f;
		}
		else if(albumType == "soundtrack")
		{
			releaseTypeRating = 0.6f;
		}
		else if(albumType == "live")
		{
			releaseTypeRating = 0.5f;
		}
		else if(albumType == "other")
		{
			releaseTypeRating = 0.4f;
		}
		else if(albumType == "remix")
		{
			releaseTypeRating = 0.3f;
		}
		else
		{
			releaseTypeRating = 1.0f;
		}
	}

	totalRating += releaseTypeRating * releaseTypeWeighting;
	
	static float albumArtistWeighting = 1.0f;
	float albumArtistRating = 1.0f;

	if(fileInfo.meta_exists("album artist") && fileInfo.meta_exists("artist"))
	{
		const std::string artist = fileInfo.meta_get("artist", 0);
		const std::string albumArtist = fileInfo.meta_get("album artist", 0);

		if(albumArtist == artist)
		{
			albumArtistRating = 1.0f;
		}
		else if(albumArtist == "Various Artists")
		{
			albumArtistRating = 0.333f;
		}
		else
		{
			// Artist appearing on someone else's album?
			albumArtistRating = 0.666f;
		}
	}

	totalRating += albumArtistRating * albumArtistWeighting;

	return totalRating;
}

//------------------------------------------------------------------------------

// all tracks will be analysed; try to cut the size of the list down before calling.
metadb_handle_ptr getBestTrackByTitle(const std::string& title, const pfc::list_base_t<metadb_handle_ptr>& tracks)
{
	if(tracks.get_count() == 1)
	{
		console::info(("Only one version of " + title + " exists in library").c_str());
		return tracks[0];
	}

	console::info(("Finding best version of " + title + ". " + to_string(tracks.get_count()) + " candidates").c_str());

	metadb_handle_ptr bestTrack = 0;
	float bestTrackRating = std::numeric_limits<float>::min();

	for(t_size index = 0; index < tracks.get_count(); index++)
	{
		const float trackRating = calculateTrackRating(title, tracks[index]);

		console::info(("Rating: " + to_string(trackRating, 2) + ": " + tracks[index]->get_path()).c_str());

		if(trackRating >= 0.0f && trackRating > bestTrackRating)
		{
			bestTrackRating = trackRating;
			bestTrack = tracks[index];
		}
	}

	if(bestTrack == 0)
	{
		console::info(("Couldn't find a match for " + title).c_str());
	}
	else
	{
		console::info(("Picked track with rating: " + to_string(bestTrackRating, 2) + ": " + bestTrack->get_path()).c_str());
	}

	return bestTrack;
}

//------------------------------------------------------------------------------

} // namespace bestversion
