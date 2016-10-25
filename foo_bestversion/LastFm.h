#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "FoobarSDKWrapper.h"

namespace bestversion {

typedef std::vector<std::pair<unsigned long, std::string>> ArtistChart;

ArtistChart getArtistChart(const std::string& artist, const std::function<void (const std::string&)>& log, foobar2000_io::abort_callback& callback);

struct ArtistAndTrack
{
	std::string artist;
	std::string artistMBID;
	std::string track;
	std::string trackMBID;
};

typedef std::vector<ArtistAndTrack> SimilarTracks;

SimilarTracks getTrackSimilarTracks(const std::string& artist, const std::string& track, const std::function<void (const std::string&)>& log, foobar2000_io::abort_callback& callback);

} // namespace bestversion
