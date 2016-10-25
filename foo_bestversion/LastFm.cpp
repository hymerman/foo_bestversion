#include "LastFm.h"

#include "RapidJsonWrapper.h"

#include "Component.h"
#include "FoobarSDKWrapper.h"
#include "ToString.h"

#include <string>

namespace {

const std::string apiKey = "6fcd59047568e89b1615975081258990";

char to_hex(const char c)
{
	return c < 0xa ? '0' + c : 'a' - 0xa + c;
}

std::string url_encode(const std::string& in)
{
	std::string out;
	out.reserve(in.length());

	for(unsigned char c : in)
	{
		if(isalnum(c))
		{
			out.push_back(c);
		}
		else if(isspace(c))
		{
			out.push_back('+');
		}
		else
		{
			out.push_back('%');
			out.push_back(to_hex(c >> 4));
			out.push_back(to_hex(c % 16));
		}
	}

	return out;
}

}

namespace bestversion {

ArtistChart getArtistChart(const std::string& artist, const std::function<void (const std::string&)>& log, foobar2000_io::abort_callback& callback)
{
	ArtistChart artistChart;

	const size_t limit = 100;

	const std::string uri = std::string("http://ws.audioscrobbler.com/2.0?format=json")
		+ "&api_key=" + apiKey
		+ "&method=artist.getTopTracks"
		+ "&limit=" + to_string(limit)
		+ "&artist=" + url_encode(artist);

	log("Creating client");

	static_api_ptr_t<http_client> http;

	auto request = http->create_request("GET");
	request->add_header("User-Agent", COMPONENT_NAME "/" COMPONENT_VERSION);

	auto response = request->run_ex(uri.c_str(), callback);

	pfc::string8 buffer;
	response->read_string_raw(buffer, callback);

	if(buffer.is_empty())
	{
		throw std::exception("No content returned from last.fm. Perhaps last.fm is currently down?");
	}

	// Take a copy of the string as a vector of chars
	std::vector<char> json(buffer.get_ptr(), buffer.get_ptr() + buffer.get_length());
	json.push_back('\0');

	rapidjson::Document document;
	if (document.Parse<0>(&json[0]).HasParseError())
	{
		throw std::exception("Parse error in json");
	}

	if(!document.IsObject())
	{
		throw std::exception("json document must be an object");
	}

	const auto& topTracks = document["toptracks"];

	if(!topTracks.IsObject())
	{
		throw std::exception("toptracks json value is not an object; last.fm data format not as expected!");
	}

	const auto& tracks = topTracks["track"];

	if(!tracks.IsArray())
	{
		throw std::exception("tracks json value is not an array; last.fm data format not as expected!");
	}

	log("Found " + to_string(tracks.Size()) + " tracks");

	// Iterate over all tracks.
	for(rapidjson::SizeType i = 0; i < tracks.Size(); ++i)
	{
		const auto& track = tracks[i];

		if(!track.IsObject())
		{
			throw std::exception("tracks json array element is not an object; last.fm data format not as expected!");
		}

		// Extract name and playcount and bail out if they're not present or are not strings.
		const auto& nameValue = track["name"];
		const auto& playCountValue = track["playcount"];

		if(!nameValue.IsString() || !playCountValue.IsString())
		{
			throw std::exception("name, mbid or playcount values on a track are not strings; last.fm data format not as expected!");
		}

		// MBID is optional.
		const auto& mbidValue = track["mbid"];

		// Extract a string name, mbid and playCount from the track.
		const std::string name = nameValue.GetString();
		const std::string mbid = mbidValue.IsString() ? mbidValue.GetString() : "No MBID";
		const std::string playCountAsString = playCountValue.GetString();

		// Parse the reach string as an integer.
		const unsigned long playCount = from_string<unsigned long>(playCountAsString);

		log("track: " + name + ", " + mbid + ", " + to_string(playCount));

		// Add the result to our map.
		artistChart.push_back(std::make_pair(playCount, name));
	}

	return artistChart;
}

SimilarTracks getTrackSimilarTracks(const std::string& artist, const std::string& track, const std::function<void (const std::string&)>& log, foobar2000_io::abort_callback& callback)
{
	const size_t limit = 100;

	const std::string uri = std::string("http://ws.audioscrobbler.com/2.0?format=json")
		+ "&api_key=" + apiKey
		+ "&method=track.getSimilar"
		+ "&limit=" + to_string(limit)
		+ "&track=" + url_encode(track)
		+ "&artist=" + url_encode(artist);

	log("Creating client");

	static_api_ptr_t<http_client> http;

	auto request = http->create_request("GET");
	request->add_header("User-Agent", COMPONENT_NAME "/" COMPONENT_VERSION);

	auto response = request->run_ex(uri.c_str(), callback);

	pfc::string8 buffer;
	response->read_string_raw(buffer, callback);

	if(buffer.is_empty())
	{
		throw std::exception("No content returned from last.fm. Perhaps last.fm is currently down?");
	}

	// Take a copy of the string as a vector of chars
	std::vector<char> json(buffer.get_ptr(), buffer.get_ptr() + buffer.get_length());
	json.push_back('\0');

	rapidjson::Document document;
	if (document.Parse<0>(&json[0]).HasParseError())
	{
		throw std::exception("Parse error in json");
	}

	if(!document.IsObject())
	{
		throw std::exception("json document must be an object");
	}

	const auto& topTracks = document["similartracks"];

	if(!topTracks.IsObject())
	{
		throw std::exception("similartracks json value is not an object; last.fm data format not as expected!");
	}

	const auto& tracks = topTracks["track"];

	if(!tracks.IsArray())
	{
		throw std::exception("tracks json value is not an array; last.fm data format not as expected!");
	}

	log("Found " + to_string(tracks.Size()) + " tracks");

	SimilarTracks similarTracks;
	similarTracks.reserve(tracks.Size());

	// Iterate over all tracks.
	for(rapidjson::SizeType i = 0; i < tracks.Size(); ++i)
	{
		const auto& trackValue = tracks[i];

		if(!trackValue.IsObject())
		{
			throw std::exception("tracks json array element is not an object; last.fm data format not as expected!");
		}

		// Extract name and playcount and bail out if they're not present or are not strings.
		const auto& trackNameValue = trackValue["name"];

		if(!trackNameValue.IsString())
		{
			throw std::exception("name value on a track is not a string; last.fm data format not as expected!");
		}

		// MBID is optional.
		const auto& trackMBIDValue = trackValue["mbid"];

		// Extract a string name, mbid and playCount from the track.
		const std::string trackName = trackNameValue.GetString();
		const std::string trackMBID = trackMBIDValue.IsString() ? trackMBIDValue.GetString() : "";
		const auto& artistValue = trackValue["artist"];

		if(!artistValue.IsObject())
		{
			throw std::exception("artist value on a track is not an object; last.fm data format not as expected!");
		}

		const auto& artistNameValue = artistValue["name"];

		if(!artistNameValue.IsString())
		{
			throw std::exception("Artist name value on a track is not a string; last.fm data format not as expected!");
		}

		// MBID is optional.
		const auto& artistMBIDValue = artistValue["mbid"];

		const std::string artistName = artistNameValue.GetString();
		const std::string artistMBID = artistMBIDValue.IsString() ? artistMBIDValue.GetString() : "";

		log("track: " + trackName + ", " + trackMBID + ", " + artistName + ", " + artistMBID);

		// Add the result to our map.
		similarTracks.push_back(ArtistAndTrack{ artistName, artistMBID, trackName, trackMBID });
	}

	return similarTracks;
}

} // namespace bestversion
