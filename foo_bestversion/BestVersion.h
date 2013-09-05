#pragma once

#include "FoobarSDKWrapper.h"

#include <string>

namespace bestversion {

// database has to be locked before calling this function
std::string getMainArtist(metadb_handle_list_cref tracks);

// database has to be locked before calling this function
bool isTrackByArtist(const std::string& artist, const metadb_handle_ptr& track);

// database has to be locked before calling this function
bool doesTrackHaveSimilarTitle(const std::string& title, const metadb_handle_ptr& track);

// database has to be locked before calling this function
void filterTracksByArtist(const std::string& artist, pfc::list_base_t<metadb_handle_ptr>& tracks);

// database has to be locked before calling this function
void filterTracksByCloseTitle(const std::string& title, pfc::list_base_t<metadb_handle_ptr>& tracks);

bool fileTitlesMatchExcludingBracketsOnLhs(const std::string& lhs, const std::string& rhs);

// database has to be locked before calling this function
float calculateTrackRating(const std::string& title, const metadb_handle_ptr& track);

// database has to be locked before calling this function
// all tracks will be analysed; try to cut the size of the list down before calling.
metadb_handle_ptr getBestTrackByTitle(const std::string& title, const pfc::list_base_t<metadb_handle_ptr>& tracks);

} // namespace bestversion
