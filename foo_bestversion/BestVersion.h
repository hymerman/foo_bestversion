#pragma once

#include "FoobarSDKWrapper.h"

#include <string>

namespace bestversion {

std::string getMainArtist(metadb_handle_list_cref tracks);

bool isTrackByArtist(const std::string& artist, const metadb_handle_ptr& track);

bool doesTrackHaveExactTagFieldValue(const std::string& field_name, const std::string& field_value, const metadb_handle_ptr& track);
bool doesTrackHaveSimilarTitle(const std::string& title, const metadb_handle_ptr& track, bool exact=false);

void filterTracksByTagField(const std::string& field_name, const std::string& field_value, pfc::list_base_t<metadb_handle_ptr>& tracks);
void filterTracksByArtist(const std::string& artist, pfc::list_base_t<metadb_handle_ptr>& tracks);

void filterTracksByCloseTitle(const std::string& title, pfc::list_base_t<metadb_handle_ptr>& tracks, bool exact=false);

bool fileTitlesMatchExcludingBracketsOnLhs(const std::string& lhs, const std::string& rhs);

float calculateTrackRating(const std::string& title, const metadb_handle_ptr& track);

// all tracks will be analysed; try to cut the size of the list down before calling.
metadb_handle_ptr getBestTrackByTitle(const std::string& title, const pfc::list_base_t<metadb_handle_ptr>& tracks);

} // namespace bestversion
