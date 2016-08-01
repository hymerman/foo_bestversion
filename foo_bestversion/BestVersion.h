#pragma once

#include "FoobarSDKWrapper.h"

#include <string>

namespace bestversion {

std::string getMainArtist(metadb_handle_list_cref tracks);

bool isTrackByArtist(const std::string& artist, const metadb_handle_ptr& track);

bool doesTrackHaveSimilarTitle(const std::string& title, const metadb_handle_ptr& track);
bool doesTrackHaveExactTitle(const std::string& title, const metadb_handle_ptr& track);
bool doesTrackHaveExactAlbum(const std::string& album, const metadb_handle_ptr& track);
bool doesTrackHaveExactTrackNumber(const std::string& tracknumber, const metadb_handle_ptr& track);

void filterTracksByArtist(const std::string& artist, pfc::list_base_t<metadb_handle_ptr>& tracks);
void filterTracksByAlbum(const std::string& album, pfc::list_base_t<metadb_handle_ptr>& tracks);
void filterTracksByTrackNumber(const std::string& tracknumber, pfc::list_base_t<metadb_handle_ptr>& tracks);

void filterTracksByCloseTitle(const std::string& title, pfc::list_base_t<metadb_handle_ptr>& tracks);
void filterTracksByExactTitle(const std::string& title, pfc::list_base_t<metadb_handle_ptr>& tracks);

bool fileTitlesMatchExcludingBracketsOnLhs(const std::string& lhs, const std::string& rhs);

float calculateTrackRating(const std::string& title, const metadb_handle_ptr& track);

// all tracks will be analysed; try to cut the size of the list down before calling.
metadb_handle_ptr getBestTrackByTitle(const std::string& title, const pfc::list_base_t<metadb_handle_ptr>& tracks);

} // namespace bestversion
