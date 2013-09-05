#include "BestVersion.h"

#include "DatabaseScopeLock.h"
#include "Maths.h"
#include "ToString.h"

#include <limits>
#include <map>

namespace bestversion {

//------------------------------------------------------------------------------

// database has to be locked before calling this function
std::string getMainArtist(metadb_handle_list_cref tracks)
{
	// Lock the database for the duration of this scope.
	DatabaseScopeLock databaseLock;

	// Keep track of the number of each artist name we encounter.
	std::map<const char*, t_size> artists;

	// For each track, increment the number of occurrences of its artist.
	for(t_size i = 0; i < tracks.get_count(); i++)
	{
		const file_info* fileInfo;
		if(tracks[i]->get_info_async_locked(fileInfo) && fileInfo->meta_exists("artist"))
		{
			const char * artist = fileInfo->meta_get("artist",0);
			++artists[artist];
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

// database has to be locked before calling this function
inline bool isTrackByArtist(const std::string& artist, const metadb_handle_ptr& track)
{
	// todo: ignore slight differences, e.g. in punctuation
	const file_info* fileInfo;

	if(track->get_info_async_locked(fileInfo))
	{
		for(t_size j = 0; j < fileInfo->meta_get_count_by_name("artist"); j++)
		{
			if(stricmp_utf8(fileInfo->meta_get("artist", j), artist.c_str()) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------------

// database has to be locked before calling this function
bool doesTrackHaveSimilarTitle(const std::string& title, const metadb_handle_ptr& track)
{
	// todo: ignore slight differences, e.g. in punctuation
	const file_info * fileInfo;
	if(!track->get_info_async_locked(fileInfo) || !fileInfo->meta_exists("title"))
	{
		return false;
	}

	const std::string fileTitle = fileInfo->meta_get("title", 0);

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

// database has to be locked before calling this function
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

// database has to be locked before calling this function
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

// database has to be locked before calling this function
float calculateTrackRating(const std::string& title, const metadb_handle_ptr& track)
{
	const file_info * fileInfo;
	if(!track->get_info_async_locked(fileInfo) || !fileInfo->meta_exists("title"))
	{
		// Don't pick it if it doesn't have a title.
		return -1.0f;
	}

	float totalRating = 0.0f;

	const std::string fileTitle = fileInfo->meta_get("title", 0);

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

	if(fileInfo->meta_exists("PLAY_COUNTER"))
	{
		const int playCount = atoi(fileInfo->meta_get("PLAY_COUNTER",0));

		static const float lowPlayCount = 0.0f;
		static const float highPlayCount = 10.0f;

		static const float lowPlayCountRating = 0.0f;
		static const float highPlayCountRating = 0.5f;

		const float playCountRating = maths::map(static_cast<float>(playCount), lowPlayCount, highPlayCount, lowPlayCountRating, highPlayCountRating);

		totalRating += playCountRating;
	}

	const auto bitrate = fileInfo->info_get_bitrate();

	static const float lowBitrate = 0.0f;
	static const float highBitrate = 1000.0f;

	static const float lowBitrateRating = 0.0f;
	static const float highBitrateRating = 2.0f;

	const float bitrateRating = maths::map(static_cast<float>(bitrate), lowBitrate, highBitrate, lowBitrateRating, highBitrateRating);

	totalRating += bitrateRating;

	static const float releaseTypeWeighting = 3.0f;
	float releaseTypeRating = 0.55f;	// Default for if nothing is set; assume it's somewhere between a live album and a soundtrack.

	if(fileInfo->meta_exists("musicbrainz album type") || fileInfo->meta_exists("releasetype"))
	{
		const std::string albumType = fileInfo->meta_exists("musicbrainz album type") ? fileInfo->meta_get("musicbrainz album type", 0) : fileInfo->meta_get("releasetype", 0);

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

	if(fileInfo->meta_exists("album artist") && fileInfo->meta_exists("artist"))
	{
		const std::string artist = fileInfo->meta_get("artist", 0);
		const std::string albumArtist = fileInfo->meta_get("album artist", 0);

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

// database has to be locked before calling this function
// all tracks will be analysed; try to cut the size of the list down before calling.
metadb_handle_ptr getBestTrackByTitle(const std::string& title, const pfc::list_base_t<metadb_handle_ptr>& tracks)
{
	if(tracks.get_count() == 1)
	{
		console::printf(("Only one version of " + title + " exists in library").c_str());
		return tracks[0];
	}

	console::printf(("Finding best version of " + title + ". " + to_string(tracks.get_count()) + " candidates").c_str());

	metadb_handle_ptr bestTrack = 0;
	float bestTrackRating = std::numeric_limits<float>::min();

	for(t_size index = 0; index < tracks.get_count(); index++)
	{
		const float trackRating = calculateTrackRating(title, tracks[index]);

		console::printf(("Rating: " + to_string(trackRating, 2) + ": " + tracks[index]->get_path()).c_str());

		if(trackRating >= 0.0f && trackRating > bestTrackRating)
		{
			bestTrackRating = trackRating;
			bestTrack = tracks[index];
		}
	}

	if(bestTrack == 0)
	{
		console::printf(("Couldn't find a match for " + title).c_str());
	}
	else
	{
		console::printf(("Picked track with rating: " + to_string(bestTrackRating, 2) + ": " + bestTrack->get_path()).c_str());
	}

	return bestTrack;
}

//------------------------------------------------------------------------------

} // namespace bestversion
